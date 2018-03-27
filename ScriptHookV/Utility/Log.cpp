
#include "Console.h"
#include "General.h"

#include <time.h>

const std::string fileName = "ScriptHookV.log";

namespace Utility {

	static Log g_Log;

	Log::Log() {

		logTypeToColorMap[Utility::LogTypePrint] =		ConsoleForeground::WHITE;
		logTypeToColorMap[Utility::LogTypeDebug] =		ConsoleForeground::GRAY;
		logTypeToColorMap[Utility::LogTypeWarning] =	ConsoleForeground::YELLOW;
		logTypeToColorMap[Utility::LogTypeError] =		ConsoleForeground::RED;

		logTypeToFormatMap[Utility::LogTypePrint] =		"";
		logTypeToFormatMap[Utility::LogTypeDebug] =		" [Debug]";
		logTypeToFormatMap[Utility::LogTypeWarning]	=	" [Warning]";
		logTypeToFormatMap[Utility::LogTypeError] =		" [Error]";
	}

	Log::~Log() {

	}

	void Log::Clean()
	{
		remove((GetOurModuleFolder() + "\\" + fileName).c_str());
	}

	void Log::Write( eLogType logType, const char * fmt, ... ) {

		char buf[2048] = { 0 };
		va_list va_alist;

		va_start( va_alist, fmt );
		vsprintf_s( buf, fmt, va_alist );
		va_end( va_alist );

		GetConsole()->SetTextColor( logTypeToColorMap[logType] );

		char buff2[2048] = { 0 };
		sprintf_s( buff2, "%s %s\n", GetTimeFormatted().c_str(), buf );
		// Print to console
		printf( buff2 );

#ifndef _DEVBUILD
		if ( logType == LogTypeDebug ) {
			return;
		}
#endif

		sprintf_s( buff2, "%s%s %s\n", GetTimeFormatted().c_str(), logTypeToFormatMap[logType].c_str(), buf );
		// Write to log file
		LogToFile( buff2 );
	}

	const std::string Log::GetTimeFormatted() const {

		struct tm timeStruct;
		time_t currTime = time( NULL );
		localtime_s( &timeStruct, &currTime );

		char buff[48];
		sprintf_s( buff, "[%02d:%02d:%02d]", timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec );
		return buff;
	}

	void Log::LogToFile( const char * buff ) {

		const std::string path = GetOurModuleFolder() + "\\" + fileName;

		std::ofstream logFile;
		logFile.open( path, std::ios_base::app );
		if ( firstEntry ) {

			logFile << std::endl;
			firstEntry = false;
		}
		logFile << buff;	
	}

	void Log::LogMessage(const std::string& title, const std::string& message)
	{
		MessageBoxA(NULL, message.c_str(), title.c_str(), MB_OK | MB_SETFOREGROUND | MB_TOPMOST | MB_APPLMODAL);
	}

	Log * GetLog() {

		return &g_Log;
	}
}