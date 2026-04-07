#pragma once

/*
 *	Shared declarations for the d2-ds1-edit editor bridge.
 *	Used by both MapRenderer and D2World to access the editor's
 *	DT1 tile rendering pipeline.
 */

#include <allegro5/allegro.h>

// Editor bridge functions (C linkage)
extern "C" {
	void editor_bridge_init(const char *basePath);
	void editor_bridge_shutdown(void);
	int editor_bridge_load_ds1(const char *ds1RelativePath);

	// Allegro 4 BITMAP struct (from allegro4/include/allegro/gfx.h)
	// Must match the actual memory layout exactly for field access.
	typedef struct {
		int w, h;                   // +0, +4: width and height
		int clip;                   // +8: clipping flag
		int cl, cr, ct, cb;         // +12..+24: clip rect
		void *vtable;               // +28: GFX_VTABLE*
		void *write_bank;           // +32
		void *read_bank;            // +36
		void *dat;                  // +40: pixel data
		unsigned long id;           // +44
		void *extra;                // +48
		int x_ofs, y_ofs;           // +52, +56
		int seg;                    // +60
		unsigned char *line[1];     // +64: flexible array of row pointers
	} BITMAP_A4;

	// DT1 file structure from the editor globals.
	// block_zoom[0][blockIndex] gives the 1:1 pre-rendered bitmap.
	typedef struct {
		int ds1_usage;
		char name[80];
		void *buffer;
		long buff_len;
		long x1, x2;
		long block_num;
		long bh_start;
		void *bh_buffer;
		int bh_buff_len;
		BITMAP_A4 **block_zoom[5]; // ZM_MAX = 5
		int bz_size[5];
	} DT1_S_EXT;

	// Block header from DT1 (96 bytes per block in the raw file).
	// Only the fields we read are defined here.
	typedef struct {
		long direction;
		long roof_y;
		short sound;
		char animated;
		long size_y;
		long size_x;
		long zeros1;
		long orientation;
		long main_index;
		long sub_index;
		long rarity;
	} BLOCK_S_EXT;

	// Global DT1 array from the editor (300 entries max)
	extern DT1_S_EXT *glb_dt1;

	// DS1 globals (forward-declared, not needed for tile rendering)
	typedef struct DS1_S_EXT DS1_S_EXT;
	extern DS1_S_EXT *glb_ds1;
}

// Shared editor bridge initialization guard
void EnsureEditorBridgeInit();
