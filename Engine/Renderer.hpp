#pragma once
#include "Diablo2.hpp"

// The render targets
enum OpenD2RenderTargets
{
	OD2RT_ALLEGRO5,		// Allegro 5 software renderer
	OD2RT_MAX
};

// Renderer.cpp
namespace Renderer
{
	void Init(D2GameConfigStrc* pConfig, OpenD2ConfigStrc* pOpenConfig, ALLEGRO_DISPLAY* pDisplay);
	void MapRenderTargetExports(D2ModuleImportStrc* pExport);
}

extern class IRenderer* RenderTarget;	// nullptr if there isn't a render target
