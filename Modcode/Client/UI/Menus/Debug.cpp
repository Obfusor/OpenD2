#include "Debug.hpp"
#include "Main.hpp"

namespace D2Menus
{
	Debug::Debug() : D2Menu()
	{
		IGraphicsReference* background = engine->graphics->CreateReference(
			"data\\global\\ui\\FrontEnd\\gameselectscreenEXP.dc6",
			UsagePolicy_Permanent
		);

		backgroundObject = engine->renderer->AllocateObject(0);
		backgroundObject->AttachCompositeTextureResource(background, 0, -1);
		// Fill-and-crop: scale 800x600 to fill 1280x720
		backgroundObject->SetDrawCoords(0, -120, 1280, 960);
		backgroundObject->SetPalshift(0);

		pDebugPanel = new D2Panels::Debug();
		AddPanel(pDebugPanel);
	}

	Debug::~Debug()
	{
		delete pDebugPanel;
	}

	bool Debug::HandleKeyDown(DWORD dwKey)
	{
		if (dwKey == 27) // Escape
		{
			delete cl.pActiveMenu;
			cl.pActiveMenu = new Main();
			return true;
		}
		return D2Menu::HandleKeyDown(dwKey);
	}

	void Debug::Draw()
	{
		backgroundObject->Draw();

		DrawAllPanels();
	}
}