#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*term_pt)(int, void *);

extern struct util_ {
	int (*pid_check)(const char *bin);
	int (*pid_write)(const char *bin);
	void (*pid_remove)(const char *bin);

	int (*fd_limit)(int max);
	
	/** filetpl XXXX.xx. */
	int (*tempfd)(char *filetpl, int sufflen, int flags);

	void (*handle_term)(term_pt cb, void *ud);
	void (*handle_crash)(int outfd);

	int (*cpu_used)(void);
	int (*mem_used)(void);

} util;


#ifdef __cplusplus
}
#endif


#endif // UTIL_H
