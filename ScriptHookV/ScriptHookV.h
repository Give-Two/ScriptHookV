#ifndef __SCRIPT_HOOK__
#define __SCRIPT_HOOK__

#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX

#include <windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <memory>
#include <algorithm>
#include <functional>
#include <TlHelp32.h>
#include <Shlwapi.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <deque>
#include <map>
#include <set>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shlwapi.lib")

#include "Utility\General.h"
#include "Utility\Log.h"

#define FMT(fmt, ...) (false ? std::to_string(printf(fmt, ##__VA_ARGS__)) : string_format(fmt, ##__VA_ARGS__ ))
template <typename T> T process_arg(T value) noexcept { return value; }
template <typename T> T const * process_arg(std::basic_string<T> const & value) noexcept { return value.c_str(); }
template <typename ... Args> std::string string_format(const std::string& format, Args const & ... args) {
	
	const auto fmt = format.c_str();
	const auto size = std::snprintf(nullptr, 0, fmt, process_arg(args) ...) + 1;
	auto buf = std::make_unique<char[]>(size);
	std::snprintf(buf.get(), size, fmt, process_arg(args) ...);
	return std::string(buf.get(), buf.get() + size - 1);
}

template<typename InputType, typename ReturnType>
InputType RCast(InputType Input, ReturnType Ret)
{
	return reinterpret_cast<decltype(Input)>(Ret);
}

extern std::uint32_t g_ThreadHash;
extern int g_GameVersion;

#endif // __SCRIPT_HOOK__