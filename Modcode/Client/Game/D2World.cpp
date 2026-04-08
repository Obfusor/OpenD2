#include "D2World.hpp"
#include "D2Game.hpp"
#include <allegro5/allegro_primitives.h>

D2ClientWorld *gpWorld = nullptr;

// Class token names (same as CharacterScreen)
static const char *g_szClassTokens[D2CLASS_MAX] = {
    "AM", "SO", "NE", "PA", "BA", "DZ", "AI"};

D2ClientWorld::D2ClientWorld()
    : m_nCurrentLevel(0),
      m_ds1Handle(INVALID_HANDLE),
      m_mapWidth(0),
      m_mapHeight(0),
      m_cameraX(0.0f),
      m_cameraY(0.0f),
      m_playerDrawX(-1.0f),
      m_playerDrawY(-1.0f),
      m_debugText(nullptr),
      m_act(0)
{
    m_statusText[0] = '\0';
    m_debugText = engine->renderer->AllocateObject(0);
    if (m_debugText)
    {
        m_debugText->AttachFontResource(cl.font16);
        m_debugText->SetTextColor(TextColor_Gold);
        m_debugText->SetDrawCoords(10, 10, 0, 0);
    }
}

D2ClientWorld::~D2ClientWorld()
{
    Unload();
    if (m_debugText)
    {
        engine->renderer->Remove(m_debugText);
        m_debugText = nullptr;
    }
}

void D2ClientWorld::Unload()
{
    CleanupUnitRenders();

    // Free cached Allegro bitmaps
    for (auto &pair : m_tileCache)
    {
        if (pair.second)
            al_destroy_bitmap(pair.second);
    }
    m_tileCache.clear();
    m_editorTileLookup.clear();

    m_tileLookup.clear();
    m_loadedDT1s.clear();
    m_ds1Handle = INVALID_HANDLE;
    m_mapWidth = 0;
    m_mapHeight = 0;
    m_nCurrentLevel = 0;
    m_act = 0;
}

/*
 *	Build tile lookup from the editor's global DT1 array.
 *	Scans glb_dt1[0..299] for loaded DT1 files and maps
 *	(orientation, mainIndex, subIndex) to (dt1Index, blockIndex).
 */
void D2ClientWorld::BuildEditorTileLookup()
{
    m_editorTileLookup.clear();

    for (int d = 0; d < DT1_MAX; d++)
    {
        DT1_S *dt1 = &glb_dt1[d];
        if (dt1->ds1_usage <= 0 || dt1->bh_buffer == NULL || dt1->block_num <= 0)
            continue;

        BLOCK_S *blocks = (BLOCK_S *)dt1->bh_buffer;

        for (long b = 0; b < dt1->block_num; b++)
        {
            TileKey key = {blocks[b].orientation, blocks[b].main_index, blocks[b].sub_index};
            EditorTileEntry entry = {d, (int)b};
            m_editorTileLookup[key].push_back(entry);
        }
    }
}

const D2ClientWorld::EditorTileEntry *D2ClientWorld::FindEditorTile(
    long orientation, long mainIndex, long subIndex)
{
    TileKey key = {orientation, mainIndex, subIndex};
    auto it = m_editorTileLookup.find(key);
    if (it == m_editorTileLookup.end() || it->second.empty())
        return nullptr;
    return &it->second[0];
}

/*
 *	Decode a DT1 tile block into an ALLEGRO_BITMAP via CACHED_TILE.
 *	Uses the editor's block_cache (palette-indexed + RGBA buffers)
 *	and converts to an Allegro 5 bitmap. Results are cached.
 */
ALLEGRO_BITMAP *D2ClientWorld::DecodeTileBitmap(int dt1Index, int blockIndex, int act)
{
    uint64_t cacheKey = (uint64_t)dt1Index * 100000 + blockIndex;

    auto it = m_tileCache.find(cacheKey);
    if (it != m_tileCache.end())
        return it->second;

    DT1_S *dt1 = &glb_dt1[dt1Index];
    if (dt1->block_cache[0] == NULL || blockIndex >= dt1->block_num)
    {
        m_tileCache[cacheKey] = nullptr;
        return nullptr;
    }

    CACHED_TILE *cached = dt1->block_cache[0][blockIndex];
    if (cached == NULL)
    {
        m_tileCache[cacheKey] = nullptr;
        return nullptr;
    }

    ALLEGRO_BITMAP *bmp = cache_tile_to_a5_bitmap(cached, a5_current_palette);
    m_tileCache[cacheKey] = bmp;
    return bmp;
}

void D2ClientWorld::LoadDT1sForLevel(int nLevelId)
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
    DWORD dt1Mask = 0xFFFFFFFF;
    D2LvlPrestTxt *prest = D2Common_GetLvlPrestForLevel(nLevelId);
    if (prest != nullptr)
    {
        dt1Mask = prest->dwDt1Mask;
    }

    // Load each DT1 file referenced by the level type
    for (int i = 0; i < 32; i++)
    {
        if (!(dt1Mask & (1u << i)))
            continue;

        if (lvlType->szFile[i][0] == '\0')
            continue;

        char path[MAX_D2PATH];
        snprintf(path, MAX_D2PATH, "data\\global\\tiles\\%s", lvlType->szFile[i]);

        handle dt1 = engine->DT1_Load(path);
        if (dt1 == INVALID_HANDLE)
            continue;

        m_loadedDT1s.push_back(dt1);

        // Index all blocks in this DT1
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
}

const D2ClientWorld::TileEntry *D2ClientWorld::FindTileEntry(long orientation, long mainIndex, long subIndex)
{
    TileKey key = {orientation, mainIndex, subIndex};
    auto it = m_tileLookup.find(key);
    if (it == m_tileLookup.end() || it->second.empty())
        return nullptr;

    return &it->second[0];
}

void D2ClientWorld::LoadLevel(int nLevelId)
{
    Unload();

    if (sgptDataTables == nullptr ||
        sgptDataTables->pLevelDefBin == nullptr || nLevelId < 0)
    {
        snprintf(m_statusText, sizeof(m_statusText), "ERROR: No data tables (sgpt=%p, leveldef=%p)",
                 (void *)sgptDataTables,
                 sgptDataTables ? (void *)sgptDataTables->pLevelDefBin : nullptr);
        return;
    }

    if (nLevelId >= sgptDataTables->nLevelsTxtRecordCount)
    {
        snprintf(m_statusText, sizeof(m_statusText), "ERROR: Level %d >= record count %d",
                 nLevelId, sgptDataTables->nLevelsTxtRecordCount);
        return;
    }

    m_nCurrentLevel = nLevelId;

    D2LevelDefBin *levelDef = &sgptDataTables->pLevelDefBin[nLevelId];

    // Load DT1 files for this level type
    LoadDT1sForLevel(nLevelId);

    // Determine if this is a preset level (type 2)
    if (levelDef->dwDrlgType != 2)
    {
        snprintf(m_statusText, sizeof(m_statusText),
                 "Level %d: DrlgType=%d (not preset), DT1s=%d, tiles=%d",
                 nLevelId, (int)levelDef->dwDrlgType,
                 (int)m_loadedDT1s.size(), (int)m_tileLookup.size());
        return;
    }

    // Construct the level (loads DS1 into cache)
    int seed = 0;
    D2Common_ConstructSingleLevel(nLevelId, seed);

    // Load the DS1 to get the handle
    D2LvlPrestTxt *prest = D2Common_GetLvlPrestForLevel(nLevelId);
    if (prest == nullptr)
    {
        snprintf(m_statusText, sizeof(m_statusText),
                 "Level %d: No LvlPrest entry found (count=%d), DT1s=%d",
                 nLevelId, sgptDataTables->nLvlPrestTxtRecordCount,
                 (int)m_loadedDT1s.size());
        return;
    }

    // Pick the first non-empty DS1 file from the preset.
    // Note: dwFiles may read as 0 due to BIN alignment issues,
    // so we check the actual filename strings directly.
    const char *ds1Name = nullptr;
    for (int fi = 0; fi < 6; fi++)
    {
        if (prest->szFile[fi][0] != '\0')
        {
            ds1Name = prest->szFile[fi];
            break;
        }
    }
    if (ds1Name == nullptr)
    {
        snprintf(m_statusText, sizeof(m_statusText),
                 "Level %d: No DS1 filenames found in LvlPrest", nLevelId);
        return;
    }

    char ds1Path[MAX_D2PATH];
    snprintf(ds1Path, MAX_D2PATH, "data\\global\\tiles\\%s", ds1Name);
    m_ds1Handle = engine->DS1_Load(ds1Path);
    if (m_ds1Handle == INVALID_HANDLE)
    {
        snprintf(m_statusText, sizeof(m_statusText),
                 "Level %d: Failed to load DS1 '%s'", nLevelId, ds1Path);
        return;
    }

    engine->DS1_GetSize(m_ds1Handle, m_mapWidth, m_mapHeight);

    // Center camera on the isometric center of the map
    float centerIsoX = (float)(m_mapWidth / 2 - m_mapHeight / 2) * HALF_W;
    float centerIsoY = (float)(m_mapWidth / 2 + m_mapHeight / 2) * HALF_H;
    m_cameraX = centerIsoX - SCREEN_W / 2.0f;
    m_cameraY = centerIsoY - SCREEN_H / 2.0f;

    // Set the palette for the level's act
    if (nLevelId < sgptDataTables->nLevelsTxtRecordCount &&
        sgptDataTables->pLevelsTxt != nullptr)
    {
        D2LevelsTxt *lvl = &sgptDataTables->pLevelsTxt[nLevelId];
        engine->renderer->SetGlobalPalette((D2Palettes)lvl->nPal);
    }

    // Get act number for palette lookup during tile rendering
    m_act = (int)engine->DS1_GetAct(m_ds1Handle) - 1;
    if (m_act < 0) m_act = 0;
    if (m_act > 4) m_act = 4;

    // Load DT1 tile bitmaps via the editor bridge pipeline.
    EnsureEditorBridgeInit();
    EditorBridge_SetPalette(m_act);
    editor_bridge_load_ds1(ds1Name);

    // Load level type DT1s into the editor pipeline
    {
        D2LevelDefBin *ld = &sgptDataTables->pLevelDefBin[nLevelId];
        DWORD lt = ld->dwLevelType;
        D2LvlTypesTxt *ltype = &sgptDataTables->pLvlTypesTxt[lt];
        DWORD mask = 0xFFFFFFFF;
        D2LvlPrestTxt *pr = D2Common_GetLvlPrestForLevel(nLevelId);
        if (pr != nullptr)
            mask = pr->dwDt1Mask;

        for (int i = 0; i < 32; i++)
        {
            if (!(mask & (1u << i)))
                continue;
            if (ltype->szFile[i][0] == '\0')
                continue;

            char dt1Path[MAX_D2PATH];
            snprintf(dt1Path, MAX_D2PATH, "data\\global\\tiles\\%s", ltype->szFile[i]);
            dt1_add(dt1Path);
        }
    }

    // Restore default bitmap flags so subsequent bitmaps (UI, tokens)
    // are GPU-accelerated video bitmaps, not slow memory bitmaps.
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);

    BuildEditorTileLookup();

    const char *levelName = "Unknown";
    if (sgptDataTables->pLevelsTxt != nullptr &&
        nLevelId < sgptDataTables->nLevelsTxtRecordCount)
    {
        levelName = sgptDataTables->pLevelsTxt[nLevelId].szLevelName;
    }

    snprintf(m_statusText, sizeof(m_statusText),
             "%s %dx%d DT1=%d ed=%d act=%d",
             levelName, m_mapWidth, m_mapHeight,
             (int)m_loadedDT1s.size(),
             (int)m_editorTileLookup.size(), m_act);
}

/*
 *	Ensure a render object exists for the given unit.
 *	Creates a token + render object on first call, updates mode on subsequent calls.
 */
void D2ClientWorld::EnsureUnitRender(D2UnitStrc *pUnit)
{
    if (pUnit == nullptr)
        return;

    // Check if we already have a render for this unit
    for (auto &info : m_unitRenders)
    {
        if (info.dwUnitId == pUnit->dwUnitId && info.nUnitType == pUnit->nUnitType)
        {
            // Update mode if it changed
            if (info.nLastMode != (int)pUnit->dwMode && info.renderObj != nullptr)
            {
                info.renderObj->SetTokenMode(pUnit->dwMode);
                info.nLastMode = (int)pUnit->dwMode;
            }
            return;
        }
    }

    // Create new render info for this unit
    UnitRenderInfo info = {};
    info.dwUnitId = pUnit->dwUnitId;
    info.nUnitType = pUnit->nUnitType;
    info.nLastMode = (int)pUnit->dwMode;
    info.token = nullptr;
    info.renderObj = nullptr;

    if (pUnit->nUnitType == UNIT_PLAYER)
    {
        int charClass = pUnit->nCharClass;
        if (charClass < 0 || charClass >= D2CLASS_MAX)
            charClass = 0;

        info.token = engine->graphics->CreateReference(TOKEN_CHAR, g_szClassTokens[charClass]);
    }
    // Future: UNIT_MONSTER token loading will go here (S05)

    if (info.token != nullptr)
    {
        info.renderObj = engine->renderer->AllocateObject(2); // priority above tiles
        info.renderObj->AttachTokenResource(info.token);
        info.renderObj->SetTokenHitClass(WC_HTH);
        info.renderObj->SetTokenMode(pUnit->dwMode);

        // Default armor appearance
        for (int i = 0; i < COMP_MAX; i++)
        {
            info.renderObj->SetTokenArmorLevel(i, "lit");
        }
    }

    m_unitRenders.push_back(info);
}

/*
 *	Clean up all unit render objects.
 */
void D2ClientWorld::CleanupUnitRenders()
{
    for (auto &info : m_unitRenders)
    {
        if (info.renderObj)
        {
            engine->renderer->Remove(info.renderObj);
        }
        if (info.token)
        {
            engine->graphics->DeleteReference(info.token);
        }
    }
    m_unitRenders.clear();
}

/*
 *	Draw all units in the world at their tile positions.
 */
void D2ClientWorld::DrawUnits()
{
    if (gpGame == nullptr)
        return;

    D2UnitStrc *pPlayer = gpGame->GetLocalPlayer();
    if (pPlayer == nullptr)
        return;

    float targetX = (float)pPlayer->wX;
    float targetY = (float)pPlayer->wY;

    // Initialize interpolated position on first frame
    if (m_playerDrawX < 0)
    {
        m_playerDrawX = targetX;
        m_playerDrawY = targetY;
    }

    // Smooth interpolation toward actual position (lerp)
    float lerpSpeed = 0.15f;
    m_playerDrawX += (targetX - m_playerDrawX) * lerpSpeed;
    m_playerDrawY += (targetY - m_playerDrawY) * lerpSpeed;

    // Snap when very close to avoid floating
    if (fabsf(targetX - m_playerDrawX) < 0.05f) m_playerDrawX = targetX;
    if (fabsf(targetY - m_playerDrawY) < 0.05f) m_playerDrawY = targetY;

    // Center camera on interpolated player position
    m_cameraX = (m_playerDrawX - m_playerDrawY) * (float)HALF_W - SCREEN_W / 2.0f;
    m_cameraY = (m_playerDrawX + m_playerDrawY) * (float)HALF_H - SCREEN_H / 2.0f;

    // Convert interpolated position to screen coords
    float sx, sy;
    TileToScreenF(m_playerDrawX, m_playerDrawY, sx, sy);
    float drawX = sx + HALF_W;
    float drawY = sy + HALF_H;

    // Try token-based rendering
    EnsureUnitRender(pPlayer);
    bool tokenDrawn = false;
    for (auto &info : m_unitRenders)
    {
        if (info.dwUnitId == pPlayer->dwUnitId && info.renderObj != nullptr)
        {
            info.renderObj->SetDrawCoords((int)drawX, (int)drawY, 0, 0);
            info.renderObj->Draw();
            tokenDrawn = true;
            break;
        }
    }

    // Always draw player marker (visible even if token fails to load)
    // Isometric diamond shape representing the player
    float mx = drawX, my = drawY - 4.0f;
    al_draw_filled_triangle(mx, my - 12, mx - 8, my, mx + 8, my,
        al_map_rgba(220, 180, 50, 220));
    al_draw_filled_triangle(mx, my + 12, mx - 8, my, mx + 8, my,
        al_map_rgba(180, 140, 30, 220));
    al_draw_circle(mx, my, 10.0f, al_map_rgba(255, 220, 80, 255), 1.5f);
}

void D2ClientWorld::ScrollCamera(float dx, float dy)
{
    m_cameraX += dx;
    m_cameraY += dy;
}

void D2ClientWorld::CenterCameraOnTile(int tileX, int tileY)
{
    m_cameraX = (float)(tileX - tileY) * HALF_W - SCREEN_W / 2.0f;
    m_cameraY = (float)(tileX + tileY) * HALF_H - SCREEN_H / 2.0f;
}

void D2ClientWorld::Draw()
{
    static float black[] = {0.0f, 0.0f, 0.0f, 1.0f};

    engine->renderer->Clear();
    engine->renderer->DrawRectangle(0, 0, (float)SCREEN_W, (float)SCREEN_H, 0, nullptr, black);

    if (m_ds1Handle == INVALID_HANDLE || m_mapWidth <= 0 || m_mapHeight <= 0)
    {
        // Still draw debug text even when no map is loaded
        if (m_debugText && m_statusText[0] != '\0')
        {
            char16_t wideStatus[256];
            for (int i = 0; i < 256; i++)
            {
                wideStatus[i] = (char16_t)(unsigned char)m_statusText[i];
                if (m_statusText[i] == '\0')
                    break;
            }
            m_debugText->SetText(wideStatus);
            m_debugText->Draw();
        }
        return;
    }

    // Draw floor tiles
    for (int ty = 0; ty < m_mapHeight; ty++)
    {
        for (int tx = 0; tx < m_mapWidth; tx++)
        {
            float sx, sy;
            TileToScreen(tx, ty, sx, sy);

            if (sx + TILE_WIDTH < 0 || sx > SCREEN_W ||
                sy + TILE_HEIGHT < 0 || sy > SCREEN_H + 200)
                continue;

            DS1Cell *floorCell = engine->DS1_GetCellAt(m_ds1Handle, tx, ty, DS1Cell_Floor);
            if (floorCell == nullptr)
                continue;

            if (floorCell->prop1 == 0 && floorCell->prop2 == 0 &&
                floorCell->prop3 == 0 && floorCell->prop4 == 0)
                continue;

            long mainIndex, subIndex;
            DecodeCellProps(floorCell, mainIndex, subIndex);

            const EditorTileEntry *entry = FindEditorTile(0, mainIndex, subIndex);
            if (entry == nullptr)
                continue;

            ALLEGRO_BITMAP *tileBmp = DecodeTileBitmap(entry->dt1Index, entry->blockIndex, m_act);
            if (tileBmp)
                al_draw_bitmap(tileBmp, sx, sy, 0);
        }
    }

    // Draw wall tiles on top using actual DT1 tile bitmaps
    for (int ty = 0; ty < m_mapHeight; ty++)
    {
        for (int tx = 0; tx < m_mapWidth; tx++)
        {
            float sx, sy;
            TileToScreen(tx, ty, sx, sy);

            if (sx + TILE_WIDTH < 0 || sx > SCREEN_W ||
                sy + 400 < 0 || sy > SCREEN_H + 200)
                continue;

            DS1Cell *wallCell = engine->DS1_GetCellAt(m_ds1Handle, tx, ty, DS1Cell_Wall);
            if (wallCell == nullptr)
                continue;

            if (wallCell->prop1 == 0 && wallCell->prop2 == 0 &&
                wallCell->prop3 == 0 && wallCell->prop4 == 0)
                continue;

            long mainIndex, subIndex;
            DecodeCellProps(wallCell, mainIndex, subIndex);
            long orientation = wallCell->orientation;

            if (orientation == 0)
                continue;

            const EditorTileEntry *entry = FindEditorTile(orientation, mainIndex, subIndex);
            if (entry == nullptr)
                continue;

            ALLEGRO_BITMAP *tileBmp = DecodeTileBitmap(entry->dt1Index, entry->blockIndex, m_act);
            if (tileBmp)
            {
                int bmpH = al_get_bitmap_height(tileBmp);
                float wallY = sy + TILE_HEIGHT - (float)bmpH;
                al_draw_bitmap(tileBmp, sx, wallY, 0);
            }
        }
    }

    // Draw units (players, NPCs) as animated tokens
    DrawUnits();

    // Draw debug overlay with status text
    if (m_debugText && m_statusText[0] != '\0')
    {
        char16_t wideStatus[256];
        for (int i = 0; i < 256; i++)
        {
            wideStatus[i] = (char16_t)(unsigned char)m_statusText[i];
            if (m_statusText[i] == '\0')
                break;
        }
        m_debugText->SetText(wideStatus);
        m_debugText->Draw();
    }
}
