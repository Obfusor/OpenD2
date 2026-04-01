#pragma once
#include "../D2Panel.hpp"

/*
 *	Quest log panel - displays quest progress and objectives.
 *	Based on Ghidra analysis: UI/QuestLog.cpp in the original.
 */
class QuestLog : public D2Panel
{
public:
    QuestLog();
    ~QuestLog();

    void Draw() override;
    void Tick(DWORD dwDeltaMs) override;
};
