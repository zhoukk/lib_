#ifndef EVENTPOLL_H
#define EVENTPOLL_H

#include "event.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _eventpoll eventpoll_t;
typedef void (*dispatch_pt)(eventpoll_t *poll, event_t *ev, void *ud);

extern struct eventpoll_ {
	eventpoll_t *(*new)(dispatch_pt f, void *ud);
	void (*free)(eventpoll_t *poll);
	int (*apply)(eventpoll_t *poll, event_t *ev);
	void (*wakeup)(eventpoll_t *poll);
	int (*poll)(eventpoll_t *poll);
} eventpoll;

#ifdef __cplusplus
}
#endif

#endif // EVENTPOLL_H
