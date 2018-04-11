#include <stdio.h>
#include <time.h>
#include "timeUtil.h"
S_NAMESPACE_BEGIN

#if defined(S_OS_WIN)
uint64_t TimeUtil::getMsTime()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	
	uint64_t t;
	t = ((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime;
	t = t / 10000;
	return t;
}

uint64_t TimeUtil::getTick()
{
	LARGE_INTEGER  large_interger;
	if (!QueryPerformanceCounter(&large_interger))
		return 0;
	return large_interger.QuadPart;
}





#else
#include <sys/time.h>
uint64_t TimeUtil::getMsTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t TimeUtil::getTick()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec << 32 | (uint64_t)tv.tv_usec;
}

#endif

S_NAMESPACE_END
