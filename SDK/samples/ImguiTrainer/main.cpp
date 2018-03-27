/*
	THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
				http://dev-c.com			
			(C) Alexander Blade 2015
*/

#include "..\..\inc\main.h"
#include "script.h"
#include "keyboard.h"

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		scriptRegister(hInstance, ScriptMain);
		WndProcHandlerRegister(OnWndProcMessage);
		keyboardHandlerRegister(OnKeyboardMessage);
		presentCallbackRegister(PresentHook);
		break;
	case DLL_PROCESS_DETACH:
		scriptUnregister(hInstance);
		WndProcHandlerUnregister(OnWndProcMessage);
		keyboardHandlerUnregister(OnKeyboardMessage);
		presentCallbackUnregister(PresentHook);
		break;
	}		
	return TRUE;
}