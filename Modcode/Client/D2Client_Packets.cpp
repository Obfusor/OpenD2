#include "D2Client.hpp"
#include "UI/Menus/LoadError.hpp"
#include "Game/D2Game.hpp"

/*
 *	This file is responsible for handling all of the behavior in response to a received packet.
 *	It's important to note that this is not responsible for actually parsing received data. Instead,
 *	that is handled by the engine. The engine intercepts specific packets (ie, D2SPACKET_COMPRESSIONINFO)
 *	and interacts on them before they arrive on the client.
 */

namespace ClientPacket
{
	// Error codes when connecting to a server
	WORD gwaTBLErrorEntries[] = {
		0x14F5, // Unable to enter game. Bad character version
		0x14F6, // Unable to enter game. Bad character quest data
		0x14F7, // Unable to enter game. Bad character waypoint data
		0x14F8, // Unable to enter game. Bad character stats data
		0x14F9, // Unable to enter game. Bad character skills data
		0x14FB, // Unable to enter game
		0x14FA, // failed to join game
		0x14ED, // Your connection has been interrupted
		0x14EE, // The Host of this game has left
		0x14FC, // unknown failure
		0x14FD, // Unable to enter game, Bad inventory data
		0x14FE, // Unable to enter game, bad dead bodies
		0x14FF, // Unable to enter game, bad header
		0x1500, // Unable to enter game, bad hireables
		0x1501, // Unable to enter game, bad intro data
		0x1502, // Unable to enter game, bad item
		0x1503, // Unable to enter game, bad dead body item
		0x1504, // Unable to enter game, generic bad file
		0x1505, // Game is full
		0x14F0, // Versions do not match. Please log onto battle.net or go to http://www.blizzard.com/support/Diablo2 to get a patch
		0x14F4, // Unable to enter game. Your character must kill Diablo to play in a Nightmare game.
		0x14F3, // Unable to enter game. Your character must kill Diablo in Nightmare difficulty to play in a Hell game.
		0x14F2, // Unable to enter game. A normal character cannot join a game created by a hardcore character.
		0x14F1, // Unable to enter game. A hardcore character cannot join a game created by a normal character.
		0x14EF, // A dead hardcore character cannot join or create any games.
		0x2775, // Unable to enter game. A Diablo II character cannot join a game created by a Diablo II Expansion character.
		0x2776, // Unable to enter game. A Diablo II Expansion character cannot join a game created by a Diablo II character.
		0x14FA, // failed to join game
		0x14FB, // Unable to enter game
		0x0000,
	};

	/*
	 *	First stage of handshake - Server tells client what compression type is being used.
	 *	In response, the client sends a game join request packet.
	 *	@author	eezstreet
	 */
	void ProcessCompressionPacket(D2Packet *pPacket)
	{
		D2Packet response;

#if GAME_MINOR_VERSION <= 10
		if (!cl.bLocalServer)
		{ // Send a D2CPACKET_JOINREMOTE
			response.nPacketType = D2CPACKET_JOINREMOTE;
			response.packetData.ClientRemoteJoinRequest.nLocale = 0; // FIXME
			response.packetData.ClientRemoteJoinRequest.unk1 = 0x00;
			response.packetData.ClientRemoteJoinRequest.unk2 = 0x01;
			response.packetData.ClientRemoteJoinRequest.nCharClass = cl.currentSave.header.nCharClass;
			response.packetData.ClientRemoteJoinRequest.dwVersion = GAME_MINOR_VERSION;
			D2Lib::strncpyz(response.packetData.ClientRemoteJoinRequest.szCharName,
							cl.currentSave.header.szCharacterName, 16);
		}
		else
		{ // Send a D2CPACKET_JOINLOCAL
			response.nPacketType = D2CPACKET_JOINLOCAL;
		}
#endif

		// Send the response packet
		engine->NET_SendClientPacket(&response);
	}

	/*
	 *	Error code in response to retrieving a packet.
	 *	@author	eezstreet
	 */
	void ProcessSavegameStatusPacket(D2Packet *pPacket)
	{
		WORD wResponse;

		// Retrieve data.
		switch (pPacket->packetData.ServerSaveStatus.nSaveStatus)
		{
		case SAVESTATUS_1:
			wResponse = 0;
			break;
		case SAVESTATUS_2:
			wResponse = 1;
			break;
		case SAVESTATUS_3:
			wResponse = 2;
			break;
		case SAVESTATUS_4:
			wResponse = 3;
			break;
		case SAVESTATUS_5:
			wResponse = 4;
			break;
		case SAVESTATUS_6:
			wResponse = 5;
			break;
		case SAVESTATUS_7:
			wResponse = 10;
			break;
		case SAVESTATUS_8:
			wResponse = 11;
			break;
		case SAVESTATUS_9:
			wResponse = 12;
			break;
		case SAVESTATUS_10:
			wResponse = 13;
			break;
		case SAVESTATUS_11:
			wResponse = 14;
			break;
		case SAVESTATUS_12:
			wResponse = 15;
			break;
		case SAVESTATUS_13:
			wResponse = 16;
			break;
		case SAVESTATUS_14:
			wResponse = 17;
			break;
		case SAVESTATUS_15:
			wResponse = 18;
			break;
		case SAVESTATUS_16:
			wResponse = 19;
			break;
		case SAVESTATUS_17:
			wResponse = 20;
			break;
		case SAVESTATUS_18:
			wResponse = 21;
			break;
		case SAVESTATUS_19:
			wResponse = 22;
			break;
		case SAVESTATUS_20:
			wResponse = 23;
			break;
		case SAVESTATUS_21:
			wResponse = 24;
			break;
		case SAVESTATUS_23:
			wResponse = 25;
			break;
		case SAVESTATUS_24:
			wResponse = 26;
			break;
		case SAVESTATUS_25:
			wResponse = 27;
			break;
		case SAVESTATUS_26:
			wResponse = 28;
			break;
		default:
			wResponse = 9;
			break;
		}

		// Set us back to the main menu status
		cl.bLocalServer = false;
		cl.gamestate = GS_MAINMENU;
		delete cl.pActiveMenu;
		cl.pActiveMenu = new D2Menus::LoadError(gwaTBLErrorEntries[wResponse]);
		engine->NET_Disconnect();
	}

	/*
	 *	Received metadata about the server.
	 *	This is the largest step of the handshake - once we have the metadata we need to send the *entire* savegame.
	 *	@author	eezstreet
	 */
	void ProcessServerMetaPacket(D2Packet *pPacket)
	{
		// We should verify that the metadata from the server is actually accurate and makes sense.
		// Even if we charge through this step, the server will slap us with an invalid savestate packet.
		// Still, we should do this to be nice and not overuse bandwidth...
		// FIXME: ?
		fs_handle f;
		size_t fileSize, remainder, chunks;
		D2Packet packet;

		cl.bValidatedSave = true;

		memset(&packet, 0, sizeof(packet));

		// Open savegame
		fileSize = engine->FS_Open(cl.szCurrentSave, &f, FS_READ, true);
		if (f == INVALID_HANDLE || fileSize == 0)
		{
			engine->Error(__FILE__, __LINE__, "Couldn't load savegame!");
			return;
		}

		chunks = fileSize / 255;
		remainder = fileSize - (chunks * 255);

		// Send whole chunks
		packet.nPacketType = D2CPACKET_SAVECHUNK;
		packet.packetData.ClientSendSaveChunk.dwSaveSize = fileSize;
		packet.packetData.ClientSendSaveChunk.nChunkSize = 255;
		for (size_t i = 0; i < chunks; i++)
		{
			engine->FS_Read(f, packet.packetData.ClientSendSaveChunk.nChunkBytes, 255, 1);
			engine->NET_SendClientPacket(&packet);
		}

		// Send remainder
		packet.packetData.ClientSendSaveChunk.nChunkSize = remainder;
		engine->FS_Read(f, packet.packetData.ClientSendSaveChunk.nChunkBytes, remainder, 1);
		engine->NET_SendClientPacket(&packet);

		// Close savegame, send completion packet
		engine->FS_CloseFile(f);
		packet.nPacketType = D2CPACKET_SAVEEND;
		engine->NET_SendClientPacket(&packet);

		// Send a ping packet
		packet.nPacketType = D2CPACKET_PING;
		packet.packetData.Ping.dwTickCount = engine->Milliseconds();
		packet.packetData.Ping.dwUnknown = 0;
		engine->NET_SendClientPacket(&packet);
	}

	/*
	 *	Received a PONG packet, we can calculate our ping (approx)
	 *	@author	eezstreet
	 */
	void ProcessPongPacket(D2Packet *pPacket)
	{
		cl.dwPing = engine->Milliseconds() - cl.dwLastPingPacket;
	}

	/////////////////////////////////////////////////
	//
	//	In-game packet handlers
	//	Informed by Ghidra analysis of D2Client.dll: GAME/SCmd.cpp
	//	These handle the server-to-client packets for game world state.

	/*
	 *	D2SPACKET_ASSIGNPLAYER (0x59)
	 *	Server assigns us a player unit in the world.
	 */
	void ProcessAssignPlayer(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		// Parse the raw packet bytes
		BYTE *data = (BYTE *)pPacket;
		DWORD dwUnitId = *(DWORD *)(data + 1);
		BYTE nCharClass = *(data + 5);
		char *szName = (char *)(data + 6);
		WORD wX = *(WORD *)(data + 22);
		WORD wY = *(WORD *)(data + 24);

		gpGame->AddPlayer(dwUnitId, nCharClass, szName, wX, wY);
		gpGame->Initialize(dwUnitId, gpGame->GetCurrentAct(), gpGame->GetCurrentArea(),
						   0, gpGame->IsExpansion(), gpGame->GetDifficulty());
	}

	/*
	 *	D2SPACKET_PLAYERJOINED (0x5B)
	 *	Another player has joined the game.
	 */
	void ProcessPlayerJoined(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		DWORD dwPlayerId = *(DWORD *)(data + 3);
		BYTE nCharClass = *(data + 7);
		char *szName = (char *)(data + 8);
		WORD wCharLevel = *(WORD *)(data + 24);
		WORD wPartyId = *(WORD *)(data + 26);

		gpGame->AddRosterEntry(dwPlayerId, nCharClass, szName, wCharLevel, wPartyId);
	}

	/*
	 *	D2SPACKET_PLAYERLEFT (0x5C)
	 *	A player has left the game.
	 */
	void ProcessPlayerLeft(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		DWORD dwPlayerId = *(DWORD *)(data + 1);

		gpGame->RemoveRosterEntry(dwPlayerId);
		gpGame->RemoveUnit(UNIT_PLAYER, dwPlayerId);
	}

	/*
	 *	D2SPACKET_ASSIGNNPC (0xAC)
	 *	Server assigns an NPC/monster in the world.
	 */
	void ProcessAssignNPC(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		DWORD dwUnitId = *(DWORD *)(data + 1);
		WORD wClassId = *(WORD *)(data + 5);
		WORD wX = *(WORD *)(data + 7);
		WORD wY = *(WORD *)(data + 9);
		BYTE nLife = *(data + 11);

		gpGame->AddNPC(dwUnitId, wClassId, wX, wY, nLife);
	}

	/*
	 *	D2SPACKET_REMOVEOBJECT (0x0A)
	 *	Server removes an entity from the world.
	 */
	void ProcessRemoveObject(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		BYTE nUnitType = *(data + 1);
		DWORD dwUnitId = *(DWORD *)(data + 2);

		if (nUnitType < UNIT_MAX)
		{
			gpGame->RemoveUnit((D2UnitType)nUnitType, dwUnitId);
		}
	}

	/*
	 *	D2SPACKET_PLAYERSTOP (0x0D)
	 *	A player has stopped moving.
	 */
	void ProcessPlayerStop(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		BYTE nUnitType = *(data + 1);
		DWORD dwUnitId = *(DWORD *)(data + 2);
		WORD wX = *(WORD *)(data + 7);
		WORD wY = *(WORD *)(data + 9);
		BYTE nLife = *(data + 12);

		D2UnitStrc *pUnit = gpGame->GetUnits().FindUnit((D2UnitType)nUnitType, dwUnitId);
		if (pUnit != nullptr)
		{
			pUnit->wX = wX;
			pUnit->wY = wY;
			pUnit->dwHP = nLife;
			pUnit->wTargetX = wX;
			pUnit->wTargetY = wY;
		}
	}

	/*
	 *	D2SPACKET_PLAYERMOVECOORD (0x0F)
	 *	A player is moving to coordinates.
	 */
	void ProcessPlayerMoveCoord(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		BYTE nUnitType = *(data + 1);
		DWORD dwUnitId = *(DWORD *)(data + 2);
		BYTE nMoveType = *(data + 6);
		WORD wTargetX = *(WORD *)(data + 7);
		WORD wTargetY = *(WORD *)(data + 9);
		WORD wCurrentX = *(WORD *)(data + 12);
		WORD wCurrentY = *(WORD *)(data + 14);

		D2UnitStrc *pUnit = gpGame->GetUnits().FindUnit((D2UnitType)nUnitType, dwUnitId);
		if (pUnit != nullptr)
		{
			pUnit->wX = wCurrentX;
			pUnit->wY = wCurrentY;
			pUnit->wTargetX = wTargetX;
			pUnit->wTargetY = wTargetY;
			pUnit->dwMode = (nMoveType == 0x17) ? PLRMODE_RN : PLRMODE_WL;
		}
	}

	/*
	 *	D2SPACKET_NPCMOVECOORD (0x67)
	 *	An NPC/monster is moving.
	 */
	void ProcessNPCMoveCoord(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		DWORD dwUnitId = *(DWORD *)(data + 1);
		BYTE nMoveType = *(data + 5);
		WORD wX = *(WORD *)(data + 6);
		WORD wY = *(WORD *)(data + 8);

		D2UnitStrc *pUnit = gpGame->GetUnits().FindUnit(UNIT_MONSTER, dwUnitId);
		if (pUnit != nullptr)
		{
			pUnit->wTargetX = wX;
			pUnit->wTargetY = wY;
			pUnit->dwMode = (nMoveType == 0x17) ? MONMODE_RN : MONMODE_WL;
		}
	}

	/*
	 *	D2SPACKET_NPCSTOP (0x6D)
	 *	An NPC/monster has stopped moving.
	 */
	void ProcessNPCStop(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		DWORD dwUnitId = *(DWORD *)(data + 1);
		WORD wX = *(WORD *)(data + 5);
		WORD wY = *(WORD *)(data + 7);
		BYTE nLife = *(data + 9);

		D2UnitStrc *pUnit = gpGame->GetUnits().FindUnit(UNIT_MONSTER, dwUnitId);
		if (pUnit != nullptr)
		{
			pUnit->wX = wX;
			pUnit->wY = wY;
			pUnit->wTargetX = wX;
			pUnit->wTargetY = wY;
			pUnit->dwHP = nLife;
			pUnit->dwMode = MONMODE_NU;
		}
	}

	/*
	 *	D2SPACKET_NPCSTATE (0x69)
	 *	An NPC/monster state update.
	 */
	void ProcessNPCState(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		DWORD dwUnitId = *(DWORD *)(data + 1);
		BYTE nState = *(data + 5);
		WORD wX = *(WORD *)(data + 6);
		WORD wY = *(WORD *)(data + 8);
		BYTE nLife = *(data + 10);

		D2UnitStrc *pUnit = gpGame->GetUnits().FindUnit(UNIT_MONSTER, dwUnitId);
		if (pUnit != nullptr)
		{
			pUnit->wX = wX;
			pUnit->wY = wY;
			pUnit->dwHP = nLife;
			pUnit->dwMode = nState;
		}
	}

	/*
	 *	D2SPACKET_CHAT (0x26)
	 *	Chat message received. Currently just logged.
	 */
	void ProcessChat(D2Packet *pPacket)
	{
		// TODO: route to chat UI when implemented
		// From Ghidra: UI/chat.cpp handles display
	}

	/*
	 *	D2SPACKET_LIFEMANA (0x95)
	 *	Update local player's life/mana/stamina.
	 */
	void ProcessLifeMana(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		WORD wLife = *(WORD *)(data + 1);
		WORD wMana = *(WORD *)(data + 3);
		WORD wStamina = *(WORD *)(data + 5);
		WORD wX = *(WORD *)(data + 7);
		WORD wY = *(WORD *)(data + 9);

		D2UnitStrc *pPlayer = gpGame->GetLocalPlayer();
		if (pPlayer != nullptr)
		{
			pPlayer->dwHP = wLife;
			pPlayer->dwMana = wMana;
			pPlayer->dwStamina = wStamina;
			pPlayer->wX = wX;
			pPlayer->wY = wY;
		}
	}

	/*
	 *	D2SPACKET_LOADACT (0x03)
	 *	Begin loading an act.
	 */
	void ProcessLoadAct(D2Packet *pPacket)
	{
		if (gpGame == nullptr)
		{
			return;
		}

		BYTE *data = (BYTE *)pPacket;
		BYTE nAct = *(data + 1);
		WORD wAreaId = *(WORD *)(data + 6);

		gpGame->Initialize(0, nAct, wAreaId, 0,
						   gpGame->IsExpansion(), gpGame->GetDifficulty());
	}
}