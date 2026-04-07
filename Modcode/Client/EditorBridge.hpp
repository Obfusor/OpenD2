#pragma once

/*
 *	Shared declarations for the d2-ds1-edit editor bridge.
 *	Used by both MapRenderer and D2World to access the editor's
 *	DT1 tile rendering pipeline.
 *
 *	Types come directly from the reference project's headers.
 */

#include <allegro5/allegro.h>

extern "C" {
	#include "../../Engine/EditorCompat/structs.h"
	#include "../../Engine/EditorCompat/rgba_cache.h"
	#include "../../Engine/EditorCompat/palette.h"

	void editor_bridge_init(const char *basePath);
	void editor_bridge_shutdown(void);
	int editor_bridge_load_ds1(const char *ds1RelativePath);
	int dt1_add(char *dt1name);
}

// Shared editor bridge initialization guard
void EnsureEditorBridgeInit();

// Update the editor's palette from OpenD2's engine palette for the given act
void EditorBridge_SetPalette(int act);
