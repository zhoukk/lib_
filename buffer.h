/**
 * #Buffer
 *
 * The buffer APIs provids a readable, writable and
 * scalable buffer memory with fix-size.
 *
 *  already yield    read able       write able
 * |___________|__________________|____________|
 * start   read pos			write pos	    size
 *
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"{
#endif

#define BUF_SIZE_P	13

struct _buf;
typedef struct _buffer buffer_t;

extern struct buffer_ {
	/** Create a buffer with length specify of size 
	 *  if size is 0, default size 1 << BUF_SIZE_P
	 *  is selected.
	 */
	buffer_t *(*new)(uint32_t size);

	/** Free the buffer and it's memory. */
	void (*free)(buffer_t *buf);

	/** Read memory from buffer with length of len. */
	uint32_t (*read)(buffer_t *buf, void *mem, uint32_t len);

	/** Write a len of memory to buffer. will extend if not space */
	uint32_t (*write)(buffer_t *buf, const void *mem, uint32_t len);

	/** Return the readable memory of the buffer. */
	uint32_t (*avail)(buffer_t *buf);

	/** Return the read position of the buffer. */
	uint8_t *(*rpos)(buffer_t *buf);

	/** Return the space can write if not extend. */
	uint32_t (*space)(buffer_t *buf);

	/** Return the write position of the buffer. */
	uint8_t *(*wpos)(buffer_t *buf);

	/** Yield when find a record. */
	uint8_t *(*yield)(buffer_t *buf, const char *delim, uint32_t delim_len, uint32_t *len);

	/** Return the total length of the buffer. */
	uint32_t (*len)(buffer_t *buf);

	/** Extern the buffer space. */
	int (*extend)(buffer_t *buf, uint32_t len);

	/** Vprintf a format string to the buffer. */
	uint32_t (*vprintf)(buffer_t *buf, const char *fmt, va_list ap);

	/** Printf a format string to the buffer. */
	uint32_t (*printf)(buffer_t *buf, const char *fmt, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 2, 3)))
#endif
		;

} buffer;

#ifdef __cplusplus
}
#endif

#endif // BUFFER_H
