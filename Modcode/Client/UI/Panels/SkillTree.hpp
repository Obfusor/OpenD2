#pragma once
#include "../D2Panel.hpp"

/*
 *	Skill tree panel - displays skill trees and allows skill point allocation.
 *	Based on Ghidra analysis: UI/spellsel.cpp / UI/SkillDesc.cpp in the original.
 */
class SkillTree : public D2Panel
{
public:
	SkillTree();
	~SkillTree();

	void Draw() override;
	void Tick(DWORD dwDeltaMs) override;
};
