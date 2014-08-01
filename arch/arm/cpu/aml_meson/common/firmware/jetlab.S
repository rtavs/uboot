

@add empty raise() to fix (when use new EABI xscale cross compiler to) compile
@error gcc/config/arm/lib1funcs.asm:1266: undefined reference to `raise'
.globl raise
raise:
    nop
    mov pc, lr
