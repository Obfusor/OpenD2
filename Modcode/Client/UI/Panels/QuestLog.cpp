#include "QuestLog.hpp"

QuestLog::QuestLog()
{
    m_bVisible = false;
}

QuestLog::~QuestLog()
{
}

void QuestLog::Draw()
{
    if (!m_bVisible)
    {
        return;
    }

    // TODO: Draw quest list, act tabs, quest descriptions
    DrawWidgets();
}

void QuestLog::Tick(DWORD dwDeltaMs)
{
    if (!m_bVisible)
    {
        return;
    }

    D2Panel::Tick(dwDeltaMs);
}
