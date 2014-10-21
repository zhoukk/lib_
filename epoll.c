#include "_.h"
#include "eventpoll.h"
#include "log.h"

#ifdef HAVE_SYS_EPOLL_H

#include <sys/epoll.h>

#ifdef HAVE_SYS_EVENTFD_H
#include <sys/eventfd.h>
#else
#error need implement eventfd
#endif

#define MAX_POLL_EVENT	256
#define DEFAULT_POLL_MASK	EPOLLHUP | EPOLLERR | EPOLLET

struct _eventpoll {
	int pollfd;
	event_t wakev;
	dispatch_pt disp;
	void *ud;
};

static void wake_cb(struct _eventloop *loop, event_t *ev) {
	(void)loop;
	uint64_t on;
	ssize_t n = read(ev->fd, &on, sizeof on);
	if (n != sizeof on) {
		logger.warn("wake_cb read error: %s\n", strerror(errno));
	}
}

static eventpoll_t *eventpoll_new(dispatch_pt f, void *ud) {
	int wakeupfd = -1;
	struct epoll_event evt;
	eventpoll_t *poll = alloc(0, sizeof *poll);
	if (!poll) {
		return 0;
	}

#ifdef HAVE_EPOLL_CREATE1
	poll->pollfd = epoll_create1(EPOLL_CLOEXEC);
#else
	poll->pollfd = event_create(1024);
#endif

	if (poll->pollfd == -1) {
		goto err;
	}

#ifndef HAVE_EPOLL_CREATE1
	fcntl(api->epfd, F_SETFD, FD_CLOEXEC);
#endif

	wakeupfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (wakeupfd == -1) {
		goto err;
	}
	poll->disp = f;
	poll->ud = ud;

	event.init(&poll->wakev, wakeupfd, EVMASK_READ, wake_cb, 0);

	evt.events = EPOLLIN | DEFAULT_POLL_MASK;
	evt.data.ptr = &poll->wakev;
	epoll_ctl(poll->pollfd, EPOLL_CTL_ADD, wakeupfd, &evt);

	return poll;

err:
	if (poll->pollfd) {
		close(poll->pollfd);
	}
	if (wakeupfd != -1) {
		close(wakeupfd);
	}
	if (poll) {
		alloc(poll, 0);
	}
	return 0;
}


static void eventpoll_free(eventpoll_t *poll) {
	close(poll->pollfd);
	close(poll->wakev.fd);
	alloc(poll, 0);
}


static int eventpoll_apply(eventpoll_t *poll, event_t *ev) {
	struct epoll_event evt;
	int ret, newmask;

	switch (ev->mask & (EVMASK_READ | EVMASK_WRITE)) {
		case EVMASK_READ | EVMASK_WRITE:
			newmask = EPOLLIN | EPOLLOUT | DEFAULT_POLL_MASK;
			break;
		case EVMASK_READ:
			newmask = EPOLLIN | DEFAULT_POLL_MASK;
			break;
		case EVMASK_WRITE:
			newmask = EPOLLOUT | DEFAULT_POLL_MASK;
			break;
		case 0:
		default:
			newmask = 0;
			break;
	}
	if (newmask == ev->last_mask) {
		return 0;
	}
	if (newmask == 0) {
		ret = epoll_ctl(poll->pollfd, EPOLL_CTL_DEL, ev->fd, &evt);
		if (ret < 0 && errno == ENOENT) {
			ret = 0;
		}
	} else {
		int op = ev->last_mask ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
		evt.events = newmask;
		evt.data.ptr = ev;
		ev->last_mask = newmask;
		ret = epoll_ctl(poll->pollfd, op, ev->fd, &evt);
		if (ret == -1 && errno == EEXIST && op == EPOLL_CTL_ADD) {
			ret = epoll_ctl(poll->pollfd, EPOLL_CTL_MOD, ev->fd, &evt);
		}
	}
	return ret;
}


static void eventpoll_wakeup(eventpoll_t *poll) {
	uint64_t on = 1;
	ssize_t n = write(poll->wakev.fd, &on, sizeof on);
	if (n != sizeof on) {
		logger.warn("wakeup write error: %s\n", strerror(errno));
	}
}


static int eventpoll_poll(eventpoll_t *poll) {
	struct epoll_event evt[MAX_POLL_EVENT];
	event_t *ev;
	int i, n, mask;

	n = epoll_wait(poll->pollfd, evt, MAX_POLL_EVENT, -1);
	for (i = 0; i < n; i++) {
		ev = evt[i].data.ptr;
		switch (evt[i].events & (EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)) {
			case EPOLLIN:
				mask = EVMASK_READ;
				break;
			case EPOLLOUT:
				mask = EVMASK_WRITE;
				break;
			case EPOLLIN | EPOLLOUT:
				mask = EVMASK_READ | EVMASK_WRITE;
				break;
			default:
				mask = EVMASK_ERROR;
		}
		ev->last_mask = DEFAULT_POLL_MASK;
		ev->mask = mask;
		poll->disp(poll, ev, poll->ud);
	}
	return n;
}


struct eventpoll_ eventpoll = {
	eventpoll_new,
	eventpoll_free,
	eventpoll_apply,
	eventpoll_wakeup,
	eventpoll_poll
};

#endif // HAVE_SYS_EPOLL_H
