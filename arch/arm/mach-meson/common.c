

#include <common.h>
#include <asm/io.h>


/* Watchdog Control REG */
#define REG_WDT_CTRL0      0xc1109900      // 0x2640
#define REG_WDT_RESET      0xc1109904      // 0x2641



/* Watchdog Control REG Bit */

#define WDT_CPU_RESET_VAL       0xf // m8 -> 0xf; m6 - > 0x3
#define WDT_CPU_RESET_OFFSET    24
#define WDT_CPU_RESET           (WDT_CPU_RESET_VAL << WDT_CPU_RESET_OFFSET)              //
#define WDT_IRQ                 BIT(23)     /* default 0; 1 -> instead reset the chip, the dog will generate an interrupt */
#define WDT_EN                  BIT(22)     /* default 0; 1 -> ebable the dog reset */

//#define WDT_TERMINAL_CNT        /* default 0x186a0, BIT(0)~BIT(21) */

//max watchdog timer: 41.943s
#define WDT_TIME_SLICE				10	//us




#define WATCHDOG_BASE_ADDR        REG_WDT_CTRL0

struct meson_wdt {
    u32 ctrl;       /* 0x00 WATCHDOG_CONTROL */
    u32 reset;      /* 0x01 REG_WDT_RESET */
};


static struct meson_wdt *wdt = (struct meson_wdt *)WATCHDOG_BASE_ADDR;




void reset_cpu(ulong addr)
{
    puts("System is going to reboot ...\n");

    writel(0, &wdt->reset);
    //0.1ms timeout
    writel(10 | WDT_EN | WDT_CPU_RESET, &wdt->ctrl);
    while(1);
}
