#ifndef __KHLIST_H__
#define __KHLIST_H__

struct khlist;
struct khlist *khlist_create(unsigned sz);
void khlist_free(struct khlist *hl);
unsigned khlist_get(struct khlist *hl, const char *str, void **pud);
unsigned khlist_set(struct khlist *hl, const char *str, void *ud);
unsigned khlist_del(struct khlist *hl, const char *str, void **pud);
unsigned khlist_size(struct khlist *hl);
void *khlist_ud(struct khlist *hl, unsigned pos);

#endif //__KHLIST_H__