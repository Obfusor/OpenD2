#include "D2Client.hpp"
#include <cstring>

//////////////////////////////////////////////////
// D2S Save File Parsing (integrated into engine client)
//
// This parses the full .d2s binary beyond the header, populating
// D2SaveExtendedData with stats, skills, and items.
//////////////////////////////////////////////////

// D2's bit reader - reads LSB-first from a byte array
struct D2BitReader
{
    const BYTE *data;
    size_t totalBytes;
    size_t bitPos;

    D2BitReader(const BYTE *d, size_t sz) : data(d), totalBytes(sz), bitPos(0) {}

    uint32_t ReadBits(int n)
    {
        uint32_t val = 0;
        for (int i = 0; i < n; i++)
        {
            size_t byteIdx = bitPos / 8;
            int bitIdx = bitPos % 8;
            if (byteIdx < totalBytes)
                val |= ((uint32_t)((data[byteIdx] >> bitIdx) & 1)) << i;
            bitPos++;
        }
        return val;
    }

    void Skip(int n) { bitPos += n; }

    size_t GetBitPosition() const { return bitPos; }

    uint32_t PeekBits(size_t absPos, int numBits) const
    {
        uint32_t val = 0;
        for (int i = 0; i < numBits; i++)
        {
            size_t pos = absPos + i;
            size_t byteIdx = pos / 8;
            int bitIdx = pos % 8;
            if (byteIdx < totalBytes)
                val |= ((uint32_t)((data[byteIdx] >> bitIdx) & 1)) << i;
        }
        return val;
    }

    void AlignToByte()
    {
        if (bitPos % 8 != 0)
            bitPos += 8 - (bitPos % 8);
    }

    size_t GetByteOffset() const { return (bitPos + 7) / 8; }
};

//////////////////////////////////////////////////
// Character stat section bit widths (CSvBits, IDs 0-15)

static const int g_CharStatCSvBits[] = {
    10, 10, 10, 10, 10, 8, 21, 21,
    21, 21, 21, 21, 7, 32, 25, 25};
static const int NUM_CHAR_STAT_IDS = 16;

//////////////////////////////////////////////////
// Item stat section bit widths from PD2 S8 ItemStatCost
// This is a simplified version - we only need savebits/saveparambits
// to skip over stats correctly.

// We import these from a generated included header rather than
// duplicating the 507-entry table here. The engine doesn't have access
// to the standalone parser's D2SData.hpp, so we embed the essential
// arrays inline.

// Note: These are auto-generated - see Tools/D2SaveParser/gen_data.py
// For the engine integration, we embed a compact version:
// Only SaveBits and SaveParamBits matter for parsing.
#include "D2Client_StatTable.hpp"

//////////////////////////////////////////////////
// Forward declarations

static bool ParseSingleItemIntoEntry(const BYTE *fileData, size_t fileSize, size_t &offset,
                                     D2SaveItemEntry &entry, WORD mercType);
static bool ParseItemStats(D2BitReader &reader);
static bool ParseItemSection(const BYTE *fileData, size_t fileSize, size_t &offset,
                             D2SaveItemEntry *items, WORD maxItems, WORD &outCount);

//////////////////////////////////////////////////
// Main entry point: parse full save into D2SaveExtendedData

void D2Client_ParseFullSave(const char *savePath)
{
    fs_handle f;
    D2SaveExtendedData &ext = cl.currentSave.extended;

    memset(&ext, 0, sizeof(ext));

    size_t fileSize = engine->FS_Open(savePath, &f, FS_READ, true);
    if (f == INVALID_HANDLE || fileSize < 335)
    {
        ext.bFullyParsed = false;
        return;
    }

    BYTE *fileData = new BYTE[fileSize];
    engine->FS_Read(f, fileData, fileSize, 1);
    engine->FS_CloseFile(f);

    size_t offset = 335; // skip header

    // Helper: check marker at offset
    auto matchMarker = [&](const BYTE *marker, size_t len) -> bool
    {
        if (offset + len > fileSize)
            return false;
        return memcmp(fileData + offset, marker, len) == 0;
    };

    // === Quests: "Woo!" + 4 + 2 + 96*3 = 298 bytes ===
    const BYTE mqst[] = {'W', 'o', 'o', '!'};
    if (!matchMarker(mqst, 4))
        goto cleanup;
    offset += 4 + 4 + 2 + (96 * 3);

    // === Waypoints: "WS" + 4 + 2 + 24*3 = 80 bytes ===
    {
        const BYTE mwp[] = {'W', 'S'};
        if (!matchMarker(mwp, 2))
            goto cleanup;
        offset += 2 + 4 + 2 + (24 * 3);
    }

    // === NPC: 0x01 0x77 + size(2) + data ===
    if (offset + 4 > fileSize || fileData[offset] != 0x01 || fileData[offset + 1] != 0x77)
        goto cleanup;
    {
        WORD npcSize;
        memcpy(&npcSize, fileData + offset + 2, 2);
        offset += npcSize;
    }

    // === Stats: "gf" + bit-packed stats ===
    {
        const BYTE mst[] = {'g', 'f'};
        if (!matchMarker(mst, 2))
            goto cleanup;
        offset += 2;

        D2BitReader reader(fileData + offset, fileSize - offset);
        ext.nStatCount = 0;

        while (true)
        {
            uint32_t statId = reader.ReadBits(9);
            if (statId == 0x1FF)
                break;

            if ((int)statId >= NUM_CHAR_STAT_IDS)
            {
                // Unknown char stat - can't continue
                goto cleanup;
            }

            uint32_t value = reader.ReadBits(g_CharStatCSvBits[statId]);

            if (ext.nStatCount < MAX_D2SAVE_STATS)
            {
                ext.stats[ext.nStatCount].nStatId = (WORD)statId;
                ext.stats[ext.nStatCount].dwValue = value;
                ext.nStatCount++;
            }
        }

        reader.AlignToByte();
        offset += reader.GetByteOffset();
    }

    // === Skills: "if" + 30 bytes ===
    {
        const BYTE msk[] = {'i', 'f'};
        if (!matchMarker(msk, 2))
            goto cleanup;
        offset += 2;
        if (offset + MAX_D2SAVE_SKILLS > fileSize)
            goto cleanup;
        memcpy(ext.skills, fileData + offset, MAX_D2SAVE_SKILLS);
        offset += MAX_D2SAVE_SKILLS;
    }

    // === Player Items: "JM" + count(16) + items ===
    if (!ParseItemSection(fileData, fileSize, offset,
                          ext.playerItems, MAX_D2SAVE_ITEMS, ext.nPlayerItemCount))
        goto cleanup;

    // === Corpse: "JM" + corpse_count(16) + metadata + items ===
    {
        const BYTE mjm[] = {'J', 'M'};
        if (!matchMarker(mjm, 2))
            goto cleanup;
        offset += 2;
        if (offset + 2 > fileSize)
            goto cleanup;
        WORD corpseCount;
        memcpy(&corpseCount, fileData + offset, 2);
        offset += 2;

        // Skip corpse metadata (12 bytes per corpse)
        offset += (size_t)corpseCount * 12;

        // Parse corpse item lists
        for (WORD c = 0; c < corpseCount; c++)
        {
            WORD dummyCount = 0;
            // Parse items into a temp area we don't track (corpse items are temporary)
            D2SaveItemEntry dummyItems[64];
            if (!ParseItemSection(fileData, fileSize, offset, dummyItems, 64, dummyCount))
                goto cleanup;
        }
    }

    // === Merc: "jf" + optional item list ===
    {
        const BYTE mjf[] = {'j', 'f'};
        if (!matchMarker(mjf, 2))
            goto cleanup;
        offset += 2;

        if (cl.currentSave.header.mercData.wMercType != 0)
        {
            if (!ParseItemSection(fileData, fileSize, offset,
                                  ext.mercItems, MAX_D2SAVE_ITEMS, ext.nMercItemCount))
                goto cleanup;
        }
    }

    ext.bFullyParsed = true;

cleanup:
    delete[] fileData;
}

//////////////////////////////////////////////////
// Parse an item section: JM + count(16) + items

static bool ParseItemSection(const BYTE *fileData, size_t fileSize, size_t &offset,
                             D2SaveItemEntry *items, WORD maxItems, WORD &outCount)
{
    const BYTE mjm[] = {'J', 'M'};
    if (offset + 4 > fileSize)
        return false;
    if (memcmp(fileData + offset, mjm, 2) != 0)
        return false;
    offset += 2;

    WORD itemCount;
    memcpy(&itemCount, fileData + offset, 2);
    offset += 2;
    outCount = itemCount;

    WORD parsed = 0;
    for (WORD i = 0; i < itemCount; i++)
    {
        D2SaveItemEntry entry;
        memset(&entry, 0, sizeof(entry));

        if (!ParseSingleItemIntoEntry(fileData, fileSize, offset, entry,
                                      cl.currentSave.header.mercData.wMercType))
        {
            return false;
        }

        if (parsed < maxItems)
        {
            items[parsed] = entry;
        }
        parsed++;

        // Parse socketed items belonging to this item
        for (BYTE s = 0; s < entry.nGemCount; s++)
        {
            D2SaveItemEntry socketEntry;
            memset(&socketEntry, 0, sizeof(socketEntry));
            if (!ParseSingleItemIntoEntry(fileData, fileSize, offset, socketEntry,
                                          cl.currentSave.header.mercData.wMercType))
            {
                return false;
            }

            // Store in parent's socketedItems if still in bounds
            if (parsed <= maxItems && i < maxItems && entry.nSocketedItemCount < MAX_D2SAVE_SOCKETS)
            {
                D2SaveItemEntry &parent = items[i < maxItems ? i : 0]; // bounds check
                if (parent.nSocketedItemCount < MAX_D2SAVE_SOCKETS)
                {
                    memcpy(parent.socketedItems[parent.nSocketedItemCount].szCode, socketEntry.szCode, 4);
                    parent.socketedItems[parent.nSocketedItemCount].dwId = socketEntry.dwId;
                    parent.nSocketedItemCount++;
                }
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////
// Item base flags lookup (simplified version of D2SData.hpp's g_ItemBases)
// We use the same data via the generated stat table header.

#define IBASE_QUANTITY 0x01
#define IBASE_DURABILITY 0x02
#define IBASE_DEFENSE 0x04
#define IBASE_TOME 0x08

//////////////////////////////////////////////////
// Parse a single item from the bitstream

static bool ParseSingleItemIntoEntry(const BYTE *fileData, size_t fileSize, size_t &offset,
                                     D2SaveItemEntry &entry, WORD mercType)
{
    // Item starts with "JM" (2 bytes)
    if (offset + 2 > fileSize)
        return false;
    if (fileData[offset] != 'J' || fileData[offset + 1] != 'M')
        return false;
    size_t itemJmOffset = offset;
    offset += 2;

    if (offset + 14 > fileSize) // minimum item data
        return false;

    D2BitReader reader(fileData + offset, fileSize - offset);

    // Flag bits (42 total)
    reader.Skip(4);
    entry.bIdentified = reader.ReadBits(1) != 0;
    reader.Skip(6);
    entry.bSocketed = reader.ReadBits(1) != 0;
    reader.Skip(4);
    entry.bEar = reader.ReadBits(1) != 0;
    reader.Skip(4);
    entry.bSimple = reader.ReadBits(1) != 0;
    entry.bEthereal = reader.ReadBits(1) != 0;
    reader.Skip(1);
    entry.bPersonalized = reader.ReadBits(1) != 0;
    reader.Skip(1);
    entry.bRuneword = reader.ReadBits(1) != 0;
    reader.Skip(15);

    // Location (18 bits)
    entry.nParent = (BYTE)reader.ReadBits(3);
    entry.nBodyLoc = (BYTE)reader.ReadBits(4);
    entry.nColumn = (BYTE)reader.ReadBits(4);
    entry.nRow = (BYTE)reader.ReadBits(3);
    reader.Skip(1);
    entry.nStorage = (BYTE)reader.ReadBits(3);

    if (entry.bEar)
    {
        // Ear: 3-bit class + 7-bit level + name(7 bits per char, null-terminated)
        reader.Skip(3 + 7);
        for (int c = 0; c < 16; c++)
        {
            uint32_t ch = reader.ReadBits(7);
            if (ch == 0)
                break;
        }
        reader.AlignToByte();
        offset += reader.GetByteOffset();
        return true;
    }

    // Item code (4 chars × 8 bits)
    for (int i = 0; i < 4; i++)
    {
        uint32_t c = reader.ReadBits(8);
        entry.szCode[i] = (char)c;
    }
    entry.szCode[4] = '\0';

    // Look up item base flags (4-char code, 3-char items have ' ' as 4th)
    int baseFlags = D2Client_GetItemBaseFlags(entry.szCode);

    if (entry.bSimple)
    {
        reader.Skip(1); // unknown bit for simple items
        reader.AlignToByte();
        offset += reader.GetByteOffset();
        return true;
    }

    // Extended item data
    entry.nGemCount = (BYTE)reader.ReadBits(3);
    entry.dwId = reader.ReadBits(32);
    entry.nILevel = (BYTE)reader.ReadBits(7);
    entry.nQuality = (BYTE)reader.ReadBits(4);

    // Variable presence flag
    bool hasGraphic = reader.ReadBits(1) != 0;
    if (hasGraphic)
        reader.Skip(3);

    bool hasAutoAffix = reader.ReadBits(1) != 0;
    if (hasAutoAffix)
        reader.Skip(11);

    // Quality-specific data
    switch (entry.nQuality)
    {
    case 1: // Low quality
        reader.Skip(3);
        break;
    case 3: // Superior
        reader.Skip(3);
        break;
    case 4:                   // Magic
        reader.Skip(11 + 11); // prefix + suffix
        break;
    case 5: // Set
        reader.Skip(12);
        break;
    case 6:                 // Rare
    case 8:                 // Crafted
        reader.Skip(8 + 8); // first/second name
        for (int i = 0; i < 6; i++)
        {
            if (reader.ReadBits(1))
                reader.Skip(11);
        }
        break;
    case 7: // Unique
        reader.Skip(12);
        break;
    default:
        break;
    }

    // Runeword
    if (entry.bRuneword)
        reader.Skip(16);

    // Personalized name
    if (entry.bPersonalized)
    {
        for (int i = 0; i < 16; i++)
        {
            uint32_t ch = reader.ReadBits(7);
            if (ch == 0)
                break;
        }
    }

    // Tome
    if (baseFlags >= 0 && (baseFlags & IBASE_TOME))
        reader.Skip(5);

    // Realm data / timestamp (1 bit flag + 96 bits if set)
    if (reader.ReadBits(1))
        reader.Skip(96);

    // Defense
    if (baseFlags >= 0 && (baseFlags & IBASE_DEFENSE))
        entry.nDefense = (WORD)reader.ReadBits(11);

    // Durability (armor and weapons)
    if (baseFlags >= 0 && (baseFlags & (IBASE_DEFENSE | IBASE_DURABILITY)))
    {
        entry.nMaxDurability = (BYTE)reader.ReadBits(8);
        if (entry.nMaxDurability > 0)
            entry.nCurrentDurability = (WORD)reader.ReadBits(9);
    }

    // Quantity
    if (baseFlags >= 0 && (baseFlags & IBASE_QUANTITY))
        entry.nQuantity = (WORD)reader.ReadBits(9);

    // Sockets
    if (entry.bSocketed)
        entry.nTotalSockets = (BYTE)reader.ReadBits(4);

    // Set flags
    BYTE setFlags = 0;
    if (entry.nQuality == 5) // Set
        setFlags = (BYTE)reader.ReadBits(5);

    // Parse stat lists
    if (!ParseItemStats(reader))
    {
        // Deterministic fallback: scan for next JM marker from item start + 2
        // This ensures consistent recovery regardless of how many bits stats consumed
        size_t searchStart = itemJmOffset + 2;
        for (size_t s = searchStart; s + 1 < fileSize; s++)
        {
            if (fileData[s] == 'J' && fileData[s + 1] == 'M')
            {
                offset = s;
                return true;
            }
        }
        offset = fileSize;
        return true;
    }

    // Additional stat lists for set items
    for (int i = 0; i < 5; i++)
    {
        if (setFlags & (1 << i))
        {
            if (!ParseItemStats(reader))
                break;
        }
    }

    reader.AlignToByte();
    offset += reader.GetByteOffset();
    return true;
}

//////////////////////////////////////////////////
// Parse item stat list (reads 9-bit IDs + value bits until 0x1FF)
// Auto-detect follow stats: peek ahead to verify continuation after follow data.

static bool ParseItemStats(D2BitReader &reader)
{
    while (true)
    {
        uint32_t statId = reader.ReadBits(9);
        if (statId == 0x1FF)
            return true;

        if ((int)statId >= D2CLIENT_NUM_ITEM_STATS)
            return false;

        int saveBits = g_ClientSaveBits[statId];
        int paramBits = g_ClientSaveParamBits[statId];

        if (saveBits == 0)
            return false;

        reader.Skip(paramBits + saveBits);

        // Auto-detect follow stats for damage pairs
        int followCount = g_ClientFollowStats[statId];
        if (followCount > 0)
        {
            size_t afterVal = reader.GetBitPosition();
            uint32_t totalFollowBits = 0;
            bool followOk = true;

            for (int f = 0; f < followCount && followOk; f++)
            {
                int fid = (int)statId + 1 + f;
                if (fid >= D2CLIENT_NUM_ITEM_STATS || g_ClientSaveBits[fid] == 0)
                {
                    followOk = false;
                    break;
                }
                totalFollowBits += g_ClientSaveParamBits[fid] + g_ClientSaveBits[fid];
            }

            if (followOk)
            {
                uint32_t nextFollow = reader.PeekBits(afterVal + totalFollowBits, 9);
                bool followValid = (nextFollow == 0x1FF) ||
                                   ((int)nextFollow < D2CLIENT_NUM_ITEM_STATS && g_ClientSaveBits[nextFollow] > 0);

                uint32_t nextNoFollow = reader.PeekBits(afterVal, 9);
                bool noFollowValid = (nextNoFollow == 0x1FF) ||
                                     ((int)nextNoFollow < D2CLIENT_NUM_ITEM_STATS && g_ClientSaveBits[nextNoFollow] > 0);

                if (followValid && !noFollowValid)
                {
                    reader.Skip(totalFollowBits);
                }
                else if (followValid && noFollowValid)
                {
                    reader.Skip(totalFollowBits);
                }
            }
        }
    }
}
