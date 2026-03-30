#include "SkillTree.hpp"

SkillTree::SkillTree()
{
	m_bVisible = false;
}

SkillTree::~SkillTree()
{
}

void SkillTree::Draw()
{
	if (!m_bVisible)
	{
		return;
	}

	// TODO: Draw skill tree tabs, skill icons, skill point allocation UI
	DrawWidgets();
}

void SkillTree::Tick(DWORD dwDeltaMs)
{
	if (!m_bVisible)
	{
		return;
	}

	D2Panel::Tick(dwDeltaMs);
}
