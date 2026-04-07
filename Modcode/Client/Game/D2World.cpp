#include "D2World.hpp"
#include "D2Game.hpp"

// Editor pipeline types and functions (from EditorCompat)
extern "C" {
    int dt1_add(char *dt1name);

    // Use the actual BLOCK_S from structs.h for correct field access
    typedef struct {
        long       direction;
        unsigned short roof_y;
        unsigned char  sound;
        unsigned char  animated;
        long       size_y;
        long       size_x;
        long       orientation;
        long       main_index;
        long       sub_index;
        long       rarity;
        unsigned char sub_tiles_flags[25];
        long       tiles_ptr;
        long       tiles_length;
        long       tiles_number;
    } BLOCK_S_ACTUAL;
}

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

    // bh_buffer contains parsed BLOCK_S structs (from EditorCompat/structs.h).
    // Cast directly to get correct field access.
    for (int d = 0; d < 300; d++) // DT1_MAX = 300
    {
        DT1_S_EXT *dt1 = &glb_dt1[d];
        if (dt1->ds1_usage <= 0 || dt1->bh_buffer == NULL || dt1->block_num <= 0)
            continue;

        BLOCK_S_ACTUAL *blocks = (BLOCK_S_ACTUAL *)dt1->bh_buffer;

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
 *	Decode a DT1 tile block into an ALLEGRO_BITMAP with palette colors.
 *	Uses the editor's pre-rendered 8-bit indexed bitmaps and converts
 *	to RGBA using the engine's palette data. Results are cached.
 */
ALLEGRO_BITMAP *D2ClientWorld::DecodeTileBitmap(int dt1Index, int blockIndex, int act)
{
    uint64_t cacheKey = (uint64_t)dt1Index * 100000 + blockIndex;

    auto it = m_tileCache.find(cacheKey);
    if (it != m_tileCache.end())
        return it->second;

    DT1_S_EXT *dt1 = &glb_dt1[dt1Index];
    if (dt1->block_zoom[0] == NULL || blockIndex >= dt1->block_num)
    {
        static int fc1 = 0;
        if (fc1++ == 0) snprintf(m_statusText, sizeof(m_statusText),
            "FAIL1: zoom0=%p blknum=%d req=%d", (void*)dt1->block_zoom[0], (int)dt1->block_num, blockIndex);
        m_tileCache[cacheKey] = nullptr;
        return nullptr;
    }

    BITMAP_A4 *srcBmp = dt1->block_zoom[0][blockIndex]; // zoom 0 = 1:1
    if (srcBmp == NULL || srcBmp->line == NULL || srcBmp->w == 0 || srcBmp->h == 0)
    {
        static int fc2 = 0;
        if (fc2++ == 0) snprintf(m_statusText, sizeof(m_statusText),
            "FAIL2: bmp=%p line=%p w=%d h=%d", (void*)srcBmp,
            srcBmp ? (void*)srcBmp->line : nullptr,
            srcBmp ? srcBmp->w : -1, srcBmp ? srcBmp->h : -1);
        m_tileCache[cacheKey] = nullptr;
        return nullptr;
    }

    if (srcBmp->w > 1024 || srcBmp->h > 1024)
    {
        static int fc3 = 0;
        if (fc3++ == 0) snprintf(m_statusText, sizeof(m_statusText),
            "FAIL3: too big w=%d h=%d", srcBmp->w, srcBmp->h);
        m_tileCache[cacheKey] = nullptr;
        return nullptr;
    }

    // Get palette for the current act
    D2Palette *pal = engine->PAL_GetPalette(act < 5 ? act : 0);

    ALLEGRO_BITMAP *bmp = al_create_bitmap(srcBmp->w, srcBmp->h);
    if (bmp == nullptr)
    {
        static int fc4 = 0;
        if (fc4++ == 0) snprintf(m_statusText, sizeof(m_statusText),
            "FAIL4: al_create_bitmap(%d,%d) returned null, pal=%p", srcBmp->w, srcBmp->h, (void*)pal);
        m_tileCache[cacheKey] = nullptr;
        return nullptr;
    }

    ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY);
    if (lr)
    {
        for (int y = 0; y < srcBmp->h; y++)
        {
            uint32_t *dst = (uint32_t *)((char *)lr->data + y * lr->pitch);
            unsigned char *src = srcBmp->line[y];
            for (int x = 0; x < srcBmp->w; x++)
            {
                unsigned char idx = src[x];
                if (idx == 0)
                {
                    dst[x] = 0x00000000; // transparent
                }
                else if (pal)
                {
                    BYTE b = (*pal)[idx][0]; // pal.dat stores BGR
                    BYTE g = (*pal)[idx][1];
                    BYTE r = (*pal)[idx][2];
                    dst[x] = 0xFF000000 | (b << 16) | (g << 8) | r; // ABGR
                }
                else
                {
                    dst[x] = 0xFF000000 | (idx << 16) | (idx << 8) | idx; // grayscale fallback
                }
            }
        }
        al_unlock_bitmap(bmp);
    }

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
    // The DS1 file references .tg1 files (not .dt1), so editor_bridge_load_ds1
    // won't find the DT1s. We load them explicitly from the level type definition,
    // using the same paths that LoadDT1sForLevel already resolved.
    EnsureEditorBridgeInit();
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

    BuildEditorTileLookup();

    // Build status text
    const char *levelName = "Unknown";
    if (sgptDataTables->pLevelsTxt != nullptr &&
        nLevelId < sgptDataTables->nLevelsTxtRecordCount)
    {
        levelName = sgptDataTables->pLevelsTxt[nLevelId].szLevelName;
    }
    // Check first editor tile for diagnostic info
    int dbgZoom0Null = 0, dbgBmpNull = 0, dbgBmpOk = 0;
    if (!m_editorTileLookup.empty())
    {
        auto it = m_editorTileLookup.begin();
        const EditorTileEntry &ent = it->second[0];
        DT1_S_EXT *dt1 = &glb_dt1[ent.dt1Index];
        if (dt1->block_zoom[0] == NULL)
            dbgZoom0Null = 1;
        else if (dt1->block_zoom[0][ent.blockIndex] == NULL)
            dbgBmpNull = 1;
        else
            dbgBmpOk = 1;
    }

    snprintf(m_statusText, sizeof(m_statusText),
             "%s %dx%d DT1=%d t=%d ed=%d act=%d z0null=%d bnull=%d bok=%d",
             levelName, m_mapWidth, m_mapHeight,
             (int)m_loadedDT1s.size(), (int)m_tileLookup.size(),
             (int)m_editorTileLookup.size(), m_act,
             dbgZoom0Null, dbgBmpNull, dbgBmpOk);
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

    D2UnitList &units = gpGame->GetUnits();

    // Draw local player and follow with camera
    D2UnitStrc *pPlayer = gpGame->GetLocalPlayer();
    if (pPlayer != nullptr)
    {
        // Center camera on player position
        CenterCameraOnTile(pPlayer->wX, pPlayer->wY);

        EnsureUnitRender(pPlayer);

        // Find the render info and position it
        for (auto &info : m_unitRenders)
        {
            if (info.dwUnitId == pPlayer->dwUnitId && info.renderObj != nullptr)
            {
                float sx, sy;
                TileToScreen(pPlayer->wX, pPlayer->wY, sx, sy);
                // Center token on the tile (offset to center of diamond)
                info.renderObj->SetDrawCoords(
                    (int)(sx + HALF_W), (int)(sy + HALF_H), 0, 0);
                info.renderObj->Draw();
                break;
            }
        }
    }
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

    // Draw floor tiles using actual DT1 tile bitmaps
    int dbgFound = 0, dbgNotFound = 0, dbgDecoded = 0, dbgDecodeFail = 0;
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
            if (entry != nullptr)
            {
                dbgFound++;
                ALLEGRO_BITMAP *tileBmp = DecodeTileBitmap(entry->dt1Index, entry->blockIndex, m_act);
                if (tileBmp)
                {
                    dbgDecoded++;
                    al_draw_bitmap(tileBmp, sx, sy, 0);
                    continue;
                }
                dbgDecodeFail++;
            }
            else
            {
                dbgNotFound++;
            }

            // Fallback: colored diamond when editor tiles unavailable
            float r = ((mainIndex * 37) % 128 + 64) / 255.0f;
            float g = ((mainIndex * 53 + subIndex * 17) % 128 + 80) / 255.0f;
            float b = ((subIndex * 71) % 100 + 40) / 255.0f;
            float tileColor[] = {r, g, b, 1.0f};
            engine->renderer->DrawRectangle(sx + 40, sy + 10, 80, 60, 0, nullptr, tileColor);
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

    // Draw debug overlay — always show per-frame tile stats
    if (m_debugText)
    {
        char dbgBuf[256];
        // Show failure reason if any tiles failed
        static char s_failReason[128] = "";
        if (dbgDecodeFail > 0 && s_failReason[0] == '\0')
        {
            // Probe the first failing tile to capture the reason
            auto it = m_editorTileLookup.begin();
            if (it != m_editorTileLookup.end())
            {
                const EditorTileEntry &ent = it->second[0];
                DT1_S_EXT *dt1 = &glb_dt1[ent.dt1Index];
                if (dt1->block_zoom[0] == NULL)
                    snprintf(s_failReason, sizeof(s_failReason), "zoom0=NULL");
                else if (ent.blockIndex >= dt1->block_num)
                    snprintf(s_failReason, sizeof(s_failReason), "blk %d >= num %d", ent.blockIndex, (int)dt1->block_num);
                else {
                    BITMAP_A4 *sb = dt1->block_zoom[0][ent.blockIndex];
                    if (sb == NULL)
                        snprintf(s_failReason, sizeof(s_failReason), "bmp=NULL");
                    else if (sb->line == NULL)
                        snprintf(s_failReason, sizeof(s_failReason), "line=NULL w=%d h=%d", sb->w, sb->h);
                    else if (sb->w == 0 || sb->h == 0)
                        snprintf(s_failReason, sizeof(s_failReason), "size=0 w=%d h=%d", sb->w, sb->h);
                    else if (sb->w > 1024 || sb->h > 1024)
                        snprintf(s_failReason, sizeof(s_failReason), "toobig w=%d h=%d", sb->w, sb->h);
                    else {
                        D2Palette *p = engine->PAL_GetPalette(m_act < 5 ? m_act : 0);
                        ALLEGRO_BITMAP *test = al_create_bitmap(sb->w, sb->h);
                        snprintf(s_failReason, sizeof(s_failReason),
                                 "bmp w=%d h=%d pal=%p al_bmp=%p", sb->w, sb->h, (void*)p, (void*)test);
                        if (test) al_destroy_bitmap(test);
                    }
                }
            }
        }
        snprintf(dbgBuf, sizeof(dbgBuf),
                 "f=%d ok=%d fail=%d %s",
                 dbgFound, dbgDecoded, dbgDecodeFail, s_failReason);
        char16_t wideStatus[256];
        for (int i = 0; i < 256; i++)
        {
            wideStatus[i] = (char16_t)(unsigned char)dbgBuf[i];
            if (dbgBuf[i] == '\0')
                break;
        }
        m_debugText->SetText(wideStatus);
        m_debugText->Draw();
    }
}
