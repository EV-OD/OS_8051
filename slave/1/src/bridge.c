#include "slave1.h"

volatile __data unsigned char slave1_counter = 0u;

void s1_srv(void)
{
    ++slave1_counter;
    P2 = slave1_counter;
}
