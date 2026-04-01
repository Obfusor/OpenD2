#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "D2SData.hpp"
#include "D2SBitReader.hpp"

//////////////////////////////////////////////////
// D2S Save File Constants

static constexpr uint32_t D2S_MAGIC = 0xAA55AA55;
static constexpr uint32_t D2S_VERSION_110 = 96;
static constexpr size_t D2S_HEADER_SIZE = 335;
static constexpr uint16_t D2S_STAT_END = 0x1FF;

// Section markers
static constexpr uint8_t MARKER_QUESTS[] = {'W', 'o', 'o', '!'};
static constexpr uint8_t MARKER_WAYPOINTS[] = {'W', 'S'};
static constexpr uint8_t MARKER_NPC[] = {0x01, 0x77};
static constexpr uint8_t MARKER_STATS[] = {'g', 'f'};
static constexpr uint8_t MARKER_SKILLS[] = {'i', 'f'};
static constexpr uint8_t MARKER_ITEMS[] = {'J', 'M'};
static constexpr uint8_t MARKER_MERC[] = {'j', 'f'};
static constexpr uint8_t MARKER_GOLEM[] = {'k', 'f'};

static constexpr int NUM_DIFFICULTIES = 3;
static constexpr int NUM_CLASSES = 7;
static constexpr int NUM_SKILLS = 30;
static constexpr int COMP_MAX_VAL = 16;

//////////////////////////////////////////////////
// Character Status Bits

enum D2SCharStatus : uint8_t
{
    STATUS_NEWBIE = (1 << 0),
    STATUS_HARDCORE = (1 << 2),
    STATUS_DEAD = (1 << 3),
    STATUS_EXPANSION = (1 << 5),
    STATUS_LADDER = (1 << 6),
};

static const char *ClassNames[NUM_CLASSES] = {
    "Amazon", "Sorceress", "Necromancer", "Paladin", "Barbarian", "Druid", "Assassin"};

//////////////////////////////////////////////////
// Character stat bit widths (CSvBits from ItemStatCost.txt)
// These are ONLY for the character stat section (marker "gf"), IDs 0-15.

struct StatBitInfo
{
    uint16_t id;
    const char *name;
    int valueBits; // CSvBits
};

static const StatBitInfo g_CharStatTable[] = {
    {0, "Strength", 10},
    {1, "Energy", 10},
    {2, "Dexterity", 10},
    {3, "Vitality", 10},
    {4, "StatPtsRemain", 10},
    {5, "SkillPtsRemain", 8},
    {6, "Life", 21},
    {7, "LifeMax", 21},
    {8, "Mana", 21},
    {9, "ManaMax", 21},
    {10, "Stamina", 21},
    {11, "StaminaMax", 21},
    {12, "Level", 7},
    {13, "Experience", 32},
    {14, "Gold", 25},
    {15, "GoldStash", 25},
};
static constexpr int NUM_CHAR_STATS = sizeof(g_CharStatTable) / sizeof(g_CharStatTable[0]);

inline int GetCharStatBitWidth(uint16_t statId)
{
    for (int i = 0; i < NUM_CHAR_STATS; i++)
    {
        if (g_CharStatTable[i].id == statId)
            return g_CharStatTable[i].valueBits;
    }
    return -1;
}

inline const char *GetStatName(uint16_t statId)
{
    for (int i = 0; i < NUM_CHAR_STATS; i++)
    {
        if (g_CharStatTable[i].id == statId)
            return g_CharStatTable[i].name;
    }
    return nullptr;
}

//////////////////////////////////////////////////
// Fixed-size header structs (packed, matching file layout)

#pragma pack(push, 1)

struct D2SMercData
{
    uint8_t bMercDead;
    uint8_t nMercReviveCount;
    uint32_t dwMercControl;
    uint16_t wMercName;
    uint16_t wMercType;
    uint32_t dwMercExperience;
};

struct D2SHeader
{
    uint32_t dwMagic;
    uint32_t dwVersion;
    uint32_t dwFileSize;
    uint32_t dwCRC;
    uint32_t dwWeaponSet;
    char szCharacterName[16];
    uint8_t nCharStatus;
    uint8_t nCharTitle;
    uint16_t unk1;
    uint8_t nCharClass;
    uint16_t unk2;
    uint8_t nCharLevel;
    uint32_t unk3;
    uint32_t dwCreationTime;
    uint32_t dwModificationTime;
    uint32_t dwSkillKey[16];
    uint32_t dwLeftSkill1;
    uint32_t dwRightSkill1;
    uint32_t dwLeftSkill2;
    uint32_t dwRightSkill2;
    uint8_t nAppearance[COMP_MAX_VAL];
    uint8_t nColor[COMP_MAX_VAL];
    uint8_t nTowns[NUM_DIFFICULTIES];
    uint32_t dwSeed;
    uint16_t unk5;
    D2SMercData mercData;
    uint8_t nRealmData[0x90];
};

// Quest data per difficulty: 96 bytes
struct D2SQuestDifficulty
{
    uint8_t data[96];
};

struct D2SQuestSection
{
    uint8_t marker[4];  // "Woo!"
    uint32_t dwVersion; // 6
    uint16_t wSize;     // size of quest data
    D2SQuestDifficulty diffs[NUM_DIFFICULTIES];
};

// Waypoint data per difficulty: 24 bytes
struct D2SWaypointDifficulty
{
    uint16_t wUnknown;
    uint8_t data[22];
};

struct D2SWaypointSection
{
    uint8_t marker[2];  // "WS"
    uint32_t dwVersion; // 1
    uint16_t wSize;     // 80
    D2SWaypointDifficulty diffs[NUM_DIFFICULTIES];
};

// NPC section: variable but typically 52 bytes
struct D2SNPCSection
{
    uint16_t marker;  // 0x0177
    uint16_t wSize;   // size
    uint8_t data[48]; // NPC intro/congrats flags
};

#pragma pack(pop)

//////////////////////////////////////////////////
// Parsed stat entry

struct D2SStat
{
    uint16_t id;
    uint32_t value;
};

//////////////////////////////////////////////////
// Parsed item

struct D2SItem
{
    // Flags
    bool bIdentified;
    bool bSocketed;
    bool bSimple;
    bool bEar;
    bool bEthereal;
    bool bRuneword;
    bool bPersonalized;

    // Location
    uint8_t nParent;  // 0=stored, 1=equipped, 2=belt
    uint8_t nBodyLoc; // equipped position
    uint8_t nColumn;  // x position
    uint8_t nRow;     // y position
    uint8_t nStorage; // inventory panel

    // Item code
    char szCode[5]; // 4-char item code + null (3-char codes space-padded)
    int nBaseFlags; // from itemBases lookup (-1 if unknown)

    // Extended data (only if !bSimple && !bEar)
    uint8_t nGemCount; // socketed gem/jewel/rune count
    uint32_t dwId;     // unique anti-dupe ID
    uint8_t nILevel;   // item level
    uint8_t nQuality;  // D2SItemQuality

    // Item-specific
    uint16_t nDefense;     // armor defense (if has defense)
    uint8_t nMaxDur;       // max durability
    uint16_t nCurDur;      // current durability (9 bits in v1.10+)
    uint16_t nQuantity;    // for stackable items
    uint8_t nTotalSockets; // socket count (if socketed flag set)

    // Socketed items
    std::vector<D2SItem> socketedItems;
};

//////////////////////////////////////////////////
// Validation result for each section

struct D2SSectionResult
{
    bool ok;
    std::string error;

    D2SSectionResult() : ok(true) {}
    D2SSectionResult(bool ok_, const std::string &err = "") : ok(ok_), error(err) {}
};

//////////////////////////////////////////////////
// Main D2S file class

class D2SFile
{
public:
    D2SFile();
    ~D2SFile();

    // Load from file path. Returns false on read error.
    bool Load(const char *filePath);

    // Parse all sections. Call after Load().
    void Parse();

    // Validation results per section
    D2SSectionResult headerResult;
    D2SSectionResult crcResult;
    D2SSectionResult questResult;
    D2SSectionResult waypointResult;
    D2SSectionResult npcResult;
    D2SSectionResult statResult;
    D2SSectionResult skillResult;
    D2SSectionResult itemResult;
    D2SSectionResult corpseResult;
    D2SSectionResult mercResult;
    D2SSectionResult golemResult;

    // Parsed data (public for easy access)
    D2SHeader header;
    std::vector<D2SStat> stats;
    uint8_t skills[NUM_SKILLS];
    std::vector<D2SItem> playerItems;
    std::vector<D2SItem> corpseItems;
    std::vector<D2SItem> mercItems;
    uint16_t playerItemCount;
    uint16_t corpseCount; // number of corpses (0 or 1 typically)
    uint16_t corpseItemCount;
    uint16_t mercItemCount;
    bool hasGolem;
    std::string filePath;

    // Convenience accessors
    const char *GetCharClassName() const;
    std::string GetStatusString() const;
    bool AllSectionsOK() const;

    // Writer: write the raw data back to a file (with recomputed CRC).
    bool Write(const char *outPath);

    // Get a const reference to the raw data for round-trip comparison.
    const std::vector<uint8_t> &GetRawData() const { return m_data; }

private:
    std::vector<uint8_t> m_data;

    void ParseHeader();
    void ValidateCRC();
    void ParseQuests(size_t &offset);
    void ParseWaypoints(size_t &offset);
    void ParseNPC(size_t &offset);
    void ParseStats(size_t &offset);
    void ParseSkills(size_t &offset);
    void ParseItems(size_t &offset, std::vector<D2SItem> &items, uint16_t &itemCount, D2SSectionResult &result);
    bool ParseSingleItem(size_t &offset, D2SItem &item);
    bool ParseItemStats(D2SBitReader &reader, const char *dbgCode = nullptr);
    void ParseCorpse(size_t &offset);
    void ParseMerc(size_t &offset);
    void ParseGolem(size_t &offset);

    uint32_t ComputeCRC32() const;
    bool MatchMarker(size_t offset, const uint8_t *marker, size_t markerLen) const;
    size_t FindMarker(size_t startOffset, const uint8_t *marker, size_t markerLen) const;
};
