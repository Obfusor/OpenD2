#include "Debug.hpp"
#include "../D2Menu.hpp"
#include "../Menus/Main.hpp"
#include "../Menus/Loading.hpp"
#include "../../MapSelector.hpp"

#define MAIN_BUTTON_DC6			"data\\global\\ui\\FrontEnd\\3WideButtonBlank.dc6"

namespace D2Panels
{
	Debug::Debug() : D2Panel()
	{
		// Center buttons on 1280px display: (1280 - 272) / 2 = 504
		m_loadEncampmentButton = new D2Widgets::Button(504, 310, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		m_mapBrowserButton = new D2Widgets::Button(504, 350, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		m_exitButton = new D2Widgets::Button(504, 595, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);

		AddWidget(m_loadEncampmentButton);
		AddWidget(m_mapBrowserButton);
		AddWidget(m_exitButton);

		m_loadEncampmentButton->AttachText(u"ROGUE ENCAMPMENT");
		m_mapBrowserButton->AttachText(u"MAP BROWSER");
		m_exitButton->AttachText(u"EXIT");
		m_exitButton->AddEventListener(Clicked, [] {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::Main();
			});
		m_loadEncampmentButton->AddEventListener(Clicked, [] {
			delete cl.pActiveMenu;
			cl.pActiveMenu = nullptr;
			cl.pLoadingMenu = new D2Menus::Loading();
			cl.gamestate = GS_LOADING;
			cl.nLoadState = 0;
			openConfig->currentGameMode = OpenD2GameModes::MapPreviewer;
			});
		m_mapBrowserButton->AddEventListener(Clicked, [] {
			delete cl.pActiveMenu;
			cl.pActiveMenu = nullptr;
			cl.pLoadingMenu = nullptr;
			openConfig->currentGameMode = OpenD2GameModes::MapPreviewer;
			cl.gamestate = GS_LOADING;
			cl.nLoadState = 0;
			gpMapSelector = new MapSelector();
			gpMapSelector->ScanDirectory(openConfig->szBasePath);
			});
	}

	Debug::~Debug()
	{
		delete m_loadEncampmentButton;
		delete m_mapBrowserButton;
		delete m_exitButton;
	}

	void Debug::Draw()
	{
		DrawAllWidgets();
	}
}
