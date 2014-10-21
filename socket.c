#include "_.h"
#include "socket.h"
#include "timer.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>


static int try_write(socket_t *sock) {
	return stream.flush(sock->ostm);
}

static int try_read(socket_t *sock) {
	int ret = stream.fill(sock->istm, 0);
	if (ret != 0) {
		if (stream.err(sock->istm) != EAGAIN) {
			return -1;
		}
		return 0;
	}
	return 0;
}

static void _sock_dispatch(eventloop_t *loop, event_t *ev) {
	(void)loop;
	socket_t *sock = container_of(ev, socket_t, ev);
	stream.set_mask(sock->istm, 0);

	if ((ev->mask & (EVMASK_WRITE | EVMASK_ERROR)) == EVMASK_WRITE) {
		if (try_write(sock)) {
			ev->mask |= EVMASK_ERROR;
		}
	}
	if ((ev->mask & (EVMASK_READ | EVMASK_ERROR)) == EVMASK_READ) {
		if (try_read(sock)) {
			ev->mask |= EVMASK_ERROR;
		}
	}

	sock->cb(sock, ev->mask, ev->ud);
}

static socket_t *socket_new_from_fd(int fd, const sockaddr_t *sockname, const sockaddr_t *peername) {
	socket_t *sock = alloc(0, sizeof *sock);
	if (!sock) {
		return 0;
	}
	event.init(&sock->ev, fd, EVMASK_NONE, _sock_dispatch, 0);
	sock->istm = stream.open_fd(fd, 0, 0);
	sock->ostm = stream.open_fd(fd, 0, 0);
	if (!sock->istm || !sock->ostm) {
		alloc(sock, 0);
		return 0;
	}
	if (sockname) {
		sock->sockname = *sockname;
	}
	if (peername) {
		sock->peername = *peername;
	}
	sock->timeout = 60000;
	return sock;
}

static int socket_enable(socket_t *sock, int enable) {
	if (sock->enabled == enable) {
		return 0;
	}
	sock->enabled = enable;
	if (enable) {
		sock->ev.mask = EVMASK_READ;
	} else {
		sock->ev.mask = EVMASK_NONE;
	}
	return eventloop.apply(sock->loop, &sock->ev);
}

static int socket_shutdown(socket_t *sock, int how) {
	return shutdown(sock->ev.fd, how);
}

static void socket_free(socket_t *sock) {
	stream.free(sock->istm);
	stream.free(sock->ostm);
	alloc(sock, 0);
}


static void socket_nonblock(int fd, int enable) {
	int flag = fcntl(fd, F_GETFL);
	if (enable) {
		fcntl(fd, F_SETFL, flag | O_NONBLOCK);
	} else {
		fcntl(fd, F_SETFL, flag & ~O_NONBLOCK);
	}
}


static int socket_for_addr(const sockaddr_t *addr, int type, int flags) {
	int fd;
	fd = socket(addr->family, type, 0);
	if (fd == -1) {
		return fd;
	}
	if (flags & SOCK_CLOEXEC) {
		fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
	if (flags & SOCK_NONBLOCK) {
		socket_.nonblock(fd, 1);
	}
	return fd;
}


struct socket_ socket_ = {
	socket_new_from_fd,
	socket_free,
	socket_enable,
	socket_shutdown,
	socket_nonblock,
	socket_for_addr
};
