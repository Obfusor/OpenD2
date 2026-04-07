#include "D2ServerGame.hpp"

// engine is declared in D2Server.cpp
extern D2ModuleImportStrc *engine;

D2ServerGame *gpServerGame = nullptr;

/*
 *	Next unit ID counter. Incremented each time a unit is created.
 *	Starts at 1 (0 is reserved/invalid).
 */
static DWORD sgdwNextUnitId = 1;

D2ServerGame::D2ServerGame()
	: m_dwFrameCount(0),
	  m_dwMapSeed(0),
	  m_nCurrentAct(0),
	  m_nDifficulty(0),
	  m_bExpansion(false),
	  m_bInitialized(false),
	  m_dwLocalPlayerId(0)
{
}

D2ServerGame::~D2ServerGame()
{
	m_units.Clear();
}

/*
 *	Initialize the server game from game config.
 *	In single-player, this reads from the config's character settings.
 *	Creates the local player unit and sends ASSIGNPLAYER to the client.
 */
/*
 *	Derive character class from the boolean flags in D2GameConfigStrc.
 */
static BYTE DeriveCharClass(D2GameConfigStrc *pConfig)
{
	if (pConfig->bAmazon)
		return D2CLASS_AMAZON;
	if (pConfig->bSorceress)
		return D2CLASS_SORCERESS;
	if (pConfig->bNecromancer)
		return D2CLASS_NECROMANCER;
	if (pConfig->bPaladin)
		return D2CLASS_PALADIN;
	if (pConfig->bBarbarian)
		return D2CLASS_BARBARIAN;
	if (pConfig->bDruid)
		return D2CLASS_DRUID;
	if (pConfig->bAssassin)
		return D2CLASS_ASSASSIN;
	return D2CLASS_BARBARIAN; // fallback
}

void D2ServerGame::InitFromSave(D2GameConfigStrc *pConfig, OpenD2ConfigStrc *pOpenConfig)
{
	if (m_bInitialized)
	{
		return;
	}

	m_nDifficulty = pConfig->nDifficulty;
	m_nCurrentAct = (BYTE)pConfig->dwAct;
	m_bExpansion = (pConfig->dwExpansion != 0);

	BYTE nCharClass = DeriveCharClass(pConfig);

	// Create the local player
	// Position 30,30 is a reasonable town center spawn point
	D2UnitStrc *pPlayer = CreatePlayer(
		sgdwNextUnitId++,
		nCharClass,
		pConfig->szCharName,
		30, 30);

	if (pPlayer != nullptr)
	{
		m_dwLocalPlayerId = pPlayer->dwUnitId;

		// Note: In local server mode, the client creates its own player unit
		// at load state 9 (D2Client.cpp) because the server frame runs before
		// the client has created D2ClientGame. The ASSIGNPLAYER packet would
		// arrive while gpGame is still nullptr and be dropped.
		// Once we implement proper loading synchronization, the server will
		// send ASSIGNPLAYER and the client workaround will be removed.
	}

	m_bInitialized = true;

	engine->Print(PRIORITY_MESSAGE, "D2Server: Game initialized, player '%s' created (id=%d)",
				  pConfig->szCharName, m_dwLocalPlayerId);
}

/*
 *	Main server tick. Called at 25 FPS.
 *	Iterates all unit types and runs their update logic.
 */
void D2ServerGame::Tick()
{
	m_dwFrameCount++;

	UpdatePlayers();
	UpdateMonsters();
}

/*
 *	Create a player unit and add it to the unit list.
 */
D2UnitStrc *D2ServerGame::CreatePlayer(DWORD dwUnitId, BYTE nCharClass,
									   const char *szName, WORD wX, WORD wY)
{
	D2UnitStrc *pUnit = new D2UnitStrc();
	memset(pUnit, 0, sizeof(D2UnitStrc));

	pUnit->nUnitType = UNIT_PLAYER;
	pUnit->dwClassId = nCharClass;
	pUnit->dwUnitId = dwUnitId;
	pUnit->dwMode = PLRMODE_TN; // Town neutral
	pUnit->dwFlags = UNITSTATE_NONE;
	pUnit->wX = wX;
	pUnit->wY = wY;
	pUnit->wTargetX = wX;
	pUnit->wTargetY = wY;
	pUnit->nCharClass = nCharClass;

	if (szName != nullptr)
	{
		strncpy(pUnit->szName, szName, 15);
		pUnit->szName[15] = '\0';
	}

	// Default stats (will be populated from save data in slice S07)
	pUnit->dwHP = 100 << 8;
	pUnit->dwHPMax = 100 << 8;
	pUnit->dwMana = 50 << 8;
	pUnit->dwManaMax = 50 << 8;
	pUnit->dwStamina = 100 << 8;
	pUnit->dwStaminaMax = 100 << 8;
	pUnit->dwLevel = 1;
	pUnit->nLightRadius = 13;

	m_units.AddUnit(pUnit);
	return pUnit;
}

/*
 *	Create a monster/NPC unit. (Expanded in slice S05)
 */
D2UnitStrc *D2ServerGame::CreateMonster(DWORD dwUnitId, WORD wClassId,
										WORD wX, WORD wY)
{
	D2UnitStrc *pUnit = new D2UnitStrc();
	memset(pUnit, 0, sizeof(D2UnitStrc));

	pUnit->nUnitType = UNIT_MONSTER;
	pUnit->dwClassId = wClassId;
	pUnit->dwUnitId = dwUnitId;
	pUnit->dwMode = MONMODE_NU; // Neutral
	pUnit->dwFlags = UNITSTATE_NONE;
	pUnit->wX = wX;
	pUnit->wY = wY;
	pUnit->wTargetX = wX;
	pUnit->wTargetY = wY;

	pUnit->dwHP = 50 << 8;
	pUnit->dwHPMax = 50 << 8;
	pUnit->nLightRadius = 0;

	m_units.AddUnit(pUnit);
	return pUnit;
}

void D2ServerGame::RemoveUnit(D2UnitType nType, DWORD dwUnitId)
{
	m_units.RemoveUnit(nType, dwUnitId);
}

D2UnitStrc *D2ServerGame::FindUnit(D2UnitType nType, DWORD dwUnitId)
{
	return m_units.FindUnit(nType, dwUnitId);
}

/*
 *	Handle a packet from the client.
 *	This is the server-side dispatch for client actions.
 *	(Expanded in slices S02, S06, S09)
 */
void D2ServerGame::HandleClientPacket(D2Packet *pPacket)
{
	BYTE *data = (BYTE *)pPacket;

	switch (pPacket->nPacketType)
	{
	case D2CPACKET_WALKCOORD:
	case D2CPACKET_RUNCOORD:
	{
		// Walk/run to coordinate
		// Format: BYTE type, WORD x, WORD y
		WORD wTargetX = *(WORD *)(data + 1);
		WORD wTargetY = *(WORD *)(data + 3);
		bool bRunning = (pPacket->nPacketType == D2CPACKET_RUNCOORD);

		D2UnitStrc *pPlayer = m_units.FindUnit(UNIT_PLAYER, m_dwLocalPlayerId);
		if (pPlayer != nullptr)
		{
			pPlayer->wTargetX = wTargetX;
			pPlayer->wTargetY = wTargetY;
			pPlayer->dwMode = bRunning ? PLRMODE_RN : PLRMODE_WL;

			// Send PLAYERMOVECOORD to client
			// Format: BYTE type(0x0F), BYTE unitType, DWORD unitId, BYTE moveType,
			//         WORD targetX, WORD targetY, 0x00, WORD currentX, WORD currentY
			D2Packet response;
			memset(&response, 0, sizeof(response));
			response.nPacketType = D2SPACKET_PLAYERMOVECOORD;

			BYTE *rdata = (BYTE *)&response;
			*(rdata + 1) = UNIT_PLAYER;
			*(DWORD *)(rdata + 2) = pPlayer->dwUnitId;
			*(rdata + 6) = bRunning ? 0x17 : 0x01;
			*(WORD *)(rdata + 7) = wTargetX;
			*(WORD *)(rdata + 9) = wTargetY;
			*(rdata + 11) = 0x00;
			*(WORD *)(rdata + 12) = pPlayer->wX;
			*(WORD *)(rdata + 14) = pPlayer->wY;

			SendToClient(&response);
		}
		break;
	}
	case D2CPACKET_INTERACT:
	{
		// S06: Will handle unit interaction
		// Format: BYTE type(0x13), DWORD type, DWORD id
		break;
	}
	default:
		break;
	}
}

void D2ServerGame::SendToClient(D2Packet *pPacket)
{
	// Client mask of 1 = send to player 0 (local player in single-player)
	engine->NET_SendServerPacket(1, pPacket);
}

/*
 *	Update all player units.
 *	Moves players toward their target position each tick.
 */
void D2ServerGame::UpdatePlayers()
{
	D2UnitStrc *pPlayer = m_units.FindUnit(UNIT_PLAYER, m_dwLocalPlayerId);
	if (pPlayer == nullptr)
		return;

	// Only move if we have a target different from current position
	if (pPlayer->dwMode != PLRMODE_WL && pPlayer->dwMode != PLRMODE_RN)
		return;

	if (pPlayer->wX == pPlayer->wTargetX && pPlayer->wY == pPlayer->wTargetY)
	{
		// Arrived at destination — stop
		pPlayer->dwMode = PLRMODE_TN;

		D2Packet packet;
		memset(&packet, 0, sizeof(packet));
		packet.nPacketType = D2SPACKET_PLAYERSTOP;

		BYTE *data = (BYTE *)&packet;
		*(data + 1) = UNIT_PLAYER;
		*(DWORD *)(data + 2) = pPlayer->dwUnitId;
		*(data + 6) = 0x00;
		*(WORD *)(data + 7) = pPlayer->wX;
		*(WORD *)(data + 9) = pPlayer->wY;
		*(data + 11) = 0x00;
		*(data + 12) = (BYTE)(pPlayer->dwHP >> 8); // life as byte

		SendToClient(&packet);
		return;
	}

	// Move one step toward target (straight line, 1 tile per tick)
	// Speed: walk = 1 tile/tick, run = 2 tiles/tick
	int speed = (pPlayer->dwMode == PLRMODE_RN) ? 2 : 1;

	int dx = (int)pPlayer->wTargetX - (int)pPlayer->wX;
	int dy = (int)pPlayer->wTargetY - (int)pPlayer->wY;

	// Clamp movement to speed
	if (dx > speed) dx = speed;
	if (dx < -speed) dx = -speed;
	if (dy > speed) dy = speed;
	if (dy < -speed) dy = -speed;

	pPlayer->wX = (WORD)((int)pPlayer->wX + dx);
	pPlayer->wY = (WORD)((int)pPlayer->wY + dy);
}

/*
 *	Update all monster units. (Expanded in S10 for AI)
 */
void D2ServerGame::UpdateMonsters()
{
	// S10: iterate monsters and run AI behavior
}
