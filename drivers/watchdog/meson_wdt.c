#include <common.h>
#include <asm/io.h>
//#include <asm/arch/regaddr.h>


/* Watchdog Control REG */
#define REG_WDT_CTRL0      0xc1109900      // 0x2640
#define REG_WDT_RESET      0xc1109904      // 0x2641


/* Watchdog Control REG Bit */

#define WDT_CPU_RESET_VAL       0xf // m8 -> 0xf; m6 - > 0x3
#define WDT_CPU_RESET_OFFSET    24
#define WDT_CPU_RESET           (WDT_CPU_RESET_VAL << WDT_CPU_RESET_OFFSET)              //
#define WDT_IRQ                 BIT(23)     /* default 0; 1 -> instead reset the chip, the dog will generate an interrupt */


#define WDT_EN                  BIT(19)     /* BIT(19) for 805 default 0; 1 -> ebable the dog reset */
//#define WDT_EN                  BIT(22)     /*  BIT(22) for 812/802 default 0; 1 -> ebable the dog reset */

//#define WDT_TERMINAL_CNT        /* default 0x186a0, BIT(0)~BIT(21) */

//max watchdog timer: 41.943s
//#define WDT_TIME_SLICE				10	//10 us for 812
#define WDT_TIME_SLICE				128	// 128 us for 805




#define WATCHDOG_BASE_ADDR        REG_WDT_CTRL0

struct meson_wdt {
    u32 ctrl;       /* 0x00 WATCHDOG_CONTROL */
    u32 reset;      /* 0x01 REG_WDT_RESET */
};


static struct meson_wdt *wdt = (struct meson_wdt *)WATCHDOG_BASE_ADDR;

/**
 * wdt API:
 * void hw_watchdog_init(void); must
 * void hw_watchdog_reset(void); must
 * void hw_watchdog_enable(void); option
 **/


// set tms timeout
void meson_wdt_set_timeout(unsigned int tms)
{
    writel(0, &wdt->reset);
    u32 val = (int)(tms * 1000 / WDT_TIME_SLICE);
    val |=  WDT_EN | WDT_CPU_RESET;
    writel(val, &wdt->ctrl);
    return;
}



void hw_watchdog_eable(int en)
{
    /* write the enalbe bit only will reset other bits;
     * so read other bits before write the enable bit.
     */
    u32 val = readl(&wdt->ctrl);

    if (en) {
        val |= WDT_EN;
    } else {
        val &= ~WDT_EN;
    }
    writel(0, &wdt->reset);
    writel(val, &wdt->ctrl);
}

#if defined(CONFIG_HW_WATCHDOG)
void hw_watchdog_init(void)
{
    meson_wdt_set_timeout(CONFIG_HW_WATCHDOG_TIMEOUT_MS);
}

void hw_watchdog_reset(void)
{
    writel(0, &wdt->reset);
    u32 val = 10;
    val |=  WDT_EN | WDT_CPU_RESET;
    writel(val, &wdt->ctrl);
    while(1);
}
#endif
