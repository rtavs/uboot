#define  CONFIG_AMLROM_SPL 1
#include <timer.c>
#include <timming.c>
#include <uartpin.c>
#include <serial.c>

#include <pinmux.c>
#include <sdpinmux.c>

#include <memtest.c>
#include <pll.c>
#include <ddr.c>

#include <mtddevices.c>
#include <sdio.c>
#include <debug_rom.c>


#include <usb_boot/usb_boot.c>
#include <asm/arch/reboot.h>

#ifdef CONFIG_ACS
#include <storage.c>
#include <acs.c>
#endif

unsigned main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
    //Adjust 1us timer base
    timer_init();
    //default uart clock.
    serial_init(__plls.uart);
    serial_put_dword(get_utimer(0));
    writel(0,P_WATCHDOG_TC);//disable Watchdog
    // initial pll
    pll_init(&__plls);

    __udelay(100000);//wait for a uart input
	 if(serial_tstc()){
	    debug_rom(__FILE__,__LINE__);
	 }

    // initial ddr
    ddr_init_test();
    // load uboot
    writel(0,P_WATCHDOG_TC);//disable Watchdog
    reboot_mode = MESON_USB_BURNER_REBOOT;
    serial_puts("\nUSB boot Start\n");
		while(1)
		{
			//usb_boot(0);	//Elvis
			usb_boot(1);
		}
    return 0;
}
