#include "CharSelect.hpp"
#include "../Menus/CharSelect.hpp"
#include "../Menus/Main.hpp"
#include "../Menus/Loading.hpp"
#include "../Menus/CharCreate.hpp"

#define TBLTEXT_EXIT 5101
#define TBLTEXT_OK 5102
#define TBLTEXT_YES 5166
#define TBLTEXT_NO 5167
#define TBLTEXT_CREATECHAR 22743
#define TBLTEXT_DELETECHAR 22744

#define MAIN_BUTTON_DC6 "data\\global\\ui\\FrontEnd\\3WideButtonBlank.dc6"

namespace D2Panels
{
	CharSelect::CharSelect() : D2Panel()
	{
		engine->renderer->SetGlobalPalette(PAL_SKY);

		// Layout: 720px total height
		// List: 800px wide, centered. 20 rows * 24px = 480px
		// Header at y=66, rows at y=90, footer bottom at y=594
		// Keep list at y=90, move buttons down for padding
		// Footer bottom with border: ~598px. Add 35px gap → buttons at y=633
		// Buttons 35px tall → bottom at y=668. Bottom gap: 52px.
		charSelectList = new D2Widgets::CharSelectList(240, 90, 800, 480);

		// Buttons with padding below the list
		exitButton = new D2Widgets::Button(50, 640, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		createCharButton = new D2Widgets::Button(358, 640, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		deleteCharButton = new D2Widgets::Button(650, 640, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);
		okButton = new D2Widgets::Button(958, 640, MAIN_BUTTON_DC6, "3wide", 0, 1, 2, 3, 4, 5);

		AddWidget(charSelectList);
		AddWidget(createCharButton);
		AddWidget(deleteCharButton);
		AddWidget(okButton);
		AddWidget(exitButton);

		createCharButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_CREATECHAR));
		deleteCharButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_DELETECHAR));
		okButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_OK));
		exitButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_EXIT));

		createCharButton->AddEventListener(Clicked, [] {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::CharCreate();
		});

		exitButton->AddEventListener(Clicked, [] {
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::Main();
		});

		okButton->AddEventListener(Clicked, [] {
			D2Menus::CharSelect* menu = dynamic_cast<D2Menus::CharSelect*>(cl.pActiveMenu);
			if (menu != nullptr)
				menu->CharacterChosen();
		});

		deleteCharButton->AddEventListener(Clicked, [] {
			D2Menus::CharSelect* menu = dynamic_cast<D2Menus::CharSelect*>(cl.pActiveMenu);
			if (menu != nullptr)
				menu->AskForDeletionConfirmation();
		});
	}

	CharSelect::~CharSelect()
	{
		delete createCharButton;
		delete deleteCharButton;
		delete okButton;
		delete exitButton;
		delete charSelectList;
	}

	void CharSelect::Draw()
	{
		DrawAllWidgets();
	}

	void CharSelect::LoadSave(D2SaveHeader &save, char *path)
	{
		if (charSelectList != nullptr)
			charSelectList->AddSave(save, path);
	}

	void CharSelect::SelectSave(int nSaveNumber)
	{
		if (charSelectList != nullptr)
			charSelectList->Selected(nSaveNumber);
	}

	void CharSelect::AssignSelectedSave()
	{
		if (charSelectList != nullptr)
			charSelectList->LoadSave();
	}

	void CharSelect::InvalidateSelection()
	{
		deleteCharButton->Disable();
		okButton->Disable();
	}

	void CharSelect::ValidateSelection()
	{
		deleteCharButton->Enable();
		okButton->Enable();
	}

	/////////////////////////////////////////////////////////////////
	//
	//	CONFIRM DELETION PANEL

	CharDeleteConfirm::CharDeleteConfirm() : D2Panel()
	{
		confirmYesButton = new D2Widgets::Button(700, 400, "data\\global\\ui\\FrontEnd\\CancelButtonBlank.dc6", "tiny", 0, 0, 1, 1, 0, 0);
		confirmNoButton = new D2Widgets::Button(520, 400, "data\\global\\ui\\FrontEnd\\CancelButtonBlank.dc6", "tiny", 0, 0, 1, 1, 0, 0);

		renderObject = engine->renderer->AllocateObject(0);
		IGraphicsReference *backgroundHandle = engine->graphics->CreateReference("data\\global\\ui\\FrontEnd\\PopUpOkCancel2.dc6", UsagePolicy_SingleUse);
		renderObject->AttachCompositeTextureResource(backgroundHandle, 0, 1);
		renderObject->SetDrawCoords(508, 272, 264, 176);
		engine->graphics->DeleteReference(backgroundHandle);

		AddWidget(confirmYesButton);
		AddWidget(confirmNoButton);

		confirmYesButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_YES));
		confirmNoButton->AttachText(engine->TBL_FindStringFromIndex(TBLTEXT_NO));
	}

	CharDeleteConfirm::~CharDeleteConfirm()
	{
		delete confirmYesButton;
		delete confirmNoButton;
		engine->renderer->Remove(renderObject);
	}

	void CharDeleteConfirm::Draw()
	{
		renderObject->Draw();
		DrawAllWidgets();
	}
}
