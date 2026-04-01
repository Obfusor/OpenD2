#include "TCPIPJoin.hpp"
#include "../Menus/TCPIP.hpp"

#define DC6_PANEL_BACKGROUND "data\\global\\ui\\FrontEnd\\PopUpOkCancel2.dc6"
#define DC6_PANEL_BUTTON "data\\global\\ui\\FrontEnd\\MediumButtonBlank.dc6"

#define TBLTEXT_OK 5102
#define TBLTEXT_CANCEL 5103
#define TBLTEXT_DESC 5120

namespace D2Panels
{
	/*
	 *	Creates the join panel
	 *	@author	eezstreet
	 */
	TCPIPJoin::TCPIPJoin() : D2Panel()
	{
		// Create background
		IGraphicsReference *bgRef = engine->graphics->CreateReference(DC6_PANEL_BACKGROUND, UsagePolicy_SingleUse);
		panelBackground = engine->renderer->AllocateObject(0);
		panelBackground->AttachCompositeTextureResource(bgRef, 0, -1);

		// IP description text
		ipTextLabel = engine->renderer->AllocateObject(1);
		ipTextLabel->AttachFontResource(cl.font16);
		ipTextLabel->SetText(engine->TBL_FindStringFromIndex(TBLTEXT_DESC));

		// Create widgets
		m_okButton = new D2Widgets::Button(155, 130, DC6_PANEL_BUTTON, "medium", 0, 0, 1, 1, 0, 0);
		m_cancelButton = new D2Widgets::Button(15, 130, DC6_PANEL_BUTTON, "medium", 0, 0, 1, 1, 0, 0);
		m_ipEntry = new D2Widgets::TextEntry(25, 70, true, true, false, true);

		m_okButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_OK));
		m_cancelButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_CANCEL));
		m_okButton->SetFont(cl.fontRidiculous);
		m_cancelButton->SetFont(cl.fontRidiculous);

		AddWidget(m_okButton);
		AddWidget(m_cancelButton);
		AddWidget(m_ipEntry);

		m_okButton->AttachIdentifier("b_join");
		m_cancelButton->AttachIdentifier("b_cancel");

		m_cancelButton->AddEventListener(Clicked, []
										 {
			D2Menus::TCPIP* tcpip = dynamic_cast<D2Menus::TCPIP*>(cl.pActiveMenu);
			if (tcpip) { tcpip->ShowJoinSubmenu(false); } });
	}

	/*
	 *	Destroys the join panel
	 *	@author	eezstreet
	 */
	TCPIPJoin::~TCPIPJoin()
	{
		engine->renderer->Remove(panelBackground);
		engine->renderer->Remove(ipTextLabel);
		delete m_okButton;
		delete m_cancelButton;
		delete m_ipEntry;
	}

	/*
	 *	Draws the join panel
	 *	@author	eezstreet
	 */
	void TCPIPJoin::Draw()
	{
		panelBackground->SetDrawCoords(x, y, -1, -1);
		panelBackground->Draw();

		ipTextLabel->SetDrawCoords(x + 45, y + 25, 175, 40);
		ipTextLabel->Draw();

		DrawAllWidgets();
	}

	/*
	 *	Get the IP that we typed in
	 *	@author	eezstreet
	 */
	char16_t *TCPIPJoin::GetEnteredIP()
	{
		return m_ipEntry->GetText();
	}
}