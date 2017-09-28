#include "Log.hpp"

#include <vector>
#include <cstdarg>
#include <iostream>
#include <ctime>

void Log::Write(LogType type, const char *fmt, ...)
{
	std::time_t t = std::time(nullptr);
	char time_buf[100];
	std::strftime(time_buf, sizeof time_buf, "%T", std::gmtime(&t));
	va_list args1;
	va_start(args1, fmt);
	va_list args2;
	va_copy(args2, args1);
	std::vector<char> buf(1+std::vsnprintf(NULL, 0, fmt, args1));
	va_end(args1);
	std::vsnprintf(buf.data(), buf.size(), fmt, args2);
	va_end(args2);
	switch (type) {
		case INFO:
			std::cout << "\033[1;37m"<< time_buf << " INFO:\033[0m    ";
			break;
		case WARNING:
			std::cout << "\033[1;35m"<< time_buf << " WARNING:\033[0m ";
			break;
		case DEBUG:
			if(!debug)
				return;
			std::cout << "\033[0;36m"<< time_buf << " DEBUG:\033[0m   ";
			break;
		case FATAL:
			std::cout << "\033[1;31m"<< time_buf << " FATAL:\033[0m   ";
			break;
		default:
			break;
	}
	std::cout << buf.data() << std::endl;
	if (type == FATAL) {
		exit(1);
	}
}

void Log::TurnOnDebugMode(){
	Log::debug = true;
}

bool Log::debug = false;