#include "_.h"
#include "eventloop.h"
#include "lock.h"
#include "log.h"

static void _dispatch(eventpoll_t *poll, event_t *ev, void *ud) {
	(void)poll;
	eventloop_t *loop = ud;
	ev->cb(loop, ev);
}

static void eventloop_init(eventloop_t *loop) {
	memset(loop, 0, sizeof *loop);
	loop->poll = eventpoll.new(_dispatch, loop);
	loop->me = thread.self();
	loop->running = 1;
}


static void eventloop_uninit(eventloop_t *loop) {
	eventpoll.free(loop->poll);
}


static eventloop_t *eventloop_new(void) {
	eventloop_t *loop = alloc(0, sizeof *loop);
	if (!loop) {
		return 0;
	}
	eventloop_init(loop);
	return loop;
}


static void eventloop_free(eventloop_t *loop) {
	eventloop_uninit(loop);
	alloc(loop, 0);
}

static void apply_panding(eventloop_t *loop, event_t *ev) {
	lock.lock(&loop->panding_lock);
	{
		if (loop->pandings == loop->pandings_tail) {
			loop->pandings = loop->pandings_tail = ev;
		} else {
			loop->pandings_tail->next = ev;
			loop->pandings_tail = ev;
		}
	}
	lock.unlock(&loop->panding_lock);
}

static int eventloop_apply(eventloop_t *loop, event_t *ev) {
	if (loop->me == thread.self()) {
		return eventpoll.apply(loop->poll, ev);
	} else {
		apply_panding(loop, ev);
		eventpoll.wakeup(loop->poll);
		return 0;
	}
}


static void panding(eventloop_t *loop) {
	lock.lock(&loop->panding_lock);
	{
		event_t *ev = loop->pandings;
		while (ev) {
			loop->pandings = ev->next;
			eventpoll.apply(loop->poll, ev);
			ev = loop->pandings;
		}
		loop->pandings = loop->pandings_tail = 0;
	}
	lock.unlock(&loop->panding_lock);
}

static void eventloop_loop(eventloop_t *loop) {
	int ret;
	while (SYNC_GET(loop->running)) {
		ret = eventpoll.poll(loop->poll);
		if (ret == 0) {
			logger.debug("eventloop timeout\n");
		} else if (ret < 0) {
			logger.err("eventloop error: %s\n", strerror(errno));
		}
		panding(loop);
	}
}


static void eventloop_exit(eventloop_t *loop) {
	SYNC_SET(loop->running, 0);
}

struct eventloop_ eventloop = {
	eventloop_new,
	eventloop_free,
	eventloop_init,
	eventloop_uninit,
	eventloop_apply,
	eventloop_loop,
	eventloop_exit
};
