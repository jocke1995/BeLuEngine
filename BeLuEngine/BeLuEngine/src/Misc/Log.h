#ifndef LOG_H
#define LOG_H

#include <string>
#include <filesystem>
#include <vector>

enum class Severity
{
	INFO,
	WARNING,
	CRITICAL,
	OTHER
};

class Log
{
public:
	template <typename... Args>
	static void PrintSeverity(
		const std::string& fileName,
		const std::string& lineNumber,
		const Severity type,
		const std::string& string,
		const Args&... args)
	{
		std::vector<char> inputBuffer;
		inputBuffer.resize(4096);

		formatString(inputBuffer, string, args...);

		std::string prefix = std::filesystem::path(fileName).filename().string() + ": Line " + lineNumber;
		switch (type)
		{
		case Severity::INFO:
			prefix += ": INFO: ";
			break;

		case Severity::WARNING:
			prefix += ": WARNING: ";
			break;

		case Severity::CRITICAL:
			prefix += ": CRITICAL ERROR: ";
			break;

		default:
			prefix = "";
			break;
		}

		std::string finalBufferNewline = prefix + inputBuffer.data() + "\n";

		OutputDebugStringA(finalBufferNewline.c_str());
	}

	template <typename... Args>
	static void Print(const std::string string, const Args&... args)
	{
		std::vector<char> inputBuffer;
		inputBuffer.resize(4096);

		formatString(inputBuffer, string, args...);

		std::string finalBuffer = std::string(inputBuffer.data()) + "\n";

		OutputDebugStringA(finalBuffer.c_str());
	}
private:
	Log();

	// Writes the formatted message into buffer (truncated if it doesn't fit)
	template <typename... Args>
	static void formatString(std::vector<char>& buffer, const std::string& format, const Args&... args)
	{
		if constexpr (sizeof...(Args) == 0)
		{
			// No arguments: copy the message as-is, so a '%' in it can't be misinterpreted as a format specifier
			snprintf(buffer.data(), buffer.size(), "%s", format.c_str());
		}
		else
		{
			snprintf(buffer.data(), buffer.size(), format.c_str(), args...);
		}
	}
};

#ifdef DEBUG
	#define BL_LOG_INFO(...)	 Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::INFO	  , __VA_ARGS__)
	#define BL_LOG_WARNING(...)	 Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::WARNING , __VA_ARGS__)
	#define BL_LOG_CRITICAL(...) Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::CRITICAL, __VA_ARGS__)

	#define BL_ASSERT(expression)				if((expression) == false)																							\
												{																												\
													Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::CRITICAL,  "Expression: %s", #expression);	\
													DebugBreak();																								\
												}
	#define BL_ASSERT_MESSAGE(expression, ...)	if((expression) == false)																							\
												{																												\
													Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::CRITICAL, "Expression: %s", #expression);	\
													Log::PrintSeverity(__FILE__, std::to_string(__LINE__), Severity::CRITICAL, __VA_ARGS__);					\
													DebugBreak();																								\
												}
#else
	#define BL_LOG_INFO(...)
	#define BL_LOG_WARNING(...)
	#define BL_LOG_CRITICAL(...)

	#define BL_ASSERT(expression);
	#define BL_ASSERT_MESSAGE(expression, ...);
#endif

#endif