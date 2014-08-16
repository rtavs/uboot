/*
 * Command for Amlogic Custom.
 *
 * Copyright (C) 2012 Amlogic.
 * Elvis Yu <elvis.yu@amlogic.com>
 */

#include <common.h>
#include <command.h>

#ifdef CONFIG_PLATFORM_HAS_PMU
#include <amlogic/aml_pmu_common.h>
#endif

extern inline void key_init(void);
extern inline int get_key(void);
extern inline int is_ac_online(void);
extern void power_off(void);
extern inline int get_charging_percent(void);
extern inline int set_charging_current(int current);


static int do_getkey (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static int key_enable = 0;
	int key_value = 0;
	if(!key_enable){
		key_init();
		key_enable = 1;
		//wait ready for gpio power key,  frank.chen
		mdelay(20);
	}
	key_value = get_key();
	return !key_value;
}


U_BOOT_CMD(
	getkey,	1,	0,	do_getkey,
	"get POWER key",
	"/N\n"
	"This command will get POWER key'\n"
);

static inline int do_ac_online (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_PLATFORM_HAS_PMU
    struct aml_pmu_driver *pmu_driver;
    pmu_driver = aml_pmu_get_driver();
    if (pmu_driver && pmu_driver->pmu_get_extern_power_status) {
        return !pmu_driver->pmu_get_extern_power_status();
    } else {
        printf("ERROR!! No pmu_get_battery_capacity hooks!\n");
        return 0;
    }
#else
	return !is_ac_online();
#endif
}


U_BOOT_CMD(
	ac_online,	1,	0,	do_ac_online,
	"get ac adapter online",
	"/N\n"
	"This command will get ac adapter online'\n"
);


static int do_poweroff (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_PLATFORM_HAS_PMU
    struct aml_pmu_driver *pmu_driver;
    pmu_driver = aml_pmu_get_driver();
    if (pmu_driver && pmu_driver->pmu_power_off) {
        pmu_driver->pmu_power_off();
    } else {
        printf("ERROR!! NO power off hooks!\n");
    }
#else
	power_off();
#endif
	return 0;
}


U_BOOT_CMD(
	poweroff,	1,	0,	do_poweroff,
	"system power off",
	"/N\n"
	"This command will let system power off'\n"
);

static int do_get_batcap (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char percent_str[16];
	int percent = 0;
#ifdef CONFIG_PLATFORM_HAS_PMU
    struct aml_pmu_driver *pmu_driver;
    pmu_driver = aml_pmu_get_driver();
    if (pmu_driver && pmu_driver->pmu_get_battery_capacity) {
        percent = pmu_driver->pmu_get_battery_capacity();
    } else {
        printf("ERROR!! No pmu_get_battery_capacity hooks!\n");
    }
#else
	percent = get_charging_percent();
#endif
	printf("Battery CAP: %d%%\r", percent);
	sprintf(percent_str, "%d", percent);
	setenv("battery_cap", percent_str);
	return 0;
}


U_BOOT_CMD(
	get_batcap,	1,	0,	do_get_batcap,
	"get battery capability",
	"/N\n"
	"This command will get battery capability\n"
	"capability will set to 'battery_cap'\n"
);

static int do_set_chgcur (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int current = simple_strtol(argv[1], NULL, 10);

#ifdef CONFIG_PLATFORM_HAS_PMU
    struct aml_pmu_driver *pmu_driver;
    pmu_driver = aml_pmu_get_driver();
    if (pmu_driver && pmu_driver->pmu_set_charge_current) {
        pmu_driver->pmu_set_charge_current(current);
	setenv("charging_current", argv[1]);
    } else {
        printf("ERROR!! No pmu_set_charge_current hooks!\n");
    }
#else
	set_charging_current(current);

	printf("Charging current: %smA\n", argv[1]);
	setenv("charging_current", argv[1]);
#endif
	return 0;
}


U_BOOT_CMD(
	set_chgcur,	2,	0,	do_set_chgcur,
	"set battery charging current",
	"/N\n"
	"set_chgcur <current>\n"
	"unit is mA\n"
);

/* set  usb current limit */

static int pmu_set_usbcur_limit(int usbcur_limit)
{

}
static int do_set_usbcur_limit(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int usbcur_limit = simple_strtol(argv[1], NULL, 10);
    if (argc < 2)
        return cmd_usage(cmdtp);
#ifdef CONFIG_PLATFORM_HAS_PMU
    struct aml_pmu_driver *pmu_driver;
    pmu_driver = aml_pmu_get_driver();
    if (pmu_driver && pmu_driver->pmu_set_usb_current_limit) {
        pmu_driver->pmu_set_usb_current_limit(usbcur_limit);
        printf("set usbcur_limit: %smA\n", argv[1]);
        if (argc == 2 ) {
            setenv("usbcur_limit", argv[1]);
        }
    } else {
        printf("ERROR!! No pmu_set_usb_current_limit hooks!\n");
    }
#else
    pmu_set_usbcur_limit(usbcur_limit);
    printf("set usbcur_limit: %smA\n", argv[1]);
    if (argc == 2 )
		{
    setenv("usbcur_limit", argv[1]);
		}
#endif
    return 0;
}
U_BOOT_CMD(
	set_usbcur_limit,	3,	0,	do_set_usbcur_limit,
	"set pmu usb limit current",
	"/N\n"
	"set_usbcur_limit <current>\n"
	"unit is mA\n"
);

static int do_gettime (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int time = get_utimer(0);
    printf("***from powerup: %d us.\n", time);
	return time;
}


U_BOOT_CMD(
	time,	1,	0,	do_gettime,
	"get bootup time",
	"/N\n"
);
