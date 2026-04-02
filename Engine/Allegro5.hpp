#pragma once

//////////////////////////////////////////////////
//
// Allegro 5 Include Wrapper for OpenD2
//
// Central include point for Allegro 5 headers.
// Platform-specific path resolution handled here.
//
//////////////////////////////////////////////////

#ifdef _WIN32
// Windows: use bundled headers from Libraries/allegro5/
#include "../Libraries/allegro5/include/allegro5/allegro.h"
#include "../Libraries/allegro5/include/allegro5/allegro_image.h"
#include "../Libraries/allegro5/include/allegro5/allegro_font.h"
#include "../Libraries/allegro5/include/allegro5/allegro_ttf.h"
#include "../Libraries/allegro5/include/allegro5/allegro_primitives.h"
#include "../Libraries/allegro5/include/allegro5/allegro_audio.h"
#include "../Libraries/allegro5/include/allegro5/allegro_acodec.h"
#else
// Linux/macOS/ARM: use system-installed Allegro 5
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#endif

// Resolve Windows BITMAP conflict with Allegro
#ifdef _WIN32
#ifdef BITMAP
#undef BITMAP
#endif
#endif

// Message box type constants (matching SDL values for source compatibility)
#define D2_MESSAGEBOX_WARNING    0x00000020
#define D2_MESSAGEBOX_ERROR      0x00000010
