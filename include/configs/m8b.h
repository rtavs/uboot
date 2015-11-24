#ifndef __CONFIG_H
#define __CONFIG_H




#define CONFIG_SYS_TEXT_BASE    0x10000000

#define CONFIG_SYS_LOAD_ADDR		0x12000000 /* default load address */


#define CONFIG_SYS_SDRAM_BASE   0x00000000
#define CONFIG_SYS_INIT_SP_ADDR (CONFIG_SYS_SDRAM_BASE+0xF00000)


#define CONFIG_MMU_DDR_SIZE     ((PHYS_MEMORY_SIZE)>>20)

#ifdef CONFIG_DDR_SIZE_AUTO_DETECT
#undef CONFIG_MMU_DDR_SIZE
#define CONFIG_MMU_DDR_SIZE    ((0x80000000)>>20)	//max 2GB
#endif

#define CONFIG_DTB_LOAD_ADDR    0x0f000000


/**************************************************************/
/**************************************************************/
/* Serial & console */
#define CONFIG_MESON_SERIAL             1
#define CONFIG_CONS_INDEX               2       /* UART_AO */

/* baudrate */
#define CONFIG_BAUDRATE			        115200
#define CONFIG_SYS_BAUDRATE_TABLE       { 9600, 19200, 38400, 57600, 115200}


/**************************************************************/






#define CONFIG_FAT_WRITE

#if 0
#define CONFIG_DISPLAY_BOARDINFO
#define CONFIG_DISPLAY_BOARDINFO_LATE
#endif


#include <configs/meson-common.h>

#endif /* __CONFIG_H */
