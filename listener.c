#include "_.h"
#include "listener.h"
#include "event.h"
#include "eventloop.h"
#include "log.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

struct _listener {
	event_t ev;
	eventloop_t *loop;
	sockaddr_t addr;
	int enabled;
	int listening;
	int backlog;
	int flags;
	listener_accept_pt acceptor;
	char name[64];
};

static void _accept_dispatch(eventloop_t *loop, event_t *ev) {
	(void)loop;
	int fd;
	listener_t *lstn = container_of(ev, listener_t, ev);
	sockaddr_t addr;
	socklen_t alen = sizeof(addr.sa);
	socket_t *sock;


#ifdef HAVE_ACCEPT4
	int a4flags = 0;
	if (lstn->flags & SOCK_CLOEXEC) {
		a4flags |= SOCK_CLOEXEC;
	}
	if (lstn->flags & SOCK_NONBLOCK) {
		a4flags |= SOCK_NONBLOCK;
	}
	fd = accept4(ev->fd, &addr.sa.sa, &alen, a4flags);

#else
	fd = accept(ev->fd, &addr.sa.sa, &alen);
#endif

	if (fd == -1) {
		logger.warn("accept error: %s\n", strerror(errno));
		return;
	}

#ifndef HAVE_ACCEPT4
	if (lstn->flags & SOCK_CLOEXEC) {
		fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
	if (lstn->flags & SOCK_NONBLOCK) {
		socket_.nonblock(fd, 1);
	}
#endif

	addr.family = addr.sa.sa.sa_family;
	sock = socket_.new(fd, &lstn->addr, &addr);
	if (!sock) {
		close(fd);
		return;
	}

	lstn->acceptor(lstn, sock);
}


static void listener_set_backlog(listener_t *lstn, int backlog) {
	if (backlog > 0) {
		lstn->backlog = backlog;
		return;
	}

#ifdef SOMAXCONN
	lstn->backlog = SOMAXCONN;
#else
	lstn->backlog = 128;
#endif

#ifdef __linux__
	int fd = open("/proc/sys/net/core/somaxconn", O_RDONLY);
	if (fd >= 0) {
		char buf[32];
		int x;
		x = read(fd, buf, sizeof(buf));
		if (x > 0) {
			lstn->backlog = strtol(buf, 0, 10);
		}
		close(fd);
	}

#elif defined(HAVE_SYSCTLBYNAME)
	int lim = 0;
	size_t limlen = sizeof(lim);
	if (sysctlbyname("kern.ipc.somaxconn", &lim, &limlen, 0, 0) == 0) {
		lstn->backlog = lim;
	}
#endif
}


static listener_t *listener_new(const char *name, eventloop_t *loop, listener_accept_pt acceptor) {
	listener_t *lstn = alloc(0, sizeof *lstn);
	event.init(&lstn->ev, -1, EVMASK_READ, _accept_dispatch, 0);
	lstn->loop = loop;
	lstn->acceptor = acceptor;
	lstn->flags = SOCK_CLOEXEC | SOCK_NONBLOCK;
	snprintf(lstn->name, sizeof lstn->name, "%s", name);
	listener_set_backlog(lstn, 0);

	return lstn;
}

static void listener_free(listener_t *lstn) {
	alloc(lstn, 0);
}

static int listener_fd(listener_t *lstn) {
	return lstn->ev.fd;
}

static eventloop_t *listener_loop(listener_t *lstn) {
	return lstn->loop;
}

static int listener_bind(listener_t *lstn, const sockaddr_t *addr) {
	int err;

	if (lstn->ev.fd == -1) {
		int on = 1;
		lstn->ev.fd = socket_.for_addr(addr, SOCK_STREAM, lstn->flags);
		if (lstn->ev.fd == -1) {
			return -1;
		}

#ifdef SO_REUSEADDR
		setsockopt(lstn->ev.fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#endif
#ifdef SO_REUSEPORT
		setsockopt(lstn->ev.fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
	}
	if (bind(lstn->ev.fd, &addr->sa.sa, sockaddr.len(addr)) == 0) {
		lstn->addr = *addr;
		return 0;
	}
	err = errno;
	if (err == EADDRINUSE && addr->family == AF_UNIX) {
		unlink(addr->sa.ux.sun_path);
		if (bind(lstn->ev.fd, &addr->sa.sa, sockaddr.len(addr)) == 0) {
			return 0;
		}
		errno = err;
	}

	return -1;
}


static int listener_enable(listener_t *lstn, int enable) {
	if (lstn->enabled == enable) {
		return 0;
	}
	if (!enable) {
		lstn->ev.mask = EVMASK_NONE;
	} else {
		lstn->ev.mask = EVMASK_READ;
	}
	if (!lstn->listening) {
		if (listen(lstn->ev.fd, lstn->backlog)) {
			return -1;
		}
		lstn->listening = 1;
	}
	eventloop.apply(lstn->loop, &lstn->ev);
	lstn->enabled = enable;
	return 0;
}

struct listener_ listener = {
	listener_new,
	listener_free,
	listener_bind,
	listener_fd,
	listener_loop,
	listener_set_backlog,
	listener_enable
};
