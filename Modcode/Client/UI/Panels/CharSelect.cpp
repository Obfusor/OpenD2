#include "CharSelect.hpp"
#include "../Menus/CharSelect.hpp"
#include "../Menus/Main.hpp"
#include "../Menus/Loading.hpp"
#include "../Menus/CharCreate.hpp"

#define TBLTEXT_EXIT 5101		  // "Exit"
#define TBLTEXT_OK 5102			  // "OK"
#define TBLTEXT_CANCEL 5103		  // "Cancel"
#define TBLTEXT_YES 5166		  // "YES"
#define TBLTEXT_NO 5167			  // "NO"
#define TBLTEXT_CREATECHAR 22743  // "CREATE NEW CHARACTER"
#define TBLTEXT_DELETECHAR 22744  // "DELETE CHARACTER"

namespace D2Panels
{
	/*
	 *	Creates the CharSelect panel
	 */
	CharSelect::CharSelect() : D2Panel()
	{
		characterDisplayName = engine->renderer->AllocateObject(1);

		engine->renderer->SetGlobalPalette(PAL_SKY);

		// Center the two action buttons side by side
		// TallButtonBlank is 168x60. Two buttons with 20px gap = 356px total
		// Centered: (1280 - 356) / 2 = 462
		createCharButton = new D2Widgets::Button(462, 510, "data\\global\\ui\\CharSelect\\TallButtonBlank.dc6", "tallButton", 0, 0, 1, 1, 1, 1);
		deleteCharButton = new D2Widgets::Button(650, 510, "data\\global\\ui\\CharSelect\\TallButtonBlank.dc6", "tallbutton", 0, 0, 1, 1, 1, 1);

		// Exit bottom-left, OK bottom-right (medium button = 128px)
		exitButton = new D2Widgets::Button(50, 595, SMALL_BUTTON_DC6, "medium", 0, 0, 1, 1, 0, 0);
		okButton = new D2Widgets::Button(1100, 595, SMALL_BUTTON_DC6, "medium", 0, 0, 1, 1, 0, 0);

		// Center the character list: (1280 - 600) / 2 = 340
		charSelectList = new D2Widgets::CharSelectList(340, 86, 600, 400, characterDisplayName);

		AddWidget(createCharButton);
		AddWidget(deleteCharButton);
		AddWidget(okButton);
		AddWidget(exitButton);
		AddWidget(charSelectList);

		createCharButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_CREATECHAR));
		deleteCharButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_DELETECHAR));
		okButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_OK));
		exitButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_EXIT));

		createCharButton->AddEventListener(Clicked, []
										   {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::CharCreate(); });

		exitButton->AddEventListener(Clicked, []
									 {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::Main(); });

		okButton->AddEventListener(Clicked, []
								   {
			D2Menus::CharSelect* menu = dynamic_cast<D2Menus::CharSelect*>(cl.pActiveMenu);
			if (menu != nullptr)
			{
				menu->CharacterChosen();
			} });

		deleteCharButton->AddEventListener(Clicked, []
										   {
			D2Menus::CharSelect* menu = dynamic_cast<D2Menus::CharSelect*>(cl.pActiveMenu);
			if (menu != nullptr)
			{
				menu->AskForDeletionConfirmation();
			} });

		characterDisplayName->AttachFontResource(cl.font42);
	}

	/*
	 *	Destroys the CharSelect panel
	 */
	CharSelect::~CharSelect()
	{
		delete createCharButton;
		delete deleteCharButton;
		delete okButton;
		delete exitButton;
		delete charSelectList;
	}

	/*
	 *	Draws the CharSelect panel
	 */
	void CharSelect::Draw()
	{
		characterDisplayName->Draw();
		DrawAllWidgets();
	}

	/*
	 *	We just received a save from the owning menu. Add it to the CharSelectList.
	 */
	void CharSelect::LoadSave(D2SaveHeader &save, char *path)
	{
		if (charSelectList != nullptr)
		{
			charSelectList->AddSave(save, path);
		}
	}

	/*
	 *	We just got told to select a save in the save list widget.
	 */
	void CharSelect::SelectSave(int nSaveNumber)
	{
		if (charSelectList != nullptr)
		{
			charSelectList->Selected(nSaveNumber);

			// Center the character name at top of screen
			characterDisplayName->SetText(charSelectList->GetSelectedCharacterName());
			characterDisplayName->SetTextAlignment(0, 41, 1280, 30, 1, 0); // ALIGN_CENTER
			characterDisplayName->SetDrawCoords(0, 41, 1280, 30);
		}
	}

	/*
	 *	We got told to load up the save.
	 */
	void CharSelect::AssignSelectedSave()
	{
		if (charSelectList != nullptr)
		{
			charSelectList->LoadSave();
		}
	}

	/*
	 *	We just received word that we selected an invalid character.
	 */
	void CharSelect::InvalidateSelection()
	{
		deleteCharButton->Disable();
		okButton->Disable();
	}

	/*
	 *	We just received word that we selected a valid character.
	 */
	void CharSelect::ValidateSelection()
	{
		deleteCharButton->Enable();
		okButton->Enable();
	}

	/////////////////////////////////////////////////////////////////
	//
	//	CONFIRM DELETION PANEL

	/*
	 *	Initializes the delete confirmation panel
	 */
	CharDeleteConfirm::CharDeleteConfirm() : D2Panel()
	{
		// Center the confirmation dialog at 1280x720
		confirmYesButton = new D2Widgets::Button(700, 400, "data\\global\\ui\\FrontEnd\\CancelButtonBlank.dc6", "tiny", 0, 0, 1, 1, 0, 0);
		confirmNoButton = new D2Widgets::Button(520, 400, "data\\global\\ui\\FrontEnd\\CancelButtonBlank.dc6", "tiny", 0, 0, 1, 1, 0, 0);

		renderObject = engine->renderer->AllocateObject(0);

		IGraphicsReference *backgroundHandle = engine->graphics->CreateReference("data\\global\\ui\\FrontEnd\\PopUpOkCancel2.dc6", UsagePolicy_SingleUse);
		renderObject->AttachCompositeTextureResource(backgroundHandle, 0, 1);
		// Center the popup: (1280 - 264) / 2 = 508
		renderObject->SetDrawCoords(508, 272, 264, 176);
		engine->graphics->DeleteReference(backgroundHandle);

		AddWidget(confirmYesButton);
		AddWidget(confirmNoButton);

		confirmYesButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_YES));
		confirmNoButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_NO));
	}

	/*
	 *	Clean up the delete confirmation panel
	 */
	CharDeleteConfirm::~CharDeleteConfirm()
	{
		delete confirmYesButton;
		delete confirmNoButton;
		engine->renderer->Remove(renderObject);
	}

	/*
	 *	Draws the confirmation panel for deleting a character.
	 */
	void CharDeleteConfirm::Draw()
	{
		renderObject->Draw();
		DrawAllWidgets();
	}
}
