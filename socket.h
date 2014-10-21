#ifndef SOCKET_H
#define SOCKET_H

#include "sockaddr.h"
#include "event.h"
#include "stream.h"
#include "eventloop.h"

#include <time.h>

#ifdef __cplusplus
extern "C"{
#endif

#define SOCK_SHUT_RD		0
#define SOCK_SHUT_WR		1
#define SOCK_SHUT_RDWR		2


typedef struct _socket socket_t;
typedef void (*socket_pt)(socket_t *sock, int why, void *ud);
struct _socket {
	event_t ev;
	eventloop_t *loop;
	stream_t *istm, *ostm;
	uint32_t timeout;
	socket_pt cb;
	int enabled;
	sockaddr_t sockname, peername;
};


extern struct socket_ {
	/** Create a new sock object from a socket descriptor. */
	socket_t *(*new)(int fd, const sockaddr_t *sockname, const sockaddr_t *peername);

	/** Release all resources associated with a socket object. */
	void (*free)(socket_t *sock);

	/** Enable or disable IO dispatching for a socket object. */
	int (*enable)(socket_t *sock, int enable);

	/** Perform a shutdown operation on a socket object. */
	int (*shutdown)(socket_t *sock, int how);

	/** Set or disable non-blocking mode for a file descriptor. */
	void (*nonblock)(int fd, int enable);

	/** Creates a socket that can be used to connect to a sockaddr. */
	int (*for_addr)(const sockaddr_t *addr, int type, int flags);

} socket_;


#ifdef __cplusplus
}
#endif

#endif // SOCKET_H
