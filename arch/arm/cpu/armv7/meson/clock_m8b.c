#include <config.h>
#include <common.h>
#include <asm/arch/iobus.h>
#include <asm/arch/clock.h>
#include <asm/io.h>


//#define BIT(x)                        (1 << (x))

#define MOD_ETH_CLK50_I                (22)
#define CTS_ETH_RMII                   (11)
#define CLK81                          (7)

#define PLL_SYS_CLK                    (2)
#define PLL_DDR_CLK                    (3)
#define PLL_MISC_CLK                   (4)


// Clock Measurement Module Register Base
#define CCM_BASE_ADDR     0xc1108758

struct meson_cmm_reg {
    u32 clk_duty;   /* offset 0x00 */
    u32 clk_reg0;   /* offset 0x04 */
    u32 clk_reg1;   /* offset 0x08 */
    u32 clk_reg2;   /* offset 0x0c */
};

//MSR_CLK_REG0 desc
#define MSR_CLK_BUSY        BIT(31)     // R, default 0, 1->measuring
#define MSR_CLK_MUX_SEL(x)  ((x) << 20) // BIT(20) ~ BIT(25)
#define MSR_CLK_EN          BIT(19)     // RW default 0. 1->enable clcok
#define MSR_CLK_IRQ_EN      BIT(18)     // IRQ_EN: 1->enable interrupt
#define MSR_CLK_CONT_EN     BIT(17)     // 1->enable continuous measure
#define MSR_CLK_MSR_EN      BIT(16)     // 1->enable measure


static struct meson_cmm_reg *cmm = (struct meson_cmm_reg *)CCM_BASE_ADDR;


#if 0

                               // [63:50]
    am_ring_osc_out_mali[1],    // [49]
    am_ring_osc_out_mali[0],    // [48]
    am_ring_osc_out_a9[1],      // [47]
    am_ring_osc_out_a9[0],      // [46]
    cts_pwm_A_clk,              // [45]
    cts_pwm_B_clk,              // [44]
    cts_pwm_C_clk,              // [43]
    cts_pwm_D_clk,              // [42]
    cts_eth_tx_clk,             // [41]
    cts_pcm_mclk,               // [40]
    cts_pcm_sclk,               // [39]
    cts_vdin_meas_clk,          // [38]
    cts_vdac_clk[1],            // [37]
    cts_hdmi_tx_pixel_clk,      // [36]
    cts_mali_clk,               // [35]
    cts_sdhc_clk1,              // [34]
    cts_sdhc_clk0,              // [33]
    cts_vdec_clk,               // [32]
    cts_audac_clkpi,            // [31]
    cts_slow_ddr_clk,           // [30]
    cts_vdac_clk[0],            // [29]
    cts_sar_adc_clk,            // [28]
    cts_enci_clk,               // [27]
    sc_clk_int,                 // [26]
    sys_pll_div3,               // [25]
    lvds_fifo_clk,              // [24]
    HDMI_CH0_TMDSCLK,           // [23]
    clk_rmii_from_pad,          // [22]
    mod_audin_aoclk_i,          // [21]
    rtc_osc_clk_out,            // [20]
    cts_hdmi_sys_clk,           // [19]
    cts_led_pll_clk,            // [18]
    cts_vghl_pll_clk,           // [17]
    cts_FEC_CLK_2,              // [16]
    cts_FEC_CLK_1,              // [15]
    cts_FEC_CLK_0,              // [14]
    cts_amclk,                  // [13]
    vid2_pll_clk,               // [12]
    cts_eth_rmii,               // [11]
    cts_enct_clk,               // [10]
    cts_encl_clk,               // [9]
    cts_encp_clk,               // [8]
    clk81,                      // [7]
    vid_pll_clk,                // [6]
    usb1_clk_12mhz,             // [5]
    usb0_clk_12mhz,             // [4]
    ddr_pll_clk,                // [3]
    xxx,                       // [2]
    am_ring_osc_clk_out_ee[1],     // [1]
    am_ring_osc_clk_out_ee[0]} ),  // [0]

#endif

//default gata_time 60us

#define DEF_GATA_TIME 60


/**********************************************************************
 * The "gate_time(uS)" can be anything between 1uS and 65535 uS, but the
 * limitation is the circuit will only count 65536 clocks.  Therefore
 * the gate_time is limited by
 *
 *   gate_time <= 65535/(expect clock frequency in MHz)
 *
 * For example, if the expected frequency is 400Mhz, then the gate_time
 * should be less than 163.
 *
 * measurement resolution is:
 *
 *  100% / (gate_time * measure_val )
 ********************************************************************/

static ulong meson_get_clk_measured(ulong clk_mux, ulong gate_time)
{
    writel(0, &cmm->clk_reg0);

    u32 val = readl(&cmm->clk_reg0);

    /* clear */
    val &= ~0xFFFF;
    /* new gate imte */
    val |= (gate_time -1);
    // clear interrupt and concontinuous measure and clk_mux
    val &= ~(MSR_CLK_IRQ_EN | MSR_CLK_CONT_EN | (0x3f << 20));

    /** enable clk and clk measure */
    val |= (MSR_CLK_MUX_SEL(clk_mux) | MSR_CLK_EN | MSR_CLK_MSR_EN);
    writel(val, &cmm->clk_reg0);

    /** dummy read */
    readl(&cmm->clk_reg0);
    /** wait clk stable */
    while(readl(&cmm->clk_reg0) & MSR_CLK_BUSY) {
        /** busy measure */
    };

    /** disbale clk measure */
    writel(readl(&cmm->clk_reg0) & ~MSR_CLK_MSR_EN, &cmm->clk_reg0);

    // get final measure_val
    ulong msr = (readl(&cmm->clk_reg2) + 31 ) & 0x000FFFFF; //20 bits
    //return Mhz, 64us: >>6 32us >>5;
    return (msr >> 6);
}

ulong meson_get_clk_rate(ulong clk_mux)
{
    return meson_get_clk_measured(clk_mux, DEF_GATA_TIME);
}

ulong meson_get_xtal_rate(void)
{
//    ulong clk = readl();
    return 24 *1000 * 1000;
}


/**
 * clock api:
 * get_arm_clk
 **/

unsigned long m8b_get_arm_clk(void)
{
    return meson_get_clk_rate(PLL_SYS_CLK);
}

unsigned long get_arm_clk(void)
{
    return m8b_get_arm_clk();
}
