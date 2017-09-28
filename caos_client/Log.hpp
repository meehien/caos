#pragma once

class Log {
public:
	enum LogType {
		INFO,
		WARNING,
		DEBUG,
		FATAL
	};

	static bool debug;

	static void Write(LogType type, const char* format, ...);

	static void TurnOnDebugMode();
};
