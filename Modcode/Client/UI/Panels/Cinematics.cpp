#include "Cinematics.hpp"
#include "../Menus/Main.hpp"

#define MAIN_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\3WideButtonBlank.dc6"
#define SMALL_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\MediumButtonBlank.dc6"
#define TBLTEXT_CANCEL 3402

// String table indices for cinematics labels
// Using hard-coded text since the exact indices depend on the game version
static char16_t szCinematicNames[6][16] = {
    u"Act I",
    u"Act II",
    u"Act III",
    u"Act IV",
    u"Act V",
    u"End Game",
};

namespace D2Panels
{
    Cinematics::Cinematics() : D2Panel()
    {
        // Create 6 act cinematic buttons stacked vertically
        int startY = 120;
        for (int i = 0; i < 6; i++)
        {
            m_actButtons[i] = new D2Widgets::Button(265, startY + i * 40, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
            m_actButtons[i]->AttachText(szCinematicNames[i]);
            AddWidget(m_actButtons[i]);

            // Disable all cinematic buttons - video playback not implemented
            m_actButtons[i]->Disable();
        }

        m_cancelButton = new D2Widgets::Button(265, 535, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
        m_cancelButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_CANCEL));
        AddWidget(m_cancelButton);

        m_cancelButton->AddEventListener(Clicked, []
                                         {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::Main(); });
    }

    Cinematics::~Cinematics()
    {
        for (int i = 0; i < 6; i++)
        {
            delete m_actButtons[i];
        }
        delete m_cancelButton;
    }

    void Cinematics::Draw()
    {
        DrawAllWidgets();
    }
}
