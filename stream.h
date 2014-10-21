#ifndef STREAM_H
#define STREAM_H

#include "buffer.h"

#include <stdint.h>
#include <stdarg.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C"{
#endif

#define STM_BUF_SIZE_P	13

struct _stream;
typedef struct _stream stream_t;

struct stream_funcs {
	int (*close)(stream_t *stm);
	int (*readv)(stream_t *stm, const struct iovec *iov, int iovcnt, uint32_t *nread);
	int (*writev)(stream_t *stm, const struct iovec *iov, int iovcnt, uint32_t *nwrote);
	int (*seek)(stream_t *stm, int32_t delta, int whence, uint32_t *newpos);
};

extern struct stream_ {
	/** Create a stream. it provides a buffer for read and write.
	 *  if bufsize is 0, a default size is selected.
	 */
	stream_t *(*new)(void *io, int flags, uint32_t bufsize, const struct stream_funcs *funcs);

	/** Free a stream and it's buffer. */
	void (*free)(stream_t *stm);

	/** Close a stream. */
	int (*close)(stream_t *stm);

	/** Create a stream for fd. */
	stream_t *(*open_fd)(int fd, int flags, uint32_t bufsize);

	/** Create a stream for file. */
	stream_t *(*open_file)(const char *file, int flags, int mode);

	/** Lock the stream. */
	void (*lock)(stream_t *stm);

	/** Unlock the stream. */
	void (*unlock)(stream_t *stm);

	/** Return the last stream error. */
	int (*err)(stream_t *stm);

	/** Read memory from stream. first try to read stream buf,
	 *  if not enough, will read from stream.
	 */
	int (*read)(stream_t *stm, void *buf, uint32_t len, uint32_t *nread);

	/** Write a memory to the stream. */
	int (*write)(stream_t *stm, const char *buf, uint32_t len, uint32_t *nwrote);

	/** Read ahead from stream to stream buffer. */
	int (*fill)(stream_t *stm, uint32_t *nread);

	/** Flush stream buffer to stream. */
	int (*flush)(stream_t *stm);

	/** Seek the stream buffer like lseek(2).
	 *  The new file position is stored into *newpos unless it is NULL.
	 *  The return value is 0 on success, -1 on error.
	 */
	int (*seek)(stream_t *stm, int32_t delta, int whence, uint32_t *newpos);

	/** Yield a memory from stream with delim. */
	void *(*yield)(stream_t *stm, const char *delim, uint32_t delim_len, uint32_t *len);

	/** Set stream mask. */
	void (*set_mask)(stream_t *stm, int mask);

	/** Get stream mask. */
	int (*get_mask)(stream_t *stm);

	/** Return the stream buffer. */
	buffer_t *(*buffer)(stream_t *stm);

	/** Vprintf a format string to the stream. */
	uint32_t (*vprintf)(stream_t *stm, const char *fmt, va_list ap);

	/** Printf a format string to the stream. */
	uint32_t (*printf)(stream_t *stm, const char *fmt, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 2, 3)))
#endif
		;

} stream;

#ifdef __cplusplus
}
#endif

#endif // STREAM_H
