#include "_.h"
#include "event.h"

static event_t *event_new(void) {
	event_t *ev = alloc(0, sizeof *ev);
	return ev;
}

static void event_free(event_t *ev) {
	alloc(ev, 0);
}

static void event_init(event_t *ev, int fd, int mask, event_pt cb, void *ud) {
	ev->fd = fd;
	ev->mask = mask;
	ev->cb = cb;
	ev->ud = ud;
	ev->last_mask = 0;
	ev->next = 0;
}

struct event_ event = {
	event_new,
	event_free,
	event_init
};
