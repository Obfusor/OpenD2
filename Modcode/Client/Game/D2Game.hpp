#pragma once
#include "D2Unit.hpp"

/*
 *	Client-side game state.
 *	Informed by Ghidra source paths: GAME/Game.cpp, GAME/View.cpp, GAME/Roster.cpp
 *	This manages the in-game world state as seen by the client.
 */

// Forward declarations
struct D2RosterEntry;

/*
 *	View/Viewport state (from Ghidra: GAME/View.cpp functions like
 *	SetViewportCenterOffsets, GetViewportYOffsetAdjusted, ApplyPerspectiveTransform)
 */
struct D2ViewState
{
	int nCenterX;         // Viewport center X in world coords
	int nCenterY;         // Viewport center Y in world coords
	int nOffsetX;         // Pixel offset X for smooth scrolling
	int nOffsetY;         // Pixel offset Y for smooth scrolling
	bool bPerspective;    // Perspective mode enabled
};

/*
 *	Party roster entry (from Ghidra: GAME/Roster.cpp, RosterPets.cpp)
 *	Tracks other players in the game.
 */
struct D2RosterEntry
{
	DWORD           dwUnitId;
	char            szName[16];
	BYTE            nCharClass;
	WORD            wLevel;
	WORD            wPartyId;
	WORD            wX;          // Last known position
	WORD            wY;
	DWORD           dwArea;      // Current area ID
	D2RosterEntry*  pNext;
};

/*
 *	Client-side game manager.
 *	Responsible for managing the local view of the game world.
 */
class D2ClientGame
{
private:
	D2UnitList   m_units;
	D2ViewState  m_view;

	// Player state
	DWORD        m_dwLocalPlayerId;
	BYTE         m_nCurrentAct;
	WORD         m_wCurrentArea;
	DWORD        m_dwMapSeed;
	bool         m_bExpansion;
	BYTE         m_nDifficulty;
	DWORD        m_dwGameFlags;

	// Party roster
	D2RosterEntry* m_pRoster;

public:
	D2ClientGame();
	~D2ClientGame();

	// Initialization
	void Initialize(DWORD dwPlayerId, BYTE nAct, WORD wArea, DWORD dwMapSeed,
	                bool bExpansion, BYTE nDifficulty);
	void Shutdown();

	// Unit management
	D2UnitList& GetUnits() { return m_units; }
	D2UnitStrc* GetLocalPlayer();
	D2UnitStrc* AddPlayer(DWORD dwUnitId, BYTE nCharClass, const char* szName, WORD wX, WORD wY);
	D2UnitStrc* AddNPC(DWORD dwUnitId, WORD wClassId, WORD wX, WORD wY, BYTE nLife);
	void RemoveUnit(D2UnitType nType, DWORD dwUnitId);

	// View management (from Ghidra: GetViewportCenterDeltaX, GetPlayerYOffset, etc.)
	void UpdateView();
	const D2ViewState& GetView() const { return m_view; }

	// Roster management
	void AddRosterEntry(DWORD dwUnitId, BYTE nCharClass, const char* szName,
	                    WORD wLevel, WORD wPartyId);
	void RemoveRosterEntry(DWORD dwUnitId);
	void UpdateRosterPosition(DWORD dwUnitId, WORD wX, WORD wY);
	D2RosterEntry* GetRoster() { return m_pRoster; }

	// Game state accessors
	BYTE GetCurrentAct() const { return m_nCurrentAct; }
	WORD GetCurrentArea() const { return m_wCurrentArea; }
	DWORD GetGameFlags() const { return m_dwGameFlags; }
	void SetGameFlags(DWORD dwFlags) { m_dwGameFlags = dwFlags; }
	bool IsExpansion() const { return m_bExpansion; }
	BYTE GetDifficulty() const { return m_nDifficulty; }

	// Frame update
	void Tick(DWORD dwDeltaMs);
};

// Global game instance (nullptr when not in-game)
extern D2ClientGame* gpGame;
