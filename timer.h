#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif


extern struct timer_ {
	int64_t (*now)(void);

} timer;

#ifdef __cplusplus
}
#endif

#endif // TIMER_H
