#include "net.h"

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static void net_err(char *err, const char *fmt, ...) {
	va_list ap;
	if (!err) return;
	va_start(ap, fmt);
	vsnprintf(err, ERR_LEN, fmt, ap);
	va_end(ap);
}

static int reuse(char *err, int fd) {
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on) == -1) {
		net_err(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
		return -1;
	}
	return 0;
}

static int newsocket(char *err, int domain) {
	int fd = -1;
	if ((fd == socket(domain, SOCK_STREAM, 0)) == -1) {
		net_err(err, "socket: %s", strerror(errno));
		return -1;
	}
	if (reuse(err, fd) == -1) {
		close(fd);
		return -1;
	}
	return fd;
}

static int tcp_nonblock(char *err, int fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		net_err(err, "fcntl F_GETFL: %s", strerror(errno));
		return -1;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		net_err(err, "fcntl F_SETFL O_NONBLOCK: %s", strerror(errno));
		return -1;
	}
	return 0;
}

static int tcp_listen(char *err, int fd, struct sockaddr *sa, socklen_t len) {
	if (bind(fd, sa, len) == -1) {
		net_err(err, "bind: %s", strerror(errno));
		close(fd);
		return -1;
	}
	if (listen(fd, 511) == -1) {
		net_err(err, "listen: %s", strerror(errno));
		close(fd);
		return -1;
	}
	return 0;
}

static int tcp_connect(char *err, const char *addr, int port, int nonblock) {
	int fd = -1;
	int rv;
	char port_buf[6];
	struct addrinfo hints, *servinfo, *p;
	snprintf(port_buf, 6, "%d", port);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(addr, port_buf, &hints, &servinfo)) != 0) {
		net_err(err, "%s", gai_strerror(rv));
		return -1;
	}
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((fd == socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
		if (reuse(err, fd) == -1) goto e;
		if (nonblock == 1 && tcp_nonblock(err, fd) != 0) goto e;
		if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
			if (errno == EINPROGRESS && nonblock == 1) goto end;
			close(fd);
			continue;
		}
		goto end;
	}
	if (p == NULL) {
		net_err(err, "connecting %s", strerror(errno));
		goto e;
	}

e:
	fd = -1;

end:
	freeaddrinfo(servinfo);
	return fd;
}

static int unix_connect(char *err, const char *path, int nonblock) {
	int fd = -1;
	struct sockaddr_un sa;
	if ((fd == newsocket(err, AF_LOCAL)) == -1) return -1;
	sa.sun_family = AF_LOCAL;
	strncpy(sa.sun_path, path, sizeof(sa.sun_path)-1);
	if (nonblock == 1)
		if (tcp_nonblock(err, fd) == -1) return -1;
	if (connect(fd, (struct sockaddr *)&sa, sizeof sa) == -1) {
		if (errno == EINPROGRESS && nonblock == 1) return fd;
		net_err(err, "connect: %s", strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}

static ssize_t readn(int fd, char *buf, size_t sz) {
	size_t nleft = sz;
	ssize_t nread;
	while (nleft > 0) {
		if ((nread = read(fd, buf, nleft)) < 0) {
			if (errno == EINTR) nread = 0;
			else return -1;
		} else if (nread == 0) break;
		nleft -= nread;
		buf += nread;
	}
	return sz - nleft;
}

static ssize_t writen(int fd, char *buf, size_t sz) {
	size_t nleft = sz;
	ssize_t nwritten;
	while (nleft > 0) {
		if ((nwritten = write(fd, buf, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR) nwritten = 0;
			else return -1;
		}
		nleft -= nwritten;
		buf += nwritten;
	}
	return sz;
}

static int resolve(char *err, char *host, char *ip, size_t ip_len) {
	struct addrinfo hints, *info;
	int rv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
		net_err(err, "%s", gai_strerror(rv));
		return -1;
	}
	if (info->ai_family == AF_INET) {
		struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
		inet_ntop(AF_INET, &(sa->sin_addr), ip, ip_len);
	}
	freeaddrinfo(info);
	return 0;
}

static int tcp_server(char *err, int port, const char *bindaddr, int tcp6) {
	int fd = -1;
	int rv;
	char port_buf[6];
	struct addrinfo hints, *servinfo, *p;
	snprintf(port_buf, 6, "%d", port);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = tcp6 == 1 ? AF_INET6 : AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(bindaddr, port_buf, &hints, &servinfo)) != 0) {
		net_err(err, "%s", gai_strerror(rv));
		return -1;
	}
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
		if (reuse(err, fd) == -1) goto e;
		if (tcp_listen(err, fd, p->ai_addr, p->ai_addrlen) == -1) goto e;
		goto end;
	}
	if (p == NULL) {
		net_err(err, "bind to addr error: %s:%d", bindaddr, port);
		goto e;
	}

e:
	fd = -1;

end:
	freeaddrinfo(servinfo);
	return fd;
}

static int unix_server(char *err, const char *path, mode_t perm) {
	int fd;
	struct sockaddr_un sa;
	if ((fd = newsocket(err, AF_LOCAL)) == -1) return -1;
	memset(&sa, 0, sizeof sa);
	sa.sun_family = AF_LOCAL;
	strncpy(sa.sun_path, path, sizeof(sa.sun_path)-1);
	if (tcp_listen(err, fd, (struct sockaddr *)&sa, sizeof sa) == -1) return -1;
	if (perm) chmod(sa.sun_path, perm);
	return fd;
}

static int nonblock_accept(char *err, int fd, struct sockaddr *sa, socklen_t *len) {
	int cfd;
	while (1) {
		cfd = accept(fd, sa, len);
		if (cfd == -1) {
			if (errno == EINTR) continue;
			net_err(err, "accept: %s", strerror(errno));
			return -1;
		}
		break;
	}
	return cfd;
}

static int tcp_accept(char *err, int fd, char *ip, size_t ip_len, int *port) {
	int cfd = -1;
	struct sockaddr_storage sa;
	socklen_t salen = sizeof sa;
	if ((cfd = nonblock_accept(err, fd, (struct sockaddr *)&sa, &salen)) == -1) return -1;
	if (sa.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&sa;
		if (ip) inet_ntop(AF_INET, (void *)&(s->sin_addr), ip, ip_len);
		if (port) *port = ntohs(s->sin_port);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
		if (ip) inet_ntop(AF_INET6, (void *)&(s->sin6_addr), ip, ip_len);
		if (port) *port = ntohs(s->sin6_port);
	}
	return cfd;
}

static int unix_accept(char *err, int fd) {
	int cfd;
	struct sockaddr_un sa;
	socklen_t salen = sizeof sa;
	if ((cfd = nonblock_accept(err, fd, (struct sockaddr *)&sa, &salen)) == -1) return -1;
	return cfd;
}

static int tcp_nodelay(char *err, int fd, int enable) {
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof enable) == -1) {
		net_err(err, "setsockopt TCP_NODELAY: %s", strerror(errno));
		return -1;
	}
	return 0;
}

static int tcp_keepalive(char *err, int fd, int interval) {
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on) == -1) {
		net_err(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
		return -1;
	}

#ifdef __linux__
	int val = interval;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof val) == -1) {
		net_err(err, "setsockopt TCP_KEEPIDLE: %s", strerror(errno));
		return -1;
	}
	val = interval/3;
	if (val == 0) val = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof val) == -1) {
		net_err(err, "setsockopt TCP_KEEPINTVL: %s", strerror(errno));
		return -1;
	}
	val = 3;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof val) == -1) {
		net_err(err, "setsockopt TCP_KEEPCNT: %s", strerror(errno));
		return -1;
	}

#endif //__linux__
	return 0;
}

static int peer(int fd, char *ip, size_t ip_len, int *port) {
	struct sockaddr_storage sa;
	socklen_t salen = sizeof sa;
	if (getpeername(fd, (struct sockaddr *)&sa, &salen) == -1) {
		if (port) *port = 0;
		if (ip) ip[0] = '?',ip[1] = '\0';
		return -1;
	}
	if (sa.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&sa;
		if (ip) inet_ntop(AF_INET, (void *)&(s->sin_addr), ip, ip_len);
		if (port) *port = ntohs(s->sin_port);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
		if (ip) inet_ntop(AF_INET6, (void *)&(s->sin6_addr), ip, ip_len);
		if (port) *port = ntohs(s->sin6_port);
	}
	return 0;
}

static int local(int fd, char *ip, size_t ip_len, int *port) {
	struct sockaddr_storage sa;
	socklen_t salen = sizeof sa;
	if (getsockname(fd, (struct sockaddr *)&sa, &salen) == -1) {
		if (port) *port = 0;
		if (ip) ip[0] = '?',ip[1] = '\0';
		return -1;
	}
	if (sa.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&sa;
		if (ip) inet_ntop(AF_INET, (void *)&(s->sin_addr), ip, ip_len);
		if (port) *port = ntohs(s->sin_port);
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
		if (ip) inet_ntop(AF_INET6, (void *)&(s->sin6_addr), ip, ip_len);
		if (port) *port = ntohs(s->sin6_port);
	}
	return 0;
}


struct net_ net = {
	tcp_connect,
	unix_connect,
	readn,
	writen,
	resolve,
	tcp_server,
	unix_server,
	tcp_accept,
	unix_accept,
	tcp_nonblock,
	tcp_nodelay,
	tcp_keepalive,
	peer,
	local
};

