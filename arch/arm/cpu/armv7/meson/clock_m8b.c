#include <config.h>
#include <asm/arch/iobus.h>
#include <asm/arch/clock.h>

u32 get_rate_xtal(void)
{
	unsigned long clk;

	clk = (readl(P_PREG_CTLREG0_ADDR) >> 4) & 0x3f;
	clk = clk * 1000 * 1000;
	return clk;
}

u32 get_cpu_clk(void)
{
	return (clk_util_clk_msr(SYS_PLL_CLK) * 1000000);
}

u32 get_clk81(void)
{
	return (clk_util_clk_msr(CLK81) * 1000000);
}

u32 get_clk_ddr(void)
{
	return (clk_util_clk_msr(DDR_PLL_CLK) * 1000000);
}

u32 get_clk_ethernet_pad(void)
{
	return (clk_util_clk_msr(MOD_ETH_CLK50_I) * 1000000);
}

u32 get_clk_cts_eth_rmii(void)
{
	return (clk_util_clk_msr(CTS_ETH_RMII) * 1000000);
}

u32 get_misc_pll_clk(void)
{
	return (clk_util_clk_msr(MISC_PLL_CLK) * 1000000);
}

unsigned long clk_util_clk_msr(unsigned long clk_mux)
{
	u32 msr;

	writel(0,P_MSR_CLK_REG0);
	clrsetbits_le32(P_MSR_CLK_REG0, 0xffff, 64 - 1);
	clrbits_le32(P_MSR_CLK_REG0, ((1 << 18) | (1 << 17)));
	clrsetbits_le32(P_MSR_CLK_REG0, (0xf << 20), (clk_mux << 20) | (1 << 19) | (1 << 16));

	readl(P_MSR_CLK_REG0);
	while (readl(P_MSR_CLK_REG0) & (1 << 31))
		;
	clrbits_le32(P_MSR_CLK_REG0, (1 << 16));
	msr = (readl(P_MSR_CLK_REG2) + 31) & 0x000FFFFF;
	return (msr >> 6);
}

struct __clk_rate {
	unsigned clksrc;
	u32 (*get_rate)(void);
};

struct __clk_rate clkrate[] = {
    {
        .clksrc = SYS_PLL_CLK,
        .get_rate = get_cpu_clk,
    },
    {
        .clksrc = CLK81,
        .get_rate = get_clk81,
    },
    {
        .clksrc = DDR_PLL_CLK,
        .get_rate = get_clk_ddr,
    },
    {
        .clksrc = MOD_ETH_CLK50_I,
        .get_rate = get_clk_ethernet_pad,
    },
    {
        .clksrc = CTS_ETH_RMII,
        .get_rate = get_clk_cts_eth_rmii,
    },
    {
        .clksrc = MISC_PLL_CLK,
        .get_rate = get_misc_pll_clk,
    }
};

int clk_get_rate(unsigned clksrc)
{
	int i;

	for (i = 0; i < sizeof(clkrate)/sizeof(clkrate[0]); i++) {
	if(clksrc == clkrate[i].clksrc)
		return clkrate[i].get_rate();
	}

	return -1;
}
