#include "CharSelectList.hpp"
#include "../Panels/CharSelect.hpp"
#include "../Menus/CharSelect.hpp"
#include "CharSelectSave.hpp"
#include "../Widgets/CharSelectSave.hpp"

// Mappings for the class token
static char *gszClassTokens[D2CLASS_MAX] = {
	"AM",
	"SO",
	"NE",
	"PA",
	"BA",
	"DZ",
	"AI",
};

namespace D2Widgets
{
	/*
	 *	Creates a Character Select list widget.
	 *	This method is responsible for loading up all of the savegames.
	 *	We should maybe cache the results of the savegame loading so that going to the charselect page doesn't take a while.
	 */
	CharSelectList::CharSelectList(int x, int y, int w, int h, IRenderObject *renderedName)
		: D2Widget(x, y, w, h)
	{
		// Blank out our own data
		nNumberSaves = 0;
		nCurrentScroll = 0;
		nCurrentSelection = -1;
		saves = nullptr;
		pCharacterData = nullptr;
		dwLastClickTick = 0;
		nLastClickedSelection = -1;

		greyFrameRef = engine->graphics->CreateReference("data\\global\\ui\\CharSelect\\charselectboxgrey.dc6", UsagePolicy_Permanent);
		frameRef = engine->graphics->CreateReference("data\\global\\ui\\CharSelect\\charselectbox.dc6", UsagePolicy_Permanent);

		// Create scroll indicator text
		scrollUpArrow = engine->renderer->AllocateObject(1);
		scrollUpArrow->AttachFontResource(cl.font16);
		scrollUpArrow->SetText(u"-- More Above --");
		scrollUpArrow->SetTextColor(TextColor_Gold);
		scrollUpArrow->SetDrawCoords(x + w / 2 - 60, y - 2, 0, 0);

		scrollDownArrow = engine->renderer->AllocateObject(1);
		scrollDownArrow->AttachFontResource(cl.font16);
		scrollDownArrow->SetText(u"-- More Below --");
		scrollDownArrow->SetTextColor(TextColor_Gold);
		scrollDownArrow->SetDrawCoords(x + w / 2 - 60, y + h - 10, 0, 0);

		topName = renderedName;
	}

	/*
	 *	Destroys the character select list widget
	 */
	CharSelectList::~CharSelectList()
	{
		// Free out the entire linked list
		if (saves)
		{
			delete saves;
		}

		engine->graphics->DeleteReference(greyFrameRef);
		engine->graphics->DeleteReference(frameRef);
		engine->renderer->Remove(scrollUpArrow);
		engine->renderer->Remove(scrollDownArrow);
	}

	/*
	 *	Add a savegame to the list
	 */
	void CharSelectList::AddSave(D2SaveHeader &header, char *path)
	{
		// Allocate a character save entry
		D2Widgets::CharSelectSave *newSave = new D2Widgets::CharSelectSave(path, header);

		newSave->SetNextInChain(saves);
		saves = newSave;

		// Increment the save count.
		nNumberSaves++;
	}

	/*
	 *	This widget got added to the panel. Let's go ahead and tell the parent what we have selected.
	 *	@author	eezstreet
	 */
	void CharSelectList::OnWidgetAdded()
	{
		Selected(nCurrentSelection);
	}

	/*
	 *	Draws a Character Select list widget.
	 */
	void CharSelectList::Draw()
	{
		if (saves == nullptr)
		{
			return;
		}
		// Draw the savegames from the current scroll position
		saves->GetInChain(nCurrentScroll)->DrawLink(D2_NUM_VISIBLE_SAVES, true);

		// Draw scroll indicators when list extends beyond visible area
		if (nCurrentScroll > 0)
		{
			scrollUpArrow->Draw();
		}
		if (nCurrentScroll + D2_NUM_VISIBLE_SAVES < nNumberSaves)
		{
			scrollDownArrow->Draw();
		}
	}
	/*
	 *	Returns the name of the currently selected character.
	 *	@author	eezstreet
	 */
	char16_t *CharSelectList::GetSelectedCharacterName()
	{
		if (saves)
		{
			return saves->GetSelectedCharacterName();
		}

		return u"";
	}

	/*
	 *	Handles a mouse-down event on a CharSelectList widget.
	 */
	bool CharSelectList::HandleMouseDown(DWORD dwX, DWORD dwY)
	{
		if (dwX >= x && dwX <= x + w && dwY >= y && dwY <= y + h)
		{
			return true;
		}
		return false;
	}

	/*
	 *	Handles a mouse click event on a CharSelectList widget.
	 */
	bool CharSelectList::HandleMouseClick(DWORD dwX, DWORD dwY)
	{
		if (dwX >= x && dwX <= x + w && dwY >= y && dwY <= y + h)
		{
			Clicked(dwX - x, dwY - y);
			return true;
		}
		return false;
	}

	/*
	 *	Handles a click in a relative area
	 *	@author	eezstreet
	 */
	void CharSelectList::Clicked(DWORD dwX, DWORD dwY)
	{
		bool bClickedRight = dwX > (w / 2);
		int nClickY = dwY / (h / 4);
		int nClickSlot = (nClickY * 2) + bClickedRight;
		int nNewSelection = nClickSlot + nCurrentScroll;

		if (nNewSelection >= nNumberSaves)
		{
			nNewSelection = -1;
		}

		// Double-click detection: same char selected within 500ms
		DWORD dwNow = engine->Milliseconds();
		if (nNewSelection != -1 && nNewSelection == nLastClickedSelection &&
			(dwNow - dwLastClickTick) < 500)
		{
			// Double-click — enter the game
			EnterGame();
			return;
		}
		dwLastClickTick = dwNow;
		nLastClickedSelection = nNewSelection;

		nCurrentSelection = nNewSelection;
		Selected(nCurrentSelection);
	}

	/*
	 *	A new character slot was selected.
	 *	nNewSelection is an absolute index into the character list (not scroll-relative).
	 */
	void CharSelectList::Selected(int nNewSelection)
	{
		if (nNewSelection < 0 || nNewSelection >= nNumberSaves || saves == nullptr)
		{
			nNewSelection = -1;
		}

		if (saves != nullptr)
		{
			saves->DeselectAllInChain();
		}

		nCurrentSelection = nNewSelection;

		if (nNewSelection == -1)
		{
			// Grey out the "OK", "Delete Character" and "Convert to Expansion" buttons
			dynamic_cast<D2Panels::CharSelect *>(m_pOwner)->InvalidateSelection();
			topName->SetText(u"");
		}
		else
		{
			dynamic_cast<D2Panels::CharSelect *>(m_pOwner)->ValidateSelection();
			if (saves != nullptr)
			{
				saves->Select(nCurrentSelection);
				topName->SetText(saves->GetSelectedCharacterName());
			}
		}
	}

	/*
	 *	We need to load up the selected save.
	 *	@author	eezstreet
	 */
	void CharSelectList::LoadSave()
	{
		CharacterSaveData *pCurrent;

		if (nCurrentSelection == -1)
		{
			return;
		}

		// Advance nCurrentSelection times through the linked list
		pCurrent = pCharacterData;
		for (int i = 0; i < nCurrentSelection && pCurrent != nullptr; i++, pCurrent = pCurrent->pNext)
			;

		if (pCurrent == nullptr)
		{ // invalid selection
			return;
		}

		memcpy(&cl.currentSave.header, &pCurrent->header, sizeof(pCurrent->header));
		D2Lib::strncpyz(cl.szCurrentSave, pCurrent->path, MAX_D2PATH_ABSOLUTE);

		// Parse the full save file (stats, skills, items) into extended data
		D2Client_ParseFullSave(pCurrent->path);
	}

	/*
	 *	Scroll the character list up by 2 (one row in 2-column grid).
	 */
	void CharSelectList::ScrollUp()
	{
		if (nCurrentScroll >= 2)
		{
			nCurrentScroll -= 2;
		}
		else
		{
			nCurrentScroll = 0;
		}
	}

	/*
	 *	Scroll the character list down by 2 (one row in 2-column grid).
	 */
	void CharSelectList::ScrollDown()
	{
		if (nCurrentScroll + D2_NUM_VISIBLE_SAVES < nNumberSaves)
		{
			nCurrentScroll += 2;
			// Clamp so we don't scroll past the end
			if (nCurrentScroll + D2_NUM_VISIBLE_SAVES > nNumberSaves)
			{
				nCurrentScroll = nNumberSaves - D2_NUM_VISIBLE_SAVES;
				if (nCurrentScroll < 0)
					nCurrentScroll = 0;
			}
		}
	}

	/*
	 *	Ensure the current selection is visible in the scroll window.
	 */
	void CharSelectList::EnsureSelectionVisible()
	{
		if (nCurrentSelection < 0)
			return;

		// If selection is above the visible window, scroll up
		if (nCurrentSelection < nCurrentScroll)
		{
			// Align scroll to the row containing the selection (even index)
			nCurrentScroll = nCurrentSelection & ~1;
		}
		// If selection is below the visible window, scroll down
		else if (nCurrentSelection >= nCurrentScroll + D2_NUM_VISIBLE_SAVES)
		{
			nCurrentScroll = (nCurrentSelection & ~1) - D2_NUM_VISIBLE_SAVES + 2;
			if (nCurrentScroll < 0)
				nCurrentScroll = 0;
		}
	}

	/*
	 *	Transition to the game with the currently selected character.
	 */
	void CharSelectList::EnterGame()
	{
		if (nCurrentSelection < 0 || nCurrentSelection >= nNumberSaves)
			return;

		// Delegate to the menu's CharacterChosen which handles save assignment + loading transition
		D2Menus::CharSelect *menu = dynamic_cast<D2Menus::CharSelect *>(cl.pActiveMenu);
		if (menu != nullptr)
		{
			menu->CharacterChosen();
		}
	}

	/*
	 *	Handle keyboard navigation in the 2-column character grid.
	 *	Matches original D2Launch.dll behavior:
	 *	  Left/Right: move between columns (parity check)
	 *	  Up/Down: move by 2 (previous/next row)
	 *	  Home/End: jump to first/last character
	 */
	bool CharSelectList::HandleKeyDown(DWORD dwKey)
	{
		if (nNumberSaves <= 0)
			return false;

		int nOldSelection = nCurrentSelection;

		switch (dwKey)
		{
		case B_LEFTARROW:
			// Move left only if current index is odd (right column)
			if (nCurrentSelection > 0 && (nCurrentSelection & 1) != 0)
			{
				nCurrentSelection--;
			}
			break;

		case B_RIGHTARROW:
			// Move right only if current index is even (left column) and next exists
			if (nCurrentSelection >= 0 && (nCurrentSelection & 1) == 0 &&
				nCurrentSelection + 1 < nNumberSaves)
			{
				nCurrentSelection++;
			}
			break;

		case B_UPARROW:
			// Move up one row (subtract 2)
			if (nCurrentSelection >= 2)
			{
				nCurrentSelection -= 2;
			}
			break;

		case B_DOWNARROW:
			// Move down one row (add 2)
			if (nCurrentSelection >= 0 && nCurrentSelection + 2 < nNumberSaves)
			{
				nCurrentSelection += 2;
			}
			break;

		case B_HOME:
			nCurrentSelection = 0;
			nCurrentScroll = 0;
			break;

		case B_END:
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