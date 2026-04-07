#include "D2Server.hpp"
#include "D2ServerGame.hpp"

D2ModuleImportStrc *engine = nullptr;
D2GameConfigStrc *config = nullptr;
OpenD2ConfigStrc *openConfig = nullptr;

// Minimal server state
static struct
{
	bool bKillServer;
	DWORD dwLastTick;
	bool bGameStarted;
} sv;

// Server tick rate: 25 FPS = 40ms per frame
static const DWORD SERVER_TICK_MS = 40;

/*
 *	Called every frame on the server.
 *	Gates execution to 25 FPS and drives the game tick.
 *	@author	eezstreet
 */
static void D2Server_RunFrame()
{
	if (gpServerGame == nullptr || !gpServerGame->IsInitialized())
	{
		return;
	}

	DWORD dwNow = engine->Milliseconds();
	if (dwNow - sv.dwLastTick < SERVER_TICK_MS)
	{
		return;
	}
	sv.dwLastTick = dwNow;

	gpServerGame->Tick();
}

/*
 *	Start the game. Called once when the client signals readiness.
 */
static void D2Server_StartGame()
{
	if (sv.bGameStarted)
	{
		return;
	}

	if (gpServerGame == nullptr)
	{
		gpServerGame = new D2ServerGame();
	}

	gpServerGame->InitFromSave(config, openConfig);
	sv.bGameStarted = true;
	sv.dwLastTick = engine->Milliseconds();
}

/*
 *	Called when initializing the server for the first time.
 *	@author	eezstreet
 */
static void D2Server_InitializeServer(D2GameConfigStrc *pConfig, OpenD2ConfigStrc *pOpenConfig)
{
	config = pConfig;
	openConfig = pOpenConfig;
	sv.bKillServer = false;
	sv.dwLastTick = 0;
	sv.bGameStarted = false;

	D2Common_Init(engine, pConfig, pOpenConfig);
}

/*
 *	This gets called whenever the module is cleaned up.
 */
static void D2Server_Shutdown()
{
	if (gpServerGame != nullptr)
	{
		delete gpServerGame;
		gpServerGame = nullptr;
	}
	sv.bGameStarted = false;
}

/*
 *	Handle a packet received from the client.
 */
static bool D2Server_HandlePacket(D2Packet *pPacket)
{
	if (pPacket == nullptr)
	{
		return false;
	}

	// If the game hasn't started yet, check for join/save packets
	// that the engine handles. Once in-game, dispatch to the game.
	if (gpServerGame != nullptr && gpServerGame->IsInitialized())
	{
		gpServerGame->HandleClientPacket(pPacket);
	}

	return true;
}

/*
 *	This gets called every frame. We return the next module to run after this one.
 */
static OpenD2Modules D2Server_RunModuleFrame(D2GameConfigStrc *pConfig, OpenD2ConfigStrc *pOpenConfig)
{
	if (config == nullptr && openConfig == nullptr && pConfig != nullptr && pOpenConfig != nullptr)
	{
		// now is our chance! initialize!
		D2Server_InitializeServer(pConfig, pOpenConfig);
	}

	// Start the game when client is ready (for local server)
	if (!sv.bGameStarted && config != nullptr)
	{
		D2Server_StartGame();
	}

	D2Server_RunFrame();

	if (sv.bKillServer)
	{
		return MODULE_NONE;
	}

	return MODULE_CLIENT;
}

/*
 *	GetModuleAPI allows us to exchange a series of function pointers with the engine.
 *	This is effectively the entry point for the library.
 */
static D2ModuleExportStrc gExports{0};
extern "C"
{
	D2EXPORT D2ModuleExportStrc *GetModuleAPI(D2ModuleImportStrc *pImports)
	{
		if (pImports == nullptr)
		{
			return nullptr;
		}

		if (pImports->nApiVersion != D2SERVERAPI_VERSION)
		{ // not the right API version
			return nullptr;
		}

		engine = pImports;

		gExports.nApiVersion = D2SERVERAPI_VERSION;
		gExports.RunModuleFrame = D2Server_RunModuleFrame;
		gExports.CleanupModule = D2Server_Shutdown;
		gExports.HandlePacket = D2Server_HandlePacket;

		return &gExports;
	}
}
