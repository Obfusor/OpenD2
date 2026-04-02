#include "Main.hpp"
#include "../Menus/CharCreate.hpp"
#include "../Menus/CharSelect.hpp"
#include "../Menus/OtherMultiplayer.hpp"
#ifdef _DEBUG
#include "../Menus/Debug.hpp"
#endif

#define TBLTEXT_SINGLEPLAYER 5106
#define TBLTEXT_BATTLENET 5107
#define TBLTEXT_MULTIPLAYER 5108
#define TBLTEXT_EXIT 5109

#define MAIN_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\3WideButtonBlank.dc6"
#define BATTLE_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\WideButtonBlank02.dc6"
#define THIN_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\NarrowButtonBlank.dc6"

/**	Tries to advance to the character select screen.
 *	If there's no save files present, it advances to the character creation screen instead.
 */
void D2Client_AdvanceToCharSelect()
{
}

namespace D2Panels
{
	/*
	 *	Creates the main menu panel
	 */
	Main::Main() : D2Panel()
	{
		// Center buttons on 1280px display: (1280 - 272) / 2 = 504
		m_singleplayerButton = new D2Widgets::Button(504, 350, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		m_battleNetButton = new D2Widgets::Button(504, 392, BATTLE_BUTTON_DC6, "wide02", 0, 1, 2, 3, 0, 1);
		m_gatewayButton = new D2Widgets::Button(504, 426, THIN_BUTTON_DC6, "narrow", 0, 1, 2, 3, 0, 1);
		m_multiplayerButton = new D2Widgets::Button(504, 460, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		m_exitButton = new D2Widgets::Button(504, 595, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
#ifdef _DEBUG
		m_debugMapButton = new D2Widgets::Button(504, 308, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
#endif

		AddWidget(m_singleplayerButton);
		AddWidget(m_battleNetButton);
		AddWidget(m_gatewayButton);
		AddWidget(m_multiplayerButton);
		AddWidget(m_exitButton);
#ifdef _DEBUG
		AddWidget(m_debugMapButton);

		m_debugMapButton->AttachText(u"DEBUG MENU");
#endif

		m_singleplayerButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_SINGLEPLAYER));
		m_battleNetButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_BATTLENET));
		m_multiplayerButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_MULTIPLAYER));
		m_exitButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_EXIT));

		m_singleplayerButton->AddEventListener(Clicked, []
											   {
			cl.szCurrentIPDestination[0] = '\0'; // set IP to blank
			engine->NET_SetPlayerCount(1);
			cl.charSelectContext = CSC_SINGLEPLAYER;

			int nNumFiles = 0;
			char** szFileList = engine->FS_ListFilesInDirectory("Save", "*.d2s", &nNumFiles);

			delete cl.pActiveMenu;
			if (nNumFiles <= 0)
			{
				cl.pActiveMenu = new D2Menus::CharCreate();
			}
			else
			{
				cl.pActiveMenu = new D2Menus::CharSelect(szFileList, nNumFiles);
				engine->FS_FreeFileList(szFileList, nNumFiles);
			} });

		m_multiplayerButton->AddEventListener(Clicked, []
											  {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::OtherMultiplayer(); });

		m_exitButton->AddEventListener(Clicked, []
									   { cl.bKillGame = true; });

#ifdef _DEBUG
		m_debugMapButton->AddEventListener(Clicked, []
										   {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::Debug(); });
#endif

		// Disable the battle.net button and the gateway button.
		// Closed Battle.net is not allowed in OpenD2.
		m_battleNetButton->Disable();
		m_gatewayButton->Disable();
	}

	/*
	 *	Destroys the main menu panel
	 */
	Main::~Main()
	{
		delete m_singleplayerButton;
		delete m_battleNetButton;
		delete m_gatewayButton;
		delete m_multiplayerButton;
		delete m_exitButton;
	}

	/*
	 *	Draws the main menu panel
	 */
	void Main::Draw()
	{
		DrawAllWidgets();
	}
}
