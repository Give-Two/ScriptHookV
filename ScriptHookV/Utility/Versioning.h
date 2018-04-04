#ifndef __VERSION_H__
#define __VERSION_H__

#include "..\ScriptHookV.h"

class GTAVersion
{
public:
	static GTAVersion& GetInstance()
	{
		static GTAVersion mInstance;
		return mInstance;
	}
	const std::string GameDirectory() { return gameDirectory; }
	const std::string VersionString() { return versionString; }
	const int GameVersion();
private:
	int ReadVersionString();
protected:
	GTAVersion() {}
	GTAVersion(const GTAVersion&) {}
	GTAVersion& operator=(const GTAVersion&) {}
	int gameVersion;
	bool steamVersion;
	std::string versionString;
	std::string gameDirectory;
};

#endif //__VERSION_H__
