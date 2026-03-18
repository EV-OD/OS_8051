#ifndef OS8051_FIRMWARE_H
#define OS8051_FIRMWARE_H

void m_entry(void);
void m_srv(void);

extern volatile __data unsigned char master_counter;

#endif
