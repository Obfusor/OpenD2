#include "D2DebugLog.hpp"
#include <time.h>
#include <string.h>

namespace D2Log
{
    static FILE *s_logFile = nullptr;
    static bool s_catEnabled[(int)D2LogCat::MAX] = {};
    static D2LogLevel s_maxLevel = D2LogLevel::Info;

    static const char *s_catNames[] = {
        "GENERAL", "TOKEN", "TILE", "NET", "AUDIO", "FILE", "EDITOR"
    };

    static const char *s_levelNames[] = {
        "ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"
    };

    void Init(const char *logFilePath)
    {
        if (s_logFile)
            return;

        s_logFile = fopen(logFilePath, "w");
        if (!s_logFile)
            return;

        // Enable all categories by default
        for (int i = 0; i < (int)D2LogCat::MAX; i++)
            s_catEnabled[i] = true;

        s_maxLevel = D2LogLevel::Debug;

        time_t now = time(nullptr);
        fprintf(s_logFile, "=== OpenD2 Debug Log started %s", ctime(&now));
        fflush(s_logFile);
    }

    void Shutdown()
    {
        if (s_logFile)
        {
            fprintf(s_logFile, "=== Log closed ===\n");
            fclose(s_logFile);
            s_logFile = nullptr;
        }
    }

    void Enable(D2LogCat cat)
    {
        if ((int)cat < (int)D2LogCat::MAX)
            s_catEnabled[(int)cat] = true;
    }

    void Disable(D2LogCat cat)
    {
        if ((int)cat < (int)D2LogCat::MAX)
            s_catEnabled[(int)cat] = false;
    }

    bool IsEnabled(D2LogCat cat)
    {
        if ((int)cat >= (int)D2LogCat::MAX)
            return false;
        return s_catEnabled[(int)cat];
    }

    void SetLevel(D2LogLevel level)
    {
        s_maxLevel = level;
    }

    void VWrite(D2LogCat cat, D2LogLevel level, const char *fmt, va_list args)
    {
        if (!s_logFile)
            return;
        if (level > s_maxLevel && level != D2LogLevel::Error)
            return;
        if ((int)cat < (int)D2LogCat::MAX && !s_catEnabled[(int)cat])
            return;

        int catIdx = (int)cat;
        int lvlIdx = (int)level;
        if (catIdx < 0 || catIdx >= (int)D2LogCat::MAX) catIdx = 0;
        if (lvlIdx < 0 || lvlIdx > 4) lvlIdx = 2;

        fprintf(s_logFile, "[%s][%s] ", s_catNames[catIdx], s_levelNames[lvlIdx]);
        vfprintf(s_logFile, fmt, args);
        fprintf(s_logFile, "\n");
        fflush(s_logFile);
    }

    void Write(D2LogCat cat, D2LogLevel level, const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        VWrite(cat, level, fmt, args);
        va_end(args);
    }
}
