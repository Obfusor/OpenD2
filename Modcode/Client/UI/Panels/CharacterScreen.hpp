#pragma once
#include "../D2Panel.hpp"

/*
 *	Character screen panel - displays player stats and attributes.
 *	Based on Ghidra analysis: stat display and attribute allocation in the original.
 */
class CharacterScreen : public D2Panel
{
public:
	CharacterScreen();
	~CharacterScreen();

	void Draw() override;
	void Tick(DWORD dwDeltaMs) override;
};
