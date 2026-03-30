#include "D2Game.hpp"
#include <string.h>

D2ClientGame* gpGame = nullptr;

/////////////////////////////////////////////////
//
//	D2ClientGame implementation

D2ClientGame::D2ClientGame()
	: m_dwLocalPlayerId(0)
	, m_nCurrentAct(0)
	, m_wCurrentArea(0)
	, m_dwMapSeed(0)
	, m_bExpansion(false)
	, m_nDifficulty(0)
	, m_dwGameFlags(0)
	, m_pRoster(nullptr)
{
	memset(&m_view, 0, sizeof(m_view));
}

D2ClientGame::~D2ClientGame()
{
	Shutdown();
}

void D2ClientGame::Initialize(DWORD dwPlayerId, BYTE nAct, WORD wArea, DWORD dwMapSeed,
                               bool bExpansion, BYTE nDifficulty)
{
	m_dwLocalPlayerId = dwPlayerId;
	m_nCurrentAct = nAct;
	m_wCurrentArea = wArea;
	m_dwMapSeed = dwMapSeed;
	m_bExpansion = bExpansion;
	m_nDifficulty = nDifficulty;
}

void D2ClientGame::Shutdown()
{
	m_units.Clear();

	// Free roster
	D2RosterEntry* pEntry = m_pRoster;
	while (pEntry != nullptr)
	{
		D2RosterEntry* pNext = pEntry->pNext;
		delete pEntry;
		pEntry = pNext;
	}
	m_pRoster = nullptr;
}

D2UnitStrc* D2ClientGame::GetLocalPlayer()
{
	return m_units.FindUnit(UNIT_PLAYER, m_dwLocalPlayerId);
}

D2UnitStrc* D2ClientGame::AddPlayer(DWORD dwUnitId, BYTE nCharClass, const char* szName,
                                     WORD wX, WORD wY)
{
	D2UnitStrc* pUnit = new D2UnitStrc();
	memset(pUnit, 0, sizeof(D2UnitStrc));

	pUnit->nUnitType = UNIT_PLAYER;
	pUnit->dwUnitId = dwUnitId;
	pUnit->nCharClass = nCharClass;
	pUnit->wX = wX;
	pUnit->wY = wY;
	pUnit->dwMode = PLRMODE_TN;

	if (szName != nullptr)
	{
		strncpy(pUnit->szName, szName, sizeof(pUnit->szName) - 1);
		pUnit->szName[sizeof(pUnit->szName) - 1] = '\0';
	}

	m_units.AddUnit(pUnit);
	return pUnit;
}

D2UnitStrc* D2ClientGame::AddNPC(DWORD dwUnitId, WORD wClassId, WORD wX, WORD wY, BYTE nLife)
{
	D2UnitStrc* pUnit = new D2UnitStrc();
	memset(pUnit, 0, sizeof(D2UnitStrc));

	pUnit->nUnitType = UNIT_MONSTER;
	pUnit->dwClassId = wClassId;
	pUnit->dwUnitId = dwUnitId;
	pUnit->wX = wX;
	pUnit->wY = wY;
	pUnit->dwHP = nLife;
	pUnit->dwMode = MONMODE_NU;

	m_units.AddUnit(pUnit);
	return pUnit;
}

void D2ClientGame::RemoveUnit(D2UnitType nType, DWORD dwUnitId)
{
	m_units.RemoveUnit(nType, dwUnitId);
}

void D2ClientGame::UpdateView()
{
	D2UnitStrc* pPlayer = GetLocalPlayer();
	if (pPlayer == nullptr)
	{
		return;
	}

	m_view.nCenterX = pPlayer->wX;
	m_view.nCenterY = pPlayer->wY;
}

void D2ClientGame::AddRosterEntry(DWORD dwUnitId, BYTE nCharClass, const char* szName,
                                   WORD wLevel, WORD wPartyId)
{
	D2RosterEntry* pEntry = new D2RosterEntry();
	memset(pEntry, 0, sizeof(D2RosterEntry));

	pEntry->dwUnitId = dwUnitId;
	pEntry->nCharClass = nCharClass;
	pEntry->wLevel = wLevel;
	pEntry->wPartyId = wPartyId;

	if (szName != nullptr)
	{
		strncpy(pEntry->szName, szName, sizeof(pEntry->szName) - 1);
		pEntry->szName[sizeof(pEntry->szName) - 1] = '\0';
	}

	pEntry->pNext = m_pRoster;
	m_pRoster = pEntry;
}

void D2ClientGame::RemoveRosterEntry(DWORD dwUnitId)
{
	D2RosterEntry** ppEntry = &m_pRoster;
	while (*ppEntry != nullptr)
	{
		if ((*ppEntry)->dwUnitId == dwUnitId)
		{
			D2RosterEntry* pRemoved = *ppEntry;
			*ppEntry = pRemoved->pNext;
			delete pRemoved;
			return;
		}
		ppEntry = &(*ppEntry)->pNext;
	}
}

void D2ClientGame::UpdateRosterPosition(DWORD dwUnitId, WORD wX, WORD wY)
{
	D2RosterEntry* pEntry = m_pRoster;
	while (pEntry != nullptr)
	{
		if (pEntry->dwUnitId == dwUnitId)
		{
			pEntry->wX = wX;
			pEntry->wY = wY;
			return;
		}
		pEntry = pEntry->pNext;
	}
}

void D2ClientGame::Tick(DWORD dwDeltaMs)
{
	UpdateView();
}
