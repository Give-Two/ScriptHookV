#ifndef __HOOKING_H__
#define __HOOKING_H__

#include "..\ScriptHookV.h"

namespace Hooking
{
	BOOL Natives();
	BOOL NativeDetour(uint64_t hash, PVOID pHandler, PVOID* ppTarget);
	template <typename T>
	PDETOUR_TRAMPOLINE CreateDetour(T** pTarget, PVOID pHandler, const char *name = nullptr)
	{
		PVOID* ppTarget = reinterpret_cast<PVOID*>(pTarget);
		bool unnamed = (!name);

		// Should you check this before you try and hook something?
		if (g_hooks.find(ppTarget) != g_hooks.end()) {
			if (unnamed) {
				LOG_ERROR("Function Pointer is already hooked at %llX", normalise_base(ppTarget));
			}
			else {
				LOG_ERROR("%s is already hooked at %llX", name, normalise_base(ppTarget));
			}
			return nullptr;
		}

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		PDETOUR_TRAMPOLINE pTrampoline = nullptr;
		DetourAttachEx(ppTarget, pHandler, &pTrampoline, nullptr, nullptr);
		if (DetourTransactionCommit() != NO_ERROR) {
			if (unnamed) LOG_ERROR("Could not hook '%llX'", normalise_base(ppTarget));
			else LOG_ERROR("Could not hook '%s'", name);
			DetourTransactionAbort(); // Really necessary?
			return nullptr;
		}
		if (unnamed)LOG_DEBUG("Hooked function pointer at '%llX'", normalise_base(ppTarget));
		else LOG_DEBUG("Hooked '%s' at '%llX'", name, normalise_base(ppTarget));

		g_hooks[ppTarget] = pHandler;

		return pTrampoline;
	}

	VOID RemoveDetour(PVOID* ppTarget, PVOID pHandler);
};

typedef std::map<PVOID*, PVOID> HooksMapType;
extern HooksMapType g_hooks;
std::uintptr_t normalise_base(mem::handle address);

#endif // __HOOKING_H__