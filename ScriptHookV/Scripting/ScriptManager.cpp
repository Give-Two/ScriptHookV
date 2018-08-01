#include "ScriptManager.h"
#include "NativeInvoker.h"
#include "..\Utility\Log.h"
#include "..\Utility\General.h"
#include "..\ASI Loader\ASILoader.h"
#include "..\Input\InputHook.h"
#include "..\DirectX\D3d11Hook.h"
#include "..\Hooking\Hooking.h"
#include "Pools.h"

#include <StackWalker.h>
#pragma comment(lib, "StackWalker.lib")
// Specialized stackwalker-output classes
// Console (printf):
class StackWalkerToConsole : public StackWalker
{
protected:
	virtual void OnOutput(LPCSTR szText)
	{
		std::ofstream LOG;

		std::string fileName = Utility::GetOurModuleFolder() + "\\" + "StackTrace" + ".txt";

		LOG.open(fileName, std::ofstream::out | std::ofstream::app);

		LOG << szText << std::endl;

		LOG.close();
	}
};

LONG WINAPI ExpFilter(EXCEPTION_POINTERS* pExp, DWORD /*dwExpCode*/)
{
	//StackWalker sw;  // output to default (Debug-Window)
	StackWalkerToConsole sw; // output to the console
	sw.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
	return EXCEPTION_EXECUTE_HANDLER;
}

#define DLL_EXPORT __declspec( dllexport )

using namespace NativeInvoker::Helper;

ScriptManagerThread g_ScriptManagerThread;
HANDLE                      g_MainFiber;
static Script*				currentScript;

/* ####################### SCRIPT #######################*/

void Script::Tick() 
{
	if (!g_MainFiber)
	{
		g_MainFiber = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiber(nullptr);
	}
	else
	{
		if (timeGetTime() < wakeAt)
		{
			if (GetCurrentFiber() != g_MainFiber) SwitchToFiber(g_MainFiber); return;
		}

		if (scriptFiber)
		{
			currentScript = this;
			SwitchToFiber(scriptFiber);
			currentScript = nullptr;
		}

		else
		{
			scriptFiber = CreateFiber(NULL, [](LPVOID handler)
			{
				reinterpret_cast<Script*>(handler)->Run();
				SwitchToFiber(g_MainFiber);	
			}, this);
		}
	}
}

void Script::Run() 
{
	__try
	{
		callbackFunction();
	}
	__except (ExpFilter(GetExceptionInformation(), GetExceptionCode()))
	{
		g_ScriptManagerThread.RemoveScript(this->GetCallbackFunction());
	}
}

void Script::Wait( uint32_t time ) 
{
    wakeAt = timeGetTime() + time;
	if (g_MainFiber) SwitchToFiber(g_MainFiber);
}

void ScriptManagerThread::DoRun() 
{
	static bool RemoveAllScriptsKey = false;
    if (isKeyPressedOnce(RemoveAllScriptsKey, VK_END))
    {
      	g_ScriptManagerThread.RemoveAllScripts();
    }

	static bool ReloadModsKey = false;
	if (g_MainFiber && KeyStateDown(VK_CONTROL))
	{
		if (isKeyPressedOnce(ReloadModsKey, 0x52)) g_ScriptManagerThread.Reset();
	}

	while (!g_Stack.empty())
	{
		g_Stack.front();
		g_Stack.pop_front();
	}

    for (auto & pair : m_scripts) 
	{ 
		pair.second->Tick(); 
	}
	
	for (auto & pair : m_additional) 
	{
		pair.second->Tick(); 
	}
}

void ScriptManagerThread::Reset() 
{
    g_ScriptManagerThread.RemoveAllScripts();
    ASILoader::Initialize();
}

void ScriptManagerThread::AddScript( HMODULE module, void( *fn )( ) ) 
{
    const std::string moduleName = Utility::GetModuleNameWithoutExtension( module);

	if (m_scripts.find(module) != m_scripts.end()) 
	{
		LOG_ERROR("Script '%s' is already registered", moduleName.c_str()); return;
	}	
	else
	{
		ScriptEngine::Notification(FMT("Loaded '%s'", moduleName.c_str()));
		LOG_PRINT("Registering script '%s' (0x%p)", moduleName.c_str(), fn);
		m_scripts[module] = std::make_shared<Script>(fn);
	}
}

void ScriptManagerThread::AddAdditional(HMODULE module, void(*fn)()) 
{
	const std::string moduleName = Utility::GetModuleNameWithoutExtension(module);

	if (m_additional.find(module) != m_additional.end())
	{
		LOG_ERROR("Additional Thread for '%s' is already registered", moduleName.c_str()); return;
	}
	else
	{
		LOG_PRINT("Registering Additional Thread for '%s' (0x%p)", moduleName.c_str(), fn);
		m_additional[module] = std::make_shared<Script>(fn);
	}
}

void ScriptManagerThread::RemoveScript(HMODULE module)
{
	const std::string name = Utility::GetModuleNameWithoutExtension(module);
	
	if (m_additional.size())
	{
		auto foundIter = m_additional.find(module);
		if (foundIter != m_additional.end())
		{
			m_additional.erase(foundIter);
			LOG_PRINT("Removed '%s' additional thread module 0x%p", name.c_str(), module);
		}
	}
	
	if (m_scripts.size())
	{
		auto foundIter = m_scripts.find(module);
		if (foundIter != m_scripts.end())
		{
			m_scripts.erase(foundIter);
			FreeLibrary(module);
			ScriptEngine::Notification(FMT("Removed '%s'", name.c_str()));
			LOG_PRINT("Unregistered script '%s'", name.c_str());
		}
	}
}

void ScriptManagerThread::RemoveScript(void(*fn)()) 
{
    for ( auto it = m_scripts.begin(); it != m_scripts.end(); it++ ) 
	{
        auto pair = *it;

		if ( pair.second->GetCallbackFunction() == fn ) 
		{
            RemoveScript(pair.first);
        }
    }
}

void ScriptManagerThread::RemoveAllScripts()
{
	if (g_MainFiber) SwitchToFiber(g_MainFiber);
	
	if (std::size(m_additional) > 0)
	{
		for (auto & pair : m_additional)
		{
			RemoveScript(pair.first);
		}	m_additional.clear();
	}
	
	if (std::size(m_scripts) > 0)
	{
		for (auto & pair : m_scripts)
		{
			RemoveScript(pair.first);
		}	m_scripts.clear();
		Utility::playwindowsSound("Windows Default.wav");
	}
}

/* ####################### EXPORTS #######################*/

/*Input*/
typedef void(*KeyboardHandler)(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);

typedef void(*TWndProcFn)(UINT uMsg, WPARAM wParam, LPARAM lParam);

static std::set<KeyboardHandler> g_keyboardFunctions;

static std::set<TWndProcFn> g_WndProcCb;

DLL_EXPORT void WndProcHandlerRegister(TWndProcFn function) 
{
    g_WndProcCb.insert(function);
}

DLL_EXPORT void WndProcHandlerUnregister(TWndProcFn function) 
{
    g_WndProcCb.erase(function);
}

void ScriptManager::WndProc(HWND /*hwnd*/, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    for (auto & function : g_WndProcCb) function(uMsg, wParam, lParam);
    
    if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP) 
	{
        for (auto & function : g_keyboardFunctions) function((DWORD)wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFF, (lParam >> 24) & 1, (uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP), (lParam >> 30) & 1, (uMsg == WM_SYSKEYUP || uMsg == WM_KEYUP));
    }
}

/* keyboard */
DLL_EXPORT void keyboardHandlerRegister(KeyboardHandler function) 
{
    g_keyboardFunctions.insert(function);
}

DLL_EXPORT void keyboardHandlerUnregister(KeyboardHandler function) 
{
    g_keyboardFunctions.erase(function);
}

/* D3d SwapChain */
DLL_EXPORT void presentCallbackRegister(PresentCallback cb) 
{
    g_D3DHook.AddCallback(cb);
}

DLL_EXPORT void presentCallbackUnregister(PresentCallback cb) 
{
    g_D3DHook.RemoveCallback(cb);
}

/* textures */
DLL_EXPORT int createTexture(const char *texFileName) 
{
	return g_D3DHook.CreateTexture(texFileName);
}

DLL_EXPORT void drawTexture(int id, int index, int level, int time, float sizeX, float sizeY, float centerX, float centerY, float posX, float posY, float rotation, float screenHeightScaleFactor, float r, float g, float b, float a) 
{
	g_D3DHook.DrawTexture(id, index, level, time, sizeX, sizeY, centerX, centerY, posX, posY, rotation, screenHeightScaleFactor, r, g, b, a);
}

/* scripts */
DLL_EXPORT void changeScriptThread(UINT32 hash)
{
	if (g_ThreadHash != hash)
		g_ThreadHash = hash;
}

DLL_EXPORT void scriptWait(DWORD time) 
{
    currentScript->Wait(time);
}

DLL_EXPORT void scriptRegister(HMODULE module, void(*function)())
{
    g_ScriptManagerThread.AddScript(module, function);
}

DLL_EXPORT void scriptRegisterAdditionalThread(HMODULE module, void(*function)()) 
{
    g_ScriptManagerThread.AddAdditional(module, function);
}

DLL_EXPORT void scriptUnregister(HMODULE module)
{
    g_ScriptManagerThread.RemoveScript(module);
}

DLL_EXPORT void scriptUnregister(void(*function)()) 
{ 
    // deprecated
    g_ScriptManagerThread.RemoveScript(function);
}

/* natives */
DLL_EXPORT void nativeInit(UINT64 hash) 
{
	g_hash = hash;

	g_context.Reset();
}

DLL_EXPORT void nativePush64(UINT64 val) 
{
	g_context.Push(val);
}

DLL_EXPORT uint64_t* nativeCall() 
{
	NativeInvoker::Helper::CallNative(&g_context, g_hash);
	return g_Returns.getRawPtr();
}

/* global variables */
DLL_EXPORT UINT64 *getGlobalPtr(int globalId) 
{
    return ScriptEngine::getGlobal(globalId);
}

/* world pools */
DLL_EXPORT int worldGetAllPeds(int *arr, int arrSize) 
{
	return rage::GetAllWorld(PoolTypePed, arrSize, arr);
}

DLL_EXPORT int worldGetAllVehicles(int *arr, int arrSize) 
{
	return rage::GetAllWorld(PoolTypeVehicle, arrSize, arr);
}

DLL_EXPORT int worldGetAllObjects(int *arr, int arrSize) 
{
	return rage::GetAllWorld(PoolTypeObject, arrSize, arr);
}

DLL_EXPORT int worldGetAllPickups(int *arr, int arrSize) 
{
	return rage::GetAllWorld(PoolTypePickup, arrSize, arr);
}

/* game version */
DLL_EXPORT eGameVersion getGameVersion() 
{
	return static_cast<eGameVersion>(g_GameVersion);
}

/* misc */
DLL_EXPORT BYTE* getScriptHandleBaseAddress(int handle) 
{
    return (BYTE*)rage::GetEntityAddress(handle);
}

DLL_EXPORT UINT32 registerRawStreamingFile(const char* fileName, const char* registerAs, bool errorIfFailed)
{
	UINT32 textureID;
	return rage::FileRegister(&textureID, fileName, true, registerAs, errorIfFailed) ? textureID : 0;
}

DLL_EXPORT PVOID createDetour(PVOID* pTarget, PVOID pHandler, const char* name = nullptr)
{
	return Hooking::CreateDetour(pTarget, pHandler, name);
}

DLL_EXPORT void removeDetour(PVOID* ppTarget, PVOID pHandler)
{
	Hooking::RemoveDetour(ppTarget, pHandler);
}

















