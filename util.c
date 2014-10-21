#include "_.h"
#include "util.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/resource.h>
#include <signal.h>
#include <signal.h>
#include <execinfo.h>
#include <ucontext.h>

static const char *end = ".pid";

static int pid_check(const char *bin) {
	char pidfile[strlen(bin)+strlen(end)+1];
	snprintf(pidfile, sizeof(pidfile), "%s%s", bin, end);
	if (0 == access(pidfile, F_OK)){
		FILE *fp = fopen(pidfile, "r");
		if (NULL == fp) return -1;
		char tmp[32] = {0};
		fread(tmp, sizeof tmp, 1, fp);
		fclose(fp);
		return atoi(tmp);
	}
	return 0;
}

static int pid_write(const char *bin) {
	char pidfile[strlen(bin)+strlen(end)+1];
	sprintf(pidfile, "%s%s", bin, end);
	FILE *fp = fopen(pidfile, "w");
	if (!fp) return 0;
	char buf[32] = {0};
	pid_t pid = getpid();
	snprintf(buf, sizeof(buf), "%d", pid);
	fwrite(buf, 1, strlen(buf), fp);
	fclose(fp);
	return pid;
}

static void pid_remove(const char *bin) {
	char pidfile[strlen(bin)+strlen(end)+1];
	sprintf(pidfile, "%s%s", bin, end);
	remove(pidfile);
}

static int fd_limit(int max) {
	rlim_t maxfile = max;
	struct rlimit limit;
	if (getrlimit(RLIMIT_NOFILE, &limit) == -1) return 1024;
	rlim_t oldlimit = limit.rlim_cur;
	if (oldlimit < maxfile) {
		rlim_t f = maxfile;
		while (f > oldlimit) {
			limit.rlim_cur = f;
			limit.rlim_max = f;
			if (setrlimit(RLIMIT_NOFILE, &limit) != -1) break;
			f -= 128;
		}
		if (f < oldlimit) f = oldlimit;
		if (f != maxfile) return f;
		else return maxfile;
	}
	return maxfile;
}

static uint64_t noddy_rando(char *file) {
	uint64_t base;

#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	base = ts.tv_nsec;
#elif defined(__MACH__)
	base = mach_absolute_time();
#else
# error implement me
#endif
	return (base * 65537 ^ (uintptr_t)&base / 16) + (uintptr_t)file;
}


static void _make_temp_name(char *file, int len) {
	uint64_t r = noddy_rando(file);
	int i;

	for (i = 0; i < len; i++, r >>= 5) {
		file[i] = 'A' + (r & 15) + (r & 16) * 2;
	}
}


static int tempfd(char *filetpl, int sufflen, int flags) {
	int len = strlen(filetpl);
	char *base = filetpl + (len - sufflen) -1;
	int tlen = 0;
	int fd, retries = 100;

	while (*base == 'X' && base > filetpl) {
		tlen++;
		base--;
	}
	if (tlen == 0) {
		errno = EINVAL;
		return -1;
	}
	base++;
	while (retries--) {
		_make_temp_name(base, tlen);
		fd = open(filetpl, flags | O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd > 0) {
			return fd;
		}
		if (errno != EEXIST) {
			return -1;
		}
	}
	return -1;
}


static void _stack_trace(ucontext_t *uc, int out) {
	void *trace[128];
	int trace_size = 0;
	trace_size = backtrace(trace, 128);
#if defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
#if defined(__x86_64__)
	trace[1] = (void *)uc->uc_mcontext->__ss.__rip;
#elif defined(__i386)
	trace[1] = (void *)uc->uc_mcontext->__ss.__eip;
#else
	trace[1] = (void *)uc->uc_mcontext->__ss.__srr0;
#endif
#elif defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)
#if defined(_STRUCT_X86_THREAD_STATE64) && !defined(__i386__)
	trace[1]= (void*) uc->uc_mcontext->__ss.__rip;
#else
	trace[1] = (void*) uc->uc_mcontext->__ss.__eip;
#endif
#elif defined(__linux__)
#if defined(__i386__)
	trace[1]= (void*) uc->uc_mcontext.gregs[14];
#elif defined(__X86_64__) || defined(__x86_64__)
	trace[1] = (void*) uc->uc_mcontext.gregs[16];
#elif defined(__ia64__)
	trace[1] = (void*) uc->uc_mcontext.sc_ip;
#endif
#else
	trace[1] =  NULL;
#endif
	backtrace_symbols_fd(trace, trace_size, out);
}

static int _out = STDOUT_FILENO;

static void _sig_segv_handler(int sig, siginfo_t *info, void *ucontext) {
	(void)sig;(void)info;
	ucontext_t *uc = (ucontext_t*) ucontext;
	_stack_trace(uc, _out);
}

static void handle_crash(int outfd) {
	struct rlimit r;
	r.rlim_cur = r.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &r);
	setrlimit(RLIMIT_NPROC, &r);

	struct sigaction act;
	_out = outfd;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
	act.sa_sigaction = _sig_segv_handler;
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGBUS, &act, NULL);
	sigaction(SIGFPE, &act, NULL);
	sigaction(SIGILL, &act, NULL);
	sigaction(SIGABRT, &act, NULL);
	const char * cmd = "echo \"core-%e-%p-%s-%t\" > /proc/sys/kernel/core_pattern";
	system(cmd);
}

static term_pt _term_cb;
static void *_ud = NULL;

static void _sig_term_handler(int sig){
	if (_term_cb) _term_cb(sig, _ud);
}

static void handle_term(term_pt cb, void *ud) {
	struct sigaction act;
	_term_cb = cb;
	_ud = ud;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = _sig_term_handler;
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
}

static int cpu_used(void) {
	static int lastcputime = 0;
	static int lastptime = 0;
	static int cpus = 0;
	int cupinfp = 0;
	int cpu = 0;
	char buff[256] = {0};
	FILE * f = fopen("/proc/stat", "r");
	if (NULL == f) {
		return cpu;
	}
	int totalcputime = 0;
	fscanf(f, "%s", buff);
	for (int i = 0; i < 8; i++) {
		cupinfp = 0;
		fscanf(f, " %d", &cupinfp);
		totalcputime += cupinfp;
	}
	fclose(f);
	char filename[256] = {0};
	sprintf(filename, "/proc/%d/stat", getpid());
	f = fopen(filename, "r");
	if (0 == f) {
		return cpu;
	}

	int totalptime = 0;
	fscanf(f, "%d %s %s", &totalptime, buff, buff);
	for (int i = 0; i < 10; i++) {
		fscanf(f, " %d", &totalptime);
	}
	totalptime = 0;
	for (int i = 0; i < 4; i++) {
		cupinfp = 0;
		fscanf(f, " %d", &cupinfp);
		totalptime += cupinfp;
	}
	fclose(f);
	if (0 == cpus) {
		f = fopen("/proc/cpuinfo", "r");
		if (0 == f) {
			return cpu;
		}
		while (NULL != (fgets(buff, 256, f))) {
			sscanf(buff, "%s %d", buff, &cupinfp);
			if (NULL != strstr(buff, "processor")) {
				cpus ++;
			}
		}
		fclose(f);
	}
	if (0 == lastcputime) {
		lastcputime = totalcputime;
		lastptime = totalptime;
		return cpu;
	}
	if (totalcputime == lastcputime) {
		return cpu;
	}
	cpu = 100 * (totalptime - lastptime) / (totalcputime - lastcputime) * 100 * cpus;
	lastcputime = totalcputime;
	lastptime = totalptime;
	return cpu;
}

static int mem_used(void) {
	char filename[256] = {0};
	sprintf(filename, "/proc/%d/status", getpid());
	FILE * f = fopen(filename, "r");
	if (NULL == f) {
		return -1;
	}
	char buff[256] = {0};
	char key[256] = {0};
	int val = 0;
	while (NULL != (fgets(buff, 256, f))) {
		sscanf(buff, "%s %d", key, &val);
		if (NULL != strstr(key, "VmRSS")) {
			break;
		}
	}
	fclose(f);
	return val;
}


struct util_ util = {
	pid_check,
	pid_write,
	pid_remove,
	fd_limit,
	tempfd,
	handle_term,
	handle_crash,
	cpu_used,
	mem_used
};
