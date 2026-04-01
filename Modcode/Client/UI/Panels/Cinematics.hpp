#pragma once
#include "../D2Panel.hpp"
#include "../Widgets/Button.hpp"

namespace D2Panels
{
    class Cinematics : public D2Panel
    {
    private:
        D2Widgets::Button *m_actButtons[6]; // Acts 1-5 + End Game
        D2Widgets::Button *m_cancelButton;

    public:
        Cinematics();
        virtual ~Cinematics();
        virtual void Draw();
    };
}
