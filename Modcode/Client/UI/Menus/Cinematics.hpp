#pragma once
#include "../D2Menu.hpp"
#include "../Widgets/Button.hpp"

namespace D2Panels
{
    class Cinematics;
}

namespace D2Menus
{
    class Cinematics : public D2Menu
    {
    private:
        IRenderObject *backgroundObject;
        IGraphicsReference *background;

        IRenderObject *titleText;

        D2Panels::Cinematics *m_panel;

    public:
        Cinematics();
        virtual ~Cinematics();

        virtual void Draw();
    };
}
