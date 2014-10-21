#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"{
#endif

#define ERR_LEN 256

extern struct net_ {
	int (*tcp_connect)(char *err, const char *addr, int port, int nonblock);
	int (*unix_connect)(char *err, const char *path, int nonblock);
	ssize_t (*readn)(int fd, char *buf, size_t sz);
	ssize_t (*writen)(int fd, char *buf, size_t sz);
	int (*resolve)(char *err, char *host, char *ip, size_t ip_len);
	int (*tcp_server)(char *err, int port, const char *bindaddr, int tcp6);
	int (*unix_server)(char *err, const char *path, mode_t perm);
	int (*tcp_accept)(char *err, int fd, char *ip, size_t ip_len, int *port);
	int (*unix_accept)(char *err, int fd);
	int (*nonblock)(char *err, int fd);
	int (*tcp_nodelay)(char *err, int fd, int enable);
	int (*tcp_keepalive)(char *err, int fd, int interval);
	int (*peer)(int fd, char *ip, size_t ip_len, int *port);
	int (*local)(int fd, char *ip, size_t ip_len, int *port);
} net;

#ifdef __cplusplus
}
#endif

#endif // NET_H