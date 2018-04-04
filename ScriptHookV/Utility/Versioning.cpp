#include "versioning.h"
#pragma comment(lib,"Version.lib")

const std::unordered_map<const char*, eGameVersion> versionTable
{
	{ ("1.0.335.2"), eGameVersion::VER_1_0_335_2_STEAM },
	{ ("1.0.350.1"), eGameVersion::VER_1_0_350_1_STEAM },
	{ ("1.0.393.2"), eGameVersion::VER_1_0_393_2_STEAM },
	{ ("1.0.393.4"), eGameVersion::VER_1_0_393_4_STEAM },
	{ ("1.0.463.1"), eGameVersion::VER_1_0_463_1_STEAM },
	{ ("1.0.505.2"), eGameVersion::VER_1_0_505_2_STEAM },
	{ ("1.0.573.1"), eGameVersion::VER_1_0_573_1_STEAM },
	{ ("1.0.617.1"), eGameVersion::VER_1_0_617_1_STEAM },
	{ ("1.0.678.1"), eGameVersion::VER_1_0_678_1_STEAM },
	{ ("1.0.757.2"), eGameVersion::VER_1_0_757_2_STEAM },
	{ ("1.0.757.4"), eGameVersion::VER_1_0_757_4_STEAM },
	{ ("1.0.791.2"), eGameVersion::VER_1_0_791_2_STEAM },
	{ ("1.0.877.1"), eGameVersion::VER_1_0_877_1_STEAM },
	{ ("1.0.944.2"), eGameVersion::VER_1_0_944_2_STEAM },
	{ ("1.0.1011.1"), eGameVersion::VER_1_0_1011_1_STEAM },
	{ ("1.0.1032.1"), eGameVersion::VER_1_0_1032_1_STEAM },
	{ ("1.0.1103.1"), eGameVersion::VER_1_0_1103_2_STEAM },
	{ ("1.0.1103.2"), eGameVersion::VER_1_0_1103_2_STEAM },
	{ ("1.0.1290.1"), eGameVersion::VER_1_0_1290_1_STEAM },
	{ ("1.0.1365.1"), eGameVersion::VER_1_0_1365_1_STEAM },
};

int GTAVersion::ReadVersionString()
{
	char fileName[MAX_PATH];
	GetModuleFileNameA(NULL, fileName, MAX_PATH);
	std::string currentPath = fileName;
	if (currentPath.empty()) return 1;

	gameDirectory = currentPath.substr(0, currentPath.find_last_of("\\"));

	DWORD dwHandle, sz = GetFileVersionInfoSizeA(currentPath.c_str(), &dwHandle);
	if (0 == sz)
	{
		return 2;
	}
	char *buf = new char[sz];
	if (!GetFileVersionInfoA(currentPath.c_str(), dwHandle, sz, &buf[0]))
	{
		delete buf;
		return 3;
	}
	VS_FIXEDFILEINFO * pvi;
	sz = sizeof(VS_FIXEDFILEINFO);
	if (!VerQueryValueA(&buf[0], "\\", (LPVOID*)&pvi, (unsigned int*)&sz))
	{
		delete buf;
		return 4;
	}
	versionString = FMT("%d.%d.%d.%d"
		, pvi->dwProductVersionMS >> 16
		, pvi->dwFileVersionMS & 0xFFFF
		, pvi->dwFileVersionLS >> 16
		, pvi->dwFileVersionLS & 0xFFFF);

	delete buf;
	return 0;
}

const int GTAVersion::GameVersion()
{
	gameVersion = -1;

	if (ReadVersionString() == 0)
	{
		for (auto& iter : versionTable)
		{
			if (VersionString().compare(iter.first) == 0)
			{
				gameVersion = iter.second;
				break;
			}
		}
	}

	return gameVersion;
}