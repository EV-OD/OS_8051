#include "slave1.h"

void main(void)
{
    s1_ent();

    for (;;)
    {
        s1_srv();
    }
}
