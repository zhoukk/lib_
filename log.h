#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"{
#endif

#define LOG_DEBUG		0
#define LOG_WARN		1
#define LOG_ERROR		2

extern struct logger_ {
	/** Set log level. Return the old level. */
	uint8_t (*set_level)(uint8_t level);

	/** Get current log level. */
	uint8_t (*get_level)(void);

	/** Log a format string with level. */
	void (*loglvl)(uint8_t level, const char *fmt, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 2, 3)))
#endif
		;

	/** Log with va_arg. */
	void (*logv)(uint8_t level, const char *fmt, va_list ap);

	/** Log the stack with level. */
	void (*stacktrace)(uint8_t level);

	void (*debug)(const char *fmt, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 1, 2)))
#endif
		;

	void (*warn)(const char *fmt, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 1, 2)))
#endif
		;

	void (*err)(const char *fmt, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 1, 2)))
#endif
		;
} logger;

#ifdef __cplusplus
}
#endif

#endif // LOG_H
