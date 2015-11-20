/*
 *
 * (C) Copyright (C) 2014 Kaspter Ju
 * Kaspter Ju <nigh0st3018@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <div64.h>
#include <asm/io.h>


DECLARE_GLOBAL_DATA_PTR;

/*
 * uboot timer api: lib/time.c
 * int timer_init(void); platform must
 * ulong get_timer(ulong base); platform must
 * void __udelay(unsigned long usec); platform must
 * unsigned long long get_ticks(void); platform must
 * unsigned long timer_read_counter(void); platform option
 * unsigned long timer_get_us(void); platform option
 *
 * meson: user TimerE for meson series CPU
 */


/*
 * CLKIN_SEL0: A/B/C/D/E 5 timers on CLKIN_SEL0, EE domain
 * CLKIN_SEL1: F/G/H/I 4 times on CLKIN_SEL1, EE domain
 **/

#define CONFIG_SYS_TIMERBASE    0xc1109940


/* Timer Clock Input Selection REG */
#define TIMER_CLKIN_SEL         0x00        //phy addr: 0xc1109940, cbus offset 0x2650
#define ISA_TIMERA              0x04
#define ISA_TIMERB              0x08
#define ISA_TIMERC              0x0C
#define ISA_TIMERD              0x10
#define ISA_TIMERE              0x14

/* Timer Clock Input Selection REG Bit*/

#define TIMERD_EN           BIT(19)     /* default 1; 1 -> enable,*/
#define TIMERC_EN           BIT(18)     /* ditto */
#define TIMERB_EN           BIT(17)     /* ditto */
#define TIMERA_EN           BIT(16)     /* ditto */
#define TIMERD_MODE         BIT(15)     /* default 0; 1 -> periodic, 0 -> one-shot timer */
#define TIMERC_MODE         BIT(14)     /* default 0; 1 -> periodic, 0 -> one-shot timer */
#define TIMERB_MODE         BIT(13)     /* default 1; 1 -> periodic, 0 -> one-shot timer */
#define TIMERA_MODE         BIT(12)     /* default 1; 1 -> periodic, 0 -> one-shot timer */

#define TIMERE_CLK_SEL_OFFSET   8

#define TIMERE_CLK_SEL_MASK     (7UL << TIMERE_CLK_SEL_OFFSET)
#define TIMERE_CLK_SEL_SYS      (0 << TIMERE_CLK_SEL_OFFSET)
#define TIMERE_CLK_SEL_1us      (1 << TIMERE_CLK_SEL_OFFSET)
#define TIMERE_CLK_SEL_10us     (2 << TIMERE_CLK_SEL_OFFSET)
#define TIMERE_CLK_SEL_100us    (3 << TIMERE_CLK_SEL_OFFSET)
#define TIMERE_CLK_SEL_1ms      (4 << TIMERE_CLK_SEL_OFFSET)


#define ISA_TIMER_MUX1          0x00        //phy addr: 0xc1109990
#define ISA_TIMERF              0x04
#define ISA_TIMERG              0x08
#define ISA_TIMERH              0x0C
#define ISA_TIMERI              0x10



/* TimerE: a 32bit counter that increments at a programmable rate */
#define TIMERE_INDEX        4



#define TIMER_LOAD_VAL		0
#define TIMER_OVERFLOW_VAL	0xffffffff


struct meson_timer {
    u32 clkin_sel;        /* 0x00 ISA_TIMER_MUX*/
    u32 timer[5];   /* 0x01~0x05 TIMER_A ~ TIMER_E or TIMER_F ~ TIMER_I */
};

static struct meson_timer *timer_base = (struct meson_timer *)CONFIG_SYS_TIMERBASE;



/**
 * read the timer current us count.
 */
unsigned long timer_read_counter(void)
{
    return readl(&timer_base->timer[TIMERE_INDEX]);
}


/**
 * init timer register
 */
int timer_init(void)
{
    /* init some clock here */

    /* Set 1us for timer E */
    u32 val = readl(&timer_base->clkin_sel);
    val &= ~TIMERE_CLK_SEL_MASK;
    val |= TIMERE_CLK_SEL_1us;
    writel(val, &timer_base->clkin_sel);

    /* Use this as the current monotonic time in us */
    gd->arch.timer_reset_value = 0;

    /* Use this as the last timer value we saw */
    gd->arch.lastinc = timer_read_counter();

    reset_timer_masked();

    return 0;
}


/* OK */
void reset_timer_masked(void)
{
    /* reset time */
    gd->arch.lastinc = readl(&timer_base->timer[TIMERE_INDEX]);
    gd->arch.tbl = 0;   /* start "advancing" time stamp from 0 */
}



/* timer without interrupts */
void reset_timer(void)
{
    reset_timer_masked();
}


/*
 * timer without interrupts
 */
unsigned long get_timer(unsigned long base)
{
    unsigned long long time_ms;

    ulong now = timer_read_counter();

    /*
     * Increment the time by the amount elapsed since the last read.
     * The timer may have wrapped around, but it makes no difference to
     * our arithmetic here.
     */
    gd->arch.timer_reset_value += now - gd->arch.lastinc;   //overflow ?
    gd->arch.lastinc = now;

    /* Divide by 1000 to convert from us to ms */
    time_ms = gd->arch.timer_reset_value;
    do_div(time_ms, 1000);
    return time_ms - base;
}

//??
void set_timer(ulong t)
{
//	gd->tbl = t;
}





/* OK: delay x useconds */
void __udelay(unsigned long usec)
{
	unsigned long start, now, timeout;

    timeout = usec;
    start = timer_read_counter();
    while (timeout > 0) {
        now = timer_read_counter();
        if (start > now) {  /* count up timer overflow */
            timeout -= TIMER_OVERFLOW_VAL - start + now;
        } else {    /* normal */
            timeout -= now - start;
        }
        start = now;
    }
}



unsigned long __attribute__((no_instrument_function)) timer_get_us(void)
{
	static unsigned long base_time_us;

	unsigned long now_up_us = readl(&timer_base->timer[TIMERE_INDEX]);

	if (!base_time_us)
		base_time_us = now_up_us;

	/* Note that this timer counts up. */
	return now_up_us - base_time_us;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
