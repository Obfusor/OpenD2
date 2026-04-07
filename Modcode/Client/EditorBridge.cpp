#include "EditorBridge.hpp"
#include "D2Client.hpp"

static bool s_editorBridgeInited = false;
static RGBA_PALETTE s_editorPalette;

void EnsureEditorBridgeInit()
{
	if (!s_editorBridgeInited)
	{
		editor_bridge_init(openConfig->szBasePath);
		s_editorBridgeInited = true;
	}
}

/*
 *	Update the editor's a5_current_palette from OpenD2's engine palette.
 *	OpenD2's D2Palette is BYTE[256][3] in BGR order.
 *	The editor expects RGBA_PALETTE with RGBA_COLOR entries.
 *	Call this when the act/palette changes.
 */
void EditorBridge_SetPalette(int act)
{
	D2Palette *pal = engine->PAL_GetPalette(act < 5 ? act : 0);
	if (pal == nullptr)
	{
		// Grayscale fallback
		for (int i = 0; i < 256; i++)
		{
			s_editorPalette.colors[i].r = (uint8_t)i;
			s_editorPalette.colors[i].g = (uint8_t)i;
			s_editorPalette.colors[i].b = (uint8_t)i;
			s_editorPalette.colors[i].a = (i == 0) ? 0 : 255;
		}
	}
	else
	{
		for (int i = 0; i < 256; i++)
		{
			// D2Palette stores BGR
			s_editorPalette.colors[i].b = (*pal)[i][0];
			s_editorPalette.colors[i].g = (*pal)[i][1];
			s_editorPalette.colors[i].r = (*pal)[i][2];
			s_editorPalette.colors[i].a = (i == 0) ? 0 : 255;
		}
	}
	a5_current_palette = &s_editorPalette;
}
