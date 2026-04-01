#include "TCPIPMain.hpp"
#include "../Menus/TCPIP.hpp"
#include "../Menus/OtherMultiplayer.hpp"
#include "../Menus/CharSelect.hpp"
#include "../Menus/CharCreate.hpp"
#include "../Widgets/Button.hpp"

#define MAIN_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\3WideButtonBlank.dc6"
#define SMALL_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\MediumButtonBlank.dc6"

#define TBLTEXT_HOSTGAME 5118
#define TBLTEXT_JOINGAME 5119
#define TBLTEXT_CANCEL 3402

namespace D2Panels
{
	/*
	 *	Creates the TCP/IP panel.
	 *	@author	eezstreet
	 */
	TCPIPMain::TCPIPMain() : D2Panel()
	{
		m_hostGameButton = new D2Widgets::Button(265, 170, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		m_joinGameButton = new D2Widgets::Button(265, 230, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		m_cancelButton = new D2Widgets::Button(40, 535, SMALL_BUTTON_DC6, "medium", 0, 0, 1, 1, 0, 0);

		m_hostGameButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_HOSTGAME));
		m_joinGameButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_JOINGAME));
		m_cancelButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_CANCEL));

		AddWidget(m_hostGameButton);
		AddWidget(m_joinGameButton);
		AddWidget(m_cancelButton);

		m_joinGameButton->AddEventListener(Clicked, []
										   {
			D2Menus::TCPIP* tcpip = dynamic_cast<D2Menus::TCPIP*>(cl.pActiveMenu);
			if (tcpip) { tcpip->ShowJoinSubmenu(true); } });

		m_hostGameButton->AddEventListener(Clicked, []
										   {
			cl.charSelectContext = CSC_TCPIP;
			engine->NET_SetPlayerCount(8);
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

		m_cancelButton->AddEventListener(Clicked, []
										 {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::OtherMultiplayer(); });
	}

	/*
	 *	Destroys the TCP/IP panel.
	 *	@author	eezstreet
	 */
	TCPIPMain::~TCPIPMain()
	{
		delete m_hostGameButton;
		delete m_joinGameButton;
		delete m_cancelButton;
	}

	/*
	 *	Draws the TCP/IP panel.
	 *	@author	eezstreet
	 */
	void TCPIPMain::Draw()
	{
		DrawAllWidgets();
	}

	/*
	 *	Enable/disable the Host Game and Join Game buttons (Join Game was clicked)
	 *	@author	eezstreet
	 */
	void TCPIPMain::EnableButtons(bool bEnable)
	{
		if (bEnable)
		{
			m_hostGameButton->Enable();
			m_joinGameButton->Enable();
		}
		else
		{
			m_hostGameButton->Disable();
			m_joinGameButton->Disable();
		}
	}
}