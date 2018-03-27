#ifndef __SCRIPT_HOOK__
#define __SCRIPT_HOOK__

#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX
#define _DEVBUILD
//#define _CONSOLE

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <memory>
#include <fstream>
#include <Shlwapi.h>
#include <Detours.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <map>
#include <set>

#include <deque>
#include <functional>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")

#include "Utility\Log.h"
#include "Utility\Pattern.h"
#include "..\SDK\inc\types.h"
#include "..\SDK\inc\enums.h"

extern std::deque<std::function<void()>> g_Stack;
extern std::uint32_t g_ThreadHash;
extern eGameVersion g_GameVersion;

enum eGameOffsets : DWORD
{
	Offset_CPed = 0x8,
	Offset_CPlayerInfo = 0x10b8,
	Offset_CFrameFlags = 0x1f8,
};

#define FMT(FM, ...) \
	(   false \
		? std::to_string(printf(FM, ##__VA_ARGS__))  \
		: string_format(FM, ##__VA_ARGS__ ) \
	)

/* String */
template <typename T>
T process_arg(T value) noexcept
{
	return value;
}

template <typename T>
T const * process_arg(std::basic_string<T> const & value) noexcept
{
	return value.c_str();
}

template<typename ... Args>
std::string string_format(const std::string& format, Args const & ... args)
{
	const auto fmt = format.c_str();
	const size_t size = std::snprintf(nullptr, 0, fmt, process_arg(args) ...) + 1;
	auto buf = std::make_unique<char[]>(size);
	std::snprintf(buf.get(), size, fmt, process_arg(args) ...);
	auto res = std::string(buf.get(), buf.get() + size - 1);
	return res;
}

void Cleanup();

#endif // __SCRIPT_HOOK__