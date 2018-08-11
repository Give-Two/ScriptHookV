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

class ScriptEngine 
{
public:

	static bool Initialize();

	static PUINT64 getGlobal(int globalId);

	static eGameState GetGameState();

	static uint32_t RegisterFile(const char* fullpath, const char* filename);

	static void Notification(const std::string& str, bool gxt = false);
};

#endif // __SCRIPT_ENGINE_H__