#include "Log.h"

const std::string fileName = "ScriptHookV";

namespace Utility 
{
	static Log g_Log;

	Log::Log() 
	{
		logTypeToFormatMap[Utility::LogTypePrint] =		"";
		logTypeToFormatMap[Utility::LogTypeDebug] =		" [Debug]";
		logTypeToFormatMap[Utility::LogTypeWarning]	=	" [Warning]";
		logTypeToFormatMap[Utility::LogTypeError] =		" [Error]";
	}

	Log::~Log()	{}

	const std::string Log::GetTimeFormatted() const
	{
		struct tm timeStruct;
		time_t currTime = time( NULL );
		localtime_s( &timeStruct, &currTime );

		char buff[48];
		sprintf_s( buff, "[%02d:%02d:%02d]", timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec );
		return buff;
	}

	void Log::LogToFile(eLogType logType, const std::string& text)
	{
		static std::string path = GetOurModuleFolder() + "\\" + fileName + ".log";

		std::ofstream logFile;
		
		if (firstEntry) 
		{
			if (GetRunningExecutableFolder() == GetOurModuleFolder())
			{
				logFile.open(path, std::ofstream::trunc); logFile.close();
			}	firstEntry = false;
		}
		else logFile.open(path, std::ofstream::out | std::ofstream::app);

		logFile << GetTimeFormatted().c_str() << " " << logTypeToFormatMap[logType].c_str() << " " << text.c_str() << std::endl;
	}

	void Log::LogToFile(const std::string& file, const std::string& text)
	{
		std::ofstream LOG;

		LOG.open(Utility::GetOurModuleFolder() + "\\" + file + ".txt", std::ofstream::out | std::ofstream::app);

		LOG << text.c_str() << std::endl;

		LOG.close();
	}

	Log * GetLog() 
	{
		return &g_Log;
	}
}