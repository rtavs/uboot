/*
 * (C) Copyright Carlo Caione <carlo@caione.org>
 *
 * Configuration settings for the Amlogic meson series of boards.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _MESON_COMMON_CONFIG_H
#define _MESON_COMMON_CONFIG_H

#include <linux/stringify.h>

/* Meson specific defines */

#define RAM_START			0xd9000000
#define RAM_SIZE			0x20000
#define RAM_END				RAM_START+RAM_SIZE
#define MEMORY_LOC			RAM_START
#define ROMBOOT_START			0xd9800000
#define ROM_SIZE			(16*1024)
#define ROMBOOT_END			(ROMBOOT_START+ROM_SIZE)
#define GL_DATA_ADR			(RAM_END-256)
#define READ_SIZE			(32*1024)
#define CHECK_SIZE			(8*1024)
#define ROM_STACK_END			(GL_DATA_ADR)


#define CONFIG_SPL_TEXT_BASE		0xd9000000      /* same as RAM_START */
#define CONFIG_SPL_STACK		ROM_STACK_END

#define CONFIG_SKIP_LOWLEVEL_INIT



#define CONFIG_SPL_DISPLAY_PRINT

#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT

#define CONFIG_SPL_SERIAL_SUPPORT
/*#define CONFIG_SPL_I2C_SUPPORT */
#define CONFIG_SPL_MMC_SUPPORT
#define CONFIG_SPL_FAT_SUPPORT
#define CONFIG_SPL_EXT_SUPPORT
#define CONFIG_SPL_WATCHDOG_SUPPORT
/**************************************************************/



#define CONFIG_SYS_HZ 1000

#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_ENV_OFFSET		(544 << 10) /* (8 + 24 + 512) KiB */
#define CONFIG_ENV_SIZE			(64*1024)


/* 64MB of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (64 << 20))

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_CBSIZE	1024	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	1024	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_GENERIC_BOARD


/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

/* baudrate */
#define CONFIG_BAUDRATE			115200

/* The stack sizes are set up in start.S using the settings below */
#define CONFIG_STACKSIZE		(256 << 10)	/* 256 KiB */

/* FLASH and environment organization */

#define CONFIG_SYS_NO_FLASH



#define CONFIG_HW_WATCHDOG
#ifdef CONFIG_HW_WATCHDOG
#define CONFIG_HW_WATCHDOG_TIMEOUT_MS	2000
#define CONFIG_MESON_WATCHDOG
#endif


#if 1

#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0	/* first detected MMC controller */

#else

#define CONFIG_ENV_IS_NOWHERE

#endif


#ifndef CONFIG_SPL_BUILD
#include <config_distro_defaults.h>
#endif


#endif /* _MESON_COMMON_CONFIG_H */
