
lib_LIBRARIES = lib_.a

lib__a_SOURCES = \
	_.c \
	epoll.c \
	timer.c \
	connector.c \
	event.c \
	eventloop.c \
	thread.c \
	buffer.c \
	stream.c \
	lock.c \
	sockaddr.c \
	listener.c \
	socket.c \
	log.c \
	net.c \
	util.c \
	code.c

lib__a_CFLAGS = \
	-D__IMPL

TEST = test 

noinst_PROGRAMS = $(TEST)

test_CFLAGS = 
test_LDADD = lib_.a
