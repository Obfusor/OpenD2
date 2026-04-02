#include "Renderer.hpp"
#ifdef USE_ALLEGRO5
#include "Renderer_Allegro.hpp"
#else
#include "Renderer_GL.hpp"
#endif
#include "Palette.hpp"
#include "DCC.hpp"

/*
 *	The renderer in OpenD2 is significantly different from the one in retail Diablo 2.
 *
 *	For starters, the window management and rendering are totally separated.
 *	The render target can be changed in D2.ini, registry, or commandline.
 */

IRenderer* RenderTarget = nullptr;

namespace Renderer
{
#ifdef USE_ALLEGRO5
	/*
	 *	Initializes the Allegro 5 renderer.
	 */
	void Init(D2GameConfigStrc* pConfig, OpenD2ConfigStrc* pOpenConfig, ALLEGRO_DISPLAY* pDisplay)
	{
		// Load palettes
		Pal::Init();
		DCC::GlobalInit();

		RenderTarget = new Renderer_Allegro(pConfig, pOpenConfig, pDisplay);
	}
#else
	/*
	 *	Initializes the SDL/OpenGL renderer.
	 */
	void Init(D2GameConfigStrc* pConfig, OpenD2ConfigStrc* pOpenConfig, SDL_Window* pWindow)
	{
		OpenD2RenderTargets DesiredRenderTarget = OD2RT_SDL;

		// Determine which render target to go with
		if (pConfig->bOpenGL || pConfig->bD3D || pOpenConfig->bNoSDLAccel)
		{
			DesiredRenderTarget = OD2RT_OPENGL;
		}
		else
		{
			DesiredRenderTarget = OD2RT_SDL;
		}

		// Load palettes
		Pal::Init();
		DCC::GlobalInit();

		switch (DesiredRenderTarget)
		{
			default:
			case OD2RT_OPENGL:
				RenderTarget = new Renderer_GL(pConfig, pOpenConfig, pWindow);
				break;
#if 0	// no longer available
			case OD2RT_SDL:
				RenderTarget = new Renderer_SDL(pConfig, pOpenConfig, pWindow);
				break;
#endif
		}
	}
#endif

	/*
	 *	Map rendertarget exports to game module exports
	 */
	void MapRenderTargetExports(D2ModuleImportStrc* pExport)
	{
		pExport->renderer = RenderTarget;
	}
}
