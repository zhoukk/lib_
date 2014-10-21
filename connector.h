#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "socket.h"

#ifdef __cplusplus
extern "C"{
#endif

struct _connector;
typedef struct _connector connector_t;
typedef void (*connector_pt)(connector_t *connector, int err, socket_t *sock);

extern struct connector_ {
	connector_t *(*new)(const char *name, eventloop_t *loop, connector_pt cb);
	void (*free)(connector_t *conct);
	void (*bind)(connector_t *conct, const sockaddr_t *addr);
	void (*connect)(connector_t *conct);
	eventloop_t *(*loop)(connector_t *conct);
} connector;

#ifdef __cplusplus
}
#endif

#endif // CONNECTOR_H
