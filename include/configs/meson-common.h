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

#define CONFIG_SPL_TEXT_BASE		RAM_START
#define CONFIG_SPL_STACK		ROM_STACK_END
#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_HW_WATCHDOG
#define CONFIG_HW_WATCHDOG_TIMEOUT_MS	5000

#define CONFIG_SPL_DISPLAY_PRINT

//#define CONFIG_SYS_NO_FLASH
//#define CONFIG_NR_DRAM_BANKS		1
//#define CONFIG_ENV_SIZE			(64*1024)

#endif /* _MESON_COMMON_CONFIG_H */
