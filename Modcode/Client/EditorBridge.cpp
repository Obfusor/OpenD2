#include "EditorBridge.hpp"
#include "D2Client.hpp"

static bool s_editorBridgeInited = false;

void EnsureEditorBridgeInit()
{
	if (!s_editorBridgeInited)
	{
		editor_bridge_init(openConfig->szBasePath);
		s_editorBridgeInited = true;
	}
}
