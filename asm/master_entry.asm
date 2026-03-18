.module master_entry
.globl _m_entry
.globl _m_srv

.area CODE (CODE)

_m_entry:
    lcall _m_srv
    ret
