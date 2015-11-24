#include <common.h>
#include <mmc.h>
#include <serial.h>
#include <spl.h>
#include <image.h>
#include <linux/compiler.h>
#include <watchdog.h>
#include <asm/io.h>


DECLARE_GLOBAL_DATA_PTR;



static void serial_putd(unsigned int data)
{
    int i = 0;
    char szTxt[10];
    szTxt[0] = 0x30;

    while(data) {
        szTxt[i++] = (data % 10) + 0x30;
        data = data / 10;
    }

    for(--i;i >=0;--i)
        serial_putc(szTxt[i]);
}


void spl_display_print(void)
{
    ulong begin = timer_get_us();
    serial_puts("\n\nStarting SPL ...");
    serial_putd(begin);
    serial_puts("\n\n");
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
