#define  CONFIG_AMLROM_SPL 1
#include <timer.c>
#include <timming.c>

#include <serial.c>

#include <pinmux.c>
#include <sdpinmux.c>
#include <memtest.c>
#include <pll.c>
#ifdef CONFIG_POWER_SPL
#include <hardi2c_lite.c>
#include <power.c>
#endif
#include <ddr.c>
#include <mtddevices.c>
#include <sdio.c>



#include <loaduboot.c>

#include <asm/arch/reboot.h>

#ifdef CONFIG_POWER_SPL
#include <amlogic/aml_pmu_common.h>
#endif


static void spl_hello(void)
{
    serial_puts("\n\nStarting spl ...");
    serial_puts(__TIME__);
    serial_puts(" ");
    serial_puts(__DATE__);
    serial_puts("\n\n");
}



static void jtag_init(void)
{
#if defined(CONFIG_M8)
	//A9 JTAG enable
	writel(0x102,0xda004004);
	//TDO enable
	writel(readl(0xc8100014)|0x4000,0xc8100014);

	//detect sdio debug board
	unsigned pinmux_2 = readl(P_PERIPHS_PIN_MUX_2);

	// clear sdio pinmux
	setbits_le32(P_PREG_PAD_GPIO0_O,0x3f<<22);
	setbits_le32(P_PREG_PAD_GPIO0_EN_N,0x3f<<22);
	clrbits_le32(P_PERIPHS_PIN_MUX_2,7<<12);  //clear sd d1~d3 pinmux

	if(!(readl(P_PREG_PAD_GPIO0_I)&(1<<26))){  //sd_d3 low, debug board in
		serial_puts("\nsdio debug board detected ");
		clrbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);   //clear AO uart pinmux
		setbits_le32(P_PERIPHS_PIN_MUX_8,3<<9);

		if((readl(P_PREG_PAD_GPIO0_I)&(1<<22)))
			writel(0x220,P_AO_SECURE_REG1);  //enable sdio jtag
	}
	else{
		serial_puts("\nno sdio debug board detected ");
		writel(pinmux_2,P_PERIPHS_PIN_MUX_2);
	}
#endif
}

unsigned main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
#if defined(CONFIG_M8)
	//enable watchdog for 5s
	//if bootup failed, switch to next boot device
	AML_WATCH_DOG_SET(5000); //5s
	writel(readl(0xc8100000), SKIP_BOOT_REG_BACK_ADDR); //[By Sam.Wu]backup the skip_boot flag to sram for v2_burning
#endif
	//setbits_le32(0xda004000,(1<<0));	//TEST_N enable: This bit should be set to 1 as soon as possible during the Boot process to prevent board changes from placing the chip into a production test mode

	writel((readl(0xDA000004)|0x08000000), 0xDA000004);	//set efuse PD=1

    jtag_init();
    spl_hello();

#ifdef CONFIG_POWER_SPL
    power_init(POWER_INIT_MODE_NORMAL);
#endif

    // initial pll
    pll_init(&__plls);
	serial_init(__plls.uart);

	//TEMP add
	unsigned int nPLL = readl(P_HHI_SYS_PLL_CNTL);
	unsigned int nA9CLK = ((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
	serial_puts("\nCPU clock is ");
	serial_put_dec(nA9CLK);
	serial_puts("MHz\n\n");

    unsigned int nTEBegin = TIMERE_GET();

    // initial ddr
    ddr_init_test();

    serial_puts("\nDDR init use : ");
    serial_put_dec(get_utimer(nTEBegin));
    serial_puts(" us\n");

	//asm volatile ("wfi");

    // load uboot
	if(load_uboot(__TEXT_BASE,__TEXT_SIZE)){
		serial_puts("\nload uboot error,now reset the chip");
		AML_WATCH_DOG_START();
	}

    serial_puts("\nTE : ");
	serial_put_dec(TIMERE_GET());
	serial_puts("\n");

	//asm volatile ("wfi");

    serial_puts("\nStarting uboot ...\n");

#if defined(CONFIG_M8)
	//if bootup failed, switch to next boot device
	AML_WATCH_DOG_DISABLE(); //disable watchdog
	//temp added
	writel(0,0xc8100000);
#endif

    typedef  void (*uboot_entry)(void);
    uboot_entry uboot = (uboot_entry)CONFIG_SYS_TEXT_BASE;
    uboot();

    return 0;
}
