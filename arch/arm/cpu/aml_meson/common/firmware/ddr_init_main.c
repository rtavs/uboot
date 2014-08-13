#define  CONFIG_AMLROM_SPL 1
#include <timer.c>
#include <timming.c>
#include <uartpin.c>
#include <serial.c>
#include <serial_uart_a.c>
#include <pinmux.c>
#include <sdpinmux.c>
#include <memtest.c>
#include <pll.c>
#include <hardi2c_lite.c>
#include <power.c>

#ifndef CONFIG_DUMP_DDR_INFO
#define CONFIG_DUMP_DDR_INFO
#endif// #ifndef CONFIG_DUMP_DDR_INFO
#include <ddr.c>

#include <mtddevices.c>
#include <sdio.c>
#include <debug_rom.c>

#include <loaduboot.c>
#ifdef CONFIG_ACS
#include <storage.c>
#include <acs.c>
#endif

#include <asm/arch/reboot.h>

#ifdef CONFIG_POWER_SPL
#include <amlogic/aml_pmu_common.h>
#endif

#if defined(CONFIG_M8)
#define _CHIP_ID        (0x19)
#endif//

#define BIN_RUN_INFO_MAGIC_PARA      (0X3412CDABU)
#define BIN_RUN_INFO_VERSION    (0x0100)
#define BIN_RUN_TYPE_RUN_ADDRESS    0
#define BIN_RUN_TYPE_RUN_IN_ADDR    (0)
#define BIN_RUN_TYPE_UCL_DECOMPRESS (0xc0de)

//As sram area will not cleared before poweroff, the result maigc must not be equal to para magic and clear before run
#define BIN_RUN_INFO_MAGIC_RESULT      (0X7856EFABU)

#pragma pack(push, 4)
typedef struct _BinRunInfoHead_s{
    unsigned            magic;      //should be DDR_BIN_PARA_MAGIC
    unsigned            version;    //current version is 0x0100
    unsigned            runType;    //
    int                 retVal;     //0 is OK, other is failed
}BinRunInfoHead_t;
#pragma pack(pop)

#pragma pack(push, 4)
typedef struct _RunBinInfo_s{
    BinRunInfoHead_t        binRunHead;
    unsigned                runAddr;
}RunBinInfo_t;
#pragma pack(pop)

#pragma pack(push, 4)
typedef struct _UclDecompressInfo{
    BinRunInfoHead_t    binRunHead;
    unsigned char*      srcDataAddr;//
    unsigned            srcDataLen;
    unsigned char*      decompressedAddr;
    unsigned            decompressedLen;
}UclDecompressInfo_t;
#pragma pack(pop)

static int _usb_ucl_decompress(unsigned char* compressData, unsigned char* decompressedAddr, unsigned* decompressedLen)
{
    int ret = __LINE__;

    serial_puts("\n\nucl Decompress START ====>\n");
    serial_puts("compressData "), serial_put_hex((unsigned)compressData, 32), serial_puts(",");
    serial_puts("decompressedAddr "), serial_put_hex((unsigned)decompressedAddr, 32), serial_puts(".\n");

    if(ret){
        serial_puts("decompress FAILED ret="), serial_put_dec(ret), serial_puts("\n");
    }
    else{
        serial_puts("decompressedLen "), serial_put_hex(*decompressedLen, 32), serial_puts(".\n");
    }

    return ret;
}

static unsigned _ddr_init_main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
	//setbits_le32(0xda004000,(1<<0));	//TEST_N enable: This bit should be set to 1 as soon as possible during the Boot process to prevent board changes from placing the chip into a production test mode

	writel((readl(0xDA000004)|0x08000000), 0xDA000004);	//set efuse PD=1

//write ENCI_MACV_N0 (CBUS 0x1b30) to 0, disable Macrovision
#if defined(CONFIG_M6) || defined(CONFIG_M6TV)
	writel(0, CBUS_REG_ADDR(ENCI_MACV_N0));
#endif

//Default to open ARM JTAG for M6 only
#if  defined(CONFIG_M6) || defined(CONFIG_M6TV)
	#define AML_M6_JTAG_ENABLE
	#define AML_M6_JTAG_SET_ARM

	//for M6 only. And it will cause M3 fail to boot up.
	//TEST_N enable: This bit should be set to 1 as soon as possible during the
	//Boot process to prevent board changes from placing the chip into a production test mode
	setbits_le32(0xda004000,(1<<0));

	// set bit [12..14] to 1 in AO_RTI_STATUS_REG0
	// This disables boot device fall back feature in MX Rev-D
	// This still enables bootloader to detect which boot device
	// is selected during boot time.
	switch(readl(0xc8100000))
	{
	case 0x6b730001:
	case 0x6b730002: writel(readl(0xc8100000) |(0x70<<8),0xc8100000);break;
	}

#endif


#ifdef	CONFIG_M8
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
#endif //#if defined(CONFIG_M8)


#ifdef AML_M6_JTAG_ENABLE
	#ifdef AML_M6_JTAG_SET_ARM
		//A9 JTAG enable
		writel(0x80000510,0xda004004);
		//TDO enable
		writel(readl(0xc8100014)|0x4000,0xc8100014);
	#elif AML_M6_JTAG_SET_ARC
		//ARC JTAG enable
		writel(0x80051001,0xda004004);
		//ARC bug fix disable
		writel((readl(0xc8100040)|1<<24),0xc8100040);
	#endif	//AML_M6_JTAG_SET_ARM

	//Watchdog disable
	//writel(0,0xc1109900);
	//asm volatile ("wfi");

#endif //AML_M6_JTAG_ENABLE

	//Note: Following msg is used to calculate romcode boot time
	//         Please DO NOT remove it!
    serial_puts("\nTE : ");
    unsigned int nTEBegin = TIMERE_GET();
    serial_put_dec(nTEBegin);
    serial_puts("\nBT : ");
    //Note: Following code is used to show current uboot build time
    //         For some fail cases which in SPL stage we can not target
    //         the uboot version quickly. It will cost about 5ms.
    //         Please DO NOT remove it!
    serial_puts(__TIME__);
    serial_puts(" ");
    serial_puts(__DATE__);
    serial_puts("\n");


#ifdef CONFIG_POWER_SPL
    power_init(POWER_INIT_MODE_NORMAL);
#endif

    // initial pll
    pll_init(&__plls);

	serial_init(__plls.uart);
    #if MESON_CPU_TYPE >= MESON_CPU_TYPE_MESON8
       serial_init_uart_a(__plls.uart);
    #endif

    __udelay(100);//wait for a uart input

    //TEMP add
    unsigned int nPLL = readl(P_HHI_SYS_PLL_CNTL);
    unsigned int nA9CLK = ((24 / ((nPLL>>9)& 0x1F) ) * (nPLL & 0x1FF))/ (1<<((nPLL>>16) & 0x3));
    serial_puts("\nCPU clock is ");
    serial_put_dec(nA9CLK);
    serial_puts("MHz\n");


    nTEBegin = TIMERE_GET();

    // initial ddr
    ddr_init_test();

    serial_puts("\nDDR init use : ");
    serial_put_dec(get_utimer(nTEBegin));
    serial_puts(" us\n");

    serial_puts("\nPLL & DDR init OK\n");

#if 0
    //wait serial_puts end.
    for(i = 0; i < 10; i++)
		  __udelay(1000);
#else
	serial_wait_tx_empty();
#endif


#if defined(CONFIG_AML_V2_USBTOOL) && 0
    writel(MESON_USB_BURNER_REBOOT, CONFIG_TPL_BOOT_ID_ADDR);//tell TPL it loaded from USB otg
#endif//#if defined(CONFIG_AML_V2_USBTOOL)

    serial_puts("\nEnd ddr main\n");

    return 0;
}

unsigned main(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
    BinRunInfoHead_t*       binRunInfoHead      = (BinRunInfoHead_t*)(RAM_START + 64 * 1024);//D9010000
    int ret = 0;
    const unsigned paraMagic = binRunInfoHead->magic;
    const unsigned ChipId    = readl(CBUS_REG_ADDR(0x1f53));

#ifdef CONFIG_M8
	//enable watchdog, then when bootup failed, switch to next boot device
	AML_WATCH_DOG_SET(5000); //5s
#endif// #if defined(CONFIG_M8)
    binRunInfoHead->magic = BIN_RUN_INFO_MAGIC_RESULT; binRunInfoHead->retVal = 0xdd;
    //serial_puts("\nboot_ID "), serial_put_hex(C_ROM_BOOT_DEBUG->boot_id, 32), serial_puts("\n");
    //serial_puts("binMagic "), serial_put_hex(paraMagic, 32), serial_puts("\n");
#if defined(CONFIG_M6)//Asset m6 platform
    if(22 != ChipId){
            binRunInfoHead->retVal = ChipId + (22<<16);//Error value for pc
            return __LINE__;
    }
    else{
            const unsigned encryptReg           = readl(0xD9018A80);
            const unsigned* dataEncryptedByTool = (unsigned*)(CONFIG_DDR_INIT_ADDR + 0x20);

            if(encryptReg & (1U<<7)){//RSA key already burned
                    int i = 0;
                    for(i=0; i < 4; ++i){
                            if(0xc003 != *dataEncryptedByTool++){
                                    binRunInfoHead->retVal = 0xc003 + i;//Error value for pc
                                    return __LINE__;
                            }
                    }
            }
    }
#endif//

#if 1
    if(BIN_RUN_INFO_MAGIC_PARA != paraMagic)//default to run ddr_init.bin, Attention that sram area will not clear if not poweroff!
    {
        ret = _ddr_init_main(__TEXT_BASE, __TEXT_SIZE);
        binRunInfoHead->retVal = ret;

    serial_puts(__TIME__);
	serial_puts(" ");
	serial_puts(__DATE__);
	serial_puts("\n");

        return ret;
    }
#else
    binRunInfoHead->runType = BIN_RUN_TYPE_UCL_DECOMPRESS;
    ((UclDecompressInfo_t*)binRunInfoHead)->srcDataAddr = (unsigned char*)(4U<<20);

	writel(((__ddr_setting.phy_memory_size)>>20), CONFIG_DDR_SIZE_IND_ADDR);
#endif//#if 1

    if(BIN_RUN_INFO_VERSION != binRunInfoHead->version){
        serial_puts("run info version "), serial_put_hex(binRunInfoHead->version, 16), serial_puts("error\n");
    }

    switch(binRunInfoHead->runType)
    {
        case BIN_RUN_TYPE_UCL_DECOMPRESS:
            {
                UclDecompressInfo_t*    uclDecompressInfo   = (UclDecompressInfo_t*)binRunInfoHead;
                unsigned char*          tplSrcDataAddr      = uclDecompressInfo->srcDataAddr;
                unsigned                secureosOffset      = 0;
                unsigned*               ubootBinAddr        = (unsigned*)tplSrcDataAddr;

                uclDecompressInfo->decompressedAddr = (unsigned char*)CONFIG_SYS_TEXT_BASE;
                ret = _usb_ucl_decompress(tplSrcDataAddr, uclDecompressInfo->decompressedAddr, &uclDecompressInfo->decompressedLen);
                if(ret){
                    break;
                }
                else
                {
                    ret = check_sum((unsigned*)uclDecompressInfo->decompressedAddr, 0, 0);
                    if(ret){
                        serial_puts("check magic error\n");
                        break;
                    }
                }
            }
            break;

        default:
            serial_puts("Error run type "), serial_put_hex(binRunInfoHead->runType, 32), serial_puts("\n");
            ret = __LINE__;
    }

    binRunInfoHead->retVal  = ret;
    return ret;
}
