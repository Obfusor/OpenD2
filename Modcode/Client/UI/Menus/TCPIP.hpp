#pragma once
#include "../D2Menu.hpp"
#include "../Panels/TCPIPJoin.hpp"
#include "../Panels/TCPIPMain.hpp"

namespace D2Menus
{
	class TCPIP : public D2Menu
	{
	private:
		IRenderObject *backgroundObject;
		IRenderObject *flameLeft;
		IRenderObject *flameRight;
		IRenderObject *blackLeft;
		IRenderObject *blackRight;
		IRenderObject *versionText;
		IRenderObject *yourIPLabel;
		IRenderObject *yourIPValue;
		IRenderObject *titleText;

		IGraphicsReference *background;

		D2Panels::TCPIPJoin *m_joinMenu;
		D2Panels::TCPIPMain *m_mainMenu;

	public:
		TCPIP();
		virtual ~TCPIP();
		virtual void Draw();

		void ShowJoinSubmenu(bool bShow);
	};
}