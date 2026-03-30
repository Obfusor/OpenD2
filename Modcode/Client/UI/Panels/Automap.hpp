#pragma once
#include "../D2Panel.hpp"

/*
 *	Automap overlay panel - draws the map overlay on top of the game world.
 *	Based on Ghidra analysis: UI/automap.cpp in the original.
 */
class Automap : public D2Panel
{
public:
	Automap();
	~Automap();

	void Draw() override;
	void Tick(DWORD dwDeltaMs) override;
};
