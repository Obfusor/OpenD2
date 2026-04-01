#pragma once
#include "../../../Shared/D2Common_Shared.hpp"

/*
 *	Unit types in Diablo II.
 *	From Ghidra analysis of D2Client.dll: GetUnitTypeWithSync, GetUnitOrdinalValue, etc.
 *	Units are the core entity type for all interactive objects in the game world.
 */

enum D2UnitType
{
    UNIT_PLAYER = 0,
    UNIT_MONSTER = 1,
    UNIT_OBJECT = 2,
    UNIT_MISSILE = 3,
    UNIT_ITEM = 4,
    UNIT_TILE = 5,
    UNIT_MAX
};

// Unit state flags (from Ghidra: SetUnitStateFlags, GetUnitLifeStateFlags)
enum D2UnitStateFlags : DWORD
{
    UNITSTATE_NONE = 0x00000000,
    UNITSTATE_DEAD = 0x00000001,
    UNITSTATE_INTRANSITION = 0x00000002,
    UNITSTATE_HASANIM = 0x00000004,
    UNITSTATE_INTOWNPORTAL = 0x00000008,
};

/*
 *	Core unit structure.
 *	This represents any entity in the game world: players, monsters, objects, missiles, items.
 *	Informed by Ghidra function signatures (GetUnitTypeWithSync, GetUnitOrdinalValue,
 *	GetUnitIdWithTransform, GetUnitLightRadius, GetUnitYCoord, etc.)
 */
struct D2UnitStrc
{
    D2UnitType nUnitType; // Unit type (player/monster/object/missile/item)
    DWORD dwClassId;      // Class ID within the unit type (e.g. monster type)
    DWORD dwUnitId;       // Unique unit ID assigned by the server
    DWORD dwMode;         // Current mode/animation state
    DWORD dwFlags;        // D2UnitStateFlags

    // Position (from Ghidra: GetUnitYCoord, FindRoomAtCoordinates)
    WORD wX;
    WORD wY;
    WORD wTargetX; // Movement target X
    WORD wTargetY; // Movement target Y

    // Stats
    DWORD dwHP;         // Current hit points (shifted by 8)
    DWORD dwHPMax;      // Maximum hit points
    DWORD dwMana;       // Current mana
    DWORD dwManaMax;    // Maximum mana
    DWORD dwStamina;    // Current stamina
    DWORD dwStaminaMax; // Maximum stamina
    DWORD dwLevel;      // Character/monster level
    DWORD dwExp;        // Experience points

    // From Ghidra: GetUnitLightRadius, GetMonsterLightRadius
    BYTE nLightRadius;

    // Display
    char szName[16]; // Unit name (for players)
    BYTE nCharClass; // Character class (for players, from GetPlayerClassIdFromUnit)

    // Linked list for unit hash table (from Ghidra: AllocateUnitUpdateNode, etc.)
    D2UnitStrc *pNext;

    bool IsDead() const { return (dwFlags & UNITSTATE_DEAD) != 0; }
    bool IsPlayer() const { return nUnitType == UNIT_PLAYER; }
    bool IsMonster() const { return nUnitType == UNIT_MONSTER; }
};

/*
 *	Unit list / hash table manager.
 *	From Ghidra: ResizeEntitySlotPool, ReallocatePoolSlotBuffer, etc.
 *	The original uses a hash table indexed by unit ID for fast lookup.
 */
#define UNIT_HASH_SIZE 128

class D2UnitList
{
private:
    D2UnitStrc *m_hashTable[UNIT_MAX][UNIT_HASH_SIZE];

    static int HashUnit(DWORD dwUnitId)
    {
        return dwUnitId & (UNIT_HASH_SIZE - 1);
    }

public:
    D2UnitList();
    ~D2UnitList();

    void AddUnit(D2UnitStrc *pUnit);
    void RemoveUnit(D2UnitType nType, DWORD dwUnitId);
    D2UnitStrc *FindUnit(D2UnitType nType, DWORD dwUnitId);
    D2UnitStrc *GetPlayerUnit();
    void Clear();
};
