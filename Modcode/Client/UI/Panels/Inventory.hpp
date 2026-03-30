#pragma once
#include "../D2Panel.hpp"

/*
 *	Inventory panel - displays the player's equipped items and inventory grid.
 *	Based on Ghidra analysis: UI/inv.cpp in the original D2Client.dll.
 */
class Inventory : public D2Panel
{
public:
	Inventory();
	~Inventory();

	void Draw() override;
	void Tick(DWORD dwDeltaMs) override;
};
