#include "TCPIP.hpp"
#include "OtherMultiplayer.hpp"

#define TBLTEXT_YOURIP 5121
#define TBLTEXT_TCPIPTITLE 5117

namespace D2Menus
{
	/*
	 *	Creates the TCP/IP Menu.
	 *	@author	eezstreet
	 */
	TCPIP::TCPIP() : D2Menu()
	{
		IGraphicsReference *flameTexLeft = engine->graphics->CreateReference(
			"data\\global\\ui\\FrontEnd\\D2LogoFireLeft.dc6",
			UsagePolicy_Permanent);
		IGraphicsReference *flameTexRight = engine->graphics->CreateReference(
			"data\\global\\ui\\FrontEnd\\D2LogoFireRight.dc6",
			UsagePolicy_Permanent);
		IGraphicsReference *blackTexLeft = engine->graphics->CreateReference(
			"data\\global\\ui\\FrontEnd\\D2LogoBlackLeft.dc6",
			UsagePolicy_Permanent);
		IGraphicsReference *blackTexRight = engine->graphics->CreateReference(
			"data\\global\\ui\\FrontEnd\\D2LogoBlackRight.dc6",
			UsagePolicy_Permanent);

		background = engine->graphics->CreateReference(
			"data\\global\\ui\\FrontEnd\\TCPIPscreen.dc6",
			UsagePolicy_SingleUse);

		backgroundObject = engine->renderer->AllocateObject(0);
		backgroundObject->AttachCompositeTextureResource(background, 0, -1);
		// Fill-and-crop: scale 800x600 to fill 1280x720
		backgroundObject->SetDrawCoords(0, -120, 1280, 960);
		backgroundObject->SetPalshift(0);

		flameLeft = engine->renderer->AllocateObject(0);
		flameRight = engine->renderer->AllocateObject(0);
		blackLeft = engine->renderer->AllocateObject(0);
		blackRight = engine->renderer->AllocateObject(0);

		flameLeft->AttachAnimationResource(flameTexLeft, true);
		flameRight->AttachAnimationResource(flameTexRight, true);
		blackLeft->AttachAnimationResource(blackTexLeft, true);
		blackRight->AttachAnimationResource(blackTexRight, true);

		flameLeft->SetDrawMode(3);
		flameRight->SetDrawMode(3);

		// Center logo on 1280px display
		flameLeft->SetDrawCoords(640, -285, -1, -1);
		flameRight->SetDrawCoords(640, -285, -1, -1);
		blackLeft->SetDrawCoords(640, -285, -1, -1);
		blackRight->SetDrawCoords(640, -285, -1, -1);

		// Title text: "TCP/IP Game"
		titleText = engine->renderer->AllocateObject(1);
		titleText->AttachFontResource(cl.font42);
		titleText->SetText(engine->TBL_FindStringFromIndex(TBLTEXT_TCPIPTITLE));
		titleText->SetDrawCoords(0, 70, 1280, 50);

		// "Your IP Address is:" label
		yourIPLabel = engine->renderer->AllocateObject(1);
		yourIPLabel->AttachFontResource(cl.fontFormal12);
		yourIPLabel->SetText(engine->TBL_FindStringFromIndex(TBLTEXT_YOURIP));
		yourIPLabel->SetDrawCoords(0, 110, 1280, 15);

		// Actual IP address
		yourIPValue = engine->renderer->AllocateObject(1);
		yourIPValue->AttachFontResource(cl.fontFormal12);
		yourIPValue->SetText(engine->NET_GetLocalIP());
		yourIPValue->SetDrawCoords(0, 130, 1280, 15);

		versionText = engine->renderer->AllocateObject(1);
		versionText->AttachFontResource(cl.font16);
		versionText->SetText(GAME_FULL_UTF16);
		versionText->SetDrawCoords(20, 695, 0, 0);

		m_joinMenu = new D2Panels::TCPIPJoin();
		m_mainMenu = new D2Panels::TCPIPMain();

		m_joinMenu->x = 504;
		m_joinMenu->y = 160;

		AddPanel(m_mainMenu);
		AddPanel(m_joinMenu);

		HidePanel(m_joinMenu);
	}

	/*
	 *	Destroys the TCP/IP Menu
	 *	@author	eezstreet
	 */
	TCPIP::~TCPIP()
	{
		engine->renderer->Remove(backgroundObject);
		engine->renderer->Remove(flameLeft);
		engine->renderer->Remove(flameRight);
		engine->renderer->Remove(blackLeft);
		engine->renderer->Remove(blackRight);
		engine->renderer->Remove(versionText);
		engine->renderer->Remove(titleText);
		engine->renderer->Remove(yourIPLabel);
		engine->renderer->Remove(yourIPValue);
		delete m_joinMenu;
		delete m_mainMenu;
	}

	/*
	 *	Draws the TCP/IP menu
	 *	@author	eezstreet
	 */
	void TCPIP::Draw()
	{
		backgroundObject->Draw();
		blackLeft->Draw();
		blackRight->Draw();
		flameLeft->Draw();
		flameRight->Draw();

		titleText->Draw();
		yourIPLabel->Draw();
		yourIPValue->Draw();
		versionText->Draw();

		DrawAllPanels();
	}

	/*
	 *	Show or hide the Join Game submenu
	 *	@author	eezstreet
	 */
	void TCPIP::ShowJoinSubmenu(bool bShow)
	{
		m_mainMenu->EnableButtons(!bShow);
		if (bShow)
		{
			ShowPanel(m_joinMenu);
		}
		else
		{
			HidePanel(m_joinMenu);
		}
	}

	bool TCPIP::HandleKeyDown(DWORD dwKey)
	{
		if (dwKey == 27) // Escape
		{
			delete cl.pActiveMenu;
			cl.pActiveMenu = new OtherMultiplayer();
			return true;
		}
		return D2Menu::HandleKeyDown(dwKey);
	}
}