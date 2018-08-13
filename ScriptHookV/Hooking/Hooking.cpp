#include "Hooking.h"

#include "..\Input\InputHook.h"

#include "..\DirectX\D3d11Hook.h"

#include "..\Scripting\ScriptEngine.h"
#include "..\Scripting\NativeInvoker.h"
#include "..\Scripting\ScriptThread.h"
#include "..\Scripting\ScriptManager.h"

using namespace Hooking;

std::uintptr_t Hooking::normalise_base(mem::handle address)
{
	auto module = mem::module::main();

	if (module.contains(address))
	{
		address = address.translate(module.base(), 0x140000000);
	}

	return address.as<std::uintptr_t>();
};

// Natives

DetourHook<void(scrNativeCallContext*)> Detour_System_Wait;
void System_Wait(scrNativeCallContext* context)
{
	switch (g_HookState)
	{
	case HookStateRunning:
	{
		ScriptManager::MainFiber();
	} break;

	case HookStateExiting:
	{
		ScriptManager::UnloadHook();
	} break;

	default:
		break;
	}

	return Detour_System_Wait(context);
}

bool Hooking::HookNatives()
{
	auto native = NativeInvoker::NativeInfo(0x4EDE34FBADD967A6);
	if (auto handler = NativeInvoker::GetNativeHandler(native))
	{
		Detour_System_Wait.Hook(handler, System_Wait, "System_Wait");
		return true;
	}

	return false;
}

void Hooking::UnHookNatives()
{
	Detour_System_Wait.UnHook();
}
		
	