#include "Hooking.h"

#include "..\Input\InputHook.h"

#include "..\DirectX\D3d11Hook.h"

#include "..\Scripting\ScriptEngine.h"
#include "..\Scripting\NativeInvoker.h"
#include "..\Scripting\ScriptThread.h"
#include "..\Scripting\ScriptManager.h"

using namespace Hooking;

// Gta Natives

// system::wait
DetourHook<NativeHandler> Detour_System_Wait;
void __fastcall System_Wait(scrNativeCallContext* context)
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

// Gta Functions

// NativeRegistration

DetourHook<__int64 __fastcall(__int64, __int64, __int64)> Detour_NativeRegistration;
__int64 __fastcall NativeRegistration(__int64 a1, __int64 hash, __int64 address)
{
	LOG_FILE("Registration", "%i, 0x%016llx, %llx", *(BYTE*)a1, hash, normalise_base(address));

	return Detour_NativeRegistration(a1, hash, address);
}

// Hooking - Functions

std::uintptr_t Hooking::normalise_base(mem::handle address)
{
	auto module = mem::module::main();

	if (module.contains(address))
	{
		address = address.translate(module.base(), 0x140000000);
	}

	return address.as<std::uintptr_t>();
};

bool Hooking::HookFunctions()
{
	if (g_GameVersion == -1)
	{
		return Detour_NativeRegistration.Hook("48 8B C4 48 89 58 08 48 89 68 18 48 89 70 20 48 89 50 10 57 48 83 EC 20 0F B6 C2"_Scan.as<decltype(&NativeRegistration)>(), &NativeRegistration, "NativeRegistration");
	}
	else
	{
		return true

			&& Detour_System_Wait.Hook(0x4EDE34FBADD967A6_handler, System_Wait, "System_Wait");
			;
	}
}

bool Hooking::UnHookFunctions()
{
	if (g_GameVersion == -1)
	{
		return Detour_NativeRegistration.UnHook();
}
	else
	{
		return true
			&& Detour_System_Wait.UnHook()
			;
	}
}
		
	