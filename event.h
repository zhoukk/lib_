#ifndef EVENT_H
#define EVENT_H

#ifdef __cplusplus
extern "C"{
#endif

#define EVMASK_NONE		0x00
#define EVMASK_READ		0x01
#define EVMASK_WRITE	0x02
#define EVMASK_ERROR	0x04
#define EVMASK_TIME		0x08

struct _eventloop;
typedef struct _event event_t;
typedef void (*event_pt)(struct _eventloop *loop, event_t *ev);

struct _event {
	int fd;
	int mask;
	event_pt cb;
	void *ud;
	int last_mask;
	struct _event *next;
};

extern struct event_ {
	event_t *(*new)(void);
	void (*free)(event_t *ev);
	void (*init)(event_t *ev, int fd, int mask, event_pt cb, void *ud);
} event;

#ifdef __cplusplus
}
#endif

#endif // EVENT_H
