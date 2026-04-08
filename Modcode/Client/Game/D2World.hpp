#pragma once
#include "../D2Client.hpp"
#include "../../Common/D2Common.hpp"
#include "../EditorBridge.hpp"
#include "D2Unit.hpp"
#include <map>
#include <vector>

/*
 *	Client-side world renderer.
 *	Loads DT1 tiles for the current level, builds a tile lookup, and renders the
 *	isometric tile map from a loaded DS1 file.
 */

class D2ClientWorld
{
public:
    D2ClientWorld();
    ~D2ClientWorld();

    // Load the specified level: loads DT1s, DS1, builds tile lookup
    void LoadLevel(int nLevelId);

    // Unload the current level and free all resources
    void Unload();

    // Render the world tiles (call each frame)
    void Draw();

    // Camera controls
    void ScrollCamera(float dx, float dy);
    void CenterCameraOnTile(int tileX, int tileY);

    // Getters
    bool IsLoaded() const { return m_ds1Handle != INVALID_HANDLE; }
    int GetLevelId() const { return m_nCurrentLevel; }
    int GetMapWidth() const { return m_mapWidth; }
    int GetMapHeight() const { return m_mapHeight; }

    // Coordinate conversion (public for click-to-move)
    void ScreenToTile(float screenX, float screenY, int &tileX, int &tileY)
    {
        float wx = screenX + m_cameraX;
        float wy = screenY + m_cameraY;
        tileX = (int)((wx / (float)HALF_W + wy / (float)HALF_H) / 2.0f);
        tileY = (int)((wy / (float)HALF_H - wx / (float)HALF_W) / 2.0f);
    }

private:
    // Tile constants (Diablo 2 isometric)
    static const int TILE_WIDTH = 160;
    static const int TILE_HEIGHT = 80;
    static const int HALF_W = TILE_WIDTH / 2;  // 80
    static const int HALF_H = TILE_HEIGHT / 2; // 40
    static const int SCREEN_W = 1280;
    static const int SCREEN_H = 720;

    // Tile lookup types
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

    // Level state
    int m_nCurrentLevel;
    handle m_ds1Handle;
    int32_t m_mapWidth;
    int32_t m_mapHeight;
    char m_statusText[256];

    // Camera
    float m_cameraX;
    float m_cameraY;

    // Smooth unit position interpolation (sub-tile precision)
    float m_playerDrawX; // interpolated tile X (float)
    float m_playerDrawY; // interpolated tile Y (float)

    // Debug overlay
    IRenderObject *m_debugText;

    // Unit rendering (S01: token-based character rendering in world)
    struct UnitRenderInfo
    {
        ITokenReference *token;
        IRenderObject *renderObj;
        DWORD dwUnitId;
        D2UnitType nUnitType;
        int nLastMode; // track mode changes for animation updates
    };
    std::vector<UnitRenderInfo> m_unitRenders;

    void EnsureUnitRender(D2UnitStrc *pUnit);
    void CleanupUnitRenders();
    void DrawUnits();

    // DT1 handles and tile lookup (engine API — kept for compatibility)
    std::vector<handle> m_loadedDT1s;
    std::map<TileKey, std::vector<TileEntry>> m_tileLookup;

    // Editor pipeline tile rendering (actual bitmaps from d2-ds1-edit)
    struct EditorTileEntry
    {
        int dt1Index;
        int blockIndex;
    };
    std::map<TileKey, std::vector<EditorTileEntry>> m_editorTileLookup;
    std::map<uint64_t, ALLEGRO_BITMAP *> m_tileCache;
    int m_act;

    void BuildEditorTileLookup();
    const EditorTileEntry *FindEditorTile(long orientation, long mainIndex, long subIndex);
    ALLEGRO_BITMAP *DecodeTileBitmap(int dt1Index, int blockIndex, int act);

    // Internal helpers
    void LoadDT1sForLevel(int nLevelId);
    const TileEntry *FindTileEntry(long orientation, long mainIndex, long subIndex);

    // Isometric projection (integer tile coords)
    inline void TileToScreen(int tileX, int tileY, float &screenX, float &screenY)
    {
        screenX = (float)(tileX - tileY) * HALF_W - m_cameraX;
        screenY = (float)(tileX + tileY) * HALF_H - m_cameraY;
    }

    // Isometric projection (float tile coords for smooth interpolation)
    inline void TileToScreenF(float tileX, float tileY, float &screenX, float &screenY)
    {
        screenX = (tileX - tileY) * (float)HALF_W - m_cameraX;
        screenY = (tileX + tileY) * (float)HALF_H - m_cameraY;
    }

    // DS1 cell prop decoding
    inline void DecodeCellProps(const DS1Cell *cell, long &mainIndex, long &subIndex)
    {
        mainIndex = ((cell->prop4 & 0x03) << 4) | (cell->prop3 >> 4);
        subIndex = cell->prop2;
    }
};

// Global world instance (nullptr when not in-game)
extern D2ClientWorld *gpWorld;
