#include "ChatPanel.hpp"

ChatPanel::ChatPanel()
{
	m_bVisible = false;
}

ChatPanel::~ChatPanel()
{
}

void ChatPanel::Draw()
{
	if (!m_bVisible)
	{
		return;
	}

	// TODO: Draw chat message history, input field
	DrawWidgets();
}

void ChatPanel::Tick(DWORD dwDeltaMs)
{
	if (!m_bVisible)
	{
		return;
	}

	D2Panel::Tick(dwDeltaMs);
}
