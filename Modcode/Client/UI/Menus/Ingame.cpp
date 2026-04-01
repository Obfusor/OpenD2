#include "Ingame.hpp"
#include "../Panels/IngameMain.hpp"
#include "../Panels/Inventory.hpp"
#include "../Panels/CharacterScreen.hpp"
#include "../Panels/SkillTree.hpp"
#include "../Panels/Automap.hpp"
#include "../Panels/QuestLog.hpp"
#include "../Panels/ChatPanel.hpp"
#include "../../Game/D2Game.hpp"
#include "../../Game/D2World.hpp"

namespace D2Menus
{
    Ingame::Ingame()
    {
        m_mainPanel = new IngameMain();
        m_inventoryPanel = new Inventory();
        m_characterPanel = new CharacterScreen();
        m_skillTreePanel = new SkillTree();
        m_automapPanel = new Automap();
        m_questLogPanel = new QuestLog();
        m_chatPanel = new ChatPanel();

        AddPanel(m_mainPanel, true);
        AddPanel(m_inventoryPanel, false);
        AddPanel(m_characterPanel, false);
        AddPanel(m_skillTreePanel, false);
        AddPanel(m_automapPanel, false);
        AddPanel(m_questLogPanel, false);
        AddPanel(m_chatPanel, false);
    }

    Ingame::~Ingame()
    {
        delete m_mainPanel;
        delete m_inventoryPanel;
        delete m_characterPanel;
        delete m_skillTreePanel;
        delete m_automapPanel;
        delete m_questLogPanel;
        delete m_chatPanel;
    }

    void Ingame::Draw()
    {
        // Draw the in-game world (always call Draw so debug text shows even on failure)
        if (gpWorld != nullptr)
        {
            gpWorld->Draw();
        }

        // Draw the HUD panels on top
        DrawAllPanels();
    }

    void Ingame::Tick(DWORD dwDeltaMs)
    {
        // Tick the game state
        if (gpGame != nullptr)
        {
            gpGame->Tick(dwDeltaMs);
        }

        // Tick panels (for animations, etc.)
        D2Menu::Tick(dwDeltaMs);
    }

    bool Ingame::HandleKeyDown(DWORD keyButton)
    {
        static const float SCROLL_SPEED = 16.0f;

        if (gpWorld != nullptr)
        {
            switch (keyButton)
            {
            case B_UPARROW:
                gpWorld->ScrollCamera(0, -SCROLL_SPEED);
                return true;
            case B_DOWNARROW:
                gpWorld->ScrollCamera(0, SCROLL_SPEED);
                return true;
            case B_LEFTARROW:
                gpWorld->ScrollCamera(-SCROLL_SPEED, 0);
                return true;
            case B_RIGHTARROW:
                gpWorld->ScrollCamera(SCROLL_SPEED, 0);
                return true;
            case B_PAGEUP:
            {
                int lvl = gpWorld->GetLevelId() - 1;
                if (lvl < D2LEVEL_ACT1_TOWN)
                    lvl = D2LEVEL_ACT1_MOOMOOFARM;
                gpWorld->LoadLevel(lvl);
                return true;
            }
            case B_PAGEDOWN:
            {
                int lvl = gpWorld->GetLevelId() + 1;
                if (lvl > D2LEVEL_ACT1_MOOMOOFARM)
                    lvl = D2LEVEL_ACT1_TOWN;
                gpWorld->LoadLevel(lvl);
                return true;
            }
            }
        }

        return D2Menu::HandleKeyDown(keyButton);
    }
}
