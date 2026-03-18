#include "firmware.h"

void main(void)
{
    m_entry();

    for (;;)
    {
        m_srv();
    }
}
