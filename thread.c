#include "_.h"
#include "thread.h"
#include "lock.h"

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE___THREAD
__thread thread_t *__thread_self;
#endif
pthread_key_t __thread_key;

static pthread_once_t thread_once = PTHREAD_ONCE_INIT;

struct _thread {
	pthread_t tid;
	char name[64];
};

struct _thread_boot_arg {
	pthread_t tid;
	thread_t *thrd;
	thread_pt runable;
	void *ud;
};


static void thread_free(void *ud) {
#ifdef HAVE___THREAD
	__thread_self = 0;
#endif
	thread_t *thrd = ud;
	if (!thrd) {
		return;
	}
	alloc(thrd, 0);
}

static void thread_init_once(void) {
	pthread_key_create(&__thread_key, thread_free);
}

static void thread_init(void) {
	pthread_once(&thread_once, thread_init_once);
}

static void thread_uninit(void) {
	pthread_exit(NULL);
}

static thread_t *thread_self(void) {
	thread_t *thrd = 0;
#ifdef HAVE___THREAD
	thrd = __thread_self;
#endif
	if (thrd) {
		return thrd;
	}
	thrd = pthread_getspecific(__thread_key);
	if (thrd) {
		return thrd;
	}
	thrd = alloc(0, sizeof *thrd);
	if (!thrd) {
		return 0;
	}
	thrd->tid = pthread_self();
#ifdef HAVE___THREAD
	__thread_self = thrd;
#endif
	pthread_setspecific(__thread_key, thrd);
	return thrd;
}


static void *thread_boot(void *ud) {
	struct _thread_boot_arg *arg = ud;
	arg->thrd = thread_self();

	void *ret = arg->runable(arg->ud);
	pthread_exit(NULL);
	return ret;
}

static thread_t *thread_new(const char *name, thread_pt runable, void *ud) {
	struct _thread_boot_arg arg;
	arg.runable = runable;
	arg.ud = ud;
	arg.thrd = 0;

	pthread_create(&arg.tid, 0, thread_boot, &arg);

	while (!SYNC_GET(arg.thrd)) {}
	snprintf(arg.thrd->name, 64, name);
	return arg.thrd;
}


static int thread_join(thread_t *thrd, void **retval) {
	return pthread_join(thrd->tid, retval);
}


struct thread_ thread = {
	thread_init,
	thread_uninit,
	thread_new,
	thread_join,
	thread_self
};
