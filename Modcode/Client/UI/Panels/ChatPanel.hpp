#pragma once
#include "../D2Panel.hpp"

/*
 *	Chat panel - in-game chat input and message display.
 *	Based on Ghidra analysis: UI/chat.cpp in the original.
 */
class ChatPanel : public D2Panel
{
public:
	ChatPanel();
	~ChatPanel();

	void Draw() override;
	void Tick(DWORD dwDeltaMs) override;
};
