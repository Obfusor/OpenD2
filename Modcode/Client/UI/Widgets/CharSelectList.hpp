#pragma once
#include "../D2Widget.hpp"
#include "Scrollbar.hpp"
#include <vector>

namespace D2Widgets
{
	class CharSelectSave;

	enum SortColumn
	{
		SORT_TITLE,
		SORT_NAME,
		SORT_LEVEL,
		SORT_CLASS,
		SORT_DIFFICULTY,
		SORT_NONE
	};

	class CharSelectList : public D2Widget
	{
	private:
		struct CharacterSaveData
		{
			D2SaveHeader header;
			char16_t name[16];
			char path[MAX_D2PATH_ABSOLUTE];
			CharacterSaveData *pNext;
		};
		CharacterSaveData *pCharacterData;
		D2Widgets::CharSelectSave *saves;
		int nCurrentScroll;
		int nCurrentSelection;
		int nNumberSaves;

		// Sorted index for random access
		std::vector<CharSelectSave *> sortedSaves;

		// Sort state
		SortColumn sortColumn;
		bool sortAscending;

		// Double-click tracking
		DWORD dwLastClickTick;
		int nLastClickedSelection;

		void Clicked(DWORD dwX, DWORD dwY);
		void HeaderClicked(DWORD dwX);
		void ScrollUp();
		void ScrollDown();
		void EnsureSelectionVisible();
		void EnterGame();
		void RebuildSortedList();
		void SortBy(SortColumn col);

	public:
		CharSelectList(int x, int y, int w, int h);
		virtual ~CharSelectList();

		void AddSave(D2SaveHeader &header, char *path);
		char16_t *GetSelectedCharacterName();
		void LoadSave();

		virtual void OnWidgetAdded();
		virtual void Draw();
		virtual bool HandleMouseDown(DWORD dwX, DWORD dwY);
		virtual bool HandleMouseClick(DWORD dwX, DWORD dwY);
		virtual bool HandleMouseWheel(int delta);
		virtual bool HandleKeyDown(DWORD dwKey);

		void Selected(int nNewSelection);
	};
}
