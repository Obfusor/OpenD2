#include "D2Unit.hpp"
#include <string.h>

/////////////////////////////////////////////////
//
//	D2UnitList implementation

D2UnitList::D2UnitList()
{
	memset(m_hashTable, 0, sizeof(m_hashTable));
}

D2UnitList::~D2UnitList()
{
	Clear();
}

void D2UnitList::AddUnit(D2UnitStrc* pUnit)
{
	if (pUnit == nullptr || pUnit->nUnitType >= UNIT_MAX)
	{
		return;
	}

	int hash = HashUnit(pUnit->dwUnitId);
	pUnit->pNext = m_hashTable[pUnit->nUnitType][hash];
	m_hashTable[pUnit->nUnitType][hash] = pUnit;
}

void D2UnitList::RemoveUnit(D2UnitType nType, DWORD dwUnitId)
{
	if (nType >= UNIT_MAX)
	{
		return;
	}

	int hash = HashUnit(dwUnitId);
	D2UnitStrc** ppCurrent = &m_hashTable[nType][hash];

	while (*ppCurrent != nullptr)
	{
		if ((*ppCurrent)->dwUnitId == dwUnitId)
		{
			D2UnitStrc* pRemoved = *ppCurrent;
			*ppCurrent = pRemoved->pNext;
			delete pRemoved;
			return;
		}
		ppCurrent = &(*ppCurrent)->pNext;
	}
}

D2UnitStrc* D2UnitList::FindUnit(D2UnitType nType, DWORD dwUnitId)
{
	if (nType >= UNIT_MAX)
	{
		return nullptr;
	}

	int hash = HashUnit(dwUnitId);
	D2UnitStrc* pCurrent = m_hashTable[nType][hash];

	while (pCurrent != nullptr)
	{
		if (pCurrent->dwUnitId == dwUnitId)
		{
			return pCurrent;
		}
		pCurrent = pCurrent->pNext;
	}
	return nullptr;
}

D2UnitStrc* D2UnitList::GetPlayerUnit()
{
	// Scan all player hash buckets for the local player
	for (int i = 0; i < UNIT_HASH_SIZE; i++)
	{
		D2UnitStrc* pCurrent = m_hashTable[UNIT_PLAYER][i];
		while (pCurrent != nullptr)
		{
			return pCurrent; // Return first player found (local player)
			pCurrent = pCurrent->pNext;
		}
	}
	return nullptr;
}

void D2UnitList::Clear()
{
	for (int type = 0; type < UNIT_MAX; type++)
	{
		for (int i = 0; i < UNIT_HASH_SIZE; i++)
		{
			D2UnitStrc* pCurrent = m_hashTable[type][i];
			while (pCurrent != nullptr)
			{
				D2UnitStrc* pNext = pCurrent->pNext;
				delete pCurrent;
				pCurrent = pNext;
			}
			m_hashTable[type][i] = nullptr;
		}
	}
}
