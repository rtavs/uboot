/*
 * Amlogic Configurable SPL
 * entry point
 * @author xiaobo.gu
 */

#include <config.h>

#ifdef CONFIG_ACS
.globl _acs_start
_acs_start:
  .word __acs_set
#endif

@add empty raise() to fix (when use new EABI xscale cross compiler to) compile
@error gcc/config/arm/lib1funcs.asm:1266: undefined reference to `raise'
.globl raise
raise:
    nop
    mov pc, lr
