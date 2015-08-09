#ifndef _MESON_CLOCK_H
#define _MESON_CLOCK_H

#include <linux/types.h>

#if defined(CONFIG_MACH_M8B)
#include <asm/arch/clock_m8b.h>
#endif

int clk_get_rate(unsigned clksrc);
unsigned long clk_util_clk_msr(unsigned long clk_mux);
u32 get_cpu_clk(void);
u32 get_clk_ddr(void);
u32 get_clk81(void);
u32 get_misc_pll_clk(void);

#endif /* _MESON_CLOCK_H */
