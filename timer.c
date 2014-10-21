#include "_.h"
#include "timer.h"

#include <sys/time.h>

static int64_t timer_now(void) {
	int64_t t;
	struct timeval tv;
	gettimeofday(&tv, 0);
	t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return t;
}

struct timer_ timer = {
	timer_now
};
