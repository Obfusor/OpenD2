#include "Inventory.hpp"

Inventory::Inventory()
{
	m_bVisible = false;
}

Inventory::~Inventory()
{
}

void Inventory::Draw()
{
	if (!m_bVisible)
	{
		return;
	}

	// TODO: Draw inventory background, equipment slots, inventory grid, gold display
	DrawWidgets();
}

void Inventory::Tick(DWORD dwDeltaMs)
{
	if (!m_bVisible)
	{
		return;
	}

	D2Panel::Tick(dwDeltaMs);
}
