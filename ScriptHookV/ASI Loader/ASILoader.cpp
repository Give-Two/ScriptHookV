#include "ASILoader.h"
#include "..\Utility\PEImage.h"

using namespace Utility;

void LoadPlugins(const std::string& asiSearchFolder) 
{
	const std::string asiSearchQuery = asiSearchFolder + "\\*.asi";
	WIN32_FIND_DATAA fileData;
	HANDLE fileHandle = FindFirstFileA(asiSearchQuery.c_str(), &fileData);
	if (fileHandle != INVALID_HANDLE_VALUE) 
	{
		do 
		{
			const std::string pluginPath = asiSearchFolder + "\\" + fileData.cFileName;

			LOG_PRINT("Loading \"%s\"", pluginPath.c_str());
			
			// Load Image
			if (HMODULE module = LoadLibraryA(pluginPath.c_str()))
			{
				LOG_PRINT("\tLoaded \"%s\" => 0x%p", fileData.cFileName, module);
				Utility::playwindowsSound("ding.wav");
			}
			else 
			{
				DWORD errorMessageID = ::GetLastError();
				if (errorMessageID)
				{
					LPSTR messageBuffer = nullptr;
					size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
					std::string message(messageBuffer, size);
					//Free the buffer.
					LocalFree(messageBuffer);
					LOG_ERROR("\tFailed to load: %s", message.c_str());
				}
				else LOG_ERROR("\tFailed to load");
				Utility::playwindowsSound("chord.wav");
			}

		} while (FindNextFileA(fileHandle, &fileData));

		FindClose(fileHandle);
	}
}

void ASILoader::Initialize() 
{
	LOG_PRINT( "Loading *.asi plugins" );

	std::string ScriptHookFolder = GetOurModuleFolder() + "\\asi";
	std::string GtaVFolder = GetRunningExecutableFolder();

	LoadPlugins(ScriptHookFolder);
	LoadPlugins(GtaVFolder);

	LOG_PRINT( "Finished loading *.asi plugins" );
}
