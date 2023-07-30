#pragma once


#include <memory>
#include "cstdio"

#define LOG_ERROR(...) printf("[ERROR]"  __VA_ARGS__)
#define LOG_WARN(...)  printf("[WARN]"  __VA_ARGS__)
#define LOG_INFO(...)  printf("[INFO]"  __VA_ARGS__)

#ifdef DEBUG
	#define ASSERT(x,file,line) { if (!(x)) {LOG_ERROR("Assertion failed! File: %s Line: %d!\n", file, line); __debugbreak(); } }
#else
#define ASSERT(x,file,line)
#endif
