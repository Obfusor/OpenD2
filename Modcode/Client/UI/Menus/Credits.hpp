#pragma once
#include "../D2Menu.hpp"

// Maximum number of visible credit lines on screen at once
#define CREDITS_MAX_VISIBLE_LINES 20

namespace D2Menus
{
    class Credits : public D2Menu
    {
    private:
        IRenderObject *backgroundObject;
        IGraphicsReference *background;

        // Scrolling credit text lines
        IRenderObject *creditLines[CREDITS_MAX_VISIBLE_LINES];

        // The full loaded credits text (one big buffer)
        char *m_pRawCredits;
        size_t m_nRawSize;

        // Parsed line pointers (into m_pRawCredits)
        char **m_ppLines;
        int m_nTotalLines;

        // Scroll position (in pixels)
        float m_fScrollY;
        static constexpr float SCROLL_SPEED = 30.0f; // pixels per second
        static constexpr float LINE_HEIGHT = 20.0f;

        void ParseCreditsText();

    public:
        Credits();
        virtual ~Credits();

        virtual void Draw();
        virtual void Tick(DWORD dwDeltaMs) override;
        virtual bool HandleMouseClicked(DWORD dwX, DWORD dwY);
        virtual bool HandleKeyUp(DWORD keyButton);
    };
}
