.module slave1_entry
.globl _s1_ent
.globl _s1_srv

.area CODE (CODE)

_s1_ent:
    lcall _s1_srv
    ret
