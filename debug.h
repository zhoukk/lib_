#ifndef DEBUG_H
#define DEBUG_H

#include "log.h"

#define assert(e, fmt, ...) \
	do {(e)? (void)0 : (logger.err("----------ASSERT----------\n"), \
			logger.err(#e " " fmt, __VA_ARGS__), \
			logger.err(__FILE__ " %s:%d\n", __FUNCTION__, __LINE__), \
			logger.stacktrace(LOG_ERROR), \
			*((char *)0) = '\0',_exit(1));} while (0)

#endif // DEBUG_H
