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

static const std::unordered_map<std::uint64_t, HashMapStruct> FromNew
{
#define X(Name, OldHash, NewHash) { NewHash, { #Name, OldHash, NewHash } },
#include "HashMapData.h"
#undef X
};

static const std::unordered_map<std::uint64_t, HashMapStruct> FromOld
{
#define X(Name, OldHash, NewHash) { OldHash, { #Name, OldHash, NewHash } },
#include "HashMapData.h"
#undef X
};

HashMapStruct NativeInvoker::NativeInfo(std::uint64_t hash, bool is_newhash)
{
	if (is_newhash)
	{
		auto findFromNew = FromNew.find(hash);

		if (findFromNew != FromNew.end())
		{
			return findFromNew->second;
		}
	}
	else
	{
		auto findFromOld = FromOld.find(hash);

		if (findFromOld != FromOld.end())
		{
			return findFromOld->second;
		}
	}

	throw "Native Info Not Found";
}

NativeHandler NativeInvoker::GetNativeHandler(std::uint64_t hash)
{
	auto nativeInfo = NativeInfo(hash, false);

	if (nativeInfo.NewHash)
	{
		g_last_native = nativeInfo;
		
        return pGetNativeHandler(NativeRegistrationTable, nativeInfo.NewHash);
	}

	return nullptr;
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
		throw std::runtime_error(FMT("Failed to find native 0x%016llX", hash));
	}
}