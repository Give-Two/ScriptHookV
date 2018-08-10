#ifndef __LOG_H__
#define __LOG_H__

#include "..\ScriptHookV.h"
#define LOG_PRINT( fmt, ...)	Utility::GetLog()->Write( Utility::eLogType::LogTypePrint,	    fmt, ##__VA_ARGS__ )
#define LOG_DEBUG( fmt, ...)	Utility::GetLog()->Write( Utility::eLogType::LogTypeDebug,	    fmt, ##__VA_ARGS__ )
#define LOG_WARNING( fmt, ...)	Utility::GetLog()->Write( Utility::eLogType::LogTypeWarning,	fmt, ##__VA_ARGS__ )
#define LOG_ERROR( fmt, ...)	Utility::GetLog()->Write( Utility::eLogType::LogTypeError,	    fmt, ##__VA_ARGS__ )
#define LOG_ADDRESS(string, function) \
		LOG_DEBUG("%s\t\t 0x%012llx (0x%llX)", string, reinterpret_cast<uint64_t>(function) , reinterpret_cast<uint64_t>(function) - Module().base())

namespace Utility
{
	enum eLogType
	{
		LogTypePrint,
		LogTypeDebug,
		LogTypeWarning,
		LogTypeError,
	};

	typedef std::map<int32_t, std::string> intStringMap;

	class Log 
	{
	public:

		Log();
		~Log();

		void				Clean();

		void				Write( eLogType logType, const char * fmt, ... );
				
	private:

		void				LogToFile( const char * buff );

		const std::string	GetTimeFormatted() const;

		intStringMap		logTypeToFormatMap;
		bool				firstEntry = true;
	};

	Log *					GetLog();
}

#endif // __LOG_H__