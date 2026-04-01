#ifdef _DEBUG
#include "../Common/D2Common.hpp"
#include "D2Client.hpp"
#include <map>
#include <vector>

namespace Debug
{
	int currentLevel = 1; // rogue encampment
	IRenderObject *text = nullptr;

	//--- Camera state ---
	float cameraX = 0.0f;
	float cameraY = 0.0f;
	static const float SCROLL_SPEED = 16.0f;
	static const int SCREEN_W = 800;
	static const int SCREEN_H = 600;

	//--- Tile constants (Diablo 2 isometric) ---
	static const int TILE_WIDTH = 160;		   // floor tile pixel width
	static const int TILE_HEIGHT = 80;		   // floor tile pixel height
	static const int HALF_W = TILE_WIDTH / 2;  // 80
	static const int HALF_H = TILE_HEIGHT / 2; // 40

	//--- Level state ---
	handle ds1Handle = INVALID_HANDLE;
	int32_t mapWidth = 0;
	int32_t mapHeight = 0;

	//--- DT1 tile lookup ---
	struct TileKey
	{
		long orientation;
		long mainIndex;
		long subIndex;
		bool operator<(const TileKey &o) const
		{
			if (orientation != o.orientation)
				return orientation < o.orientation;
			if (mainIndex != o.mainIndex)
				return mainIndex < o.mainIndex;
			return subIndex < o.subIndex;
		}
	};

	struct TileEntry
	{
		handle dt1Handle;
		int blockIndex;
	};

	std::vector<handle> loadedDT1Handles;
	std::map<TileKey, std::vector<TileEntry>> tileLookup;

	//--- Decoded tile cache ---
	struct TileTexture
	{
		IRenderObject *renderObj;
		int width;
		int height;
	};

	// Cache key: dt1Handle * 10000 + blockIndex
	std::map<uint64_t, TileTexture> tileTextureCache;

	uint64_t MakeCacheKey(handle dt1, int block)
	{
		return (uint64_t)(uint32_t)dt1 * 10000 + block;
	}

	void FreeTileTextures()
	{
		for (auto &pair : tileTextureCache)
		{
			if (pair.second.renderObj)
			{
				engine->renderer->Remove(pair.second.renderObj);
			}
		}
		tileTextureCache.clear();
	}

	void UnloadCurrentWorld()
	{
		FreeTileTextures();
		tileLookup.clear();

		for (auto &h : loadedDT1Handles)
		{
			// DT1 handles are managed by the engine's HashMap, no explicit free needed
		}
		loadedDT1Handles.clear();

		ds1Handle = INVALID_HANDLE;
		mapWidth = 0;
		mapHeight = 0;
	}

	void LoadDT1sForLevel(int nLevelId)
	{
		if (sgptDataTables == nullptr)
			return;

		D2LevelDefBin *levelDef = &sgptDataTables->pLevelDefBin[nLevelId];
		DWORD levelType = levelDef->dwLevelType;

		if (sgptDataTables->pLvlTypesTxt == nullptr ||
			(int)levelType >= sgptDataTables->nLvlTypesTxtRecordCount)
			return;

		D2LvlTypesTxt *lvlType = &sgptDataTables->pLvlTypesTxt[levelType];

		// Get the DT1 mask from LvlPrest if available
		DWORD dt1Mask = 0xFFFFFFFF; // load all by default
		D2LvlPrestTxt *prest = D2Common_GetLvlPrestForLevel(nLevelId);
		if (prest != nullptr)
		{
			dt1Mask = prest->dwDt1Mask;
		}

		// Load each DT1 file referenced by the level type
		for (int i = 0; i < 32; i++)
		{
			// Check dt1Mask bit
			if (!(dt1Mask & (1u << i)))
				continue;

			if (lvlType->szFile[i][0] == '\0')
				continue;

			char path[MAX_D2PATH];
			snprintf(path, MAX_D2PATH, "data\\global\\tiles\\%s", lvlType->szFile[i]);

			handle dt1 = engine->DT1_Load(path);
			if (dt1 == INVALID_HANDLE)
				continue;

			loadedDT1Handles.push_back(dt1);

			// Index all blocks in this DT1
			DWORD numBlocks = engine->DT1_GetNumBlocks(dt1);
			for (DWORD b = 0; b < numBlocks; b++)
			{
				DT1BlockInfo info;
				if (!engine->DT1_GetBlockInfo(dt1, b, &info))
					continue;

				TileKey key = {info.orientation, info.mainIndex, info.subIndex};
				TileEntry entry = {dt1, (int)b};
				tileLookup[key].push_back(entry);
			}
		}
	}

	const TileEntry *FindTileEntry(long orientation, long mainIndex, long subIndex)
	{
		TileKey key = {orientation, mainIndex, subIndex};
		auto it = tileLookup.find(key);
		if (it == tileLookup.end() || it->second.empty())
			return nullptr;

		// Pick the first entry (could pick by rarity later)
		return &it->second[0];
	}

	void LoadWorld()
	{
		char levelName[128];
		char16_t unicodeLevelName[128];
		int seed = rand();
		UnloadCurrentWorld();

		// Guard against missing data tables
		if (sgptDataTables == nullptr ||
			sgptDataTables->nLevelsTxtRecordCount == 0 ||
			sgptDataTables->pLevelsTxt == nullptr ||
			sgptDataTables->pLevelDefBin == nullptr)
		{
			return;
		}

		if (currentLevel >= sgptDataTables->nLevelsTxtRecordCount)
		{
			currentLevel = 1;
		}

		// Load DT1 files for this level type
		LoadDT1sForLevel(currentLevel);

		// Construct the level (loads DS1)
		D2Common_ConstructSingleLevel(currentLevel, seed);

		// Now load the DS1 ourselves to get the handle
		D2LvlPrestTxt *prest = D2Common_GetLvlPrestForLevel(currentLevel);
		if (prest != nullptr && prest->dwFiles > 0)
		{
			// Pick the first DS1 file
			const char *ds1Name = prest->szFile[0];
			if (ds1Name[0] != '\0')
			{
				char ds1Path[MAX_D2PATH];
				snprintf(ds1Path, MAX_D2PATH, "data\\global\\tiles\\%s", ds1Name);
				ds1Handle = engine->DS1_Load(ds1Path);
				if (ds1Handle != INVALID_HANDLE)
				{
					engine->DS1_GetSize(ds1Handle, mapWidth, mapHeight);
				}
			}
		}

		// Center camera on the isometric center of the map
		float centerIsoX = (float)(mapWidth / 2 - mapHeight / 2) * HALF_W;
		float centerIsoY = (float)(mapWidth / 2 + mapHeight / 2) * HALF_H;
		cameraX = centerIsoX - SCREEN_W / 2.0f;
		cameraY = centerIsoY - SCREEN_H / 2.0f;

		// Set the palette for the act
		D2LevelsTxt *lvl = &sgptDataTables->pLevelsTxt[currentLevel];
		engine->renderer->SetGlobalPalette((D2Palettes)lvl->nPal);

		if (text == nullptr)
		{
			text = engine->renderer->AllocateObject(0);
			text->AttachFontResource(cl.font16);
			text->SetTextColor(TextColor_Gold);
			text->SetDrawCoords(10, 10, 0, 0);
		}

		D2Lib::strncpyz(levelName, sgptDataTables->pLevelsTxt[currentLevel].szLevelName, 128);
		D2Lib::qmbtowc(unicodeLevelName, 128, levelName);
		text->SetText(unicodeLevelName);
	}

	// Convert tile coordinates to screen pixel coordinates (isometric projection)
	inline void TileToScreen(int tileX, int tileY, float &screenX, float &screenY)
	{
		screenX = (float)(tileX - tileY) * HALF_W - cameraX;
		screenY = (float)(tileX + tileY) * HALF_H - cameraY;
	}

	// Decode a DS1 cell's prop bytes into mainIndex and subIndex
	// D2 encoding: DWORD = prop1 | (prop2 << 8) | (prop3 << 16) | (prop4 << 24)
	// mainIndex = bits 20-25, subIndex = bits 8-15
	inline void DecodeCellProps(const DS1Cell *cell, long &mainIndex, long &subIndex)
	{
		mainIndex = ((cell->prop4 & 0x03) << 4) | (cell->prop3 >> 4);
		subIndex = cell->prop2;
	}

	void DrawWorld()
	{
		static float black[] = {0.0f, 0.0f, 0.0f, 1.0f};

		// Blank out the background
		engine->renderer->Clear();
		engine->renderer->DrawRectangle(0, 0, (float)SCREEN_W, (float)SCREEN_H, 0, nullptr, black);

		if (ds1Handle != INVALID_HANDLE && mapWidth > 0 && mapHeight > 0)
		{
			// Calculate visible tile range with margin
			// Rough inverse: for each screen corner, estimate which tiles might be visible
			int margin = 3;

			for (int ty = 0; ty < mapHeight; ty++)
			{
				for (int tx = 0; tx < mapWidth; tx++)
				{
					float sx, sy;
					TileToScreen(tx, ty, sx, sy);

					// Skip tiles that are off-screen
					if (sx + TILE_WIDTH < 0 || sx > SCREEN_W ||
						sy + TILE_HEIGHT < 0 || sy > SCREEN_H + 200)
					{
						continue;
					}

					// Get floor cell for layer 0
					DS1Cell *floorCell = engine->DS1_GetCellAt(ds1Handle, tx, ty, DS1Cell_Floor);
					if (floorCell == nullptr)
						continue;

					// Skip empty cells
					if (floorCell->prop1 == 0 && floorCell->prop2 == 0 &&
						floorCell->prop3 == 0 && floorCell->prop4 == 0)
						continue;

					long mainIndex, subIndex;
					DecodeCellProps(floorCell, mainIndex, subIndex);

					// Find matching DT1 block (orientation 0 for floors)
					const TileEntry *entry = FindTileEntry(0, mainIndex, subIndex);
					if (entry == nullptr)
						continue;

					uint64_t cacheKey = MakeCacheKey(entry->dt1Handle, entry->blockIndex);
					auto cacheIt = tileTextureCache.find(cacheKey);

					if (cacheIt == tileTextureCache.end())
					{
						// Decode the DT1 block
						uint32_t bw, bh;
						int32_t bx, by;
						void *pixels = engine->DT1_DecodeBlock(entry->dt1Handle, entry->blockIndex, bw, bh, bx, by);
						if (pixels == nullptr || bw == 0 || bh == 0)
							continue;

						// Create a render object for this tile
						IRenderObject *tileObj = engine->renderer->AllocateObject(0);
						if (tileObj == nullptr)
							continue;

						// Attach the decoded bitmap as a texture
						// We need to use CreateReference for proper texture loading
						// Instead, create a temporary IGraphicsReference... but we can't from modcode.
						// Use the raw DrawRectangle approach with colored tiles as fallback
						// Actually, we'll draw colored diamonds to represent tiles
						engine->renderer->Remove(tileObj);

						// Store a placeholder
						TileTexture tex = {nullptr, (int)bw, (int)bh};
						tileTextureCache[cacheKey] = tex;
						cacheIt = tileTextureCache.find(cacheKey);
					}

					// Draw a colored diamond to represent the floor tile
					// Color based on mainIndex for variety
					float r = ((mainIndex * 37) % 128 + 64) / 255.0f;
					float g = ((mainIndex * 53 + subIndex * 17) % 128 + 80) / 255.0f;
					float b = ((subIndex * 71) % 100 + 40) / 255.0f;
					float tileColor[] = {r, g, b, 1.0f};

					// Draw the isometric diamond as a small rectangle for now
					// A proper diamond would need triangle rendering
					float diamondW = (float)TILE_WIDTH * 0.6f;
					float diamondH = (float)TILE_HEIGHT * 0.6f;
					float drawX = sx + (TILE_WIDTH - diamondW) / 2.0f;
					float drawY = sy + (TILE_HEIGHT - diamondH) / 2.0f;
					engine->renderer->DrawRectangle(drawX, drawY, diamondW, diamondH, 0, nullptr, tileColor);
				}
			}

			// Draw wall tiles on top
			for (int ty = 0; ty < mapHeight; ty++)
			{
				for (int tx = 0; tx < mapWidth; tx++)
				{
					float sx, sy;
					TileToScreen(tx, ty, sx, sy);

					if (sx + TILE_WIDTH < 0 || sx > SCREEN_W ||
						sy + 200 < 0 || sy > SCREEN_H + 200)
					{
						continue;
					}

					DS1Cell *wallCell = engine->DS1_GetCellAt(ds1Handle, tx, ty, DS1Cell_Wall);
					if (wallCell == nullptr)
						continue;

					if (wallCell->prop1 == 0 && wallCell->prop2 == 0 &&
						wallCell->prop3 == 0 && wallCell->prop4 == 0)
						continue;

					long mainIndex, subIndex;
					DecodeCellProps(wallCell, mainIndex, subIndex);
					long orientation = wallCell->orientation;

					if (orientation == 0)
						continue; // skip floor-oriented walls

					const TileEntry *entry = FindTileEntry(orientation, mainIndex, subIndex);
					if (entry == nullptr)
						continue;

					// Draw walls as darker/taller rectangles
					float wallColor[] = {0.3f, 0.25f, 0.2f, 0.85f};
					float wallH = 40.0f;
					engine->renderer->DrawRectangle(sx + 60, sy - wallH, 40, wallH + 20, 0, nullptr, wallColor);
				}
			}
		}

		// Draw level name on top
		if (text != nullptr)
		{
			text->Draw();
		}
	}

	bool HandleKeyInput(int keyNum)
	{
		if (keyNum == B_UPARROW)
		{
			if (currentLevel <= 1)
			{
				return true;
			}
			currentLevel--;
			LoadWorld();
			return true;
		}
		else if (keyNum == B_DOWNARROW)
		{
			if (sgptDataTables == nullptr ||
				currentLevel >= sgptDataTables->nLevelsTxtRecordCount - 1)
			{
				return true;
			}
			currentLevel++;
			LoadWorld();
			return true;
		}
		else if (keyNum == B_LEFTARROW)
		{
			cameraX -= SCROLL_SPEED * 4;
			return true;
		}
		else if (keyNum == B_RIGHTARROW)
		{
			cameraX += SCROLL_SPEED * 4;
			return true;
		}
		else if (keyNum == 'w' || keyNum == 'W')
		{
			cameraY -= SCROLL_SPEED * 4;
			return true;
		}
		else if (keyNum == 's' || keyNum == 'S')
		{
			cameraY += SCROLL_SPEED * 4;
			return true;
		}
		return false;
	}
}
#endif