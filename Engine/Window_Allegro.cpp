#include "Diablo2.hpp"
#include "DCC.hpp"
#include "Logging.hpp"
#include "Renderer.hpp"
#include "imgui.h"
#include "imgui_impl_allegro5.h"

///////////////////////////////////////////////////////
//
//	OpenD2 Window Management - Allegro 5 Backend
//

namespace Window
{
	static ALLEGRO_DISPLAY *gpDisplay = nullptr;
	static ALLEGRO_EVENT_QUEUE *gpEventQueue = nullptr;

	/*
	 *	Gets the Allegro display pointer (used by renderer)
	 */
	ALLEGRO_DISPLAY *GetDisplay()
	{
		return gpDisplay;
	}

	/*
	 *	Gets the event queue (used by input)
	 */
	ALLEGRO_EVENT_QUEUE *GetEventQueue()
	{
		return gpEventQueue;
	}

	/*
	 *	Inits Allegro 5 and creates a display
	 */
	void InitAllegro(D2GameConfigStrc *pConfig, OpenD2ConfigStrc *pOpenConfig)
	{
		if (!al_init())
		{
			Log::Print(PRIORITY_MESSAGE, "Failed to initialize Allegro 5");
			return;
		}

		// Install subsystems
		al_install_keyboard();
		al_install_mouse();
		al_init_image_addon();
		al_init_font_addon();
		al_init_ttf_addon();
		al_init_primitives_addon();

		// Set display flags
		int displayFlags = ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE;
		if (!pConfig->bWindowed)
		{
			displayFlags = ALLEGRO_FULLSCREEN;
		}

		al_set_new_display_flags(displayFlags);

		// Create 1280x720 display
		gpDisplay = al_create_display(1280, 720);
		if (!gpDisplay)
		{
			Log::Print(PRIORITY_MESSAGE, "Failed to create Allegro display");
			return;
		}

		al_set_window_title(gpDisplay, "Diablo II (OpenD2)");

		// Create event queue and register sources
		gpEventQueue = al_create_event_queue();
		al_register_event_source(gpEventQueue, al_get_display_event_source(gpDisplay));
		al_register_event_source(gpEventQueue, al_get_keyboard_event_source());
		al_register_event_source(gpEventQueue, al_get_mouse_event_source());

		// Initialize renderer
		Renderer::Init(pConfig, pOpenConfig, gpDisplay);

		// Initialize Dear ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		ImGui::StyleColorsDark();
		ImGui_ImplAllegro5_Init(gpDisplay);
	}

	/*
	 *	Shuts down Allegro 5
	 */
	void ShutdownAllegro()
	{
		ImGui_ImplAllegro5_Shutdown();
		ImGui::DestroyContext();

		DCC::GlobalShutdown();
		delete RenderTarget;

		if (gpEventQueue)
		{
			al_destroy_event_queue(gpEventQueue);
			gpEventQueue = nullptr;
		}
		if (gpDisplay)
		{
			al_destroy_display(gpDisplay);
			gpDisplay = nullptr;
		}

		al_uninstall_keyboard();
		al_uninstall_mouse();
	}

	/*
	 *	Show a message box
	 */
	void ShowMessageBox(int nMessageBoxType, char *szTitle, char *szMessage)
	{
		// Allegro 5 native dialog addon could be used here
		// For now, just log it
		Log::Print(PRIORITY_MESSAGE, "MessageBox [%s]: %s", szTitle, szMessage);
	}

	/*
	 *	Check if window is in focus
	 */
	bool InFocus(DWORD nWindowID)
	{
		// Allegro 5 doesn't use window IDs the same way
		// Always return true for single-window application
		(void)nWindowID;
		return true;
	}
}
