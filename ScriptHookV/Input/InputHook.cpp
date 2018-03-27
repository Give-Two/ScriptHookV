#include "InputHook.h"
#include "..\Utility\Log.h"
#include "..\Scripting\ScriptManager.h"

WNDPROC	oWndProc;
HWND	hWindow;

LRESULT APIENTRY InputHook::WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {

	ScriptManager::WndProc( hwnd, uMsg, wParam, lParam );

	return CallWindowProc( oWndProc, hwnd, uMsg, wParam, lParam );
}

bool InputHook::Initialize() {

	hWindow = FindWindowA("grcWindow", NULL);

	oWndProc = (WNDPROC)SetWindowLongPtr(hWindow, GWLP_WNDPROC, (LONG_PTR)WndProc );
	if ( oWndProc == NULL ) {

		LOG_ERROR( "Failed to attach input hook" );
		return false;
	} else {

		LOG_PRINT( "Input hook attached: WndProc 0x%p", (DWORD_PTR)oWndProc );
		return true;
	}
}

void InputHook::Remove() {

	SetWindowLongPtr(hWindow, GWLP_WNDPROC, (LONG_PTR)oWndProc );
	LOG_DEBUG( "Removed input hook" );
}

bool isKeyPressedOnce(bool& bIsPressed, DWORD vk)
{
	return KeyStateDown(vk) ? bIsPressed ? bIsPressed = false : bIsPressed = true : false;
}

