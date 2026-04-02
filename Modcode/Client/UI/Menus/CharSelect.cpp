#include "CharSelect.hpp"
#include "Main.hpp"
#include "Loading.hpp"

namespace D2Menus
{
	CharSelect::CharSelect(char **pszSavePaths, int nNumFiles) : D2Menu()
	{
		bool bPreloadedSave = (pszSavePaths != nullptr);
		D2SaveHeader header{0};
		fs_handle f;

		// Use the same background as the main menu
		backgroundTexture = engine->graphics->CreateReference(
			"data\\global\\ui\\FrontEnd\\gameselectscreenEXP.dc6",
			UsagePolicy_Permanent);
		backgroundObject = engine->renderer->AllocateObject(0);
		backgroundObject->AttachCompositeTextureResource(backgroundTexture, 0, -1);
		// Fill-and-crop: scale 800x600 to fill 1280x720
		backgroundObject->SetDrawCoords(0, -120, 1280, 960);

		// Create the panels
		m_charSelectPanel = new D2Panels::CharSelect();
		m_charDeletePanel = new D2Panels::CharDeleteConfirm();

		AddPanel(m_charSelectPanel, true);
		AddPanel(m_charDeletePanel, false);

		// Load saves
		if (!bPreloadedSave)
			pszSavePaths = engine->FS_ListFilesInDirectory("Save", "*.d2s", &nNumFiles);

		for (int i = 0; i < nNumFiles; i++)
		{
			engine->FS_Open(pszSavePaths[i], &f, FS_READ, true);
			if (f == INVALID_HANDLE)
				continue;
			engine->FS_Read(f, &header, sizeof(header), 1);
			engine->FS_CloseFile(f);

			m_charSelectPanel->LoadSave(header, pszSavePaths[i]);
		}

		if (!bPreloadedSave)
			engine->FS_FreeFileList(pszSavePaths, nNumFiles);

		m_charSelectPanel->SelectSave(0);
	}

	CharSelect::~CharSelect()
	{
		engine->renderer->Remove(backgroundObject);
		delete m_charSelectPanel;
		delete m_charDeletePanel;
	}

	void CharSelect::Draw()
	{
		backgroundObject->Draw();
		DrawAllPanels();
	}

	void CharSelect::AskForDeletionConfirmation()
	{
		ShowPanel(m_charDeletePanel);
	}

	void CharSelect::DeleteConfirmed()
	{
		HidePanel(m_charDeletePanel);
	}

	void CharSelect::DeleteCanceled()
	{
		HidePanel(m_charDeletePanel);
	}

	bool CharSelect::CharacterChosen()
	{
		m_charSelectPanel->AssignSelectedSave();

		bJoiningGame = true;
		cl.pLoadingMenu = new D2Menus::Loading();
		cl.gamestate = GS_LOADING;
		cl.nLoadState = 0;

		return true;
	}

	bool CharSelect::HandleKeyDown(DWORD dwKey)
	{
		if (dwKey == 27) // Escape
		{
			delete cl.pActiveMenu;
			cl.pActiveMenu = new Main();
			return true;
		}
		return D2Menu::HandleKeyDown(dwKey);
	}
}
