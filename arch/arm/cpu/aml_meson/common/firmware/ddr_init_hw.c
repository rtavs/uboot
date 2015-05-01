
void init_dmc(struct ddr_set * timing_set)
{


	//~ while((readl(P_MMC_RST_STS)&0x1FFFF000) != 0x0);
	//~ while((readl(P_MMC_RST_STS1)&0xFFE) != 0x0);
	//deseart all reset.
	//~ writel(0x1FFFF000, P_MMC_SOFT_RST);
	//~ writel(0xFFE, P_MMC_SOFT_RST1);
	//delay_us(100); //No delay need.
	//~ while((readl(P_MMC_RST_STS) & 0x1FFFF000) != (0x1FFFF000));
	//~ while((readl(P_MMC_RST_STS1) & 0xFFE) != (0xFFE));
	//delay_us(100); //No delay need.

	writel(timing_set->t_mmc_ddr_ctrl, P_MMC_DDR_CTRL);

	writel(0x00000000, DMC_SEC_RANGE0_ST);
	writel(0xffffffff, DMC_SEC_RANGE0_END);
	writel(0xffff, DMC_SEC_PORT0_RANGE0);
	writel(0xffff, DMC_SEC_PORT1_RANGE0);
	writel(0xffff, DMC_SEC_PORT2_RANGE0);
	writel(0xffff, DMC_SEC_PORT3_RANGE0);
	writel(0xffff, DMC_SEC_PORT4_RANGE0);
	writel(0xffff, DMC_SEC_PORT5_RANGE0);
	writel(0xffff, DMC_SEC_PORT6_RANGE0);
	writel(0xffff, DMC_SEC_PORT7_RANGE0);
	writel(0xffff, DMC_SEC_PORT8_RANGE0);
	writel(0xffff, DMC_SEC_PORT9_RANGE0);
	writel(0xffff, DMC_SEC_PORT10_RANGE0);
	writel(0xffff, DMC_SEC_PORT11_RANGE0);
	writel(0x80000000, DMC_SEC_CTRL);
	while( readl(DMC_SEC_CTRL) & 0x80000000 ) {}

	//for MMC low power mode. to disable PUBL and PCTL clocks
	//writel(readl(P_DDR0_CLK_CTRL) & (~1), P_DDR0_CLK_CTRL);
	//writel(readl(P_DDR1_CLK_CTRL) & (~1), P_DDR1_CLK_CTRL);

	//enable all channel
	writel(0xfff, P_MMC_REQ_CTRL);

	//for performance enhance
	//MMC will take over DDR refresh control
	writel(timing_set->t_mmc_ddr_timming0, P_MMC_DDR_TIMING0);
	writel(timing_set->t_mmc_ddr_timming1, P_MMC_DDR_TIMING1);
	writel(timing_set->t_mmc_ddr_timming2, P_MMC_DDR_TIMING2);
	writel(timing_set->t_mmc_arefr_ctrl, P_MMC_AREFR_CTRL);

	//Fix retina mid graphic flicker issue
	writel(0, P_MMC_PARB_CTRL);
}

int ddr_init_hw(struct ddr_set * timing_set)
{
    int ret = 0;
#ifdef DDR_SCRAMBE_ENABLE
	unsigned int dmc_sec_ctrl_value;
	unsigned int ddr_key;
#endif

    ret = timing_set->init_pctl(timing_set);

    if(ret)
    {
	serial_puts("\nPUB init fail! Reset...\n");
	__udelay(10000);
	AML_WATCH_DOG_START();
    }

    //asm volatile("wfi");

    init_dmc(timing_set);

#ifdef DDR_SCRAMBE_ENABLE
	ddr_key = readl(P_RAND64_ADDR0);
	writel(ddr_key &0x0000ffff, DMC_SEC_KEY0);
	writel((ddr_key >>16)&0x0000ffff, DMC_SEC_KEY1);

	dmc_sec_ctrl_value = 0x80000000 | (1<<0);
	writel(dmc_sec_ctrl_value, DMC_SEC_CTRL);
	while( readl(DMC_SEC_CTRL) & 0x80000000 ) {}

#ifdef CONFIG_DUMP_DDR_INFO
	serial_put_dword(ddr_key);
	dmc_sec_ctrl_value = readl(DMC_SEC_CTRL);
	if(dmc_sec_ctrl_value & (1<<0)){
		serial_puts("ddr scramb EN\n");
	}
#endif
#endif

#ifdef CONFIG_DUMP_DDR_INFO
	int nPLL = readl(AM_DDR_PLL_CNTL);
	int nDDRCLK = 2*((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
	serial_puts("\nDDR clock is ");
	serial_put_dec(nDDRCLK);
	serial_puts("MHz with ");
#ifdef CONFIG_DDR_LOW_POWER
	serial_puts("Low Power & ");
#endif
	if((timing_set->t_pctl_mcfg) & (1<<3)) //DDR0, DDR1 same setting?
		serial_puts("2T mode\n");
	else
		serial_puts("1T mode\n");
#endif

    return 0;
}
