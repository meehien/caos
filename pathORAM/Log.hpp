#pragma once

class Log {
public:
	enum LogType {
		INFO,
		WARNING,
		DEBUG,
		FATAL
	};

	static void Write(LogType type, const char* format, ...);
};
