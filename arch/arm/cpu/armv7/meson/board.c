#include <common.h>
#include <config.h>
#include <spl.h>
#include <image.h>
#include <linux/compiler.h>
#include <watchdog.h>
#include <arch/asm/timer.h>

#ifdef CONFIG_SPL_BUILD
DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPL_BUILD
void spl_display_print(void)
{
	unsigned int nTEBegin = TIMERE_GET();
	serial_puts("\nTE : ");

}
#endif

void board_init_f(ulong dummy)
{
	hw_watchdog_init();
	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	board_init_r(NULL, 0);
}
#endif
