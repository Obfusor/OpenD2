#pragma once
#include "../Shared/D2Shared.hpp"
#include "Allegro5.hpp"
#include <unordered_map>
#include <string>

// Forward declare
class Renderer_Allegro;

/*
 *	Allegro 5 Render Object - implements IRenderObject
 *	Loads pre-converted PNG frames from DC6 assets.
 */
class AllegroRenderObject : public IRenderObject
{
	friend class Renderer_Allegro;
private:
	enum RenderType { RT_NONE, RT_TEXTURE, RT_COMPOSITE, RT_ANIMATION, RT_FONT_TEXT, RT_TOKEN };

	// Core state
	int m_x, m_y, m_w, m_h;
	RenderType m_type;
	Renderer_Allegro *m_pRenderer;

	// RT_TEXTURE: single frame (cached bitmap, do NOT destroy)
	ALLEGRO_BITMAP *m_texture;

	// RT_COMPOSITE: stitched multi-frame (owned bitmap, destroy on reset)
	ALLEGRO_BITMAP *m_compositeBitmap;

	// RT_ANIMATION: frame array
	ALLEGRO_BITMAP **m_animFrames;
	bool m_animFramesOwned; // true = DCC-decoded (destroy on reset), false = PNG-cached (don't destroy)
	int m_animFrameCount;
	int m_animCurrentFrame;
	double m_lastDrawTime;
	float m_animFramerate;
	bool m_animLoop;
	int *m_frameOffsetX;
	int *m_frameOffsetY;

	// RT_FONT_TEXT: font + text (temporary: uses Allegro built-in font)
	IGraphicsReference *m_fontRef;
	char16_t m_textBuf[512];
	int m_textColor;
	int m_textAlignX, m_textAlignY, m_textAlignW, m_textAlignH;
	int m_horzAlign, m_vertAlign;

	// RT_TOKEN: character token with multi-layer DCC composition
	ITokenReference *m_tokenRef;
	int m_tokenMode;
	int m_tokenHitClass;
	int m_tokenDirection;
	char m_tokenArmorLevel[COMP_MAX][8];
	bool m_tokenDirty; // needs to reload DCCs when settings change
	void LoadTokenLayers();

	// Shared properties
	float m_colorR, m_colorG, m_colorB, m_colorA;
	int m_drawMode;
	BYTE m_palshift;

	void Reset();

public:
	AllegroRenderObject();

	// IRenderObject interface
	virtual void Draw() override;
	virtual void AttachTextureResource(IGraphicsReference *ref, int32_t frame) override;
	virtual void AttachCompositeTextureResource(IGraphicsReference *ref, int32_t startFrame, int32_t endFrame) override;
	virtual void AttachAnimationResource(IGraphicsReference *ref, bool bResetFrame) override;
	virtual void AttachTokenResource(ITokenReference *ref) override;
	virtual void AttachFontResource(IGraphicsReference *ref) override;
	virtual void SetPalshift(BYTE palette) override;
	virtual void SetDrawCoords(int x, int y, int w, int h) override;
	virtual void GetDrawCoords(int *x, int *y, int *w, int *h) override;
	virtual void SetColorModulate(float r, float g, float b, float a) override;
	virtual void SetDrawMode(int drawMode) override;
	virtual bool PixelPerfectDetection(int x, int y) override;
	virtual void SetText(const char16_t *text) override;
	virtual void SetTextAlignment(int x, int y, int w, int h, int horzAlignment, int vertAlignment) override;
	virtual void SetTextColor(int color) override;
	virtual void SetFramerate(int framerate) override;
	virtual void SetAnimationLoop(bool bLoop) override;
	virtual void AddAnimationFinishedCallback(void *extraData, AnimationFinishCallback callback) override;
	virtual void AddAnimationFrameCallback(int32_t frame, void *extraData, AnimationFrameCallback callback) override;
	virtual void RemoveAnimationFinishCallbacks() override;
	virtual void SetAnimationDirection(int direction) override;
	virtual void SetTokenMode(int newMode) override;
	virtual void SetTokenArmorLevel(int component, const char *armorLevel) override;
	virtual void SetTokenHitClass(int hitclass) override;
	virtual int GetAnimFrameCount() override { return m_animFrameCount; }
};

/*
 *	Allegro 5 Renderer - implements IRenderer
 */
class Renderer_Allegro : public IRenderer
{
private:
	ALLEGRO_DISPLAY *m_pDisplay;
	ALLEGRO_FONT *m_pBuiltinFont;
	D2Palettes m_currentPalette;
	char m_basePath[1024];

	// Render object pool
	static const int MAX_RENDER_OBJECTS = 4096;
	AllegroRenderObject m_renderObjects[MAX_RENDER_OBJECTS];
	bool m_objectInUse[MAX_RENDER_OBJECTS];
	int m_numAllocated;

	// Bitmap cache: PNG path -> ALLEGRO_BITMAP*
	std::unordered_map<std::string, ALLEGRO_BITMAP *> m_bitmapCache;

public:
	Renderer_Allegro(D2GameConfigStrc *pConfig, OpenD2ConfigStrc *pOpenConfig, ALLEGRO_DISPLAY *pDisplay);
	~Renderer_Allegro();

	// PNG loading
	ALLEGRO_BITMAP *LoadOrGetBitmap(const char *pngPath);
	static bool ResolvePNGPath(const char *basePath, const char *dc6Path,
		int direction, int frame, int numDirections, char *outPath, size_t outLen);

	// Accessors
	ALLEGRO_DISPLAY *GetDisplay() { return m_pDisplay; }
	ALLEGRO_FONT *GetBuiltinFont() { return m_pBuiltinFont; }
	const char *GetBasePath() { return m_basePath; }

	// IRenderer interface
	virtual void Present() override;
	virtual void Clear() override;
	virtual void SetGlobalPalette(const D2Palettes palette) override;
	virtual D2Palettes GetGlobalPalette() override;
	virtual IRenderObject *AllocateObject(int stage) override;
	virtual void Remove(IRenderObject *Object) override;
	virtual void DeleteLoadedGraphicsData(void *loadedData, IGraphicsReference *ref) override;
	virtual void DrawRectangle(float x, float y, float w, float h, float strokeWidth,
							   float *strokeColor, float *fillColor) override;
	virtual void DrawLine(float x1, float x2, float y1, float y2, float strokeWidth,
						  float *strokeColor) override;
};
