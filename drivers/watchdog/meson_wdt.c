#include <common.h>
#include <watchdog.h>
#include <asm/io.h>
#include <asm/arch/regaddr.h>

#define MESON_WDT_TIME_SLICE		128
#define MESON_WDT_ENABLE_OFFSET		19
#define MESON_WDT_CPU_RESET_CNTL	0xf
#define MESON_WDT_CPU_RESET_OFFSET	24

int meson_wdt_settimeout(unsigned int timeout)
{
	unsigned int reg = 0;

	writel(0, P_WATCHDOG_RESET);

	reg = timeout * 1000 / MESON_WDT_TIME_SLICE;
	reg |= (1 << MESON_WDT_ENABLE_OFFSET);
	reg |= (MESON_WDT_CPU_RESET_CNTL << MESON_WDT_CPU_RESET_OFFSET);
	writel(reg, P_WATCHDOG_TC);

	return 0;
}

void meson_wdt_reset(void)
{
	unsigned int reg = 0;

	writel(0, P_WATCHDOG_RESET);
	reg = 10 | (1 << MESON_WDT_ENABLE_OFFSET);
	reg |= (MESON_WDT_CPU_RESET_CNTL << MESON_WDT_CPU_RESET_OFFSET);
	writel(reg, P_WATCHDOG_TC);

	while(1);
}

#if defined(CONFIG_HW_WATCHDOG)
void hw_watchdog_reset(void)
{
	meson_wdt_reset();
}

void hw_watchdog_init(void)
{
	/* set timer in ms */
	meson_wdt_settimeout(CONFIG_HW_WATCHDOG_TIMEOUT_MS);
}
#endif
