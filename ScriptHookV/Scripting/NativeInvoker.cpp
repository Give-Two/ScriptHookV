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

const std::unordered_map<uint64_t, HashMapTuple> mHashMapTuple
{
#define X(OldHash, NewHash, Offset, Name, Version) { OldHash, std::make_tuple( NewHash, Offset, #Name, Version ) },
#include "HashMapData.h"
#undef X
};

HashMapTuple NativeInvoker::GetNativeTuple(uint64_t hash)
{
	HashMapTuple tuple;
	return Utility::GetMapValue(mHashMapTuple, hash, tuple) ? tuple : HashMapTuple();
}

NativeHandler NativeInvoker::GetNativeHandler(uint64_t hash)
{
	HashMapTuple tuple = NativeInvoker::GetNativeTuple(hash);

	if (g_IsRetail)
	{
		// Direct to handler without accessing the NativeRegistrationTable
		if (auto offSet = std::get<HashMapIndice::HMT_OFF>(tuple))
		{
			return mem::module::main().base().add(offSet).as<NativeHandler>();
		}
	}

	if (auto newHash = std::get<HashMapIndice::HMT_NEW>(tuple))
	{
		return pGetNativeHandler(NativeRegistrationTable, newHash);
	}

	return nullptr;
}

void NativeInvoker::DumpNativeList()
{
	for (const auto& map : mHashMapTuple)
	{
		HashMapTuple tuple = NativeInvoker::GetNativeTuple(map.first);

	//	auto pair = map.second;
	//	auto hashOld = map.first;
	//	auto hashNew = std::get<HashMapIndice::HMT_NEW>(tuple);
		auto name = std::get<HashMapIndice::HMT_NME>(tuple);
		auto offset = std::get<HashMapIndice::HMT_OFF>(tuple);
	//	auto offset = Hooking::normalise_base((void*)pGetNativeHandler(NativeRegistrationTable, hashNew), 0);
	//	auto vers = pair.second;
	//	LOG_FILE("Natives", "X(0x%016llX, 0x%016llX, 0x%08llX, %s, %d)", hashOld, hashNew, offset, name, vers);
		LOG_FILE("IDA-python", "['%s', 0x%x],", name, offset);
	}
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