AC_INIT([_], [0.1], [], [_])
AC_CONFIG_SRCDIR([LICENSE])
AC_CANONICAL_TARGET
AC_LANG(C)

case "$target_cpu" in
	x86_64|amd64)
	CFLAGS="$CFLAGS -m64 -fno-omit-frame-pointer"
	;;
	i*86|pentium*|athlon*|k5|k6|k6_2|k6_3)
	target_cpu=i686
	CFLAGS="$CFLAGS -m32 -march=i686 -msse -msse2"
	;;
esac

CFLAGS="$CFLAGS -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64"

AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects])

AC_PROG_CC
AC_C_BIGENDIAN
AC_C_INLINE
AC_PROG_RANLIB

AM_PROG_CC_C_O
AM_PROG_AS

AC_SEARCH_LIBS([pthread_create], [pthread])
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_SEARCH_LIBS([inet_pton], [nsl])

have__thread=no
AC_TRY_COMPILE([__thread int tls;],[tls;],
			   [have__thread=yes])
if test "$have__thread" == "yes" ; then
	case "$target_os" in
		solaris*)
		dnl I get linker errors with this enabled; seems broken
		;;
		*)
		AC_DEFINE(HAVE___THREAD, 1, [Have __thread TLS])
		;;
	esac
fi

AC_CHECK_HEADERS(\
alloca.h \
inttypes.h \
locale.h \
port.h \
pthread.h \
stdbool.h \
stddef.h \
stdint.h \
string.h \
sys/inotify.h \
sys/epoll.h \
sys/event.h \
sys/eventfd.h \
sys/param.h \
sys/processor.h \
sys/procset.h \
sys/resource.h \
sys/timerfd.h \
)

AC_CHECK_HEADERS([sys/cpuset.h],[],[],[[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
]])

AC_CHECK_HEADERS([pthread_np.h],[],[],[[
#include <pthread.h>
]])

AC_CHECK_FUNCS(\
accept4 \
asprintf \
backtrace \
backtrace_symbols \
backtrace_symbols_fd \
clock_gettime \
cpuset_setaffinity \
epoll_create \
epoll_create1 \
getpagesize \
inotify_init \
kqueue \
localeconv \
pipe2 \
port_create \
processor_bind \
pthread_getname_np \
pthread_set_name_np \
pthread_setname_np \
pthread_setaffinity_np \
pthread_mach_thread_np \
strerror_r \
strtoll \
sysctlbyname \
thread_policy_set \
timerfd_create \
)

CFLAGS="$CFLAGS -std=gnu99 \
	-g -gdwarf-2 -Werror \
	-pedantic -Wall -Wextra -Wno-declaration-after-statement \
	-W -Wundef -Wendif-labels -Wshadow -Wpointer-arith -Wcast-align \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes \
	-Wno-nested-externs -Winline -Wdisabled-optimization \
	-fstrict-aliasing -pipe -Wno-parentheses"

AC_CONFIG_HEADER([_config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
