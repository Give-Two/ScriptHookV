#ifndef __HOOKING_H__
#define __HOOKING_H__

#include "..\ScriptHookV.h"
#include <Detours.h>

namespace Hooking
{
	BOOL Natives();
	BOOL NativeDetour(uint64_t hash, PVOID pHandler, PVOID* ppTarget);
	template <typename T>
	PDETOUR_TRAMPOLINE CreateDetour(T** pTarget, PVOID pHandler, const char *name)
	{
		bool unnamed = (name == nullptr);

		// Should you check this before you try and hook something?
		if (g_hooks.find(pTarget) != g_hooks.end()) 
		{
			if (unnamed) 
			{
				LOG_ERROR("Function Pointer is already hooked at %llX", normalise_base(pTarget));
			}
			else 
			{
				LOG_ERROR("%s is already hooked at %llX", name, normalise_base(pTarget));
			}
			return nullptr;
		}

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		PDETOUR_TRAMPOLINE pTrampoline = nullptr;
		DetourAttachEx(pTarget, pHandler, &pTrampoline, nullptr, nullptr);
		if (DetourTransactionCommit() != NO_ERROR) 
		{
			if (unnamed) LOG_ERROR("Could not hook '%llX'", normalise_base(pTarget));
			else LOG_ERROR("Could not hook '%s'", name);
			DetourTransactionAbort(); // Really necessary?
			return nullptr;
		}
		if (unnamed)LOG_DEBUG("Hooked function pointer at '%llX'", normalise_base(pTarget));
		else LOG_DEBUG("Hooked '%s' at '%llX'", name, normalise_base(pTarget));

		g_hooks[pTarget] = std::make_pair(pHandler, name);

		return pTrampoline;
	}

	VOID RemoveDetour(PVOID* ppTarget);

	VOID RemoveAllDetours();
};

typedef std::map<PVOID*, std::pair<PVOID, const char*>> HooksMapType;
extern HooksMapType g_hooks;
std::uintptr_t normalise_base(mem::handle address);

#endif // __HOOKING_H__