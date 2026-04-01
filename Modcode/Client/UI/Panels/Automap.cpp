#include "Automap.hpp"

Automap::Automap()
{
    m_bVisible = false;
}

Automap::~Automap()
{
}

void Automap::Draw()
{
    if (!m_bVisible)
    {
        return;
    }

    // TODO: Draw automap tiles, player/NPC markers, waypoint icons
    DrawWidgets();
}

void Automap::Tick(DWORD dwDeltaMs)
{
    if (!m_bVisible)
    {
        return;
    }

    D2Panel::Tick(dwDeltaMs);
}
