#pragma once
#include "../D2Menu.hpp"

class IngameMain;
class Inventory;
class CharacterScreen;
class SkillTree;
class Automap;
class QuestLog;
class ChatPanel;

namespace D2Menus
{
	class Ingame : public D2Menu
	{
	private:
		IngameMain* m_mainPanel;
		Inventory* m_inventoryPanel;
		CharacterScreen* m_characterPanel;
		SkillTree* m_skillTreePanel;
		Automap* m_automapPanel;
		QuestLog* m_questLogPanel;
		ChatPanel* m_chatPanel;

	public:
		Ingame();
		~Ingame();

		void Draw() override;
		void Tick(DWORD dwDeltaMs) override;

		// Panel toggle accessors
		Inventory* GetInventory() { return m_inventoryPanel; }
		CharacterScreen* GetCharacterScreen() { return m_characterPanel; }
		SkillTree* GetSkillTree() { return m_skillTreePanel; }
		Automap* GetAutomap() { return m_automapPanel; }
		QuestLog* GetQuestLog() { return m_questLogPanel; }
		ChatPanel* GetChatPanel() { return m_chatPanel; }
	};
}