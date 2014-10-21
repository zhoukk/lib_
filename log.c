#include "_.h"
#include "log.h"

static uint8_t log_level = LOG_ERROR;
static const char *log_labels[] = {
	"Debug",
	"Warn",
	"Error",
};

static uint8_t log_setlevel(uint8_t level) {
	uint8_t old_level = log_level;
	log_level = level;
	return old_level;
}

static uint8_t log_getlevel(void) {
	return log_level;
}

static void logv(uint8_t level, const char *fmt, va_list ap) {
	va_list copy;

	if (level < log_level) {
		return;
	}

	va_copy(copy, ap);
	fprintf(stderr, "[%s] ", log_labels[level]);
	vfprintf(stderr, fmt, copy);
	va_end(copy);
}

static void loglvl(uint8_t level, const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	logv(level, fmt, ap);
	va_end(ap);
}

#if defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS)
# include <execinfo.h>
#endif

static void log_stacktrace(uint8_t level) {
	(void)level;
#if defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS)
	void *array[24];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(array, sizeof(array)/sizeof(array[0]));
	strings = backtrace_symbols(array, size);

	for (i = 0; i < size; i++) {
		loglvl(level, "%s\n", strings[i]);
	}

	free(strings);
#endif
}

static void log_debug(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	logv(LOG_DEBUG, fmt, ap);
	va_end(ap);
}


static void log_warn(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	logv(LOG_WARN, fmt, ap);
	va_end(ap);
}

static void log_err(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	logv(LOG_ERROR, fmt, ap);
	va_end(ap);
}

struct logger_ logger = {
	log_setlevel,
	log_getlevel,
	loglvl,
	logv,
	log_stacktrace,
	log_debug,
	log_warn,
	log_err,
};
