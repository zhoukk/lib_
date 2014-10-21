#include "lock.h"

static void mtx_lock(lock_t *v) {
	while (__sync_lock_test_and_set(v, 1)) {}
}

static void mtx_unlock(lock_t *v) {
	__sync_lock_release(v);
}

static void rwlock_init(rwlock_t *v) {
	v->read = 0;
	v->write = 0;
}

static void rlock(rwlock_t *v) {
	for (;;) {
		while (v->write) {
			__sync_synchronize();
		}
		__sync_add_and_fetch(&v->read, 1);
		if (v->write) {
			__sync_sub_and_fetch(&v->read, 1);
		} else {
			break;
		}
	}
}

static void runlock(rwlock_t *v) {
	__sync_sub_and_fetch(&v->read, 1);
}

static void wlock(rwlock_t *v) {
	while (__sync_lock_test_and_set(&v->write, 1)) {}
	while (v->read) {
		__sync_synchronize();
	}
}

static void wunlock(rwlock_t *v) {
	__sync_lock_release(&v->write);
}

struct lock_ lock = {
	mtx_lock,
	mtx_unlock
};

struct rwlock_ rwlock = {
	rwlock_init,
	rlock,
	runlock,
	wlock,
	wunlock
};
