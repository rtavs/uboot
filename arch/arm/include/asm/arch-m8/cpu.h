/*

 *
 */

#ifndef _CPU_H
#define _CPU_H
#include <config.h>
#include <asm/plat-cpu.h>
#if CONFIG_AML_MESON==0
#error please define CONFIG_AML_MESON
#endif
//U boot code control

//DDR training address, DO NOT modify
//DDR0: 0x0F00 - 0x0F7F (128Bytes)
#define CONFIG_M8_DDR0_DTAR_ADDR (0x3000000)

//DDR1: 0x1000 - 0x107F (128Bytes) for all setting except CONFIG_M8_DDRX2_S30
//DDR1: 0x40000000 - 0x4000007F (128Bytes) for CONFIG_M8_DDRX2_S30
#define CONFIG_M8_DDR1_DTAR_ADDR (0x3000000 | ((CONFIG_M8_DDRX2_S30 == CONFIG_M8_DDR_CHANNEL_SET) ? (1<<30) : (1<<12)))


//M8 DDR0/1 channel select
#define CONFIG_M8_DDRX2_S12  (0)
#define CONFIG_M8_DDR1_ONLY  (1)
#define CONFIG_M8_DDRX2_S30  (2)
#define CONFIG_M8_DDR0_ONLY  (3)
//#define CONFIG_M8_DDR_CHANNEL_SET (CONFIG_M8_DDRX2_S12) //board.h

//M8 DDR0/1 address map bank mode
#define CONFIG_M8_DDR_ADDR_MAP_BANK_MODE_2_BNK   (0)
#define CONFIG_M8_DDR_ADDR_MAP_BANK_MODE_4_BNK   (1)
//#define CONFIG_M8_DDR_AMBM_SET   (CONFIG_M8_DDR_ADDR_MAP_BANK_MODE_4_BNK) //board.h

//M8 no need to define column bit length in board.h
#if !defined(CONFIG_M8_DDR_COL_BITS)
	#define CONFIG_M8_DDR_COL_BITS  (10)
#endif //CONFIG_M8_DDR_COL_BITS

#define M8_DDR_DTAR_BANK_GET(addr,bit_row,bit_col,ambm_set,chn_set)  ((((addr) >> ((bit_row)+(bit_col)+ (chn_set ? 2 : 3))) & (ambm_set ? 4 : 6)) | \
																		(((addr) >> ((bit_col)+( chn_set ? 2 : 3))) & (ambm_set ? 3 : 1)))

#define M8_DDR_DTAR_DTROW_GET(addr,bit_row,bit_col,ambm_set,chn_set) (( (addr) >> (( chn_set ? (ambm_set+1) : (ambm_set+2))+(bit_col)+2)) & ((1<< (bit_row))-1))
#define M8_DDR_DTAR_DTCOL_GET(addr,bit_col)                 (( (addr) >> 2) & ((1<< (bit_col))-1))

#define CONFIG_M8_DDR0_DTAR_DTBANK  M8_DDR_DTAR_BANK_GET(CONFIG_M8_DDR0_DTAR_ADDR,CONFIG_M8_DDR_ROW_BITS,CONFIG_M8_DDR_COL_BITS,CONFIG_M8_DDR_AMBM_SET,CONFIG_M8_DDR_CHANNEL_SET)
#define CONFIG_M8_DDR0_DTAR_DTROW   M8_DDR_DTAR_DTROW_GET(CONFIG_M8_DDR0_DTAR_ADDR,CONFIG_M8_DDR_ROW_BITS,CONFIG_M8_DDR_COL_BITS,CONFIG_M8_DDR_AMBM_SET,CONFIG_M8_DDR_CHANNEL_SET)
#define CONFIG_M8_DDR0_DTAR_DTCOL   M8_DDR_DTAR_DTCOL_GET(CONFIG_M8_DDR0_DTAR_ADDR,CONFIG_M8_DDR_COL_BITS)

#define CONFIG_M8_DDR1_DTAR_DTBANK  M8_DDR_DTAR_BANK_GET(CONFIG_M8_DDR1_DTAR_ADDR,CONFIG_M8_DDR_ROW_BITS,CONFIG_M8_DDR_COL_BITS,CONFIG_M8_DDR_AMBM_SET,CONFIG_M8_DDR_CHANNEL_SET)
#define CONFIG_M8_DDR1_DTAR_DTROW   M8_DDR_DTAR_DTROW_GET(CONFIG_M8_DDR1_DTAR_ADDR,CONFIG_M8_DDR_ROW_BITS,CONFIG_M8_DDR_COL_BITS,CONFIG_M8_DDR_AMBM_SET,CONFIG_M8_DDR_CHANNEL_SET)
#define CONFIG_M8_DDR1_DTAR_DTCOL   M8_DDR_DTAR_DTCOL_GET(CONFIG_M8_DDR1_DTAR_ADDR,CONFIG_M8_DDR_COL_BITS)


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//NOTE: IMPORTANT! DO NOT TRY TO MODIFY FOLLOWING CODE!
//           It is used to get DDR0/1 training address from PUB_DTAR0. The DDR training address is just the
//           start address and it will occupy 128bytes for each channel.
//
//How to fetch the DDR training address for M8:
//           1. enable PCTL clock before access
//           2. load MMC DDR setting from P_MMC_DDR_CTRL  (#define P_MMC_DDR_CTRL        0xc8006000 + (0x00 << 2 ) )
//           3. load the DTAR0 value from DDR0/DDR1 PUB register according to the channel setting from MMC_DDR_CTRL
//           4. disable PCTL clock for power saving
//
//Demo code:
/*
	//enable clock
	writel(readl(P_DDR0_CLK_CTRL)|(1), P_DDR0_CLK_CTRL);
	writel(readl(P_DDR1_CLK_CTRL)|(1), P_DDR1_CLK_CTRL);

	unsigned int nDDR0_BaseAddr,nDDR1_BaseAddr;
	unsigned int nMMC_DDR_SET  = readl(0xc8006000);
	unsigned int nPUB0_DTAR0   = readl(0xc80010b4);
	unsigned int nPUB1_DTAR0   = readl(0xc80030b4);
	serial_puts("\nAml log : nMMC_DDR_SET is 0x");
	serial_put_hex(nMMC_DDR_SET,32);
	serial_puts("\nAml log : nPUB0_DTAR0 is 0x");
	serial_put_hex(nPUB0_DTAR0,32);
	serial_puts("\nAml log : nPUB1_DTAR0 is 0x");
	serial_put_hex(nPUB1_DTAR0,32);

	switch( ((nMMC_DDR_SET >> 24) & 3))
	{
	case CONFIG_M8_DDR1_ONLY:
		{
			nDDR1_BaseAddr = CFG_M8_GET_DDR1_TA_FROM_DTAR0(nPUB1_DTAR0,nMMC_DDR_SET);
			serial_puts("\nAml log : DDR1 DTAR is 0x");
			serial_put_hex(nDDR1_BaseAddr,32);
			serial_puts("\n");

		}break;
	default:
		{
			nDDR0_BaseAddr = CFG_M8_GET_DDR0_TA_FROM_DTAR0(nPUB0_DTAR0,nMMC_DDR_SET);
			nDDR1_BaseAddr = CFG_M8_GET_DDR1_TA_FROM_DTAR0(nPUB1_DTAR0,nMMC_DDR_SET);

			serial_puts("\nAml log : DDR0 DTAR is 0x");
			serial_put_hex(nDDR0_BaseAddr,32);

			serial_puts("\nAml log : DDR1 DTAR is 0x");
			serial_put_hex(nDDR1_BaseAddr,32);
			serial_puts("\n");

		}break;
	}

	//disable clock
	writel(readl(P_DDR0_CLK_CTRL) & (~1), P_DDR0_CLK_CTRL);
	writel(readl(P_DDR1_CLK_CTRL) & (~1), P_DDR1_CLK_CTRL);

*/

#define CFG_M8_GET_DDR0_TA_FROM_DTAR0(dtar0_set,mmc_ddr_set) ((((dtar0_set) & 0xFFF) << 2) | \
(((((dtar0_set)>>28)&7) & ((((mmc_ddr_set) >> 5) & 0x1)? 3 : 1))<<((((mmc_ddr_set)&3)+8)+((((mmc_ddr_set) >> 24) & 0x3)? 0 : 1)+2))| \
	((((((dtar0_set)>>28)&7) >> ((((mmc_ddr_set) >> 5) & 0x1)+1)) & ((((mmc_ddr_set) >> 5) & 0x1)?1:3)) << ((((mmc_ddr_set)&3)+8)+\
		((((mmc_ddr_set)>>2)&3) ? ((((mmc_ddr_set)>>2)&3)+12) : (16))+2+((((mmc_ddr_set) >> 24) & 0x3)?0:1)+((((mmc_ddr_set) >> 5) & 0x1)+1)))|\
		((((dtar0_set) >> 12 ) & 0xFFFF)<<((((mmc_ddr_set)&3)+8)+2+((((mmc_ddr_set) >> 24) & 0x3)? 0:1)+((((mmc_ddr_set) >> 5) & 0x1)+1))))

#define CFG_M8_GET_DDR1_TA_FROM_DTAR0(dtar0_set,mmc_ddr_set) ((((dtar0_set) & 0xFFF) << 2) | (1<<((((mmc_ddr_set) >> 24) & 0x3) ? \
 (((((mmc_ddr_set) >> 24) & 0x3) == 2 ? 30 : 32)) : (12))) | \
	(((((dtar0_set)>>28)&7) & ((((mmc_ddr_set) >> 5) & 0x1)? 3 : 1))<<((((mmc_ddr_set)&3)+8)+((((mmc_ddr_set) >> 24) & 0x3)? 0 : 1)+2))| \
		((((((dtar0_set)>>28)&7) >> ((((mmc_ddr_set) >> 5) & 0x1)+1)) & ((((mmc_ddr_set) >> 5) & 0x1)?1:3)) << ((((mmc_ddr_set)&3)+8)+\
			((((mmc_ddr_set)>>2)&3) ? ((((mmc_ddr_set)>>2)&3)+12) : (16))+2+((((mmc_ddr_set) >> 24) & 0x3)?0:1)+((((mmc_ddr_set) >> 5) & 0x1)+1)))|\
			((((dtar0_set) >> 12 ) & 0xFFFF)<<((((mmc_ddr_set)&3)+8)+2+((((mmc_ddr_set) >> 24) & 0x3)? 0:1)+((((mmc_ddr_set) >> 5) & 0x1)+1))))


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//timer
#define CONFIG_SYS_HZ 1000

#define CONFIG_SYS_NO_FLASH 1
#define CONFIG_NR_DRAM_BANKS 1

#define CONFIG_BAUDRATE                 115200
#define CONFIG_SYS_BAUDRATE_TABLE       { 9600, 19200, 38400, 57600, 115200}
#define CONFIG_SERIAL_MULTI             1

#if 0
//no need to keep but for develop & verify
#define CONFIG_SYS_SDRAM_BASE   0x80000000
#define CONFIG_SYS_INIT_SP_ADDR (CONFIG_SYS_SDRAM_BASE+0xF00000)
#define CONFIG_SYS_TEXT_BASE    0x8F800000
#define CONFIG_MMU_DDR_SIZE     (0xc00)
#define CONFIG_SYS_LOAD_ADDR    0x82000000
#define CONFIG_DTB_LOAD_ADDR    0x83000000
#else
#define CONFIG_SYS_SDRAM_BASE   0x00000000
#define CONFIG_SYS_INIT_SP_ADDR (CONFIG_SYS_SDRAM_BASE+0xF00000)
#define CONFIG_SYS_TEXT_BASE    0x10000000
#define CONFIG_MMU_DDR_SIZE     ((PHYS_MEMORY_SIZE)>>20)
#define CONFIG_SYS_LOAD_ADDR    0x12000000
#define CONFIG_DTB_LOAD_ADDR    0x0f000000
#endif

#define CONFIG_SECURE_UBOOT_SIZE     0x100000

#define CONFIG_SYS_MALLOC_LEN   (12<<20)

#define CONFIG_SYS_MAXARGS      16
#define CONFIG_SYS_CBSIZE          1024
#define CONFIG_SYS_PBSIZE (CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16) /* Print Buffer Size */

#define CONFIG_VPU_PRESET		1

/** Timer relative Configuration **/
#define CONFIG_CRYSTAL_MHZ  24
/** Internal storage setting **/
//size Limitation
//#include "romboot.h"
//#warning todo implement CONFIG_BOARD_SIZE_LIMIT
//#define CONFIG_BOARD_SIZE_LIMIT 600000
#define IO_REGION_BASE                0xe0000000
#define CONFIG_SYS_CACHE_LINE_SIZE 32
#define CONFIG_CMD_CACHE	1
//#define CONFIG_SYS_NO_CP15_CACHE	1
//#define CONFIG_DCACHE_OFF    		1
//#define CONFIG_ICACHE_OFF    		1

//#define CONFIG_EFUSE 1

#ifdef CONFIG_CMD_NAND
	#define CONFIG_NAND_AML_M3 1
	#define CONFIG_NAND_AML  1
	#define CONFIG_NAND_AML_M8
	//#define CONFIG_SYS_MAX_NAND_DEVICE	1		/* Max number of */
	#define CONFIG_SYS_NAND_MAX_CHIPS	4
	#ifndef CONFIG_NAND_SP_BLOCK_SIZE
		#define CONFIG_NAND_SP_BLOCK_SIZE 32
	#endif
	//#warning todo implement nand driver later
	#define CONFIG_SYS_MAX_NAND_DEVICE  2  //make uboot happy
	#define CONFIG_SYS_NAND_BASE_LIST   {0}//make uboot happy
	//#define CONFIG_SYS_NAND_BASE 0 //make uboot happy
#endif

#ifdef CONFIG_CMD_SF
	#define CONFIG_AMLOGIC_SPI_FLASH    1
	#define CONFIG_SPI_FLASH            1
	#define SPI_FLASH_CACHELINE         64 //amlogic special setting. in M1 , SPI_A for SPI flash
	#define CONFIG_SPI_FLASH_MACRONIX   1
	#define CONFIG_SPI_FLASH_EON        1
	#define CONFIG_SPI_FLASH_SPANSION   1
	#define CONFIG_SPI_FLASH_SST        1
	#define CONFIG_SPI_FLASH_STMICRO    1
	#define CONFIG_SPI_FLASH_WINBOND    1
	#define CONFIG_SPI_FLASH_GIGADEVICE     1
#endif


#if CONFIG_SDIO_B1 || CONFIG_SDIO_A || CONFIG_SDIO_B || CONFIG_SDIO_C
	#define CONFIG_CMD_MMC          1
	#define CONFIG_MMC              1
	#define CONFIG_DOS_PARTITION    1
	#define CONFIG_AML_SDIO         1
	#define CONFIG_GENERIC_MMC      1
#endif

#if CONFIG_NAND_AML_M3 || CONFIG_AMLOGIC_SPI_FLASH
	#define CONFIG_MTD_DEVICE     1
	#define CONFIG_MTD_PARTITIONS 1
	#define CONFIG_CMD_MTDPARTS   1
#endif

/*
 * File system
 */
#define CONFIG_CMD_EXT2		/* EXT2 Support			*/
#define CONFIG_CMD_FAT		/* FAT support			*/


#define CONFIG_AML_ROMBOOT    1
#define SPI_MEM_BASE                                0xcc000000
#define AHB_SRAM_BASE                               0xd9000000  // AHB-SRAM-BASE
#define CONFIG_USB_SPL_ADDR                         (CONFIG_SYS_TEXT_BASE - (32<<10)) //here need update when support 64KB SPL
#define CONFIG_DDR_INIT_ADDR                        (0xd9000000) //usb driver limit, bit4 must 1, change 0xd9000000 as ACS hard coded to 0xd9000200

#if !defined(CONFIG_AML_DISABLE_CRYPTO_UBOOT)
	#define CONFIG_AML_SECU_BOOT_V2		1
	#define CONFIG_AML_CRYPTO_UBOOT		1
#endif //CONFIG_AML_DISABLE_CRYPTO_UBOOT

#ifdef CONFIG_AML_ROMBOOT_SPL
	#define SPL_STATIC_FUNC     static
	#define SPL_STATIC_VAR      static
#else
	#define SPL_STATIC_FUNC
	#define SPL_STATIC_VAR
#endif

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1
#define CONFIG_REVISION_TAG		1
#define CONFIG_CMD_KGDB			1
////#define CONFIG_SERIAL_TAG       1*/

//#define CONFIG_AML_RTC
//#define CONFIG_RTC_DAY_TEST 1  // test RTC run 2 days

#define CONFIG_LZMA  1
#define CONFIG_LZO
#define CONFIG_DISABLE_INTERNAL_U_BOOT_CHECK
/*default command select*/
#define CONFIG_CMD_MEMORY	1 /* md mm nm mw cp cmp crc base loop mtest */
//support "bdinfo"
#define CONFIG_CMD_BDI 1
//support "coninfo"
#define CONFIG_CMD_CONSOLE 1
//support "echo"
#define CONFIG_CMD_ECHO 1
//support "loadb,loads,loady"
#define CONFIG_CMD_LOADS 1
#define CONFIG_CMD_LOADB 1
//support "run"
#define CONFIG_CMD_RUN 1
//support "true,false,test"
//#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser */
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
//#define CONFIG_SYS_PROMPT		"8726M_ref # "
//#define CONFIG_SYS_CBSIZE		256	/* Console I/O Buffer Size */
//support "imxtract"
#define CONFIG_CMD_XIMG 1
//support "itest"
#define CONFIG_CMD_ITEST 1
//support "sleep"
#define CONFIG_CMD_MISC 1
//support "source"
#define CONFIG_SOURCE 1
#define CONFIG_CMD_SOURCE 1
//support "editenv"
#define CONFIG_CMD_EDITENV 1
#define CONFIG_CMD_CALINFO 1
/*default command select end*/

//max watchdog timer: 41.943s
#define AML_WATCHDOG_TIME_SLICE				10	//us
#define AML_WATCHDOG_ENABLE_OFFSET			22
#define AML_WATCHDOG_CPU_RESET_CNTL			0xf
#define AML_WATCHDOG_CPU_RESET_OFFSET		24

#define MESON_CPU_TYPE	MESON_CPU_TYPE_MESON8
#define CONFIG_AML_SMP

//support gpio cmd
#define CONFIG_AML_GPIO_CMD 1
#define CONFIG_AML_GPIO 1

#endif /* _CPU_H */
