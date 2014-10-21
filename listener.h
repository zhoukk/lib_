#ifndef LISTENER_H
#define LISTENER_H

#include "socket.h"

#ifdef __cplusplus
extern "C"{
#endif

struct _listener;
typedef struct _listener listener_t;

typedef void (*listener_accept_pt)(listener_t *listener, socket_t *sock);

extern struct listener_ {
	/** Create a new listener with specifity name. */
	listener_t *(*new)(const char *name, eventloop_t *loop, listener_accept_pt acceptor);

	/** Free the listener. */
	void (*free)(listener_t *lstn);

	/**
	 * Bind a listener to a local address
	 *
	 * If no socket has been associated, associates one prior to calling bind(),
	 * and enables address/port reuse socket options
	 */
	int (*bind)(listener_t *listener, const sockaddr_t *addr);


	/**
	 * Returns the underlying socket descriptor for a listener
	 *
	 */
	int (*fd)(listener_t *listener);

	/**
	 * Returns the loop listener running.
	 */
	eventloop_t *(*loop)(listener_t *listener);

	/**
	 * Set the listen backlog.
	 *
	 * If `backlog` is <= 0, attempts to determine the current kernel setting
	 * for the `somaxconn` system parameter and uses that, otherwise falls back
	 * to the `SOMAXCONN` value from your system header, or if that is undefined,
	 * uses the value `128`.
	 */
	void (*set_backlog)(listener_t *listener, int backlog);


	/**
	 * Enable/Disable the listener
	 *
	 * When enabled for the first time, calls listen(2) with the backlog that you
	 * set.  If you have not set a backlog, a default value will be computed per
	 * the description in listener_set_backlog().
	 *
	 * While enabled, the acceptor function will be invoked from any NBIO thread
	 * each time a client connection is accepted.
	 */
	int (*enable)(listener_t *listener, int enable);

} listener;

#ifdef __cplusplus
}
#endif

#endif // LISTENER_H
