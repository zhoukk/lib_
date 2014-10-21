#ifndef THREAD_H
#define THREAD_H

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _thread thread_t;
typedef void *(*thread_pt)(void *);

extern struct thread_ {
	void (*init)(void);
	void (*uninit)(void);
	thread_t *(*new)(const char *name, thread_pt runable, void *ud);
	int (*join)(thread_t *thrd, void **retval);
	thread_t *(*self)(void);
} thread;

#ifdef __cplusplus
}
#endif

#endif // THREAD_H
