#include "_.h"
#include "listener.h"
#include "connector.h"
#include "sockaddr.h"
#include "event.h"
#include "stream.h"
#include "timer.h"
#include "log.h"
#include "debug.h"
#include "asciilogo.h"
#include "util.h"

#include <sysexits.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>

static void ecprocessor(socket_t *sock, int why, void *ud) {
	(void)ud;
	printf("ecprocessor, fd:%d, why:%d\n", sock->ev.fd, why);
	if (why & (EVMASK_ERROR | EVMASK_TIME)) {
		printf("disconnecting {}\n");
		eventloop_t *loop = sock->loop;
		socket_.enable(sock, 0);
		socket_.shutdown(sock, SOCK_SHUT_RDWR);
		socket_.free(sock);
		eventloop.exit(loop);
		return;
	}
	char tmp[32], tmpp[32];
	printf("ecprocessor local:%s, peer:%s\n", sockaddr.string(&sock->sockname, tmp, 32), sockaddr.string(&sock->peername, tmpp, 32));
	printf("fd:%d why :%d, %" PRIu32 ", space: %" PRIu32 "\n", sock->ev.fd, why, buffer.avail(stream.buffer(sock->istm)),
			buffer.space(stream.buffer(sock->istm)));

	uint32_t len = 0;
	char *buf = stream.yield(sock->istm, "\r\n", 2, &len);
	if (len > 0) {
		buf[len - 2] = 0;
		printf("%" PRId64 " %s\n", timer.now(), buf);
		stream.printf(sock->ostm, "%s\r\n", buf);
		stream.flush(sock->ostm);
	} else {
		printf("not find record\n");
	}
}

static void acceptor(listener_t *lstn, socket_t *sock) {
	sock->cb = ecprocessor;
	printf("accepted {}\n");
	sock->loop = listener.loop(lstn);
	if (socket_.enable(sock, 1)) {
		printf("socket enable: %s\n", strerror(errno));
	}
}


static void connected(connector_t *conct, int err, socket_t *sock) {
	printf("connected status: %d, %s\n", err, strerror(err));
	if (err) {
		printf("connected %s\n", strerror(err));
		return;
	}

	char tmp[32], tmpp[32];
	printf("connected local:%s, peer:%s\n", sockaddr.string(&sock->sockname, tmp, 32), sockaddr.string(&sock->peername, tmpp, 32));
	printf("connected :%d\n", sock->ev.fd);
	sock->cb = ecprocessor;
	sock->loop = connector.loop(conct);
	socket_.enable(sock, 1);
	stream.printf(sock->ostm, "hello:%d\r\n", sock->ev.fd);
	stream.flush(sock->ostm);
	connector.connect(conct);
}

static void _term(int sig, void *ud) {
	(void)ud;
	printf("term sig:%d\n", sig);
	exit(0);
}

int main(int argc, char *argv[]) {
	int c;
	uint16_t port = 8800;
	char *addrstr = 0;
	int use_v4 = 0;
	sockaddr_t addr;
	listener_t *lstn;
	connector_t *conct;

	logger.set_level(LOG_DEBUG);

	while ((c = getopt(argc, argv, "p:l:4s")) != -1) {
		switch (c) {
			case '4':
				use_v4 = 1;
				break;
			case 's':
				break;
			case 'l':
				addrstr = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			default:
				logger.debug(
						"Invalid parameters\n"
						" -4          - interpret address as an IPv4 address\n"
						" -l ADDRESS  - which address to listen on\n"
						" -p PORTNO   - which port to listen on\n"
						" -s          - enable SSL\n"
					  );
				exit(EX_USAGE);
		}
	}

	if ((use_v4 && sockaddr.v4(&addr, addrstr, port) != 0) ||
			(!use_v4 && sockaddr.v6(&addr, addrstr, port) != 0)) {
		printf("Invalid address [%s]:%d\n", addrstr ? addrstr : "*", port);
		exit(EX_USAGE);
	}

	logo();

	char ntpl[] = "abc_XXX.ab";

	util.tempfd(ntpl, 3, 0);

	util.handle_crash(2);

	util.handle_term(_term, 0);

	printf("%d\n", util.pid_check(argv[0]));

	printf("%d\n", util.pid_write(argv[0]));

	printf("%d\n", util.fd_limit(1026));

	*((char *)0) = 0;

	assert(1==1, "assert 1==2 failed, %d, int main\n", 3);

	thread.init();

	eventloop_t loop;
	eventloop.init(&loop);

	char buf[32];
	printf("will listen on %s\n", sockaddr.string(&addr, buf, 32));

	lstn = listener.new("echo", &loop, acceptor);
	if (0 != listener.bind(lstn, &addr)) {
		printf("err :%d\n", errno);
	}
	listener.enable(lstn, 1);

	conct = connector.new("conct", &loop, connected);
	connector.bind(conct, &addr);
	connector.connect(conct);

	eventloop.loop(&loop);

	listener.free(lstn);
	connector.free(conct);
	eventloop.uninit(&loop);

	thread.uninit();

	return 0;
}
