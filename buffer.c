#include "_.h"
#include "buffer.h"

struct _buffer {
	uint8_t *buf;
	uint32_t size;
	uint32_t rpos;
	uint32_t wpos;
};


static buffer_t *buffer_new(uint32_t size) {
	buffer_t *buf;

	buf = alloc(0, sizeof *buf);
	if (!buf) {
		return 0;
	}
	if (size == 0) {
		buf->size = 1 << BUF_SIZE_P;
	}
	buf->buf = alloc(0, buf->size);
	if (!buf->buf) {
		alloc(buf, 0);
		return 0;
	}
	buf->rpos = 0;
	buf->wpos = 0;

	return buf;
}


static void buffer_free(buffer_t *buf) {
	alloc(buf->buf, 0);
	alloc(buf, 0);
}


static int buffer_extend(buffer_t *buf, uint32_t len) {
	if (buf->rpos > 0) {
		memmove(buf->buf, buf->buf + buf->rpos, buf->wpos - buf->rpos);
		buf->wpos -= buf->rpos;
		buf->rpos = 0;
		if (buf->wpos + len <= buf->size) {
			return 0;
		}
	}
	uint32_t newlen = buf->wpos + len;
	uint8_t *mem = alloc(buf->buf, newlen);
	if (!mem) {
		return -1;
	}
	buf->size = newlen;
	buf->buf = mem;
	return 0;
}


static uint32_t buffer_read(buffer_t *buf, void *mem, uint32_t len) {
	uint32_t nread = min(len, buf->wpos - buf->rpos);
	if (nread) {
		if (mem) {
			memmove(mem, buf->buf + buf->rpos, nread);
		}
		buf->rpos += nread;
		if (buf->rpos >= buf->wpos) {
			buf->rpos = buf->wpos = 0;
		}
	}
	return nread;
}


static uint32_t buffer_write(buffer_t *buf, const void *mem, uint32_t len) {
	if (buf->wpos + len > buf->size) {
		if (-1 == buffer_extend(buf, len)) {
			return 0;
		}
	}
	if (mem) {
		memmove(buf->buf + buf->rpos, mem, len);
	}
	buf->wpos += len;
	return len;
}


static uint32_t buffer_avail(buffer_t *buf) {
	return buf->wpos - buf->rpos;
}


static uint8_t *buffer_rpos(buffer_t *buf) {
	return buf->buf + buf->rpos;
}


static uint32_t buffer_space(buffer_t *buf) {
	return buf->size - buf->wpos;
}


static uint8_t *buffer_wpos(buffer_t *buf) {
	return buf->buf + buf->wpos;
}


static uint8_t *buffer_yield(buffer_t *buf, const char *delim, uint32_t delim_len, uint32_t *len) {
	uint8_t *start = buf->buf + buf->rpos;
	uint8_t *ret;
	uint8_t *found = memmem(start, buf->wpos - buf->rpos, delim, delim_len);
	uint32_t off;
	if (found) {
		off = found + delim_len - start;
		ret = buf->buf + buf->rpos;
		buf->rpos += off;
		if (buf->rpos >= buf->wpos) {
			buf->rpos = buf->wpos = 0;
		}
		if (len) {
			*len = off;
		}
		return ret;
	}
	return 0;
}


static uint32_t buffer_len(buffer_t *buf) {
	return buf->size;
}


static uint32_t buffer_vprintf(buffer_t *buf, const char *fmt, va_list ap) {
	va_list cpy;
	uint32_t avail, ret;

	for (;;) {
		char *p = (char *)buf->buf + buf->wpos;
		avail = buf->size - buf->wpos;
		va_copy(cpy, ap);
		ret = vsnprintf(p, avail, fmt, ap);
		va_end(ap);
		if (ret <= 0) {
			return 0;
		}
		if (ret <= avail) {
			break;
		}
		buffer_extend(buf, ret);
	}
	buf->wpos += ret;

	return ret;
}


static uint32_t buffer_printf(buffer_t *buf, const char *fmt, ...) {
	uint32_t ret;
	va_list ap;

	va_start(ap, fmt);
	ret = buffer_vprintf(buf, fmt, ap);
	va_end(ap);

	return ret;
}


struct buffer_ buffer = {
	buffer_new,
	buffer_free,
	buffer_read,
	buffer_write,
	buffer_avail,
	buffer_rpos,
	buffer_space,
	buffer_wpos,
	buffer_yield,
	buffer_len,
	buffer_extend,
	buffer_vprintf,
	buffer_printf
};
