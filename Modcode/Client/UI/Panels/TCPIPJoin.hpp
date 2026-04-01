#pragma once
#include "../D2Panel.hpp"
#include "../Widgets/Button.hpp"
#include "../Widgets/TextEntry.hpp"

namespace D2Panels
{
	class TCPIPJoin : public D2Panel
	{
	private:
		IRenderObject *panelBackground;
		IRenderObject *ipTextLabel;

		D2Widgets::Button *m_okButton;
		D2Widgets::Button *m_cancelButton;
		D2Widgets::TextEntry *m_ipEntry;

	public:
		TCPIPJoin();
		virtual ~TCPIPJoin();
		virtual void Draw();
		char16_t *GetEnteredIP();
	};
}