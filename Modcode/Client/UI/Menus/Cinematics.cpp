#include "Cinematics.hpp"
#include "../Panels/Cinematics.hpp"

namespace D2Menus
{
    /*
     *	Creates the Cinematics selection menu.
     *	Background: CinematicsSelectionEXP.dc6
     *	Shows act cinematics buttons.
     */
    Cinematics::Cinematics() : D2Menu()
    {
        engine->renderer->SetGlobalPalette(PAL_SKY);

        background = engine->graphics->CreateReference(
            "data\\global\\ui\\FrontEnd\\CinematicsSelectionEXP.dc6",
            UsagePolicy_SingleUse);

        backgroundObject = engine->renderer->AllocateObject(0);
        backgroundObject->AttachCompositeTextureResource(background, 0, -1);
        backgroundObject->SetDrawCoords(0, 0, 800, 600);
        backgroundObject->SetPalshift(0);

        // Title
        titleText = engine->renderer->AllocateObject(1);
        titleText->AttachFontResource(cl.font30);
        titleText->SetText(u"Select Cinematic");
        titleText->SetDrawCoords(0, 50, 800, 30);

        m_panel = new D2Panels::Cinematics();
        AddPanel(m_panel);
    }

    Cinematics::~Cinematics()
    {
        engine->renderer->Remove(backgroundObject);
        engine->renderer->Remove(titleText);
        delete m_panel;
        m_panel = nullptr;
        m_visiblePanels = nullptr;
    }

    void Cinematics::Draw()
    {
        backgroundObject->Draw();
        titleText->Draw();
        DrawAllPanels();
    }
}
