#include <reg51.h>

#ifndef OS8051_SLAVE1_H
#define OS8051_SLAVE1_H

void s1_ent(void);
void s1_srv(void);

extern volatile __data unsigned char slave1_counter;

#endif
