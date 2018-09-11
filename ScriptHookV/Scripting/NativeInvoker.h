#ifndef __NATIVE_INVOKER_H__
#define __NATIVE_INVOKER_H__

#include "..\ScriptHookV.h"
#include "..\Utility\Pattern.h"
#include "..\..\SDK\inc\types.h"

enum HashMapIndice : std::size_t { HMT_NEW, HMT_OFF, HMT_NME, HMT_VER };
using HashMapTuple = std::tuple<uint64_t, uint32_t, const char*, int32_t>;

template <uint32_t stackSize> class NativeStack 
{
public: 

	static constexpr uint32_t size = stackSize;

protected:
	
	uint64_t stack[stackSize];

	template <typename T> T getAt(uint32_t index)const 
	{ 
		return reinterpret_cast<const T &>(stack[index]); 
	}

	template <> bool getAt<bool>(uint32_t index)const
	{
		return reinterpret_cast<const int &>(stack[index]) != 0;
	}

	template <typename T> void setAt(uint32_t index, const T& value)
	{
		reinterpret_cast<T &>(stack[index]) = value;
	}

	template <> void setAt<bool>(uint32_t index, const bool& value)
	{
		reinterpret_cast<int &>(stack[index]) = value != 0;
	}

public:

	decltype(stack)& getRawPtr() { return stack; };
	const decltype(stack)& getRawPtr()const { return stack; }
};

struct NativeArgStack : NativeStack<32u>
{
	template <typename T> NativeArgStack *setArg(const T &value, uint32_t index)
	{
		if (index < size) setAt<T>(index, value); return this;
	}

	template <typename T, uint32_t index> NativeArgStack *setArg(const T &value)
	{
		setAt<T>(index, value); return this;
	}

	template <typename T> T getArg(uint32_t index)const
	{
		return (index < size) ? getAt<T>(index) : T();
	}

	template <typename T, uint32_t index> NativeArgStack *getArg()const
	{
		return getAt<T>(index);
	}
};

struct NativeReturnStack : NativeStack<3u>
{
	template <typename T> T Get() const
	{
		return getAt<T>(0);
	}

	template <typename T> NativeReturnStack *Set(const T &value)
	{
		setAt(0, value); return this;
	}
};

class scrNativeCallContext
{
public:
	static constexpr uint32_t maxNativeArgCount = NativeArgStack::size;
	static constexpr uint32_t maxNativeReturnCount = NativeReturnStack::size;
	static void(*SetVectorResults)(scrNativeCallContext*);
private:
	NativeReturnStack* const m_pReturns;
	uint32_t m_nArgCount;
	NativeArgStack* const m_pArgs;
	uint32_t m_nDataCount;
	uint64_t reservedSpace[24] = {};
public:
	scrNativeCallContext(NativeReturnStack *returnStack, NativeArgStack *argStack)
		: m_pReturns(returnStack)
		, m_nArgCount(0)
		, m_pArgs(argStack)
		, m_nDataCount(0) {}

	void Reset()
	{
		SetArgCount<0>();
		SetDataCount<0>();
	}

	void FixVectors() { SetVectorResults(this); }

	template <typename T> scrNativeCallContext *Push(const T &value)
	{
		m_pArgs->setArg(value, m_nArgCount++); return this;
	}

	template <typename T, uint32_t index> scrNativeCallContext *Push(const T &value)
	{
		m_pArgs->setArg<T, index>(value); return *this;
	}

	template <uint32_t ArgCount> void SetArgCount()
	{
		m_nArgCount = ArgCount;
	}

	template <uint32_t DataCount> void SetDataCount()
	{
		m_nDataCount = DataCount;
	}

	template <typename T> T GetResult() const
	{
		return m_pReturns->Get<T>();
	}

	template <typename T> scrNativeCallContext *SetResult(const T &value)
	{
		m_pReturns->Set(value); return this;
	}

	template <typename T> T GetArg(uint32_t index) const
	{
		return m_pArgs->getArg<T>(index);
	}

	template <typename T> scrNativeCallContext *SetArg(uint32_t index, const T &value)
	{
		m_pArgs->setArg(index, value); return this;
	}

	uint32_t getArgCount() const { return m_nArgCount; }

	NativeArgStack &getArgStack() const { return *m_pArgs; }
	NativeReturnStack &getReturnStack() const { return *m_pReturns; }
};

using NativeHandler = void(*)(scrNativeCallContext *);

namespace NativeInvoker
{
	namespace Helper
	{
		extern uint64_t g_hash;
		extern scrNativeCallContext g_context;
		extern NativeArgStack g_Args;
		extern NativeReturnStack g_Returns;
		extern DECLSPEC_NOINLINE void CallNative(scrNativeCallContext *cxt, std::uint64_t hash);
	}
	
	NativeHandler GetNativeHandler(uint64_t hash);
	
	HashMapTuple GetNativeTuple(uint64_t hash);

	void DumpNativeList();

	// <Local Invoker>
	template <typename T, typename... TArgs> struct NativeArgument
	{
		static constexpr uint32_t paramCount = NativeArgument<T>::paramCount + NativeArgument<TArgs...>::paramCount;
		template <uint32_t index>
		inline static void Push(NativeArgStack& argStack, T value, TArgs... args)
		{
			NativeArgument<T>::Push<index>(argStack, value);
			NativeArgument<TArgs...>::Push<index + NativeArgument<T>::paramCount>(argStack, args...);
		}
	};

	template <> struct NativeArgument<Vector3> // overload the template for NativeVector3 because they push 3 items
	{
		static constexpr uint32_t paramCount = 3u;
		template <uint32_t index>
		inline static void Push(NativeArgStack& argStack, const Vector3 &value)
		{
			argStack.setArg<float, index>(value.x);
			argStack.setArg<float, index + 1>(value.y);
			argStack.setArg<float, index + 2>(value.z);
		}
	};

	template <typename T> struct NativeArgument<T>
	{
		static constexpr uint32_t paramCount = 1;
		template <uint32_t index>
		inline static void Push(NativeArgStack& argStack, T value)
		{
			argStack.setArg<T, index>(value);
		}
	};

	template <typename... TArgs>
	FORCEINLINE void PushArgs(scrNativeCallContext &cxt, TArgs... args)
	{
		cxt.SetDataCount<0>();
		NativeArgument<TArgs...>::Push<0>(cxt.getArgStack(), args...);
		cxt.SetArgCount<NativeArgument<TArgs...>::paramCount>();
	}
	FORCEINLINE void PushArgs(scrNativeCallContext &cxt) { cxt.Reset(); };

	template <typename... TArgs>
	FORCEINLINE void PushArgs(TArgs... args)
	{
		Helper::g_context.SetDataCount<0>();
		NativeArgument<TArgs...>::Push<0>(Helper::g_Args, args...);
		Helper::g_context.SetArgCount<NativeArgument<TArgs...>::paramCount>();
	}
	
	FORCEINLINE void PushArgs() { Helper::g_context.Reset(); }; //zero args

	template <typename R, typename... TArgs>
	FORCEINLINE R invoke(std::uint64_t hash, TArgs... args)
	{
		Helper::g_context.Reset();
		PushArgs(args...);
		Helper::CallNative(&Helper::g_context, hash);
		return Helper::g_Returns.Get<R>();
	};
	// <\Local Invoker end>
};

inline NativeHandler operator""_handler(uint64_t hash)
{
	return  NativeInvoker::GetNativeHandler(hash);
}

inline HashMapTuple operator""_info(uint64_t hash)
{
	return  NativeInvoker::GetNativeTuple(hash);
}

namespace rage
{
	static int64_t*(*GetLocalPlayerInfo)() = "48 8B 05 ? ? ? ? 48 8B 48 08 33 C0 48 85 C9 74 07"_Scan.as<decltype(GetLocalPlayerInfo)>();
	static int64_t(*GetEntityAddress)(int) = "83 f9 ff 74 31 4c 8b 0d ? ? ? ? 44 8b c1 49 8b 41 08"_Scan.as<decltype(GetEntityAddress)>();
	static Entity(*AddressToEntity)(int64_t) = "48 89 5c 24 ? 48 89 74 24 ? 57 48 83 ec 20 8b 15 ? ? ? ? 48 8b f9 48 83 c1 10 33 db"_Scan.as<decltype(AddressToEntity)>();
	static uint32_t*(*FileRegister)(int*, const char*, bool, const char*, bool) = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 50 48 8B EA 4C 8B FA 48 8B D9 4D 85 C9"_Scan.as<decltype(FileRegister)>();
	static int64_t(*GetGtaSwapChain)() = "48 8B 05 ? ? ? ? C3 48 8B C1 8D 4A 0E"_Scan.as<decltype(GetGtaSwapChain)>();

	FORCEINLINE void GET_SCREEN_RESOLUTION(int* x, int* y) { NativeInvoker::invoke<Void>(0x888D57E407E63624, x, y); }
	FORCEINLINE void SET_TEXT_OUTLINE() { NativeInvoker::invoke<Void>(0x2513DFB0FB8400FE); }
	FORCEINLINE void SET_TEXT_WRAP(float start, float end) { NativeInvoker::invoke<Void>(0x63145D9C883A1A70, start, end); }
	FORCEINLINE void SET_TEXT_SCALE(float p0, float size) { NativeInvoker::invoke<Void>(0x07C837F9A01C34C9, p0, size); }
	FORCEINLINE void SET_TEXT_FONT(int fontType) { NativeInvoker::invoke<Void>(0x66E0276CC5F6B9DA, fontType); }
	FORCEINLINE void SET_TEXT_COLOUR(int red, int green, int blue, int alpha) { NativeInvoker::invoke<Void>(0xBE6B23FFA53FB442, red, green, blue, alpha); }
	FORCEINLINE void SET_TEXT_JUSTIFICATION(int justifyType) { NativeInvoker::invoke<Void>(0x4E096588B13FFECA, justifyType); }
	FORCEINLINE void SET_TEXT_CENTRE(bool align) { NativeInvoker::invoke<Void>(0xC02F4DBFB51D988B, align); }
	FORCEINLINE void ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(const char* text) { NativeInvoker::invoke<Void>(0x6C188BE134E074AA, text); }
	FORCEINLINE void BEGIN_TEXT_COMMAND_DISPLAY_TEXT(const char* text) { NativeInvoker::invoke<Void>(0x25FBB336DF1804CB, text); }
	FORCEINLINE void END_TEXT_COMMAND_DISPLAY_TEXT(float x, float y, Any p2) { NativeInvoker::invoke<Void>(0xCD015E5BB0D96A57, x, y, p2); }

	FORCEINLINE bool DOES_TEXT_LABEL_EXIST(const char* gxt) { return NativeInvoker::invoke<bool>(0xAC09CA973C564252, gxt); }
	FORCEINLINE void _SET_NOTIFICATION_TEXT_ENTRY(const char* type) { NativeInvoker::invoke<Void>(0x202709F4C58A0424, type); }
	FORCEINLINE int _DRAW_NOTIFICATION(bool blink, bool p1) { return NativeInvoker::invoke<int>(0x2ED7843F8F801023, blink, p1); }
}

#endif // __NATIVE_INVOKER_H__