#include "CharSelectList.hpp"
#include "../Panels/CharSelect.hpp"
#include "../Menus/CharSelect.hpp"
#include "CharSelectSave.hpp"
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <algorithm>
#include <cstring>

// Local font for list rendering
static ALLEGRO_FONT *s_pListFont = nullptr;

static void EnsureFont()
{
	if (s_pListFont == nullptr)
	{
		s_pListFont = al_load_font("data/assets/fonts/ExocetBlizzardMedium.otf", 14, 0);
		if (!s_pListFont)
			s_pListFont = al_create_builtin_font();
	}
}

static void ToNarrow(const char16_t *src, char *dst, int dstLen)
{
	int i = 0;
	if (src)
	{
		for (; src[i] && i < dstLen - 1; i++)
			dst[i] = (char)(src[i] & 0x7F);
	}
	dst[i] = 0;
}

// Compare two char16_t strings case-insensitively
static int QStrCmpI(const char16_t *a, const char16_t *b)
{
	char na[64], nb[64];
	ToNarrow(a, na, sizeof(na));
	ToNarrow(b, nb, sizeof(nb));
	return _stricmp(na, nb);
}

namespace D2Widgets
{
	// Column layout (relative to x, total width 800px):
	#define COL_TITLE   8
	#define COL_NAME    128
	#define COL_LEVEL   520
	#define COL_CLASS   570
	#define COL_DIFF    690

	static const char *s_colNames[] = {"TITLE", "NAME", "LVL", "CLASS", "DIFFICULTY"};
	static const int s_colX[] = {COL_TITLE, COL_NAME, COL_LEVEL, COL_CLASS, COL_DIFF};
	static const SortColumn s_colEnum[] = {SORT_TITLE, SORT_NAME, SORT_LEVEL, SORT_CLASS, SORT_DIFFICULTY};
	#define NUM_COLUMNS 5

	static void DrawHeaderFooterRow(ALLEGRO_FONT *font, float fx, float fy, float fw, float rowH,
		SortColumn activeSort, bool sortAsc)
	{
		al_draw_filled_rectangle(fx, fy, fx + fw, fy + rowH,
			al_map_rgba(40, 35, 20, 220));

		ALLEGRO_COLOR hc = al_map_rgb(200, 170, 70);
		ALLEGRO_COLOR hcActive = al_map_rgb(255, 220, 100);
		float ty = fy + (rowH - al_get_font_line_height(font)) / 2.0f;

		for (int i = 0; i < NUM_COLUMNS; i++)
		{
			bool isActive = (s_colEnum[i] == activeSort);
			ALLEGRO_COLOR col = isActive ? hcActive : hc;

			char label[32];
			if (isActive)
				snprintf(label, sizeof(label), "%s %s", s_colNames[i], sortAsc ? "^" : "v");
			else
				snprintf(label, sizeof(label), "%s", s_colNames[i]);

			al_draw_text(font, col, fx + s_colX[i], ty, ALLEGRO_ALIGN_LEFT, label);
		}
	}

	CharSelectList::CharSelectList(int x, int y, int w, int h)
		: D2Widget(x, y, w, h)
	{
		nNumberSaves = 0;
		nCurrentScroll = 0;
		nCurrentSelection = -1;
		saves = nullptr;
		pCharacterData = nullptr;
		dwLastClickTick = 0;
		nLastClickedSelection = -1;
		sortColumn = SORT_NONE;
		sortAscending = true;
	}

	CharSelectList::~CharSelectList()
	{
		if (saves)
			delete saves;
	}

	void CharSelectList::AddSave(D2SaveHeader &header, char *path)
	{
		D2Widgets::CharSelectSave *newSave = new D2Widgets::CharSelectSave(path, header);
		newSave->SetNextInChain(saves);
		saves = newSave;
		nNumberSaves++;
	}

	void CharSelectList::RebuildSortedList()
	{
		sortedSaves.clear();
		CharSelectSave *cur = saves;
		while (cur)
		{
			sortedSaves.push_back(cur);
			cur = cur->GetInChain(1) != cur ? cur->GetInChain(1) : nullptr;
		}

		// Walk the chain properly
		sortedSaves.clear();
		for (int i = 0; i < nNumberSaves; i++)
		{
			sortedSaves.push_back(saves->GetInChain(i));
		}
	}

	void CharSelectList::SortBy(SortColumn col)
	{
		if (sortedSaves.empty())
			RebuildSortedList();

		if (col == sortColumn)
		{
			sortAscending = !sortAscending;
		}
		else
		{
			sortColumn = col;
			sortAscending = true;
		}

		// Remember which entry was selected
		CharSelectSave *selectedEntry = nullptr;
		if (nCurrentSelection >= 0 && nCurrentSelection < (int)sortedSaves.size())
			selectedEntry = sortedSaves[nCurrentSelection];

		bool asc = sortAscending;

		std::sort(sortedSaves.begin(), sortedSaves.end(),
			[col, asc](CharSelectSave *a, CharSelectSave *b) -> bool
			{
				int cmp = 0;
				switch (col)
				{
				case SORT_TITLE:
					cmp = QStrCmpI(a->GetTitle(), b->GetTitle());
					break;
				case SORT_NAME:
					cmp = QStrCmpI(a->GetName(), b->GetName());
					break;
				case SORT_LEVEL:
					cmp = a->GetLevel() - b->GetLevel();
					break;
				case SORT_CLASS:
					cmp = a->GetCharClass() - b->GetCharClass();
					break;
				case SORT_DIFFICULTY:
					cmp = a->GetDifficultyRank() - b->GetDifficultyRank();
					break;
				default:
					return false;
				}
				return asc ? (cmp < 0) : (cmp > 0);
			});

		// Restore selection to the same entry
		if (selectedEntry)
		{
			for (int i = 0; i < (int)sortedSaves.size(); i++)
			{
				if (sortedSaves[i] == selectedEntry)
				{
					nCurrentSelection = i;
					break;
				}
			}
			EnsureSelectionVisible();
		}
	}

	void CharSelectList::HeaderClicked(DWORD dwX)
	{
		// Determine which column was clicked based on X position
		int relX = (int)dwX;

		// Walk columns right to left to find the match
		SortColumn clicked = SORT_TITLE;
		for (int i = NUM_COLUMNS - 1; i >= 0; i--)
		{
			if (relX >= s_colX[i])
			{
				clicked = s_colEnum[i];
				break;
			}
		}

		SortBy(clicked);
	}

	void CharSelectList::OnWidgetAdded()
	{
		RebuildSortedList();
		Selected(nCurrentSelection);
	}

	void CharSelectList::Draw()
	{
		EnsureFont();
		if (!s_pListFont)
			return;

		// Build sorted list on first draw if not yet built
		if (sortedSaves.empty() && nNumberSaves > 0)
			RebuildSortedList();

		int rowH = D2_ROW_HEIGHT;
		int visibleRows = D2_NUM_VISIBLE_SAVES;
		int listH = visibleRows * rowH;

		float panelTop = (float)y - (float)rowH;
		float panelBottom = (float)(y + listH) + (float)rowH;

		// Dark background panel
		al_draw_filled_rectangle((float)x - 4, panelTop - 4,
			(float)(x + w + 4), panelBottom + 4,
			al_map_rgba(0, 0, 0, 160));

		// Gold/bronze 2px border
		al_draw_rectangle((float)x - 4, panelTop - 4,
			(float)(x + w + 4), panelBottom + 4,
			al_map_rgb(160, 130, 50), 2.0f);

		// Header row
		DrawHeaderFooterRow(s_pListFont, (float)x, panelTop, (float)w, (float)rowH,
			sortColumn, sortAscending);

		// Footer row
		DrawHeaderFooterRow(s_pListFont, (float)x, (float)(y + listH), (float)w, (float)rowH,
			sortColumn, sortAscending);

		if (sortedSaves.empty())
			return;

		// Draw each visible row
		for (int i = 0; i < visibleRows && (nCurrentScroll + i) < (int)sortedSaves.size(); i++)
		{
			int saveIdx = nCurrentScroll + i;
			CharSelectSave *entry = sortedSaves[saveIdx];
			if (!entry)
				break;

			float rowY = (float)(y + i * rowH);
			bool bAltRow = (i % 2 == 1);
			bool bSelected = (saveIdx == nCurrentSelection);

			// Row background
			if (bSelected)
			{
				al_draw_filled_rectangle((float)x, rowY, (float)(x + w), rowY + rowH,
					al_map_rgba(60, 50, 20, 220));
				al_draw_filled_rectangle((float)x, rowY, (float)x + 3, rowY + rowH,
					al_map_rgb(200, 170, 60));
			}
			else if (bAltRow)
			{
				al_draw_filled_rectangle((float)x, rowY, (float)(x + w), rowY + rowH,
					al_map_rgba(20, 20, 25, 180));
			}
			else
			{
				al_draw_filled_rectangle((float)x, rowY, (float)(x + w), rowY + rowH,
					al_map_rgba(12, 12, 16, 180));
			}

			char tmp[64];
			float textY = rowY + (rowH - al_get_font_line_height(s_pListFont)) / 2.0f;

			// Row text color
			ALLEGRO_COLOR nameColor;
			if (entry->IsHardcore())
				nameColor = al_map_rgb(255, 80, 80);
			else if (entry->HasTitle())
				nameColor = al_map_rgb(200, 170, 60);
			else
				nameColor = al_map_rgb(220, 220, 220);

			// TITLE
			if (entry->HasTitle())
			{
				ToNarrow(entry->GetTitle(), tmp, sizeof(tmp));
				al_draw_text(s_pListFont, nameColor, (float)(x + COL_TITLE), textY, ALLEGRO_ALIGN_LEFT, tmp);
			}

			// NAME (faux-bold)
			char nameStr[64];
			ToNarrow(entry->GetName(), nameStr, sizeof(nameStr));
			al_draw_text(s_pListFont, nameColor, (float)(x + COL_NAME), textY, ALLEGRO_ALIGN_LEFT, nameStr);
			al_draw_text(s_pListFont, nameColor, (float)(x + COL_NAME + 1), textY, ALLEGRO_ALIGN_LEFT, nameStr);

			// LEVEL + CLASS
			char classAndLevel[64];
			ToNarrow(entry->GetClassAndLevel(), classAndLevel, sizeof(classAndLevel));

			char levelStr[16] = "";
			char classStr[32] = "";
			int lvl = 0;
			char clsName[32] = "";
			if (sscanf(classAndLevel, "Level %d %31[^\n]", &lvl, clsName) == 2)
			{
				snprintf(levelStr, sizeof(levelStr), "%d", lvl);
				snprintf(classStr, sizeof(classStr), "%s", clsName);
			}
			else
			{
				snprintf(classStr, sizeof(classStr), "%s", classAndLevel);
			}

			al_draw_text(s_pListFont, al_map_rgb(180, 180, 180),
				(float)(x + COL_LEVEL), textY, ALLEGRO_ALIGN_LEFT, levelStr);
			al_draw_text(s_pListFont, al_map_rgb(180, 180, 180),
				(float)(x + COL_CLASS), textY, ALLEGRO_ALIGN_LEFT, classStr);

			// DIFFICULTY
			char diffCol[32];
			ToNarrow(entry->GetDifficulty(), diffCol, sizeof(diffCol));

			ALLEGRO_COLOR diffColor;
			if (strcmp(diffCol, "Hell") == 0)
				diffColor = al_map_rgb(255, 100, 100);
			else if (strcmp(diffCol, "Nightmare") == 0)
				diffColor = al_map_rgb(200, 170, 60);
			else
				diffColor = al_map_rgb(140, 140, 140);

			al_draw_text(s_pListFont, diffColor,
				(float)(x + COL_DIFF), textY, ALLEGRO_ALIGN_LEFT, diffCol);
		}

		// Scroll indicators
		if (nCurrentScroll > 0)
		{
			al_draw_text(s_pListFont, al_map_rgb(200, 170, 60),
				(float)(x + w / 2), panelTop + 4, ALLEGRO_ALIGN_CENTER, "^ More Above ^");
		}
		if (nCurrentScroll + visibleRows < nNumberSaves)
		{
			al_draw_text(s_pListFont, al_map_rgb(200, 170, 60),
				(float)(x + w / 2), (float)(y + listH) + 4, ALLEGRO_ALIGN_CENTER, "v More Below v");
		}
	}

	char16_t *CharSelectList::GetSelectedCharacterName()
	{
		if (nCurrentSelection >= 0 && nCurrentSelection < (int)sortedSaves.size())
			return sortedSaves[nCurrentSelection]->GetSelectedCharacterName();
		if (saves)
			return saves->GetSelectedCharacterName();
		return u"";
	}

	bool CharSelectList::HandleMouseDown(DWORD dwX, DWORD dwY)
	{
		// Include header row in clickable area
		int headerTop = y - D2_ROW_HEIGHT;
		if ((int)dwX >= x && (int)dwX <= x + w && (int)dwY >= headerTop && (int)dwY <= y + h)
			return true;
		return false;
	}

	bool CharSelectList::HandleMouseClick(DWORD dwX, DWORD dwY)
	{
		int headerTop = y - D2_ROW_HEIGHT;

		if ((int)dwX < x || (int)dwX > x + w)
			return false;

		// Click on header row → sort
		if ((int)dwY >= headerTop && (int)dwY < y)
		{
			HeaderClicked((DWORD)((int)dwX - x));
			return true;
		}

		// Click on footer row → sort (same behavior)
		int footerTop = y + (D2_NUM_VISIBLE_SAVES * D2_ROW_HEIGHT);
		if ((int)dwY >= footerTop && (int)dwY < footerTop + D2_ROW_HEIGHT)
		{
			HeaderClicked((DWORD)((int)dwX - x));
			return true;
		}

		// Click on data rows
		if ((int)dwY >= y && (int)dwY <= y + h)
		{
			Clicked(dwX - x, dwY - y);
			return true;
		}

		return false;
	}

	void CharSelectList::Clicked(DWORD dwX, DWORD dwY)
	{
		int clickedRow = (int)dwY / D2_ROW_HEIGHT;
		int nNewSelection = clickedRow + nCurrentScroll;

		if (nNewSelection >= nNumberSaves)
			nNewSelection = -1;

		// Double-click detection
		DWORD dwNow = engine->Milliseconds();
		if (nNewSelection != -1 && nNewSelection == nLastClickedSelection &&
			(dwNow - dwLastClickTick) < 500)
		{
			EnterGame();
			return;
		}
		dwLastClickTick = dwNow;
		nLastClickedSelection = nNewSelection;

		nCurrentSelection = nNewSelection;
		Selected(nCurrentSelection);
	}

	void CharSelectList::Selected(int nNewSelection)
	{
		if (nNewSelection < 0 || nNewSelection >= nNumberSaves)
			nNewSelection = -1;

		// Deselect all
		for (auto *s : sortedSaves)
			s->SetSelected(false);

		nCurrentSelection = nNewSelection;

		if (nNewSelection == -1)
		{
			dynamic_cast<D2Panels::CharSelect *>(m_pOwner)->InvalidateSelection();
		}
		else
		{
			dynamic_cast<D2Panels::CharSelect *>(m_pOwner)->ValidateSelection();
			if (nNewSelection < (int)sortedSaves.size())
				sortedSaves[nNewSelection]->SetSelected(true);
		}
	}

	void CharSelectList::LoadSave()
	{
		if (nCurrentSelection < 0 || nCurrentSelection >= (int)sortedSaves.size())
			return;

		// The saves linked list still holds the path data — but we can get it from sortedSaves too
		// For now, use the linked list via pCharacterData
		CharacterSaveData *pCurrent = pCharacterData;
		for (int i = 0; i < nCurrentSelection && pCurrent != nullptr; i++, pCurrent = pCurrent->pNext)
			;

		if (pCurrent == nullptr)
			return;

		memcpy(&cl.currentSave.header, &pCurrent->header, sizeof(pCurrent->header));
		D2Lib::strncpyz(cl.szCurrentSave, pCurrent->path, MAX_D2PATH_ABSOLUTE);

		D2Client_ParseFullSave(pCurrent->path);
	}

	void CharSelectList::ScrollUp()
	{
		if (nCurrentScroll > 0)
			nCurrentScroll--;
	}

	void CharSelectList::ScrollDown()
	{
		if (nCurrentScroll + D2_NUM_VISIBLE_SAVES < nNumberSaves)
			nCurrentScroll++;
	}

	void CharSelectList::EnsureSelectionVisible()
	{
		if (nCurrentSelection < 0)
			return;

		if (nCurrentSelection < nCurrentScroll)
			nCurrentScroll = nCurrentSelection;
		else if (nCurrentSelection >= nCurrentScroll + D2_NUM_VISIBLE_SAVES)
			nCurrentScroll = nCurrentSelection - D2_NUM_VISIBLE_SAVES + 1;
	}

	void CharSelectList::EnterGame()
	{
		if (nCurrentSelection < 0 || nCurrentSelection >= nNumberSaves)
			return;

		D2Menus::CharSelect *menu = dynamic_cast<D2Menus::CharSelect *>(cl.pActiveMenu);
		if (menu != nullptr)
			menu->CharacterChosen();
	}

	bool CharSelectList::HandleMouseWheel(int delta)
	{
		if (nNumberSaves <= D2_NUM_VISIBLE_SAVES)
			return false;

		if (delta > 0)
		{
			for (int i = 0; i < delta && nCurrentScroll > 0; i++)
				nCurrentScroll--;
		}
		else if (delta < 0)
		{
			for (int i = 0; i < -delta && nCurrentScroll + D2_NUM_VISIBLE_SAVES < nNumberSaves; i++)
				nCurrentScroll++;
		}
		return true;
	}

	bool CharSelectList::HandleKeyDown(DWORD dwKey)
	{
		if (nNumberSaves <= 0)
			return false;

		int nOldSelection = nCurrentSelection;

		switch (dwKey)
		{
		case B_UPARROW:
			if (nCurrentSelection > 0)
				nCurrentSelection--;
			else if (nCurrentSelection < 0 && nNumberSaves > 0)
				nCurrentSelection = 0;
			break;

		case B_DOWNARROW:
			if (nCurrentSelection >= 0 && nCurrentSelection + 1 < nNumberSaves)
				nCurrentSelection++;
			else if (nCurrentSelection < 0 && nNumberSaves > 0)
				nCurrentSelection = 0;
			break;

		case B_HOME:
			nCurrentSelection = 0;
			nCurrentScroll = 0;
			break;

		case B_END:
			nCurrentSelection = nNumberSaves - 1;
			break;

		case B_PAGEUP:
			nCurrentSelection -= D2_NUM_VISIBLE_SAVES;
			if (nCurrentSelection < 0)
				nCurrentSelection = 0;
			break;

		case B_PAGEDOWN:
			nCurrentSelection += D2_NUM_VISIBLE_SAVES;
			if (nCurrentSelection >= nNumberSaves)
				nCurrentSelection = nNumberSaves - 1;
			break;

		default:
			return false;
		}

		if (nCurrentSelection != nOldSelection)
		{
			EnsureSelectionVisible();
			Selected(nCurrentSelection);
		}
		return true;
	}
}
