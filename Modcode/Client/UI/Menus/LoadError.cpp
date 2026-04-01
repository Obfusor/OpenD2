#include "LoadError.hpp"

namespace D2Menus
{
	/*
	 *	Creates the LoadError menu
	 *	@author	eezstreet
	 */
	LoadError::LoadError(WORD wStringIndex) : D2Menu()
	{
		szErrorText = engine->TBL_FindStringFromIndex(wStringIndex);

		errorTextObject = engine->renderer->AllocateObject(1);
		errorTextObject->AttachFontResource(cl.font16);
		errorTextObject->SetText(szErrorText);
		errorTextObject->SetDrawCoords(20, 200, 780, 20);
	}

	LoadError::~LoadError()
	{
		engine->renderer->Remove(errorTextObject);
	}

	/*
	 *	Draw the LoadError menu
	 *	@author	eezstreet
	 */
	void LoadError::Draw()
	{
		errorTextObject->Draw();
	}

	/*
	 *	Handle a mouse click event on the LoadError menu.
	 *	@author	eezstreet
	 */
	bool LoadError::HandleMouseClicked(DWORD dwX, DWORD dwY)
	{
		D2Client_GoToContextMenu();
		return true;
	}
}