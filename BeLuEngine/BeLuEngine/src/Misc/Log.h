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
		char typeBuffer[100] = {};

		sprintf(inputBuffer.data(), string.c_str(), args...);

		switch (type)
		{
		case Severity::INFO:
			sprintf(typeBuffer, (std::filesystem::path(fileName).filename().string() + ": Line " + lineNumber + ": INFO: ").c_str());
			break;

		case Severity::WARNING:
			sprintf(typeBuffer, (std::filesystem::path(fileName).filename().string() + ": Line " + lineNumber + ": WARNING: ").c_str());
			break;

		case Severity::CRITICAL:
			sprintf(typeBuffer, (std::filesystem::path(fileName).filename().string() + ": Line " + lineNumber + ": CRITICAL ERROR: ").c_str());
			break;

		default:
			sprintf(typeBuffer, "");
			break;
		}

		std::string finalBuffer = std::string(typeBuffer) + inputBuffer.data();
		std::string finalBufferNewline = finalBuffer + "\n";

		OutputDebugStringA(finalBufferNewline.c_str());
	}

	template <typename... Args>
	static void Print(const std::string string, const Args&... args)
	{
		std::vector<char> inputBuffer;
		inputBuffer.resize(4096);

		sprintf(inputBuffer.data(), string.c_str(), args...);

		std::string finalBuffer = std::string(inputBuffer.data()) + "\n";

		OutputDebugStringA(finalBuffer.c_str());
	}
private:
	Log();
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