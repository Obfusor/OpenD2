#include "D2SFile.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

//////////////////////////////////////////////////
// Collect .d2s files from a directory

static std::vector<std::string> ListD2SFiles(const char *dirPath)
{
    std::vector<std::string> files;

#ifdef _WIN32
    std::string pattern = std::string(dirPath) + "\\*.d2s";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::string fullPath = std::string(dirPath) + "\\" + fd.cFileName;
                files.push_back(fullPath);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
#else
    DIR *dir = opendir(dirPath);
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if (name.size() >= 4 && name.substr(name.size() - 4) == ".d2s")
            {
                std::string fullPath = std::string(dirPath) + "/" + name;
                files.push_back(fullPath);
            }
        }
        closedir(dir);
    }
#endif

    std::sort(files.begin(), files.end());
    return files;
}

//////////////////////////////////////////////////
// Extract filename from path

static std::string GetFileName(const std::string &path)
{
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos)
        return path;
    return path.substr(pos + 1);
}

//////////////////////////////////////////////////
// Print a single section result column

static const char *ResultStr(const D2SSectionResult &r)
{
    return r.ok ? "OK" : "FAIL";
}

//////////////////////////////////////////////////
// Verbose dump of a single file

static void PrintVerbose(const D2SFile &d2s)
{
    printf("  File:    %s\n", d2s.filePath.c_str());
    printf("  Name:    %s\n", d2s.header.szCharacterName);
    printf("  Class:   %s\n", d2s.GetCharClassName());
    printf("  Level:   %u\n", d2s.header.nCharLevel);
    printf("  Status:  %s (0x%02X)\n", d2s.GetStatusString().c_str(), d2s.header.nCharStatus);
    printf("  Version: %u\n", d2s.header.dwVersion);
    printf("  Size:    %u bytes\n", d2s.header.dwFileSize);
    printf("  Seed:    0x%08X\n", d2s.header.dwSeed);

    // Merc info
    if (d2s.header.mercData.wMercType != 0)
    {
        printf("  Merc:    type=%u name=%u exp=%u dead=%u\n",
               d2s.header.mercData.wMercType, d2s.header.mercData.wMercName,
               d2s.header.mercData.dwMercExperience, d2s.header.mercData.bMercDead);
    }

    // Section results
    printf("  Sections:\n");
    printf("    Header:    %s  %s\n", ResultStr(d2s.headerResult), d2s.headerResult.error.c_str());
    printf("    CRC:       %s  %s\n", ResultStr(d2s.crcResult), d2s.crcResult.error.c_str());
    printf("    Quests:    %s  %s\n", ResultStr(d2s.questResult), d2s.questResult.error.c_str());
    printf("    Waypoints: %s  %s\n", ResultStr(d2s.waypointResult), d2s.waypointResult.error.c_str());
    printf("    NPC:       %s  %s\n", ResultStr(d2s.npcResult), d2s.npcResult.error.c_str());
    printf("    Stats:     %s  %s\n", ResultStr(d2s.statResult), d2s.statResult.error.c_str());
    printf("    Skills:    %s  %s\n", ResultStr(d2s.skillResult), d2s.skillResult.error.c_str());
    printf("    Items:     %s  (%u items)  %s\n", ResultStr(d2s.itemResult),
           d2s.playerItemCount, d2s.itemResult.error.c_str());
    printf("    Corpse:    %s  (%u corpses, %u items)  %s\n", ResultStr(d2s.corpseResult),
           d2s.corpseCount, d2s.corpseItemCount, d2s.corpseResult.error.c_str());
    printf("    Merc:      %s  (%u items)  %s\n", ResultStr(d2s.mercResult),
           d2s.mercItemCount, d2s.mercResult.error.c_str());
    printf("    Golem:     %s  %s\n", ResultStr(d2s.golemResult), d2s.golemResult.error.c_str());

    // Item details
    if (!d2s.playerItems.empty())
    {
        printf("  Player Items (%u):\n", d2s.playerItemCount);
        for (size_t i = 0; i < d2s.playerItems.size(); i++)
        {
            const auto &it = d2s.playerItems[i];
            if (it.bSimple || it.bEar)
            {
                printf("    [%3zu] %-4s  (simple)\n", i + 1, it.szCode);
            }
            else
            {
                printf("    [%3zu] %-4s  iLvl=%-2u %-8s", i + 1, it.szCode, it.nILevel,
                       GetQualityName(it.nQuality));
                if (it.nBaseFlags >= 0)
                {
                    if (it.nBaseFlags & IBASE_DEFENSE)
                        printf(" def=%u", it.nDefense);
                    if (it.nBaseFlags & (IBASE_DEFENSE | IBASE_DURABILITY))
                        printf(" dur=%u/%u", it.nCurDur, it.nMaxDur);
                    if (it.nBaseFlags & IBASE_QUANTITY)
                        printf(" qty=%u", it.nQuantity);
                }
                if (it.bSocketed)
                    printf(" soc=%u(%u)", it.nTotalSockets, it.nGemCount);
                if (it.bEthereal)
                    printf(" [eth]");
                if (it.bRuneword)
                    printf(" [rw]");
                printf("\n");
            }
            for (size_t j = 0; j < it.socketedItems.size(); j++)
            {
                const auto &si = it.socketedItems[j];
                printf("      +gem %-4s\n", si.szCode);
            }
        }
    }
    if (!d2s.mercItems.empty())
    {
        printf("  Merc Items (%u):\n", d2s.mercItemCount);
        for (size_t i = 0; i < d2s.mercItems.size(); i++)
        {
            const auto &it = d2s.mercItems[i];
            if (it.bSimple || it.bEar)
                printf("    [%3zu] %-4s  (simple)\n", i + 1, it.szCode);
            else
                printf("    [%3zu] %-4s  iLvl=%-2u %-8s\n", i + 1, it.szCode, it.nILevel,
                       GetQualityName(it.nQuality));
        }
    }

    // Stats dump
    if (!d2s.stats.empty())
    {
        printf("  Stats (%zu):\n", d2s.stats.size());
        for (const auto &stat : d2s.stats)
        {
            const char *name = GetStatName(stat.id);
            if (name)
                printf("    [%3u] %-16s = %u\n", stat.id, name, stat.value);
            else
                printf("    [%3u] (unknown)       = %u\n", stat.id, stat.value);
        }
    }

    // Skills dump
    printf("  Skills: ");
    for (int i = 0; i < NUM_SKILLS; i++)
    {
        printf("%u", d2s.skills[i]);
        if (i < NUM_SKILLS - 1)
            printf(",");
    }
    printf("\n");
}

//////////////////////////////////////////////////
// Usage

static void PrintUsage(const char *progName)
{
    printf("D2S Save Parser & Validator\n");
    printf("Usage:\n");
    printf("  %s <directory>              Parse all .d2s files in directory\n", progName);
    printf("  %s <file.d2s>               Parse a single save file\n", progName);
    printf("  %s -v <file.d2s>            Verbose output for a single file\n", progName);
    printf("  %s -v <directory>            Verbose output for all files\n", progName);
    printf("  %s --roundtrip <target>      Round-trip test: load -> write -> compare bytes\n", progName);
    printf("  %s --write <in.d2s> <out>    Write save file (recomputes CRC)\n", progName);
}

//////////////////////////////////////////////////
// Main

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    bool verbose = false;
    bool roundtrip = false;
    bool writeMode = false;
    const char *target = nullptr;
    const char *writeOutput = nullptr;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
            verbose = true;
        else if (strcmp(argv[i], "--roundtrip") == 0)
            roundtrip = true;
        else if (strcmp(argv[i], "--write") == 0)
        {
            writeMode = true;
            // Next two args are input and output
            if (i + 2 < argc)
            {
                target = argv[++i];
                writeOutput = argv[++i];
            }
        }
        else
            target = argv[i];
    }

    if (!target)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    // --write mode: load, parse, write to output
    if (writeMode && writeOutput)
    {
        D2SFile d2s;
        if (!d2s.Load(target))
        {
            printf("Error: Cannot load '%s'\n", target);
            return 1;
        }
        d2s.Parse();
        if (!d2s.AllSectionsOK())
        {
            printf("Warning: Parse had errors, writing anyway\n");
        }
        if (d2s.Write(writeOutput))
        {
            printf("Written to %s (%zu bytes)\n", writeOutput, d2s.GetRawData().size());
            return 0;
        }
        else
        {
            printf("Error: Failed to write '%s'\n", writeOutput);
            return 1;
        }
    }

    // Determine if target is a file or directory
    std::vector<std::string> files;

#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(target);
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        printf("Error: Cannot access '%s'\n", target);
        return 1;
    }
    bool isDir = (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;
    if (stat(target, &st) != 0)
    {
        printf("Error: Cannot access '%s'\n", target);
        return 1;
    }
    bool isDir = S_ISDIR(st.st_mode);
#endif

    if (isDir)
    {
        files = ListD2SFiles(target);
        if (files.empty())
        {
            printf("No .d2s files found in '%s'\n", target);
            return 1;
        }
    }
    else
    {
        files.push_back(target);
    }

    printf("=== D2S Save Validator ===\n");
    if (isDir)
        printf("Dir: %s  (%zu files)\n", target, files.size());
    printf("\n");

    // Round-trip mode: load -> write to temp -> compare bytes
    if (roundtrip)
    {
        int rtPass = 0, rtFail = 0;
        printf("Round-trip validation: load -> write -> byte compare\n\n");
        for (const auto &fp : files)
        {
            D2SFile d2s;
            if (!d2s.Load(fp.c_str()))
            {
                printf("  %-28s  LOAD ERROR\n", GetFileName(fp).c_str());
                rtFail++;
                continue;
            }
            d2s.Parse();

            // Write to temp file
            std::string tmpPath = fp + ".roundtrip.tmp";
            if (!d2s.Write(tmpPath.c_str()))
            {
                printf("  %-28s  WRITE ERROR\n", GetFileName(fp).c_str());
                rtFail++;
                continue;
            }

            // Re-read and compare
            D2SFile d2s2;
            if (!d2s2.Load(tmpPath.c_str()))
            {
                printf("  %-28s  RE-LOAD ERROR\n", GetFileName(fp).c_str());
                rtFail++;
                remove(tmpPath.c_str());
                continue;
            }

            const auto &orig = d2s.GetRawData();
            const auto &written = d2s2.GetRawData();
            bool match = (orig.size() == written.size()) &&
                         memcmp(orig.data(), written.data(), orig.size()) == 0;

            if (match)
            {
                printf("  %-28s  MATCH (%zu bytes)\n", GetFileName(fp).c_str(), orig.size());
                rtPass++;
            }
            else
            {
                printf("  %-28s  MISMATCH (orig=%zu, written=%zu)\n",
                       GetFileName(fp).c_str(), orig.size(), written.size());
                // Show first difference
                size_t minSz = (std::min)(orig.size(), written.size());
                for (size_t b = 0; b < minSz; b++)
                {
                    if (orig[b] != written[b])
                    {
                        printf("    First diff at byte %zu: orig=0x%02X written=0x%02X\n",
                               b, orig[b], written[b]);
                        break;
                    }
                }
                rtFail++;
            }

            remove(tmpPath.c_str());
        }
        printf("\nRound-trip: %d/%zu MATCH, %d MISMATCH\n", rtPass, files.size(), rtFail);
        return rtFail > 0 ? 1 : 0;
    }

    if (!verbose)
    {
        // Table header
        printf("%-28s %3s %-11s %3s %-10s %4s %3s %5s %5s %5s %5s %5s %5s  %s\n",
               "File", "Ver", "Class", "Lvl", "Status",
               "Head", "CRC", "Qst", "WP", "NPC", "Stats", "Skill", "Items", "Result");
        printf("%-28s %3s %-11s %3s %-10s %4s %3s %5s %5s %5s %5s %5s %5s  %s\n",
               "---", "---", "---", "---", "---",
               "----", "---", "-----", "-----", "-----", "-----", "-----", "-----", "------");
    }

    int passed = 0;
    int failed = 0;

    for (const auto &filePath : files)
    {
        D2SFile d2s;
        if (!d2s.Load(filePath.c_str()))
        {
            printf("%-28s  ** LOAD ERROR **\n", GetFileName(filePath).c_str());
            failed++;
            continue;
        }

        d2s.Parse();

        if (verbose)
        {
            printf("---\n");
            PrintVerbose(d2s);
            printf("  Result:  %s\n\n", d2s.AllSectionsOK() ? "PASS" : "FAIL");
        }
        else
        {
            printf("%-28s %3u %-11s %3u %-10s %4s %3s %5s %5s %5s %5s %5s %5u  %s\n",
                   GetFileName(filePath).c_str(),
                   d2s.header.dwVersion,
                   d2s.GetCharClassName(),
                   (unsigned)d2s.header.nCharLevel,
                   d2s.GetStatusString().c_str(),
                   ResultStr(d2s.headerResult),
                   ResultStr(d2s.crcResult),
                   ResultStr(d2s.questResult),
                   ResultStr(d2s.waypointResult),
                   ResultStr(d2s.npcResult),
                   ResultStr(d2s.statResult),
                   ResultStr(d2s.skillResult),
                   (unsigned)d2s.playerItemCount,
                   d2s.AllSectionsOK() ? "PASS" : "FAIL");
        }

        if (d2s.AllSectionsOK())
            passed++;
        else
            failed++;
    }

    printf("\nSummary: %d/%zu PASS, %d FAIL\n", passed, files.size(), failed);

    // If there were failures, print details
    if (failed > 0 && !verbose)
    {
        printf("\nFailure details:\n");
        for (const auto &filePath : files)
        {
            D2SFile d2s;
            if (!d2s.Load(filePath.c_str()))
            {
                printf("  %s: LOAD ERROR\n", GetFileName(filePath).c_str());
                continue;
            }
            d2s.Parse();
            if (!d2s.AllSectionsOK())
            {
                printf("  %s:\n", GetFileName(filePath).c_str());
                if (!d2s.headerResult.ok)
                    printf("    Header: %s\n", d2s.headerResult.error.c_str());
                if (!d2s.crcResult.ok)
                    printf("    CRC: %s\n", d2s.crcResult.error.c_str());
                if (!d2s.questResult.ok)
                    printf("    Quests: %s\n", d2s.questResult.error.c_str());
                if (!d2s.waypointResult.ok)
                    printf("    Waypoints: %s\n", d2s.waypointResult.error.c_str());
                if (!d2s.npcResult.ok)
                    printf("    NPC: %s\n", d2s.npcResult.error.c_str());
                if (!d2s.statResult.ok)
                    printf("    Stats: %s\n", d2s.statResult.error.c_str());
                if (!d2s.skillResult.ok)
                    printf("    Skills: %s\n", d2s.skillResult.error.c_str());
                if (!d2s.itemResult.ok)
                    printf("    Items: %s\n", d2s.itemResult.error.c_str());
                if (!d2s.corpseResult.ok)
                    printf("    Corpse: %s\n", d2s.corpseResult.error.c_str());
                if (!d2s.mercResult.ok)
                    printf("    Merc: %s\n", d2s.mercResult.error.c_str());
                if (!d2s.golemResult.ok)
                    printf("    Golem: %s\n", d2s.golemResult.error.c_str());
            }
        }
    }

    return failed > 0 ? 1 : 0;
}
