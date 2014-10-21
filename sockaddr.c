#include "_.h"
#include "sockaddr.h"

#include <errno.h>

static size_t sockaddr_len(const sockaddr_t *addr) {
	switch (addr->family) {
		case AF_UNIX:
			return sizeof addr->sa.ux;
		case AF_INET:
			return sizeof addr->sa.v4;
		case AF_INET6:
			return sizeof addr->sa.v6;
		default:
			return sizeof addr->sa.sa;
	}
}

static int sockaddr_set_v4(sockaddr_t *sa, const char *addr, uint16_t port) {
	int ret = 1;
	memset(sa, 0, sizeof *sa);
	if (addr) {
		ret = inet_pton(AF_INET, addr, &sa->sa.v4.sin_addr);
	}
	if (ret <= 0) {
		return -1;
	}
	sa->family = AF_INET;
	sa->sa.v4.sin_family = AF_INET;
	sa->sa.v4.sin_port = htons(port);
	return 0;
}

static int sockaddr_set_v6(sockaddr_t *sa, const char *addr, uint16_t port) {
	int ret = -1;
	struct addrinfo *ai, hints;
	memset(sa, 0, sizeof *sa);
	if (!addr) {
		sa->family = AF_INET6;
		sa->sa.v6.sin6_family = AF_INET6;
		sa->sa.v6.sin6_port = htons(port);
		return 0;
	}
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_NUMERICHOST | AI_V4MAPPED;
	hints.ai_family = AF_INET6;
	ret = getaddrinfo(addr, NULL, &hints, &ai);
	if (ret) {
		return -1;
	}
	sa->family = ai->ai_family;
	switch (sa->family) {
		case AF_INET6:
			memcpy(&sa->sa.sa, ai->ai_addr, sizeof sa->sa.v6);
			sa->sa.v6.sin6_port = htons(port);
			freeaddrinfo(ai);
			return 0;
		case AF_INET:
			memcpy(&sa->sa.sa, ai->ai_addr, sizeof sa->sa.v4);
			sa->sa.v4.sin_port = htons(port);
			freeaddrinfo(ai);
			return 0;
		default:
			freeaddrinfo(ai);
			return -1;
	}
}

static int sockaddr_set_unix(sockaddr_t *sa, const char *path, size_t len) {
	if (!path) {
		return -1;
	}
	if (*path == 0) {
		return -1;
	}
	if (len == 0) {
		len = strlen(path);
	}
	if (len + 1 >= sizeof sa->sa.ux.sun_path) {
		return -1;
	}
	memset(sa, 0, sizeof *sa);
	sa->family = AF_UNIX;
	sa->sa.ux.sun_family = AF_UNIX;
	memcpy(&sa->sa.ux.sun_path, path, len);
	return 0;
}

static int sockaddr_set_from_addrinfo(sockaddr_t *sa, struct addrinfo *ai) {
	memset(sa, 0, sizeof *sa);
	sa->family = ai->ai_family;
	switch (sa->family) {
		case AF_INET6:
			memcpy(&sa->sa.sa, ai->ai_addr, sizeof sa->sa.v6);
			return 0;
		case AF_INET:
			memcpy(&sa->sa.sa, ai->ai_addr, sizeof sa->sa.v4);
			return 0;
		default:
			return -1;
	}
}

static int sockaddr_set_from_hostent(sockaddr_t *sa, struct hostent *ent) {
	memset(sa, 0, sizeof(*sa));
	sa->family = ent->h_addrtype;
	switch (sa->family) {
		case AF_INET6:
			sa->sa.v6.sin6_family = sa->family;
			memcpy(&sa->sa.v6.sin6_addr, ent->h_addr_list[0],
					sizeof(sa->sa.v6.sin6_addr));
			return 0;
		case AF_INET:
			sa->sa.v4.sin_family = sa->family;
			memcpy(&sa->sa.v4.sin_addr, ent->h_addr_list[0],
					sizeof(sa->sa.v4.sin_addr));
			return 0;
		default:
			return -1;
	}
}

static void sockaddr_set_port(sockaddr_t *sa, uint16_t port) {
	switch (sa->family) {
		case AF_INET6:
			sa->sa.v6.sin6_port = htons(port);
			break;
		case AF_INET:
			sa->sa.v4.sin_port = htons(port);
			break;
		default:
			break;
	}
}


static int sockaddr_sockname(int fd, sockaddr_t *sa) {
	memset(sa, 0, sizeof(*sa));
	socklen_t len = sockaddr.len(sa);
	if (getsockname(fd, &sa->sa.sa, &len) == 0) {
		return 0;
	} else {
		return errno;
	}
}


static int sockaddr_peername(int fd, sockaddr_t *sa) {
	memset(sa, 0, sizeof(*sa));
	socklen_t len = sockaddr.len(sa);
	if (getpeername(fd, &sa->sa.sa, &len) == 0) {
		return 0;
	} else {
		return errno;
	}
}


static char *sockaddr_string(sockaddr_t *sa, char *buf, size_t len) {
	if (sa->family == AF_INET) {
		inet_ntop(AF_INET, (void *)&(sa->sa.v4.sin_addr), buf, len);
		snprintf(buf + strlen(buf), len - strlen(buf), ":%d", ntohs(sa->sa.v4.sin_port));
	} else {
		inet_ntop(AF_INET6, (void *)&(sa->sa.v6.sin6_addr), buf, len);
		snprintf(buf + strlen(buf), len - strlen(buf), ":%d", ntohs(sa->sa.v6.sin6_port));
	}
	return buf;
}


struct sockaddr_ sockaddr = {
	sockaddr_len,
	sockaddr_set_v4,
	sockaddr_set_v6,
	sockaddr_set_unix,
	sockaddr_set_from_addrinfo,
	sockaddr_set_from_hostent,
	sockaddr_set_port,
	sockaddr_sockname,
	sockaddr_peername,
	sockaddr_string
};
