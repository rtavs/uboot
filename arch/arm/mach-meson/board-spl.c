#include <common.h>
#include <mmc.h>
#include <serial.h>
#include <spl.h>
#include <image.h>
#include <linux/compiler.h>
#include <watchdog.h>
#include <asm/io.h>


DECLARE_GLOBAL_DATA_PTR;


void spl_display_print(void)
{
//	unsigned int nTEBegin = TIMERE_GET();
	serial_puts("\n SPL TE : ");

}



u32 spl_boot_device(void)
{
	panic("Could not determine boot source\n");
	return -1;		/* Never reached */
}

/* No confirmation data available in SPL yet. Hardcode bootmode */
u32 spl_boot_mode(void)
{
	return MMCSD_MODE_RAW;
}


void board_init_f(ulong dummy)
{
	hw_watchdog_init();
	preloader_console_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

#ifdef CONFIG_SPL_I2C_SUPPORT
	/* Needed early by sunxi_board_init if PMU is enabled */
	i2c_init(CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
#endif

	board_init_r(NULL, 0);
}
