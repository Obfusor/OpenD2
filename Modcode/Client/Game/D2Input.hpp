#pragma once
#include "../D2Client.hpp"

/*
 *	Input binding / keybinding system.
 *	Informed by Ghidra analysis: InitializeInputBindings, RegisterKeyboardShortcut,
 *	LoadAndValidateKeyBindings, HandleSkillHotkeySlot0-F, CycleActiveSkillForward,
 *	PlayWeaponSwapSoundIfNoPanel, EnableMinimapWithKeyBindings.
 *
 *	The original D2Client has 16 skill hotkey slots (F1-F8 + extended),
 *	plus keybinds for automap toggle, inventory, character screen, etc.
 */

#define MAX_SKILL_HOTKEYS 16

// Types of actions that can be bound to keys
enum D2BindAction
{
    BIND_NONE = 0,

    // Skill hotkeys (from Ghidra: HandleSkillHotkeySlot0 through HandleSkillHotkeySlotF)
    BIND_SKILL_HOTKEY_0,
    BIND_SKILL_HOTKEY_1,
    BIND_SKILL_HOTKEY_2,
    BIND_SKILL_HOTKEY_3,
    BIND_SKILL_HOTKEY_4,
    BIND_SKILL_HOTKEY_5,
    BIND_SKILL_HOTKEY_6,
    BIND_SKILL_HOTKEY_7,
    BIND_SKILL_HOTKEY_8,
    BIND_SKILL_HOTKEY_9,
    BIND_SKILL_HOTKEY_A,
    BIND_SKILL_HOTKEY_B,
    BIND_SKILL_HOTKEY_C,
    BIND_SKILL_HOTKEY_D,
    BIND_SKILL_HOTKEY_E,
    BIND_SKILL_HOTKEY_F,

    // UI toggles (from Ghidra: various toggle functions)
    BIND_TOGGLE_AUTOMAP, // EnableMinimapWithKeyBindings / DisableMinimapMode
    BIND_TOGGLE_INVENTORY,
    BIND_TOGGLE_CHARACTER,
    BIND_TOGGLE_SKILLTREE,
    BIND_TOGGLE_QUESTLOG,
    BIND_TOGGLE_CHAT,
    BIND_TOGGLE_PARTY,
    BIND_TOGGLE_BELT,

    // Actions
    BIND_SWAP_WEAPONS,        // PlayWeaponSwapSoundIfNoPanel
    BIND_TOWN_PORTAL,         // SendTownPortalCommand
    BIND_CYCLE_SKILL_FORWARD, // CycleActiveSkillForward
    BIND_RUN_TOGGLE,
    BIND_SCREENSHOT,
    BIND_CLEAR_SCREEN,

    BIND_MAX
};

// A single key binding entry
struct D2KeyBind
{
    D2BindAction action;
    DWORD dwKey;       // D2InputButton value
    DWORD dwModifiers; // D2InputModifiers mask
};

// Default key bindings (matching original Diablo II defaults)
#define MAX_KEYBINDS 64

class D2InputBindings
{
private:
    D2KeyBind m_binds[MAX_KEYBINDS];
    int m_nBindCount;

    // Skill hotkey assignments (from Ghidra: SetSkillHotkeySlotData, AssignSkillHotkeySlot)
    WORD m_skillSlots[MAX_SKILL_HOTKEYS];    // Skill ID per slot (0 = unassigned)
    bool m_skillSlotLeft[MAX_SKILL_HOTKEYS]; // true = left skill, false = right skill

    bool m_bRunToggle;

public:
    D2InputBindings();

    // Initialization
    void LoadDefaults();
    void LoadFromConfig();
    void SaveToConfig();

    // Binding management
    void SetBind(D2BindAction action, DWORD dwKey, DWORD dwModifiers = 0);
    D2BindAction GetActionForKey(DWORD dwKey, DWORD dwModifiers) const;

    // Skill hotkeys (from Ghidra: AssignSkillHotkeySlot, ClearSkillFromHotkeySlots)
    void AssignSkillHotkey(int nSlot, WORD wSkillId, bool bLeftSkill);
    WORD GetSkillHotkey(int nSlot) const;
    bool IsSkillHotkeyLeft(int nSlot) const;
    void ClearSkillHotkey(int nSlot);
    void ClearAllSkillHotkeys();

    // Run toggle
    bool IsRunToggled() const { return m_bRunToggle; }
    void ToggleRun() { m_bRunToggle = !m_bRunToggle; }

    // Process a key event and return the action triggered (or BIND_NONE)
    D2BindAction ProcessKeyUp(DWORD dwKey, DWORD dwModifiers);
};

// Global input bindings instance
extern D2InputBindings *gpInputBindings;

// Process an in-game key action
void D2Input_HandleBindAction(D2BindAction action);
