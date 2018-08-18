#include "ScriptManager.h"
#include "ScriptEngine.h"
#include "ScriptThread.h"
#include "NativeInvoker.h"
#include "Pools.h"

#include "..\ASI Loader\ASILoader.h"
#include "..\Input\InputHook.h"
#include "..\DirectX\D3d11Hook.h"
#include "..\Hooking\Hooking.h"
#include "..\..\SDK\inc\types.h"
#include "..\..\SDK\inc\enums.h"

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

ScriptThread	g_ScriptThread;
ScriptThread	g_AdditionalThread;
HANDLE          g_MainFiber;
Script*			g_CurrentScript;

/* ####################### SCRIPT #######################*/

void Script::Tick() 
{
	if (timeGetTime() < wakeAt)
	{
		if (GetCurrentFiber() != g_MainFiber) SwitchToFiber(g_MainFiber); return;
	}

	else if (scriptFiber)
	{
		g_CurrentScript = this;
		SwitchToFiber(scriptFiber);
		g_CurrentScript = nullptr;
	}

	else
	{
		scriptFiber = CreateFiber(NULL, [](LPVOID handler) {reinterpret_cast<Script*>(handler)->Run(); }, this);
	}

	SwitchToFiber(g_MainFiber);
}

void Script::Run() 
{
	__try
	{
		callbackFunction();
	}
	__except (ExpFilter(GetExceptionInformation(), GetExceptionCode()))
	{
		g_AdditionalThread.RemoveScript(this->GetCallbackFunction());
		g_ScriptThread.RemoveScript(this->GetCallbackFunction());
	}
}

void Script::Wait( uint32_t time ) 
{
    wakeAt = timeGetTime() + time;
	if (g_MainFiber) SwitchToFiber(g_MainFiber);
}

void ScriptThread::DoRun() 
{
	for (auto & pair : m_scripts)
	{
		pair.second->Tick();
	}
}

void ScriptThread::AddScript( HMODULE module, void(*fn)()) 
{
    const std::string moduleName = Utility::GetModuleNameWithoutExtension( module);

	if (m_scripts.find(module) != m_scripts.end()) 
	{
		LOG_ERROR("Script '%s' is already registered", moduleName.c_str()); return;
	}	
	else
	{
		ScriptManager::Notification(FMT("Loaded '%s'", moduleName.c_str()));
		LOG_PRINT("Registering script '%s' (0x%p)", moduleName.c_str(), fn);
		m_scripts[module] = std::make_shared<Script>(fn);
	}
}

void ScriptThread::RemoveScript(HMODULE module)
{	
	std::string msg = FMT("Removed '%s'", Utility::GetModuleNameWithoutExtension(module).c_str());
	std::shared_ptr<Script> script;
	if (Utility::GetMapValue(m_scripts, module, script))
	{
		script->Wait(0);
		m_scripts.erase(module);
		FreeLibrary(module);
		CloseHandle(module);
		ScriptManager::Notification(msg);
		LOG_PRINT(msg.c_str());
		script.reset();
	}
}

void ScriptThread::RemoveScript(void(*fn)()) 
{
	for (const auto & pair : m_scripts)
	{
		if (pair.second->GetCallbackFunction() == fn)
		{
			RemoveScript(pair.first);
		}
	}
}

void ScriptThread::RemoveAllScripts()
{	
	if (ScriptCount())
	{
		for (auto & pair : m_scripts)
		{
			RemoveScript(pair.first);
		}
		Utility::playwindowsSound("Windows Default.wav");
		m_scripts.clear();
	}
}

size_t ScriptThread::ScriptCount()
{
	return m_scripts.size();
}

/* ####################### SCRIPTMANAGER #######################*/

namespace ScriptManager
{
	std::deque<std::pair<clock_t, std::string>> notification_stack;

	void Notification(const std::string& text)
	{
		notification_stack.push_front({ clock() + 8000, text });
	}

	Vector2 GetResolution()
	{
		int scr_w, scr_h;
		rage::GET_SCREEN_RESOLUTION(&scr_w, &scr_h);
		return Vector2((float)scr_w, (float)scr_h);
	}

	void DrawScrText(const std::string& text, Vector2 pos, float scale, int font, const int rgba[4], bool outline, bool center)
	{
		rage::SET_TEXT_FONT(font);
		rage::SET_TEXT_SCALE(scale, scale);
		rage::SET_TEXT_COLOUR(rgba[0], rgba[1], rgba[2], rgba[3]);
		rage::SET_TEXT_WRAP(0.f, 1.f);
		rage::SET_TEXT_CENTRE(center);
		if (outline) rage::SET_TEXT_OUTLINE();

		rage::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("CELL_EMAIL_BCON");
		for (std::size_t i = 0; i < text.size(); i += 99)
		{
			rage::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str() + i);
		}

		rage::END_TEXT_COMMAND_DISPLAY_TEXT(pos.x, pos.y, 0);
	}

	void WndProc(HWND /*hwnd*/, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		for (auto & function : g_WndProcCb) function(uMsg, wParam, lParam);

		if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP)
		{
			for (auto & function : g_keyboardFunctions) function((DWORD)wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFF, (lParam >> 24) & 1, (uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP), (lParam >> 30) & 1, (uMsg == WM_SYSKEYUP || uMsg == WM_KEYUP));
		}
	}

	void MainFiber()
	{
		g_MainFiber = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiber(nullptr);

		if (g_MainFiber)
		{
			static scrThread* target_thread = nullptr;

			scrThread* current_thread = GetActiveThread();

			// do this while script::wait
			if (target_thread && current_thread->m_ctx.State == ThreadStateIdle)
			{
				if (current_thread->m_ctx.ScriptHash != g_ThreadHash)
				{
					SetActiveThread(target_thread);
					g_AdditionalThread.DoRun();
					SetActiveThread(current_thread);
				}
			}
			else if (current_thread->m_ctx.State == ThreadStateRunning)
			{
				if (current_thread->m_ctx.ScriptHash == g_ThreadHash)
				{
					if (!target_thread) target_thread = current_thread;
					g_ScriptThread.DoRun();
				}

				// Notifications
				while (!notification_stack.empty() && (notification_stack.back().first < clock())) { notification_stack.pop_back(); }

				for (const auto& pair : notification_stack)
				{
					const auto res = GetResolution();
					const int Color[4] = { 255, 255, 255, 255 };
					auto mBottomPos = Vector2((res.x / 2) / res.x, (res.y / res.y) - 0.049f);
					DrawScrText(pair.second, mBottomPos, 0.33f, 0, Color, true, true); mBottomPos.y -= 0.03f;

				}
			}

			static bool RemoveAllScriptsBool = false; const uint32_t RemoveAllScriptsKey = VK_NEXT; //Page Down
			static bool LoadAllScriptsBool = false; const uint32_t LoadAllScriptsKey = VK_PRIOR;//Page Up
			static bool RemoveScriptHookBool = false; const uint32_t RemoveScriptHookKey = VK_END;

			if (isKeyPressedOnce(RemoveAllScriptsBool, RemoveAllScriptsKey))
			{
				g_AdditionalThread.RemoveAllScripts();
				g_ScriptThread.RemoveAllScripts();
			}

			if (isKeyPressedOnce(LoadAllScriptsBool, LoadAllScriptsKey))
			{
				if (!g_ScriptThread.ScriptCount())
					ASILoader::Initialize();
			}

			if (isKeyPressedOnce(RemoveScriptHookBool, RemoveScriptHookKey))
			{
				g_HookState = HookStateExiting;
			}
		}
	}

	void UnloadHook()
	{
		LOG_DEBUG("Exiting GTA5.exe Process");

		g_AdditionalThread.RemoveAllScripts();

		g_ScriptThread.RemoveAllScripts();

		if (ConvertFiberToThread())
		{
			CloseHandle(g_MainFiber);
			g_HookState = HookStateUnknown;
			Utility::CreateElevatedThread([](LPVOID)->DWORD
			{
				InputHook::Remove();
				g_D3DHook.ReleaseDevices(true);
				Hooking::UnHookFunctions();
				FreeLibraryAndExitThread(Utility::GetOurModuleHandle(), ERROR_SUCCESS);
			});
		}
	}
}

/* ####################### EXPORTS #######################*/

/*Input*/
DLL_EXPORT void WndProcHandlerRegister(TWndProcFn function) 
{
    g_WndProcCb.insert(function);
}

DLL_EXPORT void WndProcHandlerUnregister(TWndProcFn function) 
{
    g_WndProcCb.erase(function);
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
	g_CurrentScript->Wait(time);
}

DLL_EXPORT void scriptRegister(HMODULE module, void(*function)())
{
    g_ScriptThread.AddScript(module, function);
}

DLL_EXPORT void scriptRegisterAdditionalThread(HMODULE module, void(*function)()) 
{
	g_AdditionalThread.AddScript(module, function);
}

DLL_EXPORT void scriptUnregister(HMODULE module)
{
	g_AdditionalThread.RemoveScript(module);
    g_ScriptThread.RemoveScript(module);
}

DLL_EXPORT void scriptUnregister(void(*function)()) 
{ 
    // deprecated
	g_AdditionalThread.RemoveScript(function);
    g_ScriptThread.RemoveScript(function);
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
















