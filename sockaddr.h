#ifndef SOCKADDR_H
#define SOCKADDR_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C"{
#endif

struct _sockaddr {
	sa_family_t family;
	union {
		struct sockaddr sa;
		struct sockaddr_un ux;
		struct sockaddr_in v4;
		struct sockaddr_in6 v6;
	} sa;
};

typedef struct _sockaddr sockaddr_t;

extern struct sockaddr_ {
	/** Return the length of the address. */
	size_t (*len)(const sockaddr_t *addr);

	/** Set a sockaddr to the specified IPv4 address string and port. */
	int (*v4)(sockaddr_t *sa, const char *addr, uint16_t port);

	/** Set a sockaddr to the specified IPv6 address string and port. */
	int (*v6)(sockaddr_t *sa, const char *addr, uint16_t port);

	/** Set a sockaddr to the specified UNIX domain address. */
	int (*ux)(sockaddr_t *sa, const char *path, size_t len);

	/** Set a sockaddr to the first entry from an addrinfo. */
	int (*from_addrinfo)(sockaddr_t *sa, struct addrinfo *ai);

	/** Set a sockaddr to the first address from a hostent. */
	int (*from_hostent)(sockaddr_t *sa, struct hostent *ent);

	/** Set the port number of a sockaddr. */
	void (*set_port)(sockaddr_t *sa, uint16_t port);

	/** Get local socket address. */
	int (*sockname)(int fd, sockaddr_t *sa);

	/** Get peer socket name. */
	int (*peername)(int fd, sockaddr_t *sa);

	/** Print address to string. xxx.xxx.xxx.xxx:xxxx */
	char *(*string)(sockaddr_t *sa, char *buf, size_t len);

} sockaddr;

#ifdef __cplusplus
}
#endif

#endif // SOCKADDR_H
