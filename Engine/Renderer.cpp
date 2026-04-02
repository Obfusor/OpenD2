#include "Renderer.hpp"
#include "Renderer_Allegro.hpp"
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

	/*
	 *	Map rendertarget exports to game module exports
	 */
	void MapRenderTargetExports(D2ModuleImportStrc* pExport)
	{
		pExport->renderer = RenderTarget;
	}
}
