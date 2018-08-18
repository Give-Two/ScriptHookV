#include "NativeInvoker.h"
#include "ScriptEngine.h"
#include "..\Hooking\Hooking.h"
#include <array>

uint64_t NativeInvoker::Helper::g_hash;
NativeArgStack NativeInvoker::Helper::g_Args;
NativeReturnStack NativeInvoker::Helper::g_Returns;
scrNativeCallContext NativeInvoker::Helper::g_context(&NativeInvoker::Helper::g_Returns, &NativeInvoker::Helper::g_Args);

void(*scrNativeCallContext::SetVectorResults)(scrNativeCallContext*) = "83 79 18 00 48 8B D1 74 4A FF 4A 18"_Scan.as<decltype(SetVectorResults)>();
void* NativeRegistrationTable = "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ?"_Scan.add(3).rip(4).as<decltype(NativeRegistrationTable)>();
NativeHandler(*pGetNativeHandler)(void*, uint64_t hash) = "48 8D 0D ? ? ? ? 48 8B 14 FA E8 ? ? ? ?"_Scan.add(12).rip(4).as<decltype(pGetNativeHandler)>();

const std::unordered_map<uint64_t, HashMapStruct> HashMap
{
#define X(Name, OldHash, NewHash) { OldHash, { #Name, OldHash, NewHash } },
#include "HashMapData.h"
#undef X
};

HashMapStruct NativeInvoker::GetNativeTuple(uint64_t hash)
{
	HashMapStruct tuple;
	return Utility::GetMapValue(HashMap, hash, tuple) ? tuple : HashMapStruct();
}

NativeHandler NativeInvoker::GetNativeHandler(uint64_t hash)
{
	auto newHash = std::get<Hash_New>(GetNativeTuple(hash));
	return newHash ? pGetNativeHandler(NativeRegistrationTable, newHash) : nullptr;
}

DECLSPEC_NOINLINE void NativeInvoker::Helper::CallNative(scrNativeCallContext *cxt, uint64_t hash)
{
	if (auto handler = GetNativeHandler(hash))
	{
		handler(cxt);

		cxt->FixVectors();
	}
	else
	{
		static std::vector<uint64_t> failed;
		if (!Utility::DoesVectorContain(failed, hash))
		{
			LOG_ERROR("Failed to find native handler for 0x%016llX", hash);
			failed.push_back(hash);
		}
	}
}