#include "CharacterScreen.hpp"

CharacterScreen::CharacterScreen()
{
    m_bVisible = false;
}

CharacterScreen::~CharacterScreen()
{
}

void CharacterScreen::Draw()
{
    if (!m_bVisible)
    {
        return;
    }

    // TODO: Draw character stats, attribute points, resistances
    DrawWidgets();
}

void CharacterScreen::Tick(DWORD dwDeltaMs)
{
    if (!m_bVisible)
    {
        return;
    }

    D2Panel::Tick(dwDeltaMs);
}
