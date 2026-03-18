#include "firmware.h"

volatile __data unsigned char master_counter = 0u;

void m_srv(void)
{
    ++master_counter;
    // P2 = master_counter;
}
