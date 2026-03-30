#include "IngameMain.hpp"

IngameMain::IngameMain()
	: m_newStatButton(nullptr)
	, m_newSkillButton(nullptr)
	, m_questLogButton(nullptr)
	, m_miniPanelButton(nullptr)
	, m_leftAttackButton(nullptr)
	, m_rightAttackButton(nullptr)
{
	x = 0;
	y = 0;
}

IngameMain::~IngameMain()
{
}

void IngameMain::Draw()
{
	// Draw the bottom HUD bar
	// TODO: load and render HUD art assets
	DrawWidgets();
}

void IngameMain::Tick(DWORD dwDeltaMs)
{
	D2Panel::Tick(dwDeltaMs);
}

