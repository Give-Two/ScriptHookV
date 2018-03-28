#include "NativeInvoker.h"
#include "ScriptEngine.h"
#include "..\Utility\General.h"
#include "..\Hooking\Hooking.h"

uint64_t NativeInvoker::Helper::g_hash;
HashMapStruct NativeInvoker::g_last_native;
NativeArgStack NativeInvoker::Helper::g_Args;
NativeReturnStack NativeInvoker::Helper::g_Returns;
scrNativeCallContext NativeInvoker::Helper::g_context(&NativeInvoker::Helper::g_Returns, &NativeInvoker::Helper::g_Args);

void* NativeRegistrationTable = "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ?"_Scan.add(3).rip(4).as<decltype(NativeRegistrationTable)>();
NativeHandler(*pGetNativeHandler)(void*, uint64_t hash) = "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ?"_Scan.add(12).rip(4).as<decltype(pGetNativeHandler)>();

void(*scrNativeCallContext::SetVectorResults)(scrNativeCallContext *) = "83 79 18 00 48 8B D1 74 4A FF 4A 18"_Scan.as<decltype(SetVectorResults)>();

static const std::unordered_map<std::uint64_t, HashMapStruct> HashMap
{
#define X(Name, OldHash, NewHash) { OldHash, { #Name, OldHash, NewHash } },
#include "HashMapData.h"
#undef X
};

HashMapStruct NativeInvoker::NativeInfo(std::uint64_t oldHash)
{
	auto findOldHash = HashMap.find(oldHash);
	if (findOldHash != HashMap.end())
	{	
		return findOldHash->second;
	}
	
	LOG_ERROR("Failed to find native 0x%016llX", oldHash);

	return { "", oldHash, 0 };
}

NativeHandler NativeInvoker::GetNativeHandler(HashMapStruct native)
{
	if (native.NewHash)
	{
		g_last_native = native;
		
        return pGetNativeHandler(NativeRegistrationTable, native.NewHash);
	}

	return nullptr;
}

DECLSPEC_NOINLINE void NativeInvoker::Helper::CallNative(scrNativeCallContext *cxt, uint64_t hash)
{
	auto native = NativeInfo(hash);

	if (native.NewHash)
	{
		auto handler = NativeInvoker::GetNativeHandler(native);

		if (handler)
		{
			handler(cxt);

			cxt->FixVectors();
		}

		else
		{
			LOG_ERROR("Failed to find native handler for 0x%016llX", hash);
			return;
		}
	}
}