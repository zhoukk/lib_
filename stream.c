#include "_.h"
#include "stream.h"
#include "buffer.h"
#include "lock.h"
#include "event.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define STM_READALL_LEN	65535

struct _stream {
	void *io;
	int flags;
	buffer_t *buf;
	int need_mask;
	lock_t lock;
	const struct stream_funcs *funcs;
	int last_err;
};


static stream_t *stream_new(void *io, int flags, uint32_t bufsize, const struct stream_funcs *funcs) {
	stream_t *stm;

	stm = alloc(0, sizeof *stm);
	if (!stm) {
		return 0;
	}
	stm->lock = 0;
	stm->buf = buffer.new(bufsize);
	if (!stm->buf) {
		alloc(stm, 0);
		errno = ENOMEM;
		return 0;
	}
	stm->flags = flags;
	stm->io = io;
	stm->funcs = funcs;

	return stm;
}


static void stream_free(stream_t *stm) {
	buffer.free(stm->buf);
	alloc(stm, 0);
}


static int stream_close(stream_t *stm) {
	if (stream.flush(stm)) {
		return -1;
	}
	if (stm->funcs->close(stm)) {
		return -1;
	}
	return 0;
}


static int fd_close(stream_t *stm) {
	int fd = (intptr_t)stm->io;
	if (close(fd) == 0) {
		return 0;
	}
	stm->last_err = errno;
	return -1;
}


static int fd_readv(stream_t *stm, const struct iovec *iov, int iovcnt, uint32_t *nread) {
	int fd = (intptr_t)stm->io;
	int ret;

	ret = readv(fd, iov, iovcnt);
	if (ret == -1) {
		stm->last_err = errno;
		if (errno == EAGAIN) {
			stm->need_mask |= EVMASK_READ;
		}
		return -1;
	}
	if (ret == 0) {
		stm->last_err = 0;
		return -1;
	}
	if (nread) {
		*nread = ret;
	}
	return 0;
}


static int fd_writev(stream_t *stm, const struct iovec *iov, int iovcnt, uint32_t *nwrote) {
	int fd = (intptr_t)stm->io;
	int ret;

	ret = writev(fd, iov, iovcnt);
	if (ret == -1) {
		stm->last_err = errno;
		if (errno == EAGAIN) {
			stm->need_mask |= EVMASK_WRITE;
		}
		return -1;
	}
	if (ret == 0) {
		stm->last_err = 0;
		return -1;
	}
	if (nwrote) {
		*nwrote = ret;
	}
	return 0;
}


static int fd_seek(stream_t *stm, int32_t delta, int whence, uint32_t *newpos) {
	int fd = (intptr_t)stm->io;
	off_t off;

	off = lseek(fd, delta, whence);
	if (off == (off_t)-1) {
		stm->last_err = errno;
		return -1;
	}
	if (newpos) {
		*newpos = off;
	}
	return 0;
}


struct stream_funcs stream_funcs_fd = {
	fd_close,
	fd_readv,
	fd_writev,
	fd_seek
};


static stream_t *stream_fd_open(int fd, int flags, uint32_t bufsize) {
	return stream_new((void *)(intptr_t)fd, flags, bufsize, &stream_funcs_fd);
}


static stream_t *stream_file_open(const char *file, int flags, int mode) {
	int fd;
	stream_t *stm;
#ifdef O_LARGEFILE
	flags |= O_LARGEFILE;
#endif
	fd = open(file, flags, mode);
	if (fd < 0) {
		return 0;
	}
	stm = stream_fd_open(fd, 0, 1 << STM_BUF_SIZE_P);
	if (!stm) {
		close(fd);
		errno = ENOMEM;
	}
	return stm;
}


static void stream_lock(stream_t *stm) {
	lock.lock(&stm->lock);
}


static void stream_unlock(stream_t *stm) {
	lock.unlock(&stm->lock);
}


static int stream_errno(stream_t *stm) {
	return stm->last_err;
}


static int stream_fill(stream_t *stm, uint32_t *nread) {
	char tmp[STM_READALL_LEN];
	stream_lock(stm);
	uint32_t space = buffer.space(stm->buf);
	uint8_t *rbuf = buffer.wpos(stm->buf);

	struct iovec vec[2] = {
		{.iov_base = rbuf,	.iov_len = space},
		{.iov_base = tmp, .iov_len = STM_READALL_LEN}
	};
	uint32_t cnt;
	if (stm->funcs->readv(stm, vec, 2, &cnt)) {
		stream_unlock(stm);
		errno = stream_errno(stm);
		return -1;
	}
	if (nread) {
		*nread = cnt;
	}

	buffer.write(stm->buf, 0, cnt);
	if (cnt > space) {
		buffer.write(stm->buf, tmp, cnt - space);
	}
	stream_unlock(stm);

	return 0;
}


static int stream_read(stream_t *stm, void *buf, uint32_t len, uint32_t *nread) {
	uint8_t *dest = (uint8_t *)buf;
	uint32_t ret = 0, nr;

	if (len == 0) {
		*nread = 0;
		return 0;
	}
	stream_lock(stm);
	stm->last_err = 0;
	nr = buffer.read(stm->buf, dest, len);
	dest += nr;
	len -= nr;
	ret += nr;
	if (len == 0) {
		if (nread) {
			*nread = ret;
		}
		stream_unlock(stm);
		return 0;
	}
	if (!buf) {
		return -1;
	}
	while (len > 0) {
		uint32_t cnt;
		uint32_t wavail = buffer.space(stm->buf);
		uint8_t *wmem = buffer.wpos(stm->buf);
		struct iovec iov[2] = {
			{ .iov_base = dest, .iov_len = len - !!wavail },
			{ .iov_base = wmem, .iov_len = wavail }
		};
		if (!stm->funcs->readv(stm, iov, 2, &cnt) || cnt == 0) {
			if (ret) {
				break;
			}
			errno = stream_errno(stm);
			stream_unlock(stm);
			return -1;
		}
		if (cnt > iov[0].iov_len) {
			cnt -= iov[0].iov_len;
			dest[len-1] = *wmem;
			cnt = len;
		}
		ret += cnt;
		dest += cnt;
		len -= cnt;
	}
	if (nread) {
		*nread = ret;
	}
	stream_unlock(stm);

	return 0;
}


static int _stream_write(stream_t *stm, const char *buf, uint32_t len, uint32_t *nwrote) {
	uint32_t towrite, ret;
	struct iovec iovs[2];
	struct iovec *iov = iovs;
	int iovcnt = 2;
	if (len && len < buffer.space(stm->buf)) {
		buffer.write(stm->buf, buf, len);
		if (nwrote) {
			*nwrote = len;
		}
		return 0;
	}
	uint32_t ravail = buffer.avail(stm->buf);
	iov[0].iov_base = buffer.rpos(stm->buf);
	iov[0].iov_len = ravail;
	iov[1].iov_base = (char *)buf;
	iov[1].iov_len = len;
	towrite = iov[0].iov_len + iov[1].iov_len;
	for (;;) {
		if (stm->funcs->writev(stm, iov, iovcnt, &ret)) {
			errno = stream_errno(stm);
			return -1;
		}
		if (ret == towrite) {
			buffer.read(stm->buf, 0, ravail);
			if (nwrote) {
				*nwrote = len;
			}
			return 0;
		}
		towrite -= ret;
		if (ret > iov[0].iov_len) {
			buffer.read(stm->buf, 0, ravail);
			ret -= iov[0].iov_len;
			iovcnt--;
			iov++;
		} else if (iovcnt == 2) {
			buffer.read(stm->buf, 0, ret);
		}
		iov[0].iov_base = (char *)iov[0].iov_base + ret;
		iov[0].iov_len -= ret;
	}
}


static int stream_write(stream_t *stm, const char *buf, uint32_t len, uint32_t *nwrote) {
	int ret;

	if (len == 0) {
		if (nwrote) {
			*nwrote = 0;
		}
		return 0;
	}
	stream_lock(stm);
	ret = _stream_write(stm, buf, len, nwrote);
	stream_unlock(stm);

	return ret;
}


static int stream_flush(stream_t *stm) {
	stream_lock(stm);
	if (buffer.avail(stm->buf)) {
		_stream_write(stm, 0, 0, 0);
		if (buffer.avail(stm->buf)) {
			errno = stream_errno(stm);
			stream_unlock(stm);
			return -1;
		}
	}
	stream_unlock(stm);
	return 0;
}


static int stream_seek(stream_t *stm, int32_t delta, int whence, uint32_t *newpos) {
	if (!stream_flush(stm)) {
		return -1;
	}
	stream_lock(stm);
	stm->last_err = 0;
	if (whence == SEEK_CUR) {
		delta -= buffer.avail(stm->buf);
	}
	if (!stm->funcs->seek(stm, delta, whence, newpos)) {
		errno = stream_errno(stm);
		stream_unlock(stm);
		return -1;
	}
	stream_unlock(stm);
	return 0;
}


static uint32_t stream_vprintf(stream_t *stm, const char *fmt, va_list ap) {
	uint32_t ret;
	va_list cpy;

	va_copy(cpy, ap);
	ret = buffer.vprintf(stm->buf, fmt, ap);
	va_end(ap);

	return ret;
}


static uint32_t stream_printf(stream_t *stm, const char *fmt, ...) {
	uint32_t ret;
	va_list ap;

	va_start(ap, fmt);
	ret = stream_vprintf(stm, fmt, ap);
	va_end(ap);

	return ret;
}


static void *stream_yield(stream_t *stm, const char *delim, uint32_t delim_len, uint32_t *len) {
	return buffer.yield(stm->buf, delim, delim_len, len);
}


static void stream_set_mask(stream_t *stm, int mask) {
	stm->need_mask = mask;
}


static int stream_get_mask(stream_t *stm) {
	return stm->need_mask;
}


static buffer_t *stream_buffer(stream_t *stm) {
	return stm->buf;
}


struct stream_ stream = {
	stream_new,
	stream_free,
	stream_close,
	stream_fd_open,
	stream_file_open,
	stream_lock,
	stream_unlock,
	stream_errno,
	stream_read,
	stream_write,
	stream_fill,
	stream_flush,
	stream_seek,
	stream_yield,
	stream_set_mask,
	stream_get_mask,
	stream_buffer,
	stream_vprintf,
	stream_printf
};
