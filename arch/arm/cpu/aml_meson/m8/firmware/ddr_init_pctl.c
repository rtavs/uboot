

#if 0
  #define ddr_udelay __udelay
#else
  #define ddr_udelay
#endif

//The nonused DDR channel need open/close to shut down power completely.
//use following to enable this feature for power optimization
#define M8_DDR_CHN_OPEN_CLOSE 1

static void delay_us(unsigned long us)
{
	int nStart = Rd_cbus(ISA_TIMERE);
	//need to check overflow?
	while((Rd_cbus(ISA_TIMERE) - nStart)< us){};
}

#if 0
    #define hx_serial_puts serial_puts
	#define hx_serial_put_hex serial_put_hex
#else
    #define hx_serial_puts
	#define hx_serial_put_hex
#endif

int init_pctl_ddr3(struct ddr_set * timing_set)
{
	int nRet = -1;

#define  UPCTL_STAT_MASK		(7)
#define  UPCTL_STAT_INIT        (0)
#define  UPCTL_STAT_CONFIG      (1)
#define  UPCTL_STAT_ACCESS      (3)
#define  UPCTL_STAT_LOW_POWER   (5)

#define UPCTL_CMD_INIT         (0)
#define UPCTL_CMD_CONFIG       (1)
#define UPCTL_CMD_GO           (2)
#define UPCTL_CMD_SLEEP        (3)
#define UPCTL_CMD_WAKEUP       (4)

//PUB PIR setting
#define PUB_PIR_INIT  		(1<<0)
#define PUB_PIR_ZCAL  		(1<<1)
#define PUB_PIR_CA  		(1<<2)
#define PUB_PIR_PLLINIT 	(1<<4)
#define PUB_PIR_DCAL    	(1<<5)
#define PUB_PIR_PHYRST  	(1<<6)
#define PUB_PIR_DRAMRST 	(1<<7)
#define PUB_PIR_DRAMINIT 	(1<<8)
#define PUB_PIR_WL       	(1<<9)
#define PUB_PIR_QSGATE   	(1<<10)
#define PUB_PIR_WLADJ    	(1<<11)
#define PUB_PIR_RDDSKW   	(1<<12)
#define PUB_PIR_WRDSKW   	(1<<13)
#define PUB_PIR_RDEYE    	(1<<14)
#define PUB_PIR_WREYE    	(1<<15)
#define PUB_PIR_ICPC     	(1<<16)
#define PUB_PIR_PLLBYP   	(1<<17)
#define PUB_PIR_CTLDINIT 	(1<<18)
#define PUB_PIR_RDIMMINIT	(1<<19)
#define PUB_PIR_CLRSR    	(1<<27)
#define PUB_PIR_LOCKBYP  	(1<<28)
#define PUB_PIR_DCALBYP 	(1<<29)
#define PUB_PIR_ZCALBYP 	(1<<30)
#define PUB_PIR_INITBYP  	(1<<31)

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//HX DDR init code
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//PUB PGSR0
#define PUB_PGSR0_IDONE     (1<<0)
#define PUB_PGSR0_PLDONE    (1<<1)
#define PUB_PGSR0_DCDONE    (1<<2)
#define PUB_PGSR0_ZCDONE    (1<<3)
#define PUB_PGSR0_DIDONE    (1<<4)
#define PUB_PGSR0_WLDONE    (1<<5)
#define PUB_PGSR0_QSGDONE   (1<<6)
#define PUB_PGSR0_WLADONE   (1<<7)
#define PUB_PGSR0_RDDONE    (1<<8)
#define PUB_PGSR0_WDDONE    (1<<9)
#define PUB_PGSR0_REDONE    (1<<10)
#define PUB_PGSR0_WEDONE    (1<<11)
#define PUB_PGSR0_CADONE    (1<<12)

#define PUB_PGSR0_ZCERR     (1<<20)
#define PUB_PGSR0_WLERR     (1<<21)
#define PUB_PGSR0_QSGERR    (1<<22)
#define PUB_PGSR0_WLAERR    (1<<23)
#define PUB_PGSR0_RDERR     (1<<24)
#define PUB_PGSR0_WDERR     (1<<25)
#define PUB_PGSR0_REERR     (1<<26)
#define PUB_PGSR0_WEERR     (1<<27)
#define PUB_PGSR0_CAERR     (1<<28)
#define PUB_PGSR0_CAWERR    (1<<29)
#define PUB_PGSR0_VTDONE    (1<<30)
#define PUB_PGSR0_DTERR     (PUB_PGSR0_RDERR|PUB_PGSR0_WDERR|PUB_PGSR0_REERR|PUB_PGSR0_WEERR)

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//HX DDR init code
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define MMC_RESET_CNT_MAX       (8)

	int nTempVal = 0;
	int nRetryCnt = 0; //mmc reset try counter; max to try 8 times

mmc_init_all:

	hx_serial_puts("\nAml log : mmc_init_all\n");
	nRetryCnt++;

	if(nRetryCnt > MMC_RESET_CNT_MAX )
		return nRet;

	//MMC/DMC reset
	writel(0, P_MMC_SOFT_RST);
	writel(0, P_MMC_SOFT_RST1);
	writel(~0, P_MMC_SOFT_RST);
	writel(~0, P_MMC_SOFT_RST1);

	//M8 DDR0/1 channel select
	//#define CONFIG_M8_DDRX2_S12  (0)
	//#define CONFIG_M8_DDR1_ONLY  (1)
	//#define CONFIG_M8_DDRX2_S30  (2)
	//#define CONFIG_M8_DDR0_ONLY  (3)
	//#define CONFIG_M8_DDR_CHANNEL_SET (CONFIG_M8_DDRX2_S12)

	unsigned int nMMC_DDR_set = timing_set->t_mmc_ddr_ctrl;
	int nM8_DDR_CHN_SET = (nMMC_DDR_set >> 24 ) & 3;

/////////////////////////////////////////////////////
pub_init_ddr0:
pub_init_ddr1:

	//uboot timming.c setting
	//UPCTL and PUB clock and reset.
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0 is used
	{
		hx_serial_puts("\nAml log : DDR0 - init start...\n");
		hx_serial_puts("Aml log : DDR0 - APD,CLK done\n");
	}

	writel(0x0, P_DDR0_SOFT_RESET);
	writel(0xf, P_DDR0_SOFT_RESET);

#if defined(CONFIG_M8_DDR_LOW_POWER_DISABLE)
	writel((timing_set->t_ddr_apd_ctrl & (~0xFF)), P_DDR0_APD_CTRL);
#else
	writel(timing_set->t_ddr_apd_ctrl, P_DDR0_APD_CTRL);
#endif

	writel(timing_set->t_ddr_clk_ctrl, P_DDR0_CLK_CTRL);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1 is used
	{
		hx_serial_puts("Aml log : DDR1 - init start...\n");
		hx_serial_puts("Aml log : DDR1 - APD,CLK done\n");
	}

	writel(0x0, P_DDR1_SOFT_RESET);
	writel(0xf, P_DDR1_SOFT_RESET);

#if defined(CONFIG_M8_DDR_LOW_POWER_DISABLE)
	writel((timing_set->t_ddr_apd_ctrl & (~0xFF)), P_DDR1_APD_CTRL);
#else
	writel(timing_set->t_ddr_apd_ctrl, P_DDR1_APD_CTRL);
#endif

	writel(timing_set->t_ddr_clk_ctrl, P_DDR1_CLK_CTRL);

	/*tuned by jiaxing, add internal ref voltage setting*/
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET){	//ddr0 in use
		writel(0x49494949, P_DDR0_PUB_IOVCR0);
		writel(0x49494949, P_DDR0_PUB_IOVCR1);
	}
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET){	//ddr1 in use
		writel(0x49494949, P_DDR1_PUB_IOVCR0);
		writel(0x49494949, P_DDR1_PUB_IOVCR1);
	}

	//DDR0  timing registers
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0 is used
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		writel(timing_set->t_pctl_1us_pck,   P_DDR0_PCTL_TOGCNT1U);	   //1us = nn cycles.
		writel(timing_set->t_pctl_100ns_pck, P_DDR0_PCTL_TOGCNT100N);  //100ns = nn cycles.
		writel(timing_set->t_pctl_init_us,   P_DDR0_PCTL_TINIT);       //200us.
		writel(timing_set->t_pctl_rsth_us,   P_DDR0_PCTL_TRSTH);       // 0 for ddr2;  2 for simulation; 500 for ddr3.
		writel(timing_set->t_pctl_rstl_us,   P_DDR0_PCTL_TRSTL);       // 0 for ddr2;  2 for simulation; 500 for ddr3.

	#if defined(CONFIG_M8_DDR_LOW_POWER_DISABLE)
		writel((timing_set->t_pctl_mcfg & (~(0xFF<<8))),  P_DDR0_PCTL_MCFG );
	#else
		writel(timing_set->t_pctl_mcfg,  P_DDR0_PCTL_MCFG );
	#endif

		writel(timing_set->t_pctl_mcfg1, P_DDR0_PCTL_MCFG1);  //enable hardware c_active_in;

		writel(timing_set->t_pub_dcr, P_DDR0_PUB_DCR);

		//PUB MRx registers.
		writel(timing_set->t_pub_mr[0], P_DDR0_PUB_MR0);
		writel(timing_set->t_pub_mr[1], P_DDR0_PUB_MR1);
		writel(timing_set->t_pub_mr[2], P_DDR0_PUB_MR2);
		writel(timing_set->t_pub_mr[3], P_DDR0_PUB_MR3);

		//DDR SDRAM timing parameter.
		writel( timing_set->t_pub_dtpr[0], P_DDR0_PUB_DTPR0);
		writel( timing_set->t_pub_dtpr[1], P_DDR0_PUB_DTPR1);
		writel( timing_set->t_pub_dtpr[2], P_DDR0_PUB_DTPR2);

		//configure auto refresh when data training.
		writel( (readl(P_DDR0_PUB_PGCR2) & 0xfffc0000) | 0xc00 ,
			P_DDR0_PUB_PGCR2 );
		writel( ((readl(P_DDR0_PUB_DTCR) & 0x00ffffbf) | (1 << 28 ) | (1 << 24) | (1 << 6)  | ( 1 << 23)),
			P_DDR0_PUB_DTCR);
		//for training gate extended gate

		writel(0x8, P_DDR0_PUB_DX0GCR3); //for pdr mode bug //for pdr mode bug this will cause some chip bit deskew  jiaxing add
		writel(0x8, P_DDR0_PUB_DX1GCR3);
		writel(0x8, P_DDR0_PUB_DX2GCR3);
		writel(0x8, P_DDR0_PUB_DX3GCR3);
		writel((readl(P_DDR0_PUB_DSGCR))|(0x1<<06), P_DDR0_PUB_DSGCR); //eanble gate extension  dqs gate can help to bit deskew also

		writel(((readl(P_DDR0_PUB_ZQCR))&0xff01fffc), P_DDR0_PUB_ZQCR); //for vt bug disable IODLMT
        #ifdef CONFIG_CMD_DDR_TEST
        if(zqcr)
            writel(zqcr, P_DDR0_PUB_ZQ0PR);
        else
        #endif
		writel((timing_set->t_pub_zq0pr)|((readl(P_DDR0_PUB_ZQ0PR))&0xffffff00),
			P_DDR0_PUB_ZQ0PR);
		writel((1<<1)|((readl(P_DDR0_PUB_ZQCR))&0xfffffffe),
			P_DDR0_PUB_ZQCR);

		writel(timing_set->t_pub_dxccr, P_DDR0_PUB_DXCCR);

		//writel(((readl(P_DDR0_PUB_ZQCR))&(~(1<<2))), P_DDR0_PUB_ZQCR);

		writel(readl(P_DDR0_PUB_ACBDLR0) | (timing_set->t_pub_acbdlr0),
			P_DDR0_PUB_ACBDLR0);  //place before write level

		writel(timing_set->t_pub_ptr[0]	, P_DDR0_PUB_PTR0);
		writel(timing_set->t_pub_ptr[1]	, P_DDR0_PUB_PTR1);

		writel(readl(P_DDR0_PUB_ACIOCR0) & 0xdfffffff,
			P_DDR0_PUB_ACIOCR0);

		//configure for phy update request and ack.
		writel( (readl(P_DDR0_PUB_DSGCR) & 0xffffffef  | (1 << 6)),
			P_DDR0_PUB_DSGCR );	 //other bits.

		//for simulation to reduce the initial time.
		//real value should be based on the DDR3 SDRAM clock cycles.
		writel(timing_set->t_pub_ptr[3]	, P_DDR0_PUB_PTR3);

		writel(timing_set->t_pub_ptr[4]	, P_DDR0_PUB_PTR4);

	}


	//DDR1 timing registers
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1 is used
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		writel(timing_set->t_pctl_1us_pck,   P_DDR1_PCTL_TOGCNT1U);	   //1us = nn cycles.
		writel(timing_set->t_pctl_100ns_pck, P_DDR1_PCTL_TOGCNT100N);  //100ns = nn cycles.
		writel(timing_set->t_pctl_init_us,   P_DDR1_PCTL_TINIT);       //200us.
		writel(timing_set->t_pctl_rsth_us,   P_DDR1_PCTL_TRSTH);       // 0 for ddr2;  2 for simulation; 500 for ddr3.
		writel(timing_set->t_pctl_rstl_us,   P_DDR1_PCTL_TRSTL);

	#if defined(CONFIG_M8_DDR_LOW_POWER_DISABLE)
		writel((timing_set->t_pctl_mcfg & (~(0xFF<<8))),   P_DDR1_PCTL_MCFG  );
	#else
		writel(timing_set->t_pctl_mcfg,   P_DDR1_PCTL_MCFG  );
	#endif

		writel( timing_set->t_pctl_mcfg1, P_DDR1_PCTL_MCFG1 );  //enable hardware c_active_in;

		writel(timing_set->t_pub_dcr, P_DDR1_PUB_DCR);

		//PUB MRx registers.
		writel(timing_set->t_pub_mr[0], P_DDR1_PUB_MR0);
		writel(timing_set->t_pub_mr[1], P_DDR1_PUB_MR1);
		writel(timing_set->t_pub_mr[2], P_DDR1_PUB_MR2);
		writel(timing_set->t_pub_mr[3], P_DDR1_PUB_MR3);

		//DDR SDRAM timing parameter.
		writel( timing_set->t_pub_dtpr[0], P_DDR1_PUB_DTPR0);
		writel( timing_set->t_pub_dtpr[1], P_DDR1_PUB_DTPR1);
		writel( timing_set->t_pub_dtpr[2], P_DDR1_PUB_DTPR2);

		writel( (readl(P_DDR1_PUB_PGCR2) & 0xfffc0000) | 0xc00 ,
				 P_DDR1_PUB_PGCR2 );
		writel( ((readl(P_DDR1_PUB_DTCR) & 0x00ffffbf) | (1 << 28 ) | (1 << 24) | (1 << 6) |(1<<23) )
					,P_DDR1_PUB_DTCR);

		writel(0x8, P_DDR1_PUB_DX0GCR3); //for pdr mode bug this will cause some chip bite deskew  jiaxing add
		writel(0x8, P_DDR1_PUB_DX1GCR3);
		writel(0x8, P_DDR1_PUB_DX2GCR3);
		writel(0x8, P_DDR1_PUB_DX3GCR3);
		writel((readl(P_DDR1_PUB_DSGCR))|(0x1<<06), P_DDR1_PUB_DSGCR); //eanble gate extension  dqs gate can help to bit deskew also

		writel(((readl(P_DDR1_PUB_ZQCR))&0xff01fffc), P_DDR1_PUB_ZQCR);  //for vt bug disable IODLMT
        #ifdef CONFIG_CMD_DDR_TEST
        if(zqcr)
            writel(zqcr, P_DDR1_PUB_ZQ0PR);
        else
        #endif
		writel((timing_set->t_pub_zq0pr)|((readl(P_DDR1_PUB_ZQ0PR))&0xffffff00), P_DDR1_PUB_ZQ0PR);
		writel((1<<1)|((readl(P_DDR1_PUB_ZQCR))&0xfffffffe), P_DDR1_PUB_ZQCR);

		writel(timing_set->t_pub_dxccr, P_DDR1_PUB_DXCCR);

		//writel(((readl(P_DDR1_PUB_ZQCR))&(~(1<<2))), P_DDR1_PUB_ZQCR);

		writel(readl(P_DDR1_PUB_ACBDLR0) | (timing_set->t_pub_acbdlr0),
			P_DDR1_PUB_ACBDLR0);  //place before write level

		writel(timing_set->t_pub_ptr[0]	, P_DDR1_PUB_PTR0);
		writel(timing_set->t_pub_ptr[1]	, P_DDR1_PUB_PTR1);

		writel(readl(P_DDR1_PUB_ACIOCR0) & 0xdfffffff, P_DDR1_PUB_ACIOCR0);

		//configure for phy update request and ack.
		writel( (readl(P_DDR1_PUB_DSGCR) & 0xffffffef | ( 1 << 6)),
					   P_DDR1_PUB_DSGCR );	 //other bits.

		//for simulation to reduce the initial time.
		//real value should be based on the DDR3 SDRAM clock cycles.
		writel(timing_set->t_pub_ptr[3] , P_DDR1_PUB_PTR3);
		writel(timing_set->t_pub_ptr[4] , P_DDR1_PUB_PTR4);

	}

	// Monitor DFI initialization status.
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)  //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while(!(readl(P_DDR0_PCTL_DFISTSTAT0) & 1)) {}
		hx_serial_puts("Aml log : DDR0 - DFI status check done\n");
	}

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)  //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while(!(readl(P_DDR1_PCTL_DFISTSTAT0) & 1)) {}
		hx_serial_puts("Aml log : DDR1 - DFI status check done\n");
	}




	//power up PCTL
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)  //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(1, P_DDR0_PCTL_POWCTL);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)  //DDR 1
#endif
		writel(1, P_DDR1_PCTL_POWCTL);

	//check power state
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)  //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		while(!(readl(P_DDR0_PCTL_POWSTAT) & 1) ) {}

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) 	//DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		while(!(readl(P_DDR1_PCTL_POWSTAT) & 1) ) {}


	//DDR0 PCTL timming
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		//DDR0 PCTL
		writel(timing_set->t_pctl_trefi,          P_DDR0_PCTL_TREFI);
		writel(timing_set->t_pctl_trefi_mem_ddr3, P_DDR0_PCTL_TREFI_MEM_DDR3);
		writel(timing_set->t_pctl_trefi|(1<<31),  P_DDR0_PCTL_TREFI);

		writel(timing_set->t_pctl_tmrd, P_DDR0_PCTL_TMRD);
		writel(timing_set->t_pctl_trfc, P_DDR0_PCTL_TRFC);
		writel(timing_set->t_pctl_trp,  P_DDR0_PCTL_TRP);
		writel(timing_set->t_pctl_tal,  P_DDR0_PCTL_TAL);
		writel(timing_set->t_pctl_tcwl, P_DDR0_PCTL_TCWL);

		//DDR0 PCTL
		writel(timing_set->t_pctl_tcl,  P_DDR0_PCTL_TCL);
		writel(timing_set->t_pctl_tras, P_DDR0_PCTL_TRAS);
		writel(timing_set->t_pctl_trc,  P_DDR0_PCTL_TRC);
		writel(timing_set->t_pctl_trcd, P_DDR0_PCTL_TRCD);
		writel(timing_set->t_pctl_trrd, P_DDR0_PCTL_TRRD);

		writel(timing_set->t_pctl_trtp,   P_DDR0_PCTL_TRTP);
		writel(timing_set->t_pctl_twr,    P_DDR0_PCTL_TWR);
		writel(timing_set->t_pctl_twtr,   P_DDR0_PCTL_TWTR);
		writel(timing_set->t_pctl_texsr,  P_DDR0_PCTL_TEXSR);
		writel(timing_set->t_pctl_txp,    P_DDR0_PCTL_TXP);

		//DDR0 PCTL
		writel(timing_set->t_pctl_tdqs,   P_DDR0_PCTL_TDQS);
		writel(timing_set->t_pctl_trtw,   P_DDR0_PCTL_TRTW);
		writel(timing_set->t_pctl_tcksre, P_DDR0_PCTL_TCKSRE);
		writel(timing_set->t_pctl_tcksrx, P_DDR0_PCTL_TCKSRX);
		writel(timing_set->t_pctl_tmod,   P_DDR0_PCTL_TMOD);

		writel(timing_set->t_pctl_tcke,   P_DDR0_PCTL_TCKE);
		writel(timing_set->t_pctl_tzqcs,  P_DDR0_PCTL_TZQCS);
		writel(timing_set->t_pctl_tzqcl,  P_DDR0_PCTL_TZQCL);
		writel(timing_set->t_pctl_txpdll, P_DDR0_PCTL_TXPDLL);
		writel(timing_set->t_pctl_tzqcsi, P_DDR0_PCTL_TZQCSI);
		writel(timing_set->t_pctl_scfg,   P_DDR0_PCTL_SCFG);
	}

	//DDR1 PCTL timming
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		//DDR1 PCTL
		writel(timing_set->t_pctl_trefi,          P_DDR1_PCTL_TREFI);
		writel(timing_set->t_pctl_trefi_mem_ddr3, P_DDR1_PCTL_TREFI_MEM_DDR3);
		writel(timing_set->t_pctl_trefi|(1<<31),  P_DDR1_PCTL_TREFI);

		writel(timing_set->t_pctl_tmrd, P_DDR1_PCTL_TMRD);
		writel(timing_set->t_pctl_trfc, P_DDR1_PCTL_TRFC);
		writel(timing_set->t_pctl_trp,  P_DDR1_PCTL_TRP);
		writel(timing_set->t_pctl_tal,  P_DDR1_PCTL_TAL);
		writel(timing_set->t_pctl_tcwl, P_DDR1_PCTL_TCWL);

		//DDR1 PCTL
		writel(timing_set->t_pctl_tcl,  P_DDR1_PCTL_TCL);
		writel(timing_set->t_pctl_tras, P_DDR1_PCTL_TRAS);
		writel(timing_set->t_pctl_trc,  P_DDR1_PCTL_TRC);
		writel(timing_set->t_pctl_trcd, P_DDR1_PCTL_TRCD);
		writel(timing_set->t_pctl_trrd, P_DDR1_PCTL_TRRD);

		writel(timing_set->t_pctl_trtp,   P_DDR1_PCTL_TRTP);
		writel(timing_set->t_pctl_twr,    P_DDR1_PCTL_TWR);
		writel(timing_set->t_pctl_twtr,   P_DDR1_PCTL_TWTR);
		writel(timing_set->t_pctl_texsr,  P_DDR1_PCTL_TEXSR);
		writel(timing_set->t_pctl_txp,    P_DDR1_PCTL_TXP);

		//DDR1 PCTL
		writel(timing_set->t_pctl_tdqs,   P_DDR1_PCTL_TDQS);
		writel(timing_set->t_pctl_trtw,   P_DDR1_PCTL_TRTW);
		writel(timing_set->t_pctl_tcksre, P_DDR1_PCTL_TCKSRE);
		writel(timing_set->t_pctl_tcksrx, P_DDR1_PCTL_TCKSRX);
		writel(timing_set->t_pctl_tmod,   P_DDR1_PCTL_TMOD);

		writel(timing_set->t_pctl_tcke,   P_DDR1_PCTL_TCKE);
		writel(timing_set->t_pctl_tzqcs,  P_DDR1_PCTL_TZQCS);
		writel(timing_set->t_pctl_tzqcl,  P_DDR1_PCTL_TZQCL);
		writel(timing_set->t_pctl_txpdll, P_DDR1_PCTL_TXPDLL);
		writel(timing_set->t_pctl_tzqcsi, P_DDR1_PCTL_TZQCSI);
		writel(timing_set->t_pctl_scfg,   P_DDR1_PCTL_SCFG);

	}

	//Set PCTL to config mode
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(UPCTL_CMD_CONFIG, P_DDR0_PCTL_SCTL); //DDR 0

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(UPCTL_CMD_CONFIG, P_DDR1_PCTL_SCTL); //DDR 1

	//ensure DDR0 PCTL in CFG state
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		while(!(readl(P_DDR0_PCTL_STAT) & 1)) {}

	//ensure DDR1 PCTL in CFG state
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		while(!(readl(P_DDR1_PCTL_STAT) & 1)) {}


#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		//DDR 0 DFI
		writel((0xf0 << 1),P_DDR0_PCTL_PPCFG);
		writel(4,P_DDR0_PCTL_DFISTCFG0);
		writel(1,P_DDR0_PCTL_DFISTCFG1);
		writel(2,P_DDR0_PCTL_DFITCTRLDELAY);
		writel(1,P_DDR0_PCTL_DFITPHYWRDATA);

		//DDR 0 DFI
		nTempVal = timing_set->t_pctl_tcwl+timing_set->t_pctl_tal;
		nTempVal = (nTempVal - ((nTempVal%2) ? 3:4))/2;
		writel(nTempVal,P_DDR0_PCTL_DFITPHYWRLAT);

		nTempVal = timing_set->t_pctl_tcl+timing_set->t_pctl_tal;
		nTempVal = (nTempVal - ((nTempVal%2) ? 3:4))/2;
		writel(nTempVal,P_DDR0_PCTL_DFITRDDATAEN);

		//DDR 0 DFI
		writel(13,P_DDR0_PCTL_DFITPHYRDLAT);
		writel(1,P_DDR0_PCTL_DFITDRAMCLKDIS);
		writel(1,P_DDR0_PCTL_DFITDRAMCLKEN);
		writel(0x4000,P_DDR0_PCTL_DFITCTRLUPDMIN);
		writel(( 1 | (3 << 4) | (1 << 8) | (3 << 12) | (7 <<16) | (1 <<24) | ( 3 << 28)),
			P_DDR0_PCTL_DFILPCFG0);
		writel(0x200,P_DDR0_PCTL_DFITPHYUPDTYPE1);
		writel(8,P_DDR0_PCTL_DFIODTCFG);
		writel(( 0x0 | (0x6 << 16)),P_DDR0_PCTL_DFIODTCFG1);

		//CONFIG_M8_DDR0_DTAR_ADDR//0x8/0x10/0x18
		writel((0x0  + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR0);
		writel((0x08 + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR1);
		writel((0x10 + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR2);
		writel((0x18 + timing_set->t_pub0_dtar), P_DDR0_PUB_DTAR3);

	}

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET)
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		//DDR 1 DFI
		writel((0xf0 << 1),P_DDR1_PCTL_PPCFG);
		writel(4,P_DDR1_PCTL_DFISTCFG0);
		writel(1,P_DDR1_PCTL_DFISTCFG1);
		writel(2,P_DDR1_PCTL_DFITCTRLDELAY);
		writel(1,P_DDR1_PCTL_DFITPHYWRDATA);

		//DDR 1 DFI
		nTempVal = timing_set->t_pctl_tcwl+timing_set->t_pctl_tal;
		nTempVal = (nTempVal - ((nTempVal%2) ? 3:4))/2;
		writel(nTempVal,P_DDR1_PCTL_DFITPHYWRLAT);

		nTempVal = timing_set->t_pctl_tcl+timing_set->t_pctl_tal;
		nTempVal = (nTempVal - ((nTempVal%2) ? 3:4))/2;
		writel(nTempVal,P_DDR1_PCTL_DFITRDDATAEN);

		//DDR 1 DFI
		writel(13,P_DDR1_PCTL_DFITPHYRDLAT);
		writel(1,P_DDR1_PCTL_DFITDRAMCLKDIS);
		writel(1,P_DDR1_PCTL_DFITDRAMCLKEN);
		writel(0x4000,P_DDR1_PCTL_DFITCTRLUPDMIN);
		writel(( 1 | (3 << 4) | (1 << 8) | (3 << 12) | (7 <<16) | (1 <<24) | ( 3 << 28)),
			P_DDR1_PCTL_DFILPCFG0);
		writel(0x200,P_DDR1_PCTL_DFITPHYUPDTYPE1);
		writel(8,P_DDR1_PCTL_DFIODTCFG);
		writel(( 0x0 | (0x6 << 16)),P_DDR1_PCTL_DFIODTCFG1);

		//CONFIG_M8_DDR1_DTAR_ADDR/0x8/0x10/0x18
		writel((0x0  + timing_set->t_pub1_dtar), P_DDR1_PUB_DTAR0);
		writel((0x08 + timing_set->t_pub1_dtar), P_DDR1_PUB_DTAR1);
		writel((0x10 + timing_set->t_pub1_dtar), P_DDR1_PUB_DTAR2);
		writel((0x18 + timing_set->t_pub1_dtar), P_DDR1_PUB_DTAR3);

	}

	//PCTL CMDTSTATEN
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(1,P_DDR0_PCTL_CMDTSTATEN);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(1,P_DDR1_PCTL_CMDTSTATEN);

	//PCTL CMDTSTAT
#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		while(!(readl(P_DDR0_PCTL_CMDTSTAT) & 0x1)) {}

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		while(!(readl(P_DDR1_PCTL_CMDTSTAT) & 0x1)) {}



	//===============================================
	//HX PUB INIT

	hx_serial_puts("Aml log : HX DDR PUB training begin:\n");

	//===============================================
	//PLL,DCAL,PHY RST,ZCAL
	nTempVal =	PUB_PIR_ZCAL | PUB_PIR_PLLINIT | PUB_PIR_DCAL | PUB_PIR_PHYRST;

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal, P_DDR0_PUB_PIR);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal, P_DDR1_PUB_PIR);

	ddr_udelay(1);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR);

//#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
//#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x8000000f) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC000000f))
		{
			ddr_udelay(10);
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_ZCERR)
			{
				serial_puts("Aml log : DDR0 - PUB_PGSR0_ZCERR with [");
				serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr0;
			}
		}
		hx_serial_puts("Aml log : DDR0 - PLL,DCAL,RST,ZCAL done\n");
	}

//#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
//#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000000f) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC000000f))
		{
			ddr_udelay(10);
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_ZCERR)
			{
				serial_puts("Aml log : DDR1 - PUB_PGSR0_ZCERR with [");
				serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr1;
			}
		}
		hx_serial_puts("Aml log : DDR1 - PLL,DCAL,RST,ZCAL done\n");
	}

	//===============================================
	//DRAM INIT
	nTempVal =	PUB_PIR_DRAMRST | PUB_PIR_DRAMINIT ;

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal, P_DDR0_PUB_PIR);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal, P_DDR1_PUB_PIR);

	//ddr_udelay(1);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR);

//#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
//#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x8000001f) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC000001f))
		{
			//ddr_udelay(10);
			if(readl(P_DDR0_PUB_PGSR0) & 1)
				break;
		}
		hx_serial_puts("Aml log : DDR0 - DRAM INIT done\n");
	}

//#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
//#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000001f) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC000001f))
		{
			//ddr_udelay(10);
			if(readl(P_DDR1_PUB_PGSR0) & 1)
				break;
		}
		hx_serial_puts("Aml log : DDR1 - DRAM INIT done\n");
	}

#if defined(CONFIG_M8_PXP_EMULATOR)
	goto m8_ddr_pxp_emulator_step;
#endif //CONFIG_M8_PXP_EMULATOR

#if defined(CONFIG_M8_ZEBU_EMULATOR)
	goto m8_ddr_vlsi_emulator_done;
#endif //CONFIG_M8_ZEBU_EMULATOR

	//===============================================
	//WL init
	nTempVal =	PUB_PIR_WL ;
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR);

	//ddr_udelay(1);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x8000003f) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC000003f))
		{
			//ddr_udelay(10);
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_WLERR)
			{
				serial_puts("Aml log : DDR0 - PUB_PGSR0_WLERR with [");
				serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr0;
			}
		}
		hx_serial_puts("Aml log : DDR0 - WL done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000003f) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC000003f))
		{
			//ddr_udelay(10);
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_WLERR)
			{
				serial_puts("Aml log : DDR1 - PUB_PGSR0_WLERR with [");
				serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr1;
			}
		}
		hx_serial_puts("Aml log : DDR1 - WL done\n");
	}

#if defined(CONFIG_M8_PXP_EMULATOR)
m8_ddr_pxp_emulator_step:
#endif //CONFIG_M8_PXP_EMULATOR

	//===============================================
	//DQS Gate training
	nTempVal =	PUB_PIR_QSGATE ;
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal, P_DDR1_PUB_PIR);

	//ddr_udelay(1);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{

		while((readl(P_DDR0_PUB_PGSR0) != 0x8000007f) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC000007f))
		{
			//ddr_udelay(10);
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_QSGERR)
			{
				serial_puts("Aml log : DDR0 - PUB_PGSR0_QSGERR with [");
				serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr0;
			}
		}
		hx_serial_puts("Aml log : DDR0 - DQS done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x8000007f) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC000007f))
		{
			//ddr_udelay(10);
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_QSGERR)
			{
				serial_puts("Aml log : DDR1 - PUB_PGSR0_QSGERR with [");
				serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr1;
			}
		}
		hx_serial_puts("Aml log : DDR1 - DQS done\n");
	}

#if defined(CONFIG_M8_PXP_EMULATOR)
	goto m8_ddr_vlsi_emulator_done;
#endif //CONFIG_M8_PXP_EMULATOR

	//===============================================
	//Write leveling ADJ
	nTempVal =	PUB_PIR_WLADJ ;
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR);

	//ddr_udelay(1);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x800000ff) &&
			(readl(P_DDR0_PUB_PGSR0) != 0xC00000ff))
		{
			//ddr_udelay(10);
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_WLAERR)
			{
				serial_puts("Aml log : DDR0 - PUB_PGSR0_WLAERR with [");
				serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr0;
			}
		}
		hx_serial_puts("Aml log : DDR0 - WLADJ done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x800000ff) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC00000ff))
		{
			//ddr_udelay(10);
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_WLAERR)
			{
				serial_puts("Aml log : DDR1 - PUB_PGSR0_WLAERR with [");
				serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);
				serial_puts("] retry...\n");
				goto pub_init_ddr1;
			}
		}
		hx_serial_puts("Aml log : DDR1 - WLADJ done\n");
	}


	//===============================================
	//Data bit deskew & data eye training
	nTempVal =	PUB_PIR_RDDSKW | PUB_PIR_WRDSKW | PUB_PIR_RDEYE | PUB_PIR_WREYE;
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal, P_DDR0_PUB_PIR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
		writel(nTempVal, P_DDR1_PUB_PIR);

	//ddr_udelay(1);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
		writel(nTempVal|PUB_PIR_INIT, P_DDR0_PUB_PIR);

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
		writel(nTempVal|PUB_PIR_INIT, P_DDR1_PUB_PIR);

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{
		while((readl(P_DDR0_PUB_PGSR0) != 0x80000fff)&&
			(readl(P_DDR0_PUB_PGSR0) != 0xC0000fff))
		{
			ddr_udelay(1);
			if(readl(P_DDR0_PUB_PGSR0) & PUB_PGSR0_DTERR)
			{
			    serial_puts("Aml log : DDR0 - PUB_PGSR0_DTERR with [");
				serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);
				serial_puts("] retry...\n");
			    goto pub_init_ddr0;
			}
		}
		hx_serial_puts("Aml log : DDR0 - BIT deskew & data eye done\n");
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
	{
		while((readl(P_DDR1_PUB_PGSR0) != 0x80000fff) &&
			(readl(P_DDR1_PUB_PGSR0) != 0xC0000fff))
		{
			ddr_udelay(1);
			if(readl(P_DDR1_PUB_PGSR0) & PUB_PGSR0_DTERR)
			{
			    serial_puts("Aml log : DDR1 - PUB_PGSR0_DTERR with [");
				serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);
				serial_puts("] retry...\n");
			    goto pub_init_ddr1;
			}
		}
		hx_serial_puts("Aml log : DDR1 - BIT deskew & data eye done\n");
	}


	//===============================================

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(UPCTL_CMD_GO, P_DDR0_PCTL_SCTL);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
		writel(UPCTL_CMD_GO, P_DDR1_PCTL_SCTL);

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while ((readl(P_DDR0_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_ACCESS ) {}
		hx_serial_puts("Aml log : DDR0 - PCTL enter GO state\n");
	}

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	{
		while ((readl(P_DDR1_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_ACCESS ) {}
		hx_serial_puts("Aml log : DDR1 - PCTL enter GO state\n");
	}

	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{
		nTempVal = readl(P_DDR0_PUB_PGSR0);
#ifdef CONFIG_M8_NO_DDR_PUB_VT_CHECK
		if ((( (nTempVal >> 20) & 0xfff ) != 0xC00 ) &&
			(( (nTempVal >> 20) & 0xfff ) != 0x800 ))
#else
		if (( (nTempVal >> 20) & 0xfff ) != 0xC00 )
#endif
	    {
			serial_puts("\nAml log : DDR0 - PUB init fail with PGSR0 : 0x");
			serial_put_hex(nTempVal,32);
			serial_puts("\nAml log : Try again with MMC reset...\n\n");
			goto mmc_init_all;
	    }
	    else
	    {
	        hx_serial_puts("\nAml log : DDR0 - init pass with\n  PGSR0 : 0x");
			hx_serial_put_hex(readl(P_DDR0_PUB_PGSR0),32);
			hx_serial_puts("\n");
			//serial_puts("\n  PGSR1 : 0x");
			//serial_put_hex(readl(P_DDR0_PUB_PGSR1),32);
			//serial_puts("\n");
	    }
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
	{
		nTempVal = readl(P_DDR1_PUB_PGSR0);
#ifdef CONFIG_M8_NO_DDR_PUB_VT_CHECK
		if ((( (nTempVal >> 20) & 0xfff ) != 0xC00 ) &&
			(( (nTempVal >> 20) & 0xfff ) != 0x800 ))
#else
		if (( (nTempVal >> 20) & 0xfff ) != 0xC00 )
#endif
	    {
			serial_puts("\nAml log : DDR1 - PUB init fail with PGSR0 : 0x");
			serial_put_hex(nTempVal,32);
			serial_puts("\nAml log : Try again with MMC reset...\n\n");
			goto mmc_init_all;
	    }
	    else
	    {
	        hx_serial_puts("\nAml log : DDR1 - init pass with\n  PGSR0 : 0x");
			hx_serial_put_hex(readl(P_DDR1_PUB_PGSR0),32);
			hx_serial_puts("\n");
			//serial_puts("\n  PGSR1 : 0x");
			//serial_put_hex(readl(P_DDR1_PUB_PGSR1),32);
			//serial_puts("\n");
	    }
	}

#if defined(CONFIG_M8_PUB_WLWDRDRGLVTWDRDBVT_DISABLE)
	writel((readl(P_DDR0_PUB_PGCR0) & (~0x3F)),P_DDR0_PUB_PGCR0);
	writel((readl(P_DDR1_PUB_PGCR0) & (~0x3F)),P_DDR1_PUB_PGCR0);
#endif

	nRet = 0;

#if !defined(M8_DDR_CHN_OPEN_CLOSE)
	if(CONFIG_M8_DDR1_ONLY != nM8_DDR_CHN_SET) //DDR 0
	{

	#if defined(CONFIG_M8_GATEACDDRCLK_DISABLE)
		writel((0x7d<<9)|(readl(P_DDR0_PUB_PGCR3)),
			P_DDR0_PUB_PGCR3);
	#else
		writel((0x7f<<9)|(readl(P_DDR0_PUB_PGCR3)),
			P_DDR0_PUB_PGCR3);
	#endif

		writel(readl(P_DDR0_PUB_ZQCR) | (0x4),
			P_DDR0_PUB_ZQCR); //for power PZQ CELL save VDDQ power be careful with de sequence ,close zqcr then close clock
        #ifndef CONFIG_CMD_DDR_TEST
		writel(readl(P_DDR0_CLK_CTRL) & (~1),
			P_DDR0_CLK_CTRL);  //for power pctl gate clk save EE power
        #endif
	}
	else
	{	//DDR1 only and make DDR0 to sleep for power saving
		writel(1<<29, P_DDR0_PUB_PLLCR);
		writel((1<<6)|(1<<4), P_DDR0_CLK_CTRL);
	}

	if(CONFIG_M8_DDR0_ONLY != nM8_DDR_CHN_SET) //DDR 1
	{
	#if defined(CONFIG_M8_GATEACDDRCLK_DISABLE)
		writel((0x7d<<9)|(readl(P_DDR1_PUB_PGCR3)),
			P_DDR1_PUB_PGCR3);
	#else
		writel((0x7f<<9)|(readl(P_DDR1_PUB_PGCR3)),
			P_DDR1_PUB_PGCR3);
	#endif
		writel(readl(P_DDR1_PUB_ZQCR) | (0x4),
			P_DDR1_PUB_ZQCR);//for power PZQ CELL save VDDQ power be careful with de sequence ,close zqcr then close clock
        #ifndef CONFIG_CMD_DDR_TEST
		writel(readl(P_DDR1_CLK_CTRL) & (~1),
			P_DDR1_CLK_CTRL); //for power pctl gate clk save EE power
        #endif
	}
	else
	{	//DDR0 only and make DDR1 to sleep for power saving
		writel(1<<29, P_DDR1_PUB_PLLCR);
		writel((1<<6)|(1<<4), P_DDR1_CLK_CTRL);
	}
#else //#if !defined(M8_DDR_CHN_OPEN_CLOSE)

  #if defined(CONFIG_M8_GATEACDDRCLK_DISABLE)
	writel((0x7d<<9)|(readl(P_DDR0_PUB_PGCR3)),
		P_DDR0_PUB_PGCR3);
  #else
	writel((0x7f<<9)|(readl(P_DDR0_PUB_PGCR3)),
		P_DDR0_PUB_PGCR3);
  #endif

	writel(readl(P_DDR0_PUB_ZQCR) | (0x4),
		P_DDR0_PUB_ZQCR);

  #if defined(CONFIG_M8_GATEACDDRCLK_DISABLE)
	writel((0x7d<<9)|(readl(P_DDR1_PUB_PGCR3)),
		P_DDR1_PUB_PGCR3);
  #else
	writel((0x7f<<9)|(readl(P_DDR1_PUB_PGCR3)),
		P_DDR1_PUB_PGCR3);
  #endif

	writel(readl(P_DDR1_PUB_ZQCR) | (0x4),
		P_DDR1_PUB_ZQCR);

	__udelay(1);

	switch(nM8_DDR_CHN_SET)
	{
	case CONFIG_M8_DDR0_ONLY:
		{
			//close DDR-1
			writel(UPCTL_CMD_SLEEP, P_DDR1_PCTL_SCTL);
			while ((readl(P_DDR1_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_LOW_POWER ) {}

			writel(1<<29, P_DDR1_PUB_PLLCR);
			__udelay(1);
			writel((1<<2)|(1<<6)|(1<<4),
				P_DDR1_CLK_CTRL);

			writel(readl(P_DDR0_CLK_CTRL) & (~1),
				P_DDR0_CLK_CTRL);
		}break;
	case CONFIG_M8_DDR1_ONLY:
		{
			//close DDR-0
			writel(UPCTL_CMD_SLEEP, P_DDR0_PCTL_SCTL);
			while ((readl(P_DDR0_PCTL_STAT) & UPCTL_STAT_MASK ) != UPCTL_STAT_LOW_POWER ) {}

			writel(1<<29, P_DDR0_PUB_PLLCR);
			__udelay(1);
			writel((1<<2)|(1<<6)|(1<<4),
				P_DDR0_CLK_CTRL);

			writel(readl(P_DDR1_CLK_CTRL) & (~1),
				P_DDR1_CLK_CTRL);
		}break;
	default:
		{
			writel(readl(P_DDR0_CLK_CTRL) & (~1),
				P_DDR0_CLK_CTRL);
			writel(readl(P_DDR1_CLK_CTRL) & (~1),
				P_DDR1_CLK_CTRL);
		}break;
	}

	__udelay(1);

	#undef M8_DDR_CHN_OPEN_CLOSE
#endif //#if !defined(M8_DDR_CHN_OPEN_CLOSE)

	return nRet;
}
