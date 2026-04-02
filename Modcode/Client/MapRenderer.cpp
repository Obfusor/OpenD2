#include "MapRenderer.hpp"
#include "D2Client.hpp"
#include <cstring>
#include <cstdio>

#ifdef USE_ALLEGRO5
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

static ALLEGRO_FONT *s_pFont = nullptr;
static void EnsureFont()
{
	if (s_pFont == nullptr)
		s_pFont = al_create_builtin_font();
}
#endif

MapRenderer *gpMapRenderer = nullptr;

MapRenderer::MapRenderer()
	: m_ds1Handle(INVALID_HANDLE), m_mapWidth(0), m_mapHeight(0), m_act(1),
	  m_cameraX(0), m_cameraY(0)
{
}

MapRenderer::~MapRenderer()
{
	UnloadMap();
}

void MapRenderer::UnloadMap()
{
#ifdef USE_ALLEGRO5
	for (auto &pair : m_tileCache)
	{
		if (pair.second)
			al_destroy_bitmap(pair.second);
	}
	m_tileCache.clear();
#endif

	m_tileLookup.clear();
	m_loadedDT1s.clear();
	m_ds1Handle = INVALID_HANDLE;
	m_mapWidth = 0;
	m_mapHeight = 0;
}

bool MapRenderer::LoadMap(const char *relativePath)
{
	UnloadMap();

	// Build full data path
	char ds1Path[MAX_D2PATH];
	snprintf(ds1Path, MAX_D2PATH, "data\\global\\tiles\\%s", relativePath);

	// Normalize slashes
	for (char *p = ds1Path; *p; p++)
		if (*p == '/') *p = '\\';

	engine->Print(PRIORITY_MESSAGE, "MapRenderer: Loading %s", ds1Path);

	m_ds1Handle = engine->DS1_Load(ds1Path);
	if (m_ds1Handle == INVALID_HANDLE)
	{
		engine->Print(PRIORITY_MESSAGE, "MapRenderer: Failed to load DS1");
		return false;
	}

	engine->DS1_GetSize(m_ds1Handle, m_mapWidth, m_mapHeight);
	m_act = engine->DS1_GetAct(m_ds1Handle);

	engine->Print(PRIORITY_MESSAGE, "MapRenderer: Map size %dx%d, Act %d", m_mapWidth, m_mapHeight, m_act);

	// Set palette for this act
	if (m_act >= 1 && m_act <= 5)
		engine->renderer->SetGlobalPalette((D2Palettes)(m_act - 1));

	// Load DT1 files referenced by this DS1
	LoadDT1sFromDS1();

	// Build lookup from tile properties to DT1 blocks
	BuildTileLookup();

	// Center camera on the map
	CenterCamera();

	return true;
}

void MapRenderer::LoadDT1sFromDS1()
{
	DWORD fileCount = engine->DS1_GetFileCount(m_ds1Handle);
	engine->Print(PRIORITY_MESSAGE, "MapRenderer: DS1 has %d file references", fileCount);

	for (DWORD i = 0; i < fileCount; i++)
	{
		const char *filename = engine->DS1_GetFileName(m_ds1Handle, i);
		if (filename == nullptr || filename[0] == '\0')
			continue;

		// Skip non-DT1 files (some DS1s reference .tgl files)
		size_t len = strlen(filename);
		if (len < 4)
			continue;
		const char *ext = filename + len - 4;
		if (_stricmp(ext, ".dt1") != 0 && _stricmp(ext, ".tgl") != 0)
			continue;

		// Build full path - DS1 files embed paths in various formats:
		//   "\d2\data\global\tiles\expansion\baallair\floor.dt1"
		//   "data\global\tiles\act3\jungle\pygmy.tg1"
		// We need to normalize to "data\global\tiles\..."
		char dt1Path[MAX_D2PATH];
		const char *tilesPos = nullptr;

		// Look for "data\global\tiles\" or "data/global/tiles/" in the filename
		// (case insensitive, handle both slash types)
		for (const char *p = filename; *p; p++)
		{
			if ((*p == 'd' || *p == 'D') &&
				_strnicmp(p, "data", 4) == 0)
			{
				// Check if this is followed by \global\tiles or /global/tiles
				const char *q = p + 4;
				if (*q == '\\' || *q == '/')
				{
					tilesPos = p;
					break;
				}
			}
		}

		if (tilesPos)
		{
			snprintf(dt1Path, MAX_D2PATH, "%s", tilesPos);
		}
		else
		{
			// No recognizable prefix — try as-is under tiles/
			snprintf(dt1Path, MAX_D2PATH, "data\\global\\tiles\\%s", filename);
		}

		// Normalize slashes
		for (char *p = dt1Path; *p; p++)
			if (*p == '/') *p = '\\';

		handle dt1 = engine->DT1_Load(dt1Path);
		if (dt1 != INVALID_HANDLE)
		{
			m_loadedDT1s.push_back(dt1);
			engine->Print(PRIORITY_MESSAGE, "MapRenderer: Loaded DT1 %s (%d blocks)",
				filename, engine->DT1_GetNumBlocks(dt1));
		}
	}

	engine->Print(PRIORITY_MESSAGE, "MapRenderer: Loaded %d DT1 files", (int)m_loadedDT1s.size());
}

void MapRenderer::BuildTileLookup()
{
	m_tileLookup.clear();

	for (auto &dt1 : m_loadedDT1s)
	{
		DWORD numBlocks = engine->DT1_GetNumBlocks(dt1);
		for (DWORD b = 0; b < numBlocks; b++)
		{
			DT1BlockInfo info;
			if (!engine->DT1_GetBlockInfo(dt1, b, &info))
				continue;

			TileKey key = {info.orientation, info.mainIndex, info.subIndex};
			TileEntry entry = {dt1, (int)b};
			m_tileLookup[key].push_back(entry);
		}
	}

	engine->Print(PRIORITY_MESSAGE, "MapRenderer: Built tile lookup with %d unique keys",
		(int)m_tileLookup.size());
}

const TileEntry *MapRenderer::FindTile(long orientation, long mainIndex, long subIndex)
{
	TileKey key = {orientation, mainIndex, subIndex};
	auto it = m_tileLookup.find(key);
	if (it == m_tileLookup.end() || it->second.empty())
		return nullptr;
	return &it->second[0];
}

#ifdef USE_ALLEGRO5
ALLEGRO_BITMAP *MapRenderer::DecodeTileBitmap(handle dt1, int blockIndex, int act)
{
	uint64_t cacheKey = (uint64_t)(uint32_t)dt1 * 100000 + blockIndex;
	auto it = m_tileCache.find(cacheKey);
	if (it != m_tileCache.end())
		return it->second;

	uint32_t w, h;
	int32_t ox, oy;
	void *pixels = engine->DT1_DecodeBlock(dt1, blockIndex, w, h, ox, oy);
	if (pixels == nullptr || w == 0 || h == 0)
	{
		m_tileCache[cacheKey] = nullptr;
		return nullptr;
	}

	// Sanity check dimensions (prevent absurd allocations)
	if (w > 1024 || h > 1024)
	{
		m_tileCache[cacheKey] = nullptr;
		return nullptr;
	}

	// Get the palette for this act
	// Note: pixels points to DT1File's internal buffer — do NOT free it
	D2Palette *pal = engine->PAL_GetPalette(act < 5 ? act : 0);

	// Create an ALLEGRO_BITMAP and convert palette indices to RGBA
	ALLEGRO_BITMAP *bmp = al_create_bitmap(w, h);
	if (bmp == nullptr)
	{
		m_tileCache[cacheKey] = nullptr;
		return nullptr;
	}

	ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY);
	if (lr)
	{
		BYTE *src = (BYTE *)pixels;
		for (uint32_t y = 0; y < h; y++)
		{
			uint32_t *dst = (uint32_t *)((char *)lr->data + y * lr->pitch);
			for (uint32_t x = 0; x < w; x++)
			{
				BYTE idx = src[y * w + x];
				if (idx == 0)
				{
					// Transparent
					dst[x] = 0x00000000;
				}
				else if (pal)
				{
					BYTE r = (*pal)[idx][0];
					BYTE g = (*pal)[idx][1];
					BYTE b = (*pal)[idx][2];
					dst[x] = 0xFF000000 | (b << 16) | (g << 8) | r; // ABGR
				}
				else
				{
					// No palette: grayscale fallback
					dst[x] = 0xFF000000 | (idx << 16) | (idx << 8) | idx;
				}
			}
		}
		al_unlock_bitmap(bmp);
	}

	// Note: do NOT free(pixels) — it's DT1File's internal reusable buffer
	m_tileCache[cacheKey] = bmp;
	return bmp;
}
#endif

void MapRenderer::CenterCamera()
{
	float centerIsoX = (float)(m_mapWidth / 2 - m_mapHeight / 2) * HALF_W;
	float centerIsoY = (float)(m_mapWidth / 2 + m_mapHeight / 2) * HALF_H;
	m_cameraX = centerIsoX - SCREEN_W / 2.0f;
	m_cameraY = centerIsoY - SCREEN_H / 2.0f;
}

void MapRenderer::ScrollCamera(float dx, float dy)
{
	m_cameraX += dx;
	m_cameraY += dy;
}

bool MapRenderer::HandleKeyDown(DWORD keyCode)
{
	switch (keyCode)
	{
	case B_LEFTARROW:
		ScrollCamera(-SCROLL_SPEED * 4, 0);
		return true;
	case B_RIGHTARROW:
		ScrollCamera(SCROLL_SPEED * 4, 0);
		return true;
	case B_UPARROW:
	case 'w':
	case 'W':
		ScrollCamera(0, -SCROLL_SPEED * 4);
		return true;
	case B_DOWNARROW:
	case 's':
	case 'S':
		ScrollCamera(0, SCROLL_SPEED * 4);
		return true;
	case B_HOME:
		CenterCamera();
		return true;
	case 27: // Escape - go back to map selector
		return false; // Let caller handle
	}
	return false;
}

void MapRenderer::Draw()
{
#ifdef USE_ALLEGRO5
	engine->renderer->Clear();

	if (m_ds1Handle == INVALID_HANDLE || m_mapWidth == 0 || m_mapHeight == 0)
		return;

	int act = (int)m_act - 1;
	if (act < 0) act = 0;
	if (act > 4) act = 4;

	// Render floor tiles
	for (int ty = 0; ty < m_mapHeight; ty++)
	{
		for (int tx = 0; tx < m_mapWidth; tx++)
		{
			float sx = (float)(tx - ty) * HALF_W - m_cameraX;
			float sy = (float)(tx + ty) * HALF_H - m_cameraY;

			// Frustum cull
			if (sx + TILE_W < 0 || sx > SCREEN_W || sy + TILE_H < 0 || sy > SCREEN_H + 200)
				continue;

			DS1Cell *cell = engine->DS1_GetCellAt(m_ds1Handle, tx, ty, DS1Cell_Floor);
			if (cell == nullptr)
				continue;
			if (cell->prop1 == 0 && cell->prop2 == 0 && cell->prop3 == 0 && cell->prop4 == 0)
				continue;

			long mainIndex = ((cell->prop4 & 0x03) << 4) | (cell->prop3 >> 4);
			long subIndex = cell->prop2;

			const TileEntry *entry = FindTile(0, mainIndex, subIndex);
			if (entry == nullptr)
				continue;

			ALLEGRO_BITMAP *tileBmp = DecodeTileBitmap(entry->dt1Handle, entry->blockIndex, act);
			if (tileBmp)
			{
				al_draw_bitmap(tileBmp, sx, sy, 0);
			}
		}
	}

	// Render wall tiles
	for (int ty = 0; ty < m_mapHeight; ty++)
	{
		for (int tx = 0; tx < m_mapWidth; tx++)
		{
			float sx = (float)(tx - ty) * HALF_W - m_cameraX;
			float sy = (float)(tx + ty) * HALF_H - m_cameraY;

			if (sx + TILE_W < 0 || sx > SCREEN_W || sy + 200 < 0 || sy > SCREEN_H + 200)
				continue;

			DS1Cell *cell = engine->DS1_GetCellAt(m_ds1Handle, tx, ty, DS1Cell_Wall);
			if (cell == nullptr)
				continue;
			if (cell->prop1 == 0 && cell->prop2 == 0 && cell->prop3 == 0 && cell->prop4 == 0)
				continue;

			long orientation = cell->orientation;
			if (orientation == 0)
				continue;

			long mainIndex = ((cell->prop4 & 0x03) << 4) | (cell->prop3 >> 4);
			long subIndex = cell->prop2;

			const TileEntry *entry = FindTile(orientation, mainIndex, subIndex);
			if (entry == nullptr)
				continue;

			ALLEGRO_BITMAP *tileBmp = DecodeTileBitmap(entry->dt1Handle, entry->blockIndex, act);
			if (tileBmp)
			{
				// Walls draw at same X but shifted down by TILE_H
				al_draw_bitmap(tileBmp, sx, sy + TILE_H, 0);
			}
		}
	}

	// HUD overlay
	EnsureFont();
	if (s_pFont)
	{
		// Dark bar at top
		al_draw_filled_rectangle(0, 0, (float)SCREEN_W, 24,
			al_map_rgba(0, 0, 0, 180));

		char info[256];
		snprintf(info, sizeof(info),
			"Map: %dx%d | Act %d | DT1s: %d | Tiles: %d | Arrows/WASD: Scroll | Home: Center | Esc: Back",
			m_mapWidth, m_mapHeight, m_act,
			(int)m_loadedDT1s.size(), (int)m_tileLookup.size());
		al_draw_text(s_pFont, al_map_rgb(200, 180, 120), 10, 6, ALLEGRO_ALIGN_LEFT, info);
	}

	engine->renderer->Present();
#endif
}
