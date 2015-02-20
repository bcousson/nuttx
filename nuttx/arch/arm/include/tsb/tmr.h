#ifndef _TMR_H_
#define _TMR_H_

#include <sys/types.h>

void tsb_timer_init(void);
void tsb_timer_get(struct timespec *ts);
#endif

