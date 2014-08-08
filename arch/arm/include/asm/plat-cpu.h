#ifndef __ARCH_MESON_PLAT_CPU_H__
#define __ARCH_MESON_PLAT_CPU_H__

#define MESON_CPU_TYPE_MESON1		0x10
#define MESON_CPU_TYPE_MESON2		0x20
#define MESON_CPU_TYPE_MESON3		0x30
#define MESON_CPU_TYPE_MESON6		0x60
#define MESON_CPU_TYPE_MESON6TV		0x70
#define MESON_CPU_TYPE_MESON6TVD	0x75
#define MESON_CPU_TYPE_MESON8		0x80
#define MESON_CPU_TYPE_MESON8B		0x8B

//set watchdog timer by ms
#define AML_WATCH_DOG_SET(time) \
	writel(0, P_WATCHDOG_RESET); \
	writel((((int)(time * 1000 / AML_WATCHDOG_TIME_SLICE)) | \
	(1<<AML_WATCHDOG_ENABLE_OFFSET) | \
	(AML_WATCHDOG_CPU_RESET_CNTL<<AML_WATCHDOG_CPU_RESET_OFFSET)), P_WATCHDOG_TC);

//disable watchdog
#define AML_WATCH_DOG_DISABLE() \
	writel(0, P_WATCHDOG_TC); \
	writel(0, P_WATCHDOG_RESET);

//start watchdog immediately
#define AML_WATCH_DOG_START() \
	do{ \
		writel(0, P_WATCHDOG_RESET); \
		writel((10|((1<<AML_WATCHDOG_ENABLE_OFFSET)| \
			(AML_WATCHDOG_CPU_RESET_CNTL<<AML_WATCHDOG_CPU_RESET_OFFSET))), P_WATCHDOG_TC); \
		while(1); \
	}while(0);

#endif
