#pragma once

#include <windows.h>
#include <string>

class Registry
{
	HKEY hKey;
	LPCSTR subKey;
	LPCSTR subValue;
	HKEY resKey;
	DWORD dataLen;
public:
	bool isRetailKey();
	bool isSteamKey();
	std::string GetValue(bool steam = false);
};
