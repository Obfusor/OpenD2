#pragma once
#include <stdio.h>
#include <stdarg.h>

/*
 *	Debug logging system with levels and categories.
 *	Writes to a log file (debug.log) in the working directory.
 *	Categories can be toggled at runtime.
 *
 *	Usage:
 *	  D2Log::Init();                          // call once at startup
 *	  D2Log::Enable(D2LogCat::Token);         // turn on a category
 *	  D2LOG(D2LogCat::Token, "Loaded %d frames", count);
 *	  D2Log::Shutdown();                      // call at exit
 */

enum class D2LogLevel
{
    Error = 0,   // always shown
    Warning = 1,
    Info = 2,
    Debug = 3,
    Verbose = 4
};

enum class D2LogCat : unsigned int
{
    General  = 0,
    Token    = 1,  // character token loading/rendering
    Tile     = 2,  // tile rendering pipeline
    Net      = 3,  // network/packets
    Audio    = 4,  // audio system
    File     = 5,  // filesystem operations
    Editor   = 6,  // editor bridge
    MAX
};

namespace D2Log
{
    void Init(const char *logFilePath = "debug.log");
    void Shutdown();

    void Enable(D2LogCat cat);
    void Disable(D2LogCat cat);
    bool IsEnabled(D2LogCat cat);

    void SetLevel(D2LogLevel level);

    void Write(D2LogCat cat, D2LogLevel level, const char *fmt, ...);
    void VWrite(D2LogCat cat, D2LogLevel level, const char *fmt, va_list args);
}

// Convenience macros
#define D2LOG(cat, ...)        D2Log::Write(cat, D2LogLevel::Info, __VA_ARGS__)
#define D2LOG_ERROR(cat, ...)  D2Log::Write(cat, D2LogLevel::Error, __VA_ARGS__)
#define D2LOG_WARN(cat, ...)   D2Log::Write(cat, D2LogLevel::Warning, __VA_ARGS__)
#define D2LOG_DEBUG(cat, ...)  D2Log::Write(cat, D2LogLevel::Debug, __VA_ARGS__)
#define D2LOG_VERBOSE(cat, ...) D2Log::Write(cat, D2LogLevel::Verbose, __VA_ARGS__)
