#ifndef __HOOKING_H__
#define __HOOKING_H__

#include "..\ScriptHookV.h"
#include <Detours.h>

namespace Hooking
{
	template <typename F>
	class DetourHook;

	template <typename R, typename... T>
	class DetourHook<R(T...)>
	{
	public:
		using F = R(*)(T...);

	protected:
		F _original;
		F _detour;
		const char* _name;
		DETOUR_TRAMPOLINE* _trampoline;

	public:
		DetourHook()
			: _original(nullptr)
			, _detour(nullptr)
			, _trampoline(nullptr)
			, _name(nullptr)
		{ }

		~DetourHook()
		{
			if (_trampoline)
			{
				UnHook();

				MessageBoxA(NULL, FMT("A hook at %llX was not removed", normalise_base(_original)).c_str(), "Error", MB_OK);
			}
		}

		bool Hook(F original, F detour, const char* name)
		{
			_original = original;
			_detour = detour;
			_name = name;

			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			DetourAttachEx(reinterpret_cast<void**>(&_original), _detour, &_trampoline, NULL, NULL);

			if (DetourTransactionCommit() != NO_ERROR)
			{
				LOG_ERROR("Could not hook '%s'", name);
				DetourTransactionAbort(); 
				return nullptr;
			}

			LOG_DEBUG("Hooked %s at %llX", name, normalise_base(original));

			return true;
		}

		bool UnHook()
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			DetourDetach(reinterpret_cast<void**>(&_original), _detour);

			bool success = DetourTransactionCommit() == NO_ERROR;

			if (!success)
			{
				LOG_ERROR("Could not unhook '%s'", _name);
				DetourTransactionAbort();
			}
			else
			{
				LOG_DEBUG("Unhooked %s at %llX", _name, normalise_base(_original));
			}

			_name = nullptr;
			_original = nullptr;
			_detour = nullptr;
			_trampoline = nullptr;

			return success;
		}

		R operator()(T&... args)
		{
			return reinterpret_cast<F>(_trampoline)(std::forward<T>(args)...);
		}
	};

	template <typename R, typename... T>
	class DetourHook<R(*)(T...)> : public DetourHook<R(T...)> { };

	template <typename R, typename... T>
	class DetourHook<R(&)(T...)> : public DetourHook<R(T...)> { };

	bool HookNatives();

	void UnHookNatives();

	std::uintptr_t normalise_base(mem::handle address);
};

#endif // __HOOKING_H__