#ifndef __INPUT_HOOK_H__
#define __INPUT_HOOK_H__

#define KeyStateDown(key)(GetAsyncKeyState(key) & 0x8000) != 0
#define KeyStateToggled(key)GetKeyState(key) & 0x0001) != 0

#include "..\ScriptHookV.h"

namespace InputHook {

	bool Initialize();

	void Remove();

	static LRESULT APIENTRY WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
}

bool isKeyPressedOnce(bool& bIsPressed, DWORD vk);

#endif // __INPUT_HOOK_H__