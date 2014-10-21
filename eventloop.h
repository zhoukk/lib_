#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "eventpoll.h"
#include "lock.h"
#include "thread.h"


#ifdef __cplusplus
extern "C"{
#endif

typedef struct _eventloop {
	eventpoll_t *poll;
	int running;
	event_t *pandings;
	event_t *pandings_tail;
	lock_t panding_lock;
	thread_t *me;
} eventloop_t;

struct eventloop_ {
	eventloop_t *(*new)(void);
	void (*free)(eventloop_t *loop);
	void (*init)(eventloop_t *loop);
	void (*uninit)(eventloop_t *loop);
	int (*apply)(eventloop_t *loop, event_t *ev);
	void (*loop)(eventloop_t *loop);
	void (*exit)(eventloop_t *loop);
} eventloop;

#ifdef __cplusplus
}
#endif

#endif // EVENTLOOP_H
