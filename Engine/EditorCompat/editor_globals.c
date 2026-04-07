/*
 *	Editor Global State and Bridge
 *	Defines editor globals and provides initialization + filesystem bridge.
 *	Uses native Allegro 5 types from d2-ds1-edit (reference project).
 *
 *	This is the ONLY OpenD2-specific file in EditorCompat.
 *	All other files are direct copies from reference/d2-ds1-edit/Sources/.
 */

#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Global variable definitions (referenced as extern in structs.h)
// ============================================================================

GLB_DS1EDIT_S glb_ds1edit;
CONFIG_S glb_config;
DS1_S *glb_ds1 = NULL;
DT1_S *glb_dt1 = NULL;

char glb_ds1edit_data_dir[80] = "";
char glb_ds1edit_tmp_dir[80] = "";
char glb_tiles_path[30] = "";
GAMMA_S glb_gamma_str[GC_MAX];
WRKSPC_DATAS_S glb_wrkspc_datas[WRKSPC_MAX];

// TXT file path globals
char *glb_path_lvltypes_mem = NULL;
char *glb_path_lvltypes_def = NULL;
char *glb_path_lvlprest_mem = NULL;
char *glb_path_lvlprest_def = NULL;
char *glb_path_obj_mem = NULL;
char *glb_path_obj_def = NULL;
char *glb_path_objects_mem = NULL;
char *glb_path_objects_def = NULL;
char **glb_txt_req_ptr[RQ_MAX] = {NULL};

// MPQ globals (glb_mpq defined in MpqView.c, glb_mpq_struct defined here)
GLB_MPQ_S glb_mpq_struct[MAX_MPQ_FILE];

// Base path for filesystem bridge
char editor_bridge_basepath[512] = "";

// ============================================================================
// Initialization
// ============================================================================

void editor_bridge_set_basepath(const char *basePath)
{
	strncpy(editor_bridge_basepath, basePath, sizeof(editor_bridge_basepath) - 1);
	editor_bridge_basepath[sizeof(editor_bridge_basepath) - 1] = '\0';

	size_t len = strlen(editor_bridge_basepath);
	if (len > 0 && editor_bridge_basepath[len - 1] != '\\' && editor_bridge_basepath[len - 1] != '/')
	{
		if (len < sizeof(editor_bridge_basepath) - 1)
		{
			editor_bridge_basepath[len] = '\\';
			editor_bridge_basepath[len + 1] = '\0';
		}
	}
}

void editor_bridge_init(const char *basePath)
{
	editor_bridge_set_basepath(basePath);

	memset(&glb_ds1edit, 0, sizeof(glb_ds1edit));
	memset(&glb_config, 0, sizeof(glb_config));
	memset(glb_mpq_struct, 0, sizeof(glb_mpq_struct));

	if (glb_ds1 == NULL)
		glb_ds1 = (DS1_S *)calloc(DS1_MAX, sizeof(DS1_S));
	if (glb_dt1 == NULL)
		glb_dt1 = (DT1_S *)calloc(DT1_MAX, sizeof(DT1_S));

	// Set mod_dir[0] to basepath so misc_load_mpq_file() can find DT1 files.
	// mod_load_in_mem() constructs "mod_dir\filename", so basepath needs no
	// trailing separator (editor_bridge_basepath has one, strip it).
	{
		char *dir = _strdup(editor_bridge_basepath);
		size_t len = strlen(dir);
		while (len > 0 && (dir[len - 1] == '\\' || dir[len - 1] == '/'))
			dir[--len] = '\0';
		glb_config.mod_dir[0] = dir;
	}

	// Connect to OpenD2's Allegro 5 display so bitmap creation works
	a5_display = al_get_current_display();

	// Force memory bitmaps for the editor's tile rendering.
	// dt1_all_zoom_make() calls a5_putpixel() per-pixel — if the bitmaps
	// are GPU (video) bitmaps, this is extremely slow due to per-pixel
	// target switches. Memory bitmaps make pixel ops fast.
	al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
}

void editor_bridge_shutdown(void)
{
	if (glb_ds1)
	{
		free(glb_ds1);
		glb_ds1 = NULL;
	}
	if (glb_dt1)
	{
		free(glb_dt1);
		glb_dt1 = NULL;
	}
}

// ============================================================================
// Load a DS1 map using the editor's pipeline
// ============================================================================

int editor_bridge_load_ds1(const char *ds1RelativePath)
{
	int ds1_idx = 0;
	unsigned char *buf;
	unsigned long *readHead;
	unsigned long version, width, height, act, tagType;
	unsigned long fileCount;
	unsigned int fi;
	int numDT1sLoaded = 0;
	long fileSize;
	FILE *f;
	char ds1FullPath[512];

	snprintf(ds1FullPath, sizeof(ds1FullPath), "%sdata\\global\\tiles\\%s",
		editor_bridge_basepath, ds1RelativePath);

	{
		char *p;
		for (p = ds1FullPath; *p; p++)
			if (*p == '/') *p = '\\';
	}

	f = fopen(ds1FullPath, "rb");
	if (f == NULL)
		return -1;

	fseek(f, 0, SEEK_END);
	fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (fileSize <= 0)
	{
		fclose(f);
		return -1;
	}

	buf = (unsigned char *)malloc(fileSize);
	fread(buf, 1, fileSize, f);
	fclose(f);

	readHead = (unsigned long *)buf;
	version = *readHead++;
	width = (*readHead++) + 1;
	height = (*readHead++) + 1;
	act = 1;

	if (version >= 8)
	{
		act = *readHead++;
		if (act > 5) act = 5;
	}

	tagType = 0;
	if (version >= 10)
	{
		tagType = *readHead++;
	}
	(void)tagType;

	memset(&glb_ds1[ds1_idx], 0, sizeof(DS1_S));
	glb_ds1[ds1_idx].act = (int)act + 1;
	glb_ds1[ds1_idx].width = (int)width;
	glb_ds1[ds1_idx].height = (int)height;
	glb_ds1[ds1_idx].version = (int)version;

	if (version >= 3)
	{
		fileCount = *readHead++;

		for (fi = 0; fi < fileCount; fi++)
		{
			char *fname = (char *)readHead;
			size_t n = strlen(fname) + 1;
			size_t flen;
			const char *ext;

			readHead = (unsigned long *)(((char *)readHead) + n);

			flen = strlen(fname);
			if (flen < 4) continue;

			ext = fname + flen - 4;
			if (_stricmp(ext, ".dt1") != 0) continue;

			{
				int dt1_idx = dt1_add(fname);
				if (dt1_idx >= 0)
				{
					if (fi < DT1_IN_DS1_MAX)
						glb_ds1[ds1_idx].dt1_idx[fi] = dt1_idx;
					numDT1sLoaded++;
				}
			}
		}
	}

	if (numDT1sLoaded > 0)
	{
		misc_make_block_table(ds1_idx);
	}

	free(buf);
	return ds1_idx;
}
