#ifndef LOCK_H
#define LOCK_H

#ifdef __cplusplus
extern "C"{
#endif

#define SYNC_GET(val)		__sync_val_compare_and_swap(&val, 0, 0)
#define SYNC_SET(val, nval)	__sync_lock_test_and_set(&val, nval)

typedef int lock_t;

struct _rwlock {
	int read;
	int write;
};

typedef struct _rwlock rwlock_t;

extern struct lock_ {
	void (*lock)(lock_t *lock);
	void (*unlock)(lock_t *lock);
} lock;

extern struct rwlock_ {
	void (*init)(rwlock_t *lock);
	void (*rlock)(rwlock_t *lock);
	void (*runlock)(rwlock_t *lock);
	void (*wlock)(rwlock_t *lock);
	void (*wunlock)(rwlock_t *lock);
} rwlock;

#ifdef __cplusplus
}
#endif

#endif // LOCK_H
