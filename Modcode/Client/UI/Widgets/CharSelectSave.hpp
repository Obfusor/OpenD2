#pragma once
#include "../D2Widget.hpp"

#define D2_NUM_VISIBLE_SAVES	20
#define D2_ROW_HEIGHT			24

namespace D2Widgets
{
	/**
	 *	CharSelectSave represents one character in the list view.
	 *	It's a linked list of entries — each one draws a single row.
	 */
	class CharSelectSave : public D2Widget
	{
	private:
		CharSelectSave* nextInChain;
		char16_t charName[32];
		char16_t charClassAndLevel[32];
		char16_t charTitle[32];
		char16_t charDifficulty[16];
		char16_t charLastPlayed[16];
		char path[MAX_D2PATH_ABSOLUTE];
		bool bIsSelected, bHasTitle, bIsDeadHardcore, bIsExpansion, bIsHardcore;
		D2SaveHeader saveHeader;

		void OnSelected();

	public:
		CharSelectSave(const char* characterSave, D2SaveHeader& header);
		virtual ~CharSelectSave();

		void DrawRow(int rowX, int rowY, int rowW, bool bAltRow);

		virtual void Draw() {}
		virtual bool HandleMouseDown(DWORD dwX, DWORD dwY) { return false; }
		virtual bool HandleMouseClick(DWORD dwX, DWORD dwY) { return false; }

		void SetNextInChain(CharSelectSave* next);
		CharSelectSave* GetInChain(int counter);
		void Select(int counter);
		void DeselectAllInChain();

		char16_t* GetSelectedCharacterName();
		bool IsSelected() const { return bIsSelected; }
		bool IsHardcore() const { return bIsHardcore; }
		bool IsDeadHardcore() const { return bIsDeadHardcore; }
		bool HasTitle() const { return bHasTitle; }
		const char16_t* GetTitle() const { return charTitle; }
		const char16_t* GetName() const { return charName; }
		const char16_t* GetClassAndLevel() const { return charClassAndLevel; }
		const char16_t* GetDifficulty() const { return charDifficulty; }
		const char16_t* GetLastPlayed() const { return charLastPlayed; }
		int GetLevel() const { return saveHeader.nCharLevel; }
		int GetCharClass() const { return saveHeader.nCharClass; }
		int GetDifficultyRank() const; // 0=Normal, 1=Nightmare, 2=Hell
		const D2SaveHeader& GetSaveHeader() const { return saveHeader; }
		const char* GetPath() const { return path; }
		void SetSelected(bool sel) { bIsSelected = sel; }
	};
}
