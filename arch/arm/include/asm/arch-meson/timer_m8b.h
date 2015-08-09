#ifndef _MESON_TIMER_M8B_H
#define _MESON_TIMER_M8B_H

#include <arch/asm/iobus.h>

#define TIMERE_SUB(timea,timeb)	((timea)-(timeb))
#define TIMERE_GET()		(READ_CBUS_REG(ISA_TIMERE))

#endif /* _MESON_TIMER_M8B_H */
