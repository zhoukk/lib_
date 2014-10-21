#include "_.h"
#include "connector.h"
#include "event.h"
#include "eventloop.h"
#include "sockaddr.h"


struct _connector {
	event_t ev;
	eventloop_t *loop;
	sockaddr_t addr;
	connector_pt cb;
	char name[64];
};


static connector_t *connector_new(const char *name, eventloop_t *loop, connector_pt cb) {
	connector_t *conct = alloc(0, sizeof *conct);
	if (!conct) {
		return 0;
	}
	conct->loop = loop;
	conct->cb = cb;
	snprintf(conct->name, sizeof conct->name, "%s", name);
	return conct;
}


static void connector_free(connector_t *conct) {
	alloc(conct, 0);
}


static void connector_bind(connector_t *conct, const sockaddr_t *addr) {
	conct->addr = *addr;
}


static void _connector_dispatch(eventloop_t *loop, event_t *ev) {
	(void)loop;
	connector_t *conct = container_of(ev, connector_t, ev);
	int err = 0;
	socket_t *sock = 0;

	if (ev->mask == EVMASK_TIME) {
		err = ETIMEDOUT;
	} else {
		socklen_t slen = sizeof err;
		if (getsockopt(ev->fd, SOL_SOCKET, SO_ERROR, (void *)&err, &slen) < 0) {
			err = errno;
		} else {
			sockaddr_t addr;
			err = sockaddr.sockname(ev->fd, &addr);
			if (err == 0) {
				sock = socket_.new(ev->fd, &addr, &conct->addr);
			}
		}
	}
	conct->cb(conct, err, sock);
}


static void connector_connect(connector_t *conct) {
	int ret;
	socket_t *sock;
	sockaddr_t addr;
	int err = 0;

	int fd = socket_.for_addr(&conct->addr, SOCK_STREAM, SOCK_CLOEXEC | SOCK_NONBLOCK);
	if (fd == -1) {
		conct->cb(conct, errno, 0);
		return;
	}
	ret = connect(fd, &conct->addr.sa.sa, sockaddr.len(&conct->addr));
	if (ret < 0 && errno == EINPROGRESS) {
		event.init(&conct->ev, fd, EVMASK_WRITE, _connector_dispatch, 0);
		eventloop.apply(conct->loop, &conct->ev);
		return;
	}
	err = sockaddr.sockname(fd, &addr);
	if (err == 0) {
		sock = socket_.new(fd, &addr, &conct->addr);
	}
	conct->cb(conct, err, sock);
}


static eventloop_t *connector_loop(connector_t *conct) {
	return conct->loop;
}

struct connector_ connector = {
	connector_new,
	connector_free,
	connector_bind,
	connector_connect,
	connector_loop
};
