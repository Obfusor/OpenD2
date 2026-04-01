#pragma once
#include "../D2Menu.hpp"
#include "../Panels/OtherMultiplayer.hpp"

namespace D2Menus
{
	class OtherMultiplayer : public D2Menu
	{
	private:
		IRenderObject *backgroundObject;
		IRenderObject *flameLeft;
		IRenderObject *flameRight;
		IRenderObject *blackLeft;
		IRenderObject *blackRight;
		IRenderObject *versionText;

		IGraphicsReference *background;

		D2Panels::OtherMultiplayer *m_panel;

	public:
		OtherMultiplayer();
		virtual ~OtherMultiplayer();

		virtual void Draw();
	};
}