#pragma once
#include "Diablo2.hpp"

// The render targets
enum OpenD2RenderTargets
{
	OD2RT_SDL,			// SDL renderer with hardware acceleration (default for now)
	OD2RT_OPENGL,		// OpenGL
	OD2RT_ALLEGRO5,		// Allegro 5 software renderer
	OD2RT_MAX
};

// Renderer.cpp
namespace Renderer
{
#ifdef USE_ALLEGRO5
	void Init(D2GameConfigStrc* pConfig, OpenD2ConfigStrc* pOpenConfig, ALLEGRO_DISPLAY* pDisplay);
#else
	void Init(D2GameConfigStrc* pConfig, OpenD2ConfigStrc* pOpenConfig, SDL_Window* pWindow);
#endif
	void MapRenderTargetExports(D2ModuleImportStrc* pExport);
}

extern class IRenderer* RenderTarget;	// nullptr if there isn't a render target
