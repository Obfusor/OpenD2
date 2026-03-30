#include "Ingame.hpp"
#include "../Panels/IngameMain.hpp"
#include "../Panels/Inventory.hpp"
#include "../Panels/CharacterScreen.hpp"
#include "../Panels/SkillTree.hpp"
#include "../Panels/Automap.hpp"
#include "../Panels/QuestLog.hpp"
#include "../Panels/ChatPanel.hpp"
#include "../../Game/D2Game.hpp"

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
	}

	void Ingame::Draw()
	{
		// Draw the in-game world (placeholder: clear screen)
		// TODO: world rendering (tiles, units, effects)

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
}

