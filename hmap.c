#include "khlist.h"

#include <stdlib.h>
#include <ctype.h>

static unsigned _crypt_table[0x500];
static char _crypt_init = 0;
static const unsigned HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;

struct hash_table{
	unsigned hasha;
	unsigned hashb;
	char exist;

	void *ud;
};

struct khlist{
	unsigned size;
	struct hash_table *hash_t;
};

static void _init_crypt_table(void){
	unsigned seed = 0x00100001, idx1 = 0, idx2 = 0, i;
	for (idx1 = 0; idx1 < 0x100; idx1++){
		for (idx2 = idx1,i = 0; i < 5; i++,idx2 += 0x100){
			unsigned tmp1, tmp2;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			tmp1 = (seed & 0xFFFF) << 0x10;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			tmp2 = (seed & 0xFFFF);
			_crypt_table[idx2] = (tmp1 | tmp2);
		}
	}
}

static unsigned _hash_string(const char *str, unsigned type){
	unsigned char *key = (unsigned char *)str;
	unsigned seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
	int ch;

	while (*key != 0){
		ch = toupper(*key++);
		seed1 = _crypt_table[(type << 8) + ch] ^ (seed1 + seed2);
		seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
	}
	return seed1;
}

struct khlist *khlist_create(unsigned sz){
	struct khlist *hl = malloc(sizeof *hl);
	if (hl == NULL)	return NULL;
	hl->size = sz;
	hl->hash_t = malloc(sz * sizeof(struct hash_table));
	int i;
	for (i = 0; i < sz; i++){
		hl->hash_t[i].hasha = -1;
		hl->hash_t[i].hashb = -1;
		hl->hash_t[i].exist = 0;
		hl->hash_t[i].ud = NULL;
	}
	if (_crypt_init == 0){
		_init_crypt_table();
		_crypt_init = 1;
	}
	return hl;
}

void khlist_free(struct khlist *hl){
	free(hl->hash_t);
	free(hl);
}

unsigned khlist_get(struct khlist *hl, const char *str, void **pud){
	unsigned hash = _hash_string(str, HASH_OFFSET);
	unsigned hasha = _hash_string(str, HASH_A);
	unsigned hashb = _hash_string(str, HASH_B);
	unsigned hash_start = hash % hl->size;

	unsigned npos = hash_start;

	while (hl->hash_t[npos].exist == 1){
		if (hl->hash_t[npos].hasha == hasha && hl->hash_t[npos].hashb == hashb){
			if (pud) *pud = hl->hash_t[npos].ud;
			return npos;
		}else
			npos = (npos + 1) % hl->size;
		if (npos == hash_start)
			break;
	}
	return -1;
}

static struct khlist *_expand(struct khlist *hl){
	struct hash_table *nht = malloc(hl->size * 2 * sizeof *nht);
	if (nht == NULL) return NULL;
	int i;
	for (i = 0; i < hl->size; i++){
		nht[i] = hl->hash_t[i];
	}
	free(hl->hash_t);
	hl->hash_t = nht;
	hl->size *= 2;
	return hl;
}

unsigned khlist_set(struct khlist *hl, const char *str, void *ud){
	unsigned hash = _hash_string(str, HASH_OFFSET);
	unsigned hasha = _hash_string(str, HASH_A);
	unsigned hashb = _hash_string(str, HASH_B);
	unsigned hash_start = hash % hl->size;

	unsigned npos = hash_start;

	while (hl->hash_t[npos].exist == 1){
		if (hl->hash_t[npos].hasha == hasha && hl->hash_t[npos].hashb == hashb)
			return -1;
		npos = (npos + 1) % hl->size;
		if (npos == hash_start){
			if (_expand(hl) == NULL) return -1;
			return khlist_set(hl, str, ud);
		}
	}
	hl->hash_t[npos].exist = 1;
	hl->hash_t[npos].hasha = hasha;
	hl->hash_t[npos].hashb = hashb;
	hl->hash_t[npos].ud = ud;
	return npos;
}

unsigned khlist_del(struct khlist *hl, const char *str, void **pud){
	unsigned hash = _hash_string(str, HASH_OFFSET);
	unsigned hasha = _hash_string(str, HASH_A);
	unsigned hashb = _hash_string(str, HASH_B);
	unsigned hash_start = hash % hl->size;

	unsigned npos = hash_start;

	while (hl->hash_t[npos].exist == 1){
		if (hl->hash_t[npos].hasha == hasha && hl->hash_t[npos].hashb == hashb)
			break;
		else
			npos = (npos + 1) % hl->size;
		if (npos == hash_start)
			return -1;
	}
	hl->hash_t[npos].exist = 0;
	hl->hash_t[npos].hasha = -1;
	hl->hash_t[npos].hashb = -1;
	*pud = hl->hash_t[npos].ud;
	hl->hash_t[npos].ud = NULL;
	return npos;
}

unsigned khlist_size(struct khlist *hl){
	return hl->size;
}

void *khlist_ud(struct khlist *hl, unsigned pos){
	return hl->hash_t[pos].ud;
}
