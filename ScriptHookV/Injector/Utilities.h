#pragma once

#include <Windows.h>
#include <string>
#include <iostream>
#include <memory>

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

DWORD GetProcessIDByName(const std::string& processName);

VOID startup(LPCTSTR lpApplicationName);

bool SetPrivilege(const char * szPrivilege, bool bState = true);

bool Is64BitProcess(HANDLE hProc);

bool IsProcessRunning(const char *filename);
