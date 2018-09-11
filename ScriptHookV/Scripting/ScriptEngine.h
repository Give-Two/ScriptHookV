#ifndef __SCRIPT_ENGINE_H__
#define __SCRIPT_ENGINE_H__

#include "..\ScriptHookV.h"

#include "NativeInvoker.h"

enum eGameState 
{
	GameStatePlaying,
	GameStateIntro,
	GameStateLicenseShit = 3,
	GameStateMainMenu = 5,
	GameStateLoadingSP_MP = 6
};

enum eHookState
{
	HookStateRunning,
	HookStateExiting,
	HookStateUnknown = -1
};	extern eHookState  g_HookState;

class ScriptEngine 
{
public:

	static bool Initialize();

	static PUINT64 getGlobal(int globalId);

	static eGameState GetGameState();
	
	static int RegisterFile(const std::string& fullPath, const std::string& fileName);
};

#endif // __SCRIPT_ENGINE_H__