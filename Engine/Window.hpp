#pragma once
#include "../Shared/D2Shared.hpp"

// Window management
namespace Window
{
#ifdef USE_ALLEGRO5
	void InitAllegro(D2GameConfigStrc* pConfig, OpenD2ConfigStrc* pOpenConfig);
	void ShutdownAllegro();

	// Allegro-specific accessors
	struct ALLEGRO_DISPLAY;
	struct ALLEGRO_EVENT_QUEUE;
	ALLEGRO_DISPLAY *GetDisplay();
	ALLEGRO_EVENT_QUEUE *GetEventQueue();
#else
	void InitSDL(D2GameConfigStrc* pConfig, OpenD2ConfigStrc* pOpenConfig);
	void ShutdownSDL();
#endif

	void ShowMessageBox(int nMessageBoxType, char* szTitle, char* szMessage);
	bool InFocus(DWORD nWindowID);
}
