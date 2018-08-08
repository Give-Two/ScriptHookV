#ifndef __NATIVE_INVOKER_H__
#define __NATIVE_INVOKER_H__

#include "..\ScriptHookV.h"

struct HashMapStruct
{
	const char* Name;
	std::uint64_t OldHash;
	std::uint64_t NewHash;
};

namespace rage
{
	static int64_t(*GetEntityAddress)(int) = "83 f9 ff 74 31 4c 8b 0d ? ? ? ? 44 8b c1 49 8b 41 08"_Scan.as<decltype(GetEntityAddress)>();
	static Entity(*AddressToEntity)(int64_t) = "48 89 5c 24 ? 48 89 74 24 ? 57 48 83 ec 20 8b 15 ? ? ? ? 48 8b f9 48 83 c1 10 33 db"_Scan.as<decltype(AddressToEntity)>();
	static uint32_t*(*FileRegister)(uint32_t*, const char*, bool, const char*, bool) = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 50 48 8B EA 4C 8B FA 48 8B D9 4D 85 C9"_Scan.as<decltype(FileRegister)>();
	static int64_t(*GetGtaSwapChain)() = "48 8B 05 ? ? ? ? C3 48 8B C1 8D 4A 0E"_Scan.as<decltype(GetGtaSwapChain)>();
}

template <uint32_t stackSize>
class NativeStack
{
public:
	static constexpr uint32_t size = stackSize;
protected:

	uint64_t stack[size];

	template <typename T>
	T getAt(uint32_t index)const
	{
		return reinterpret_cast<const T &>(stack[index]);
	}

	template <>
	bool getAt<bool>(uint32_t index)const
	{
		return reinterpret_cast<const int &>(stack[index]) != 0;
	}

	template <typename T>
	void setAt(uint32_t index, const T& value)
	{
		reinterpret_cast<T &>(stack[index]) = value;
	}

	template <>
	void setAt<bool>(uint32_t index, const bool& value)
	{
		reinterpret_cast<int &>(stack[index]) = value != 0;
	}

public:
	const decltype(stack)& getRawPtr()const
	{
		return stack;
	}

	decltype(stack)& getRawPtr()
	{
		return stack;
	}
};

struct NativeArgStack : NativeStack<25>
{
	template <typename T> NativeArgStack *setArg(const T &value, uint32_t index)
	{
		if (index < size)
		{
			setAt<T>(index, value);
		}
		return this;
	}

	template <typename T, uint32_t index> NativeArgStack *setArg(const T &value)
	{
		setAt<T>(index, value);
		return this;
	}

	template <typename T> T getArg(uint32_t index)const
	{
		if (index < size)
		{
			return getAt<T>(index);
		}
		return T();
	}

	template <typename T, uint32_t index> NativeArgStack *getArg()const
	{
		return getAt<T>(index);
	}
};

struct NativeReturnStack : NativeStack<3>
{
	template <typename T> T Get() const
	{
		return getAt<T>(0);
	}

	template <typename T> NativeReturnStack *Set(const T &value)
	{
		setAt(0, value);
		return this;
	}
};

class scrNativeCallContext
{
public:
	static constexpr uint32_t maxNativeArgCount = NativeArgStack::size;
	static constexpr uint32_t maxNativeReturnCount = NativeReturnStack::size;
	static void(*SetVectorResults)(scrNativeCallContext *);
private:
	NativeReturnStack *const m_pReturns;
	uint32_t m_nArgCount;
	NativeArgStack *const m_pArgs;
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
		m_pArgs->setArg(value, m_nArgCount++);
		return this;
	}

	template <typename T, uint32_t index>
	scrNativeCallContext *Push(const T &value)
	{
		m_pArgs->setArg<T, index>(value);
		return *this;
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
		m_pReturns->Set(value);
		return this;
	}

	template <typename T> T GetArg(uint32_t index) const
	{
		return m_pArgs->getArg<T>(index);
	}

	template <typename T>
	scrNativeCallContext *SetArg(uint32_t index, const T &value)
	{
		m_pArgs->setArg(index, value);
		return this;
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

		template <typename T> struct NativeArgument<T>
		{
			static constexpr uint32_t paramCount = 1;
			template <uint32_t index>
			inline static void Push(NativeArgStack& argStack, T value)
			{
				argStack.setArg<T, index>(value);
			}
		};

		// overload the template for NativeVector3 because they push 3 items
		template <> struct NativeArgument<Vector3>
		{
			static constexpr uint32_t paramCount = 3;
			template <uint32_t index>
			inline static void Push(NativeArgStack& argStack, const Vector3 &value)
			{
				argStack.setArg<float, index>(value.x);
				argStack.setArg<float, index + 1>(value.y);
				argStack.setArg<float, index + 2>(value.z);
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
			g_context.SetDataCount<0>();
			NativeArgument<TArgs...>::Push<0>(g_Args, args...);
			g_context.SetArgCount<NativeArgument<TArgs...>::paramCount>();
		}
	}

	template <typename R, typename... TArgs>
	FORCEINLINE R invoke(std::uint64_t hash, TArgs... args)
	{
		Helper::g_context.Reset();
		Helper::PushArgs(args...);
		Helper::CallNative(&Helper::g_context, hash);
		return Helper::g_Returns.Get<R>();
	}

	NativeHandler GetNativeHandler(HashMapStruct native);
	
	HashMapStruct NativeInfo(std::uint64_t oldHash);

	extern HashMapStruct g_last_native;
};

#endif // __NATIVE_INVOKER_H__