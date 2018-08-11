#ifndef __LOG_H__
#define __LOG_H__

#define LOG_PRINT( fmt, ...)	Utility::GetLog()->LogToFile( Utility::eLogType::LogTypePrint,	    FMT( fmt, ##__VA_ARGS__ ))
#define LOG_DEBUG( fmt, ...)	Utility::GetLog()->LogToFile( Utility::eLogType::LogTypeDebug,	    FMT( fmt, ##__VA_ARGS__ ))
#define LOG_WARNING( fmt, ...)	Utility::GetLog()->LogToFile( Utility::eLogType::LogTypeWarning,	FMT( fmt, ##__VA_ARGS__ ))
#define LOG_ERROR( fmt, ...)	Utility::GetLog()->LogToFile( Utility::eLogType::LogTypeError,	    FMT( fmt, ##__VA_ARGS__ ))

#define LOG_ADDRESS(string, function) \
		LOG_DEBUG("%s\t\t 0x%012llx (0x%llX)", string, reinterpret_cast<uint64_t>(function) , reinterpret_cast<uint64_t>(function) - mem::module::main().base().as<uint64_t>())

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

		void				LogToFile(eLogType logType, std::string text);
				
	private:

		const std::string	GetTimeFormatted() const;

		intStringMap		logTypeToFormatMap;
		bool				firstEntry = true;
	};

	Log *					GetLog();
}

#endif // __LOG_H__