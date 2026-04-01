#include "Credits.hpp"
#include "Main.hpp"
#include <cstring>

namespace D2Menus
{
	/*
	 *	Creates the credits screen.
	 *	Loads Credits.txt (or ExpansionCredits.txt) and scrolls the text.
	 */
	Credits::Credits() : D2Menu()
	{
		m_pRawCredits = nullptr;
		m_ppLines = nullptr;
		m_nTotalLines = 0;
		m_fScrollY = 600.0f; // start scrolling from bottom of screen

		engine->renderer->SetGlobalPalette(PAL_SKY);

		// Load background
		background = engine->graphics->CreateReference(
			"data\\global\\ui\\CharSelect\\creditsbckgexpand.dc6",
			UsagePolicy_SingleUse
		);

		backgroundObject = engine->renderer->AllocateObject(0);
		backgroundObject->AttachCompositeTextureResource(background, 0, -1);
		backgroundObject->SetDrawCoords(0, 0, 800, 600);
		backgroundObject->SetPalshift(0);

		// Allocate credit line render objects
		for (int i = 0; i < CREDITS_MAX_VISIBLE_LINES; i++)
		{
			creditLines[i] = engine->renderer->AllocateObject(1);
			creditLines[i]->AttachFontResource(cl.font16);
		}

		// Try to load ExpansionCredits.txt first, then Credits.txt
		fs_handle f = INVALID_HANDLE;
		m_nRawSize = engine->FS_Open("data\\local\\ui\\eng\\ExpansionCredits.txt", &f, FS_READ, true);
		if (f == INVALID_HANDLE)
		{
			m_nRawSize = engine->FS_Open("data\\local\\ui\\eng\\Credits.txt", &f, FS_READ, true);
		}

		if (f != INVALID_HANDLE && m_nRawSize > 0)
		{
			m_pRawCredits = new char[m_nRawSize + 1];
			engine->FS_Read(f, m_pRawCredits, m_nRawSize, 1);
			m_pRawCredits[m_nRawSize] = '\0';
			engine->FS_CloseFile(f);

			ParseCreditsText();
		}
	}

	/*
	 *	Parses the raw credits text buffer into lines.
	 *	Lines starting with '*' are headers (drawn in a different color).
	 */
	void Credits::ParseCreditsText()
	{
		if (!m_pRawCredits || m_nRawSize == 0)
			return;

		// Count lines
		m_nTotalLines = 1;
		for (size_t i = 0; i < m_nRawSize; i++)
		{
			if (m_pRawCredits[i] == '\n')
				m_nTotalLines++;
		}

		m_ppLines = new char*[m_nTotalLines];
		int lineIdx = 0;
		m_ppLines[0] = m_pRawCredits;

		for (size_t i = 0; i < m_nRawSize; i++)
		{
			if (m_pRawCredits[i] == '\r')
			{
				m_pRawCredits[i] = '\0';
			}
			else if (m_pRawCredits[i] == '\n')
			{
				m_pRawCredits[i] = '\0';
				lineIdx++;
				if (lineIdx < m_nTotalLines && i + 1 < m_nRawSize)
				{
					m_ppLines[lineIdx] = &m_pRawCredits[i + 1];
				}
			}
		}
		m_nTotalLines = lineIdx + 1;
	}

	Credits::~Credits()
	{
		engine->renderer->Remove(backgroundObject);
		for (int i = 0; i < CREDITS_MAX_VISIBLE_LINES; i++)
		{
			engine->renderer->Remove(creditLines[i]);
		}

		delete[] m_pRawCredits;
		delete[] m_ppLines;
	}

	void Credits::Draw()
	{
		backgroundObject->Draw();

		if (!m_ppLines)
			return;

		// Determine which lines are visible
		int firstLine = (int)(m_fScrollY / LINE_HEIGHT);
		if (firstLine < 0) firstLine = 0;

		float offsetInLine = m_fScrollY - (firstLine * LINE_HEIGHT);

		for (int i = 0; i < CREDITS_MAX_VISIBLE_LINES; i++)
		{
			int lineIdx = firstLine + i;
			if (lineIdx < 0 || lineIdx >= m_nTotalLines)
			{
				creditLines[i]->SetText(u"");
				continue;
			}

			char* line = m_ppLines[lineIdx];
			bool isHeader = (line[0] == '*');
			const char* displayText = isHeader ? (line + 1) : line;

			// Convert to UTF-16
			char16_t buf[256];
			int len = 0;
			while (displayText[len] && len < 255)
			{
				buf[len] = (char16_t)(unsigned char)displayText[len];
				len++;
			}
			buf[len] = u'\0';

			creditLines[i]->SetText(buf);

			// Headers use gold color, regular text uses white
			if (isHeader)
			{
				float r, g, b;
				engine->PAL_GetPL2ColorModulation(engine->renderer->GetGlobalPalette(), TextColor_Gold, r, g, b);
				creditLines[i]->SetColorModulate(r, g, b, 1.0f);
			}
			else
			{
				creditLines[i]->SetColorModulate(1.0f, 1.0f, 1.0f, 1.0f);
			}

			int yPos = (int)(i * LINE_HEIGHT - offsetInLine) + 80;
			creditLines[i]->SetDrawCoords(0, yPos, 800, (int)LINE_HEIGHT);
			creditLines[i]->Draw();
		}
	}

	void Credits::Tick(DWORD dwDeltaMs)
	{
		float dt = dwDeltaMs / 1000.0f;
		m_fScrollY -= SCROLL_SPEED * dt;

		// If we've scrolled past all lines, go back to main menu
		float totalHeight = m_nTotalLines * LINE_HEIGHT;
		if (m_fScrollY < -totalHeight)
		{
			delete cl.pActiveMenu;
			cl.pActiveMenu = new D2Menus::Main();
		}
	}

	bool Credits::HandleMouseClicked(DWORD dwX, DWORD dwY)
	{
		// Any click returns to main menu
		delete cl.pActiveMenu;
		cl.pActiveMenu = new D2Menus::Main();
		return true;
	}

	bool Credits::HandleKeyUp(DWORD keyButton)
	{
		// Any key returns to main menu
		delete cl.pActiveMenu;
		cl.pActiveMenu = new D2Menus::Main();
		return true;
	}
}
