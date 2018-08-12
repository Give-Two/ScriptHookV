#include "Hooking.h"

#include "..\Input\InputHook.h"

#include "..\DirectX\D3d11Hook.h"

#include "..\Scripting\ScriptEngine.h"
#include "..\Scripting\NativeInvoker.h"
#include "..\Scripting\ScriptThread.h"
#include "..\Scripting\ScriptManager.h"

HooksMapType g_hooks;

VOID Hooking::RemoveDetour(PVOID* ppTarget, PVOID pHandler)
{
	if (DetourTransactionBegin() != NO_ERROR)
		return;

	if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR)
	{
		DetourTransactionCommit();
		return;
	}

	if (DetourDetach(ppTarget, pHandler) != NO_ERROR)
	{
		DetourTransactionCommit();
		return;
	}

	if (DetourTransactionCommit() != NO_ERROR)
	{
		DetourTransactionAbort();
		return;
	}

	auto pair = g_hooks.find(ppTarget);
	if (pair == g_hooks.end()) {

		LOG_ERROR("Could not find function hook '%llX' to remove", normalise_base(ppTarget));
		return;
	}

	LOG_DEBUG("Unhooked function pointer at '%llX'", normalise_base(ppTarget));
	g_hooks.erase(pair);
}

VOID Hooking::RemoveAllDetours()
{
	for (const auto& pair : g_hooks)
	{
		RemoveDetour(pair.first, pair.second);
	}
}

std::uintptr_t normalise_base(mem::handle address)
{
	auto module = mem::module::main();

	if (module.contains(address))
	{
		address = address.translate(module.base(), 0x140000000);
	}

	return address.as<std::uintptr_t>();
};

// Natives

PVOID GAME_WAIT = NULL;
PVOID MY_WAIT(scrNativeCallContext *cxt)
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

	return reinterpret_cast<decltype(&MY_WAIT)>(GAME_WAIT)(cxt);
}

BOOL Hooking::Natives()
{
	return TRUE
		// native hooks	
		&& NativeDetour(0x4EDE34FBADD967A6, &MY_WAIT, &GAME_WAIT)
;}

BOOL Hooking::NativeDetour(uint64_t hash, PVOID pHandler, PVOID* ppTarget)
{
	auto native = NativeInvoker::NativeInfo(hash);

	if (native.NewHash)
	{
		auto handler = NativeInvoker::GetNativeHandler(native);
		if (handler)
		{
			*ppTarget = (PVOID*)handler;

			if (g_hooks.find(ppTarget) != g_hooks.end()) {
				LOG_ERROR("%s is already hooked at '%p'", native.Name, ppTarget);
				return TRUE;
			}

			if (DetourTransactionBegin() != NO_ERROR) return FALSE;

			if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR)
			{
				DetourTransactionCommit();
				return FALSE;
			}

			PDETOUR_TRAMPOLINE pTrampoline = NULL;

			if (DetourAttachEx(ppTarget, pHandler, &pTrampoline, NULL, NULL) != NO_ERROR)
			{
				DetourTransactionCommit();
				return FALSE;
			}

			if (DetourTransactionCommit() != NO_ERROR)
			{
				DetourTransactionAbort();
				return FALSE;
			}

			g_hooks[ppTarget] = pHandler;

			LOG_DEBUG("Hooked Native %s (0x%I64X) at '%p'", native.Name, native.NewHash, ppTarget);

			return TRUE;
		}
	}

	LOG_DEBUG("Failed to find %s", native.Name);

	return FALSE;
}