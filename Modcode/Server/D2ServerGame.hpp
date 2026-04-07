#pragma once
#include "D2Server.hpp"
#include "../Client/Game/D2Unit.hpp"

/*
 *	Server-side game state manager.
 *	This is the authoritative owner of all game world state.
 *	The client receives state updates via packets sent from here.
 *
 *	Slice S00a: Initial skeleton with unit storage and tick loop.
 */

class D2ServerGame
{
private:
	D2UnitList m_units;

	// Game state
	DWORD m_dwFrameCount;
	DWORD m_dwMapSeed;
	BYTE m_nCurrentAct;
	BYTE m_nDifficulty;
	bool m_bExpansion;
	bool m_bInitialized;

	// Local player info (from save data)
	DWORD m_dwLocalPlayerId;

public:
	D2ServerGame();
	~D2ServerGame();

	// Initialization
	void InitFromSave(D2GameConfigStrc *pConfig, OpenD2ConfigStrc *pOpenConfig);

	// Frame update (called at 25 FPS)
	void Tick();

	// Unit management
	D2UnitStrc *CreatePlayer(DWORD dwUnitId, BYTE nCharClass, const char *szName,
							 WORD wX, WORD wY);
	D2UnitStrc *CreateMonster(DWORD dwUnitId, WORD wClassId, WORD wX, WORD wY);
	void RemoveUnit(D2UnitType nType, DWORD dwUnitId);
	D2UnitStrc *FindUnit(D2UnitType nType, DWORD dwUnitId);
	D2UnitList &GetUnits() { return m_units; }

	// Packet handling
	void HandleClientPacket(D2Packet *pPacket);

	// Accessors
	DWORD GetFrameCount() const { return m_dwFrameCount; }
	bool IsInitialized() const { return m_bInitialized; }

private:
	// Send a server packet to the local player (client mask = 1 for single player)
	void SendToClient(D2Packet *pPacket);

	// Per-unit-type update functions (to be expanded in later slices)
	void UpdatePlayers();
	void UpdateMonsters();
};

// Global server game instance (nullptr when not in a game)
extern D2ServerGame *gpServerGame;
