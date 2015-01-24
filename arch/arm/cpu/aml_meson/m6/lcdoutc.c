/*
 * AMLOGIC lcd controller driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Author:  Elvis Yu <elvis.yu@amlogic.com>
 *
 * Modify:  Evoke Zhang <evoke.zhang@amlogic.com>
 * compatible dts
 *
 */
#include <common.h>
#include <malloc.h>
#include <asm/arch/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>
#include <asm/arch/vinfo.h>
#include <asm/arch/lcdoutc.h>
#include <asm/arch/lcd_reg.h>
#include <amlogic/aml_lcd.h>
#include <amlogic/vinfo.h>
#include <asm/arch/mlvds_regs.h>
#include <asm/arch/clock.h>
#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif
#ifdef CONFIG_AML_LCD_EXTERN
#include <amlogic/aml_lcd_extern.h>
#endif
#ifdef CONFIG_AML_BL_EXTERN
#include <amlogic/aml_bl_extern.h>
#endif
#ifdef CONFIG_PLATFORM_HAS_PMU
#include <amlogic/aml_pmu_common.h>
#define BATTERY_LOW_THRESHOLD       20
#endif

#define PANEL_NAME		"panel"
#define DRV_TYPE		"c6"
#define DRIVER_DATE		"20140325"

#define VPP_OUT_SATURATE            (1 << 0)

//#define LCD_DEBUG_INFO
#ifdef LCD_DEBUG_INFO
#define DBG_PRINT(...)		printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

typedef struct {
	Lcd_Config_t *pConf;
	Lcd_Bl_Config_t *bl_config;
	vinfo_t lcd_info;
} lcd_dev_t;

static lcd_dev_t *pDev = NULL;
static char * dt_addr;
static int dts_ready = 0;

vidinfo_t panel_info = {
	.vl_col	=	0,		/* Number of columns (i.e. 160) */
	.vl_row	=	0,		/* Number of rows (i.e. 100) */

	.vl_bpix	=	0,				/* Bits per pixel */

	.vd_console_address	=	NULL,	/* Start of console buffer */
	.console_col	=	0,
	.console_row	=	0,

	.vd_color_fg	=	0,
	.vd_color_bg	=	0,
	.max_bl_level	=	255,

	.cmap	=	NULL,		/* Pointer to the colormap */

	.priv	=	NULL,		/* Pointer to driver-specific data */
};

static unsigned bl_level;

static Lcd_Bl_Config_t bl_config = {
	.level_default = BL_LEVEL_DFT,
	.level_mid = BL_LEVEL_MID_DFT,
	.level_mid_mapping = BL_LEVEL_MID_MAPPED_DFT,
	.level_min = BL_LEVEL_MIN_DFT,
	.level_max = BL_LEVEL_MAX_DFT,
};

static LVDS_Config_t lcd_lvds_config = {
	.lvds_vswing = 1,
	.lvds_repack_user = 0,
	.lvds_repack = 0,
	.pn_swap = 0,
};

static TTL_Config_t lcd_ttl_config = {
	.rb_swap = 0,
	.bit_swap = 0,
};

static Lcd_Config_t lcd_config = {
	.lcd_timing = {
		.lcd_clk = 40000000,
		.clk_ctrl = (1<<CLK_CTRL_AUTO) | (1<<CLK_CTRL_VCLK_SEL) | (7<<CLK_CTRL_XD),
		.video_on_pixel = 120,
		.video_on_line = 32,
		.hvsync_valid = 1,
		.de_valid = 1,
		.pol_cntl_addr = (0 << LCD_CPH1_POL) |(0 << LCD_HS_POL) | (0 << LCD_VS_POL),
		.inv_cnt_addr = (0 << LCD_INV_EN) | (0 << LCD_INV_CNT),
		.tcon_misc_sel_addr = (1 << LCD_STV1_SEL) | (1 << LCD_STV2_SEL),
	},
	.lcd_effect = {
		.gamma_cntl_port = (1 << LCD_GAMMA_EN),
		.rgb_base_addr = 0xf0,
		.rgb_coeff_addr = 0x74a,
		.dith_user = 0,
		.vadj_brightness = 0x0,
		.vadj_contrast = 0x80,
		.vadj_saturation = 0x100,
		.gamma_revert = 0,
		.gamma_r_coeff = 100,
		.gamma_g_coeff = 100,
		.gamma_b_coeff = 100,
	},
	.lcd_control = {
		.lvds_config = &lcd_lvds_config,
		.ttl_config = &lcd_ttl_config,
	},
	.lcd_power_ctrl = {
		.power_on_step = 0,
		.power_off_step = 0,
	},
};

static void lcd_setup_gamma_table(Lcd_Config_t *pConf, unsigned int rgb_flag)
{
	int i;

	const unsigned short gamma_adjust[256] = {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
		32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
		64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
		96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
		128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
		160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
		192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
		224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
	};

	if (rgb_flag == 0) {	//r
		for (i=0; i<256; i++) {
			pConf->lcd_effect.GammaTableR[i] = gamma_adjust[i] << 2;
		}
	}
	else if (rgb_flag == 1) {	//g
		for (i=0; i<256; i++) {
			pConf->lcd_effect.GammaTableG[i] = gamma_adjust[i] << 2;
		}
	}
	else if (rgb_flag == 2) {	//b
		for (i=0; i<256; i++) {
			pConf->lcd_effect.GammaTableB[i] = gamma_adjust[i] << 2;
		}
	}
	else if (rgb_flag == 3) {	//rgb
		for (i=0; i<256; i++) {
			pConf->lcd_effect.GammaTableR[i] = gamma_adjust[i] << 2;
			pConf->lcd_effect.GammaTableG[i] = gamma_adjust[i] << 2;
			pConf->lcd_effect.GammaTableB[i] = gamma_adjust[i] << 2;
		}
	}
}

static void lcd_ports_ctrl_lvds(Bool_t status)
{
	if (status) {
		WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 1, 3, 1); // enable fifo
		if (pDev->pConf->lcd_basic.lcd_bits == 6)
			WRITE_LCD_REG_BITS(LVDS_PHY_CNTL4, 0x27, 0, 6);	//enable LVDS phy 3 channels
		else
			WRITE_LCD_REG_BITS(LVDS_PHY_CNTL4, 0x2f, 0, 6);	//enable LVDS phy 4 channels
	}else {
		WRITE_LCD_REG_BITS(LVDS_PHY_CNTL3, 0, 0, 1);
		WRITE_LCD_REG_BITS(LVDS_PHY_CNTL5, 0, 11, 1);  //shutdown lvds phy
		WRITE_LCD_REG_BITS(LVDS_PHY_CNTL4, 0x0, 0, 6);  //disable LVDS phy port
		WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 0, 3, 1); // disable fifo
	}
	DBG_PRINT("%s: %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static void lcd_ports_ctrl_ttl(Bool_t status)
{
	unsigned pinmux_tcon, pinmux_data;
	unsigned gpio_tcon, gpio_data;

	pinmux_tcon = (1 << 14);	//clk
	gpio_tcon = (1 << 23);	//clk_gpio
	if (pDev->pConf->lcd_timing.de_valid == 1)
		pinmux_tcon |= (1 << 17);
		gpio_tcon |= (1 << 20);

	if (pDev->pConf->lcd_timing.hvsync_valid == 1)
		pinmux_tcon |= (3 << 18);
		gpio_tcon |= (3 << 18);

	if (pDev->pConf->lcd_basic.lcd_bits == 6) {
		pinmux_data = (1 << 0) | (1 << 2) | (1 << 4);
		gpio_data = (0xfc << 0) | (0xfc << 8) | (0xfc << 16);
	}
	else {
		pinmux_data = (3 << 0) | (3 << 2) | (3 << 4);
		gpio_data = (0xff << 0) | (0xff << 8) | (0xff << 16);
	}

	if (status) {
		WRITE_LCD_CBUS_REG(PERIPHS_PIN_MUX_1, (READ_LCD_CBUS_REG(PERIPHS_PIN_MUX_1) | pinmux_tcon));
		WRITE_LCD_CBUS_REG(PERIPHS_PIN_MUX_0, (READ_LCD_CBUS_REG(PERIPHS_PIN_MUX_0) | pinmux_data));
	}else {
		WRITE_LCD_CBUS_REG(PERIPHS_PIN_MUX_1, (READ_LCD_CBUS_REG(PERIPHS_PIN_MUX_1) & ~(pinmux_tcon)));
		WRITE_LCD_CBUS_REG(PERIPHS_PIN_MUX_0, (READ_LCD_CBUS_REG(PERIPHS_PIN_MUX_0) & ~(pinmux_data)));
		WRITE_LCD_CBUS_REG(PREG_PAD_GPIO2_EN_N, (READ_LCD_CBUS_REG(PREG_PAD_GPIO2_EN_N) | gpio_tcon));
		WRITE_LCD_CBUS_REG(PREG_PAD_GPIO1_EN_N, (READ_LCD_CBUS_REG(PREG_PAD_GPIO1_EN_N) | gpio_data));
	}
	DBG_PRINT("%s: %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static void lcd_ports_ctrl_minilvds(Bool_t status)
{
	unsigned pinmux_reg, pinmux_bit;
	unsigned n, i;

	pinmux_reg = pDev->pConf->lcd_control.mlvds_config->mlvds_pinmux->tcon_pinmux[0] + PERIPHS_PIN_MUX_0;
	pinmux_bit = pDev->pConf->lcd_control.mlvds_config->mlvds_pinmux->tcon_pinmux[1];
	n = ARRAY_SIZE(pDev->pConf->lcd_control.mlvds_config->mlvds_pinmux->tcon_pinmux_pins);

	if (status) {
		WRITE_LCD_CBUS_REG(pinmux_reg, (READ_LCD_CBUS_REG(pinmux_reg) | pinmux_bit));
		aml_lcd_gpio_set(pDev->pConf->lcd_control.mlvds_config->mlvds_pinmux->tcon_gpio[0], pDev->pConf->lcd_control.mlvds_config->mlvds_pinmux->tcon_gpio[1]);
	}else {
		WRITE_LCD_CBUS_REG(pinmux_reg, (READ_LCD_CBUS_REG(pinmux_reg) & ~(pinmux_bit)));
		for (i=0; i<n; i++) {
			aml_lcd_gpio_set(pDev->pConf->lcd_control.mlvds_config->mlvds_pinmux->tcon_pinmux_pins[i], LCD_POWER_GPIO_INPUT);
		}
		aml_lcd_gpio_set(pDev->pConf->lcd_control.mlvds_config->mlvds_pinmux->tcon_gpio[0], LCD_POWER_GPIO_INPUT);
	}
	DBG_PRINT("%s: %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static void lcd_ports_ctrl(Bool_t status)
{
	switch(pDev->pConf->lcd_basic.lcd_type){
		case LCD_DIGITAL_LVDS:
			lcd_ports_ctrl_lvds(status);
			break;
		case LCD_DIGITAL_TTL:
			lcd_ports_ctrl_ttl(status);
			break;
		case LCD_DIGITAL_MINILVDS:
			lcd_ports_ctrl_minilvds(status);
			break;
		default:
			printf("Invalid LCD type.\n");
			break;
	}
}

#ifdef CONFIG_AML_BL_EXTERN
void get_bl_level(struct bl_extern_config_t *bl_ext_cfg)
{
    bl_ext_cfg->level_min = pDev->bl_config->level_min;
    bl_ext_cfg->level_max = pDev->bl_config->level_max;
}
#endif

static void lcd_backlight_power_ctrl(Bool_t status)
{
	int i, ret;
#ifdef CONFIG_AML_BL_EXTERN
	struct aml_bl_extern_driver_t *bl_extern_driver;
#endif

	if( status == ON ) {
		mdelay(pDev->bl_config->power_on_delay);

		switch (pDev->bl_config->method) {
			case BL_CTL_GPIO:
				WRITE_LCD_CBUS_REG_BITS(LED_PWM_REG0, 1, 12, 2);
				mdelay(20);
				aml_lcd_gpio_set(pDev->bl_config->gpio, LCD_POWER_GPIO_OUTPUT_HIGH);
				break;
			case BL_CTL_PWM_NEGATIVE:
			case BL_CTL_PWM_POSITIVE:
				switch (pDev->bl_config->pwm_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, (READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~(0x7f<<8)) | ((1 << 15) | (pDev->bl_config->pwm_pre_div<<8) | (1<<0)));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, (READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~(0x7f<<16)) | ((1 << 23) | (pDev->bl_config->pwm_pre_div<<16) | (1<<1)));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, (READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~(0x7f<<8)) | ((1 << 15) | (pDev->bl_config->pwm_pre_div<<8) | (1<<0)));  //enable pwm clk & pwm output
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, (READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~(0x7f<<16)) | ((1 << 23) | (pDev->bl_config->pwm_pre_div<<16) | (1<<1)));  //enable pwm clk & pwm output
						break;
					default:
						break;
				}

				for (i=0; i<pDev->bl_config->pinmux_clr_num; i++) {
					clear_mio_mux(pDev->bl_config->pinmux_clr[i][0], pDev->bl_config->pinmux_clr[i][1]);
				}
				for (i=0; i<pDev->bl_config->pinmux_set_num; i++) {
					set_mio_mux(pDev->bl_config->pinmux_set[i][0], pDev->bl_config->pinmux_set[i][1]);
				}
				mdelay(20);
				if (pDev->bl_config->pwm_gpio_used)
					aml_lcd_gpio_set(pDev->bl_config->gpio, LCD_POWER_GPIO_OUTPUT_HIGH);
				break;
			case BL_CTL_PWM_COMBO:
				switch (pDev->bl_config->combo_high_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, (READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~(0x7f<<8)) | ((1 << 15) | (pDev->bl_config->combo_high_pre_div<<8) | (1<<0)));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, (READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~(0x7f<<16)) | ((1 << 23) | (pDev->bl_config->combo_high_pre_div<<16) | (1<<1)));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, (READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~(0x7f<<8)) | ((1 << 15) | (pDev->bl_config->combo_high_pre_div<<8) | (1<<0)));  //enable pwm clk & pwm output
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, (READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~(0x7f<<16)) | ((1 << 23) | (pDev->bl_config->combo_high_pre_div<<16) | (1<<1)));  //enable pwm clk & pwm output
						break;
					default:
						break;
				}
				switch (pDev->bl_config->combo_low_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, (READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~(0x7f<<8)) | ((1 << 15) | (pDev->bl_config->combo_low_pre_div<<8) | (1<<0)));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, (READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~(0x7f<<16)) | ((1 << 23) | (pDev->bl_config->combo_low_pre_div<<16) | (1<<1)));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, (READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~(0x7f<<8)) | ((1 << 15) | (pDev->bl_config->combo_low_pre_div<<8) | (1<<0)));  //enable pwm clk & pwm output
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, (READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~(0x7f<<16)) | ((1 << 23) | (pDev->bl_config->combo_low_pre_div<<16) | (1<<1)));  //enable pwm clk & pwm output
						break;
					default:
						break;
				}
				for (i=0; i<pDev->bl_config->pinmux_clr_num; i++) {
					clear_mio_mux(pDev->bl_config->pinmux_clr[i][0], pDev->bl_config->pinmux_clr[i][1]);
				}
				for (i=0; i<pDev->bl_config->pinmux_set_num; i++) {
					set_mio_mux(pDev->bl_config->pinmux_set[i][0], pDev->bl_config->pinmux_set[i][1]);
				}
				break;
			case BL_CTL_EXTERN:
#ifdef CONFIG_AML_BL_EXTERN
				bl_extern_driver = aml_bl_extern_get_driver();
				if (bl_extern_driver == NULL) {
					printf("no bl_extern driver\n");
				}
				else {
					if (bl_extern_driver->power_on) {
						ret = bl_extern_driver->power_on();
						if (ret) {
							printf("[bl_extern] power on error\n");
						}
					}
					else {
						printf("[bl_extern] power on is null\n");
					}
				}
#endif
				break;
			default:
				printf("Wrong backlight control method\n");
				break;
		}
	}
	else {
		switch (pDev->bl_config->method) {
			case BL_CTL_GPIO:
				aml_lcd_gpio_set(pDev->bl_config->gpio, LCD_POWER_GPIO_OUTPUT_LOW);
				break;
			case BL_CTL_PWM_NEGATIVE:
			case BL_CTL_PWM_POSITIVE:
				if (pDev->bl_config->pwm_gpio_used) {
					if (pDev->bl_config->gpio)
						aml_lcd_gpio_set(pDev->bl_config->gpio, LCD_POWER_GPIO_OUTPUT_LOW);
				}
				switch (pDev->bl_config->pwm_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~((1 << 15) | (1<<0)));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~((1 << 23) | (1<<1)));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~((1 << 15) | (1<<0)));  //disable pwm_clk & pwm port
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~((1 << 23) | (1<<1)));  //disable pwm_clk & pwm port
						break;
					default:
						break;
				}
				break;
			case BL_CTL_PWM_COMBO:
				switch (pDev->bl_config->combo_high_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~((1 << 15) | (1<<0)));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~((1 << 23) | (1<<1)));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~((1 << 15) | (1<<0)));  //disable pwm_clk & pwm port
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~((1 << 23) | (1<<1)));  //disable pwm_clk & pwm port
						break;
					default:
						break;
				}
				switch (pDev->bl_config->combo_low_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~((1 << 15) | (1<<0)));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_AB, READ_LCD_CBUS_REG(PWM_MISC_REG_AB) & ~((1 << 23) | (1<<1)));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~((1 << 15) | (1<<0)));  //disable pwm_clk & pwm port
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_MISC_REG_CD, READ_LCD_CBUS_REG(PWM_MISC_REG_CD) & ~((1 << 23) | (1<<1)));  //disable pwm_clk & pwm port
						break;
					default:
						break;
				}
				break;
			case BL_CTL_EXTERN:
#ifdef CONFIG_AML_BL_EXTERN
				bl_extern_driver = aml_bl_extern_get_driver();
				if (bl_extern_driver == NULL) {
					printf("no bl_extern driver\n");
				}
				else {
					if (bl_extern_driver->power_off) {
						ret = bl_extern_driver->power_off();
						if (ret) {
							printf("[bl_extern] power off error\n");
						}
					}
					else {
						printf("[bl_extern] power off is null\n");
					}
				}
#endif
				break;
			default:
				printf("Wrong backlight control method\n");
				break;
		}
	}
	printf("%s(): %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static void set_lcd_backlight_level(unsigned level)
{
	unsigned pwm_hi = 0, pwm_lo = 0;
#ifdef CONFIG_AML_BL_EXTERN
	struct aml_bl_extern_driver_t *bl_extern_driver;
#endif
	int ret;

	DBG_PRINT("set_backlight_level: %u, last level: %u\n", level, bl_level);
	level = (level > pDev->bl_config->level_max ? pDev->bl_config->level_max : (level < pDev->bl_config->level_min ? pDev->bl_config->level_min : level));
	bl_level = level;

	//mapping
	if (level > pDev->bl_config->level_mid)
		level = ((level - pDev->bl_config->level_mid) * (pDev->bl_config->level_max - pDev->bl_config->level_mid_mapping)) / (pDev->bl_config->level_max - pDev->bl_config->level_mid) + pDev->bl_config->level_mid_mapping;
	else
		level = ((level - pDev->bl_config->level_min) * (pDev->bl_config->level_mid_mapping - pDev->bl_config->level_min)) / (pDev->bl_config->level_mid - pDev->bl_config->level_min) + pDev->bl_config->level_min;

	switch (pDev->bl_config->method) {
		case BL_CTL_GPIO:
			level = pDev->bl_config->dim_min - ((level - pDev->bl_config->level_min) * (pDev->bl_config->dim_min - pDev->bl_config->dim_max)) / (pDev->bl_config->level_max - pDev->bl_config->level_min);
			WRITE_LCD_CBUS_REG_BITS(LED_PWM_REG0, level, 0, 4);
			break;
		case BL_CTL_PWM_NEGATIVE:
		case BL_CTL_PWM_POSITIVE:
			level = (pDev->bl_config->pwm_max - pDev->bl_config->pwm_min) * (level - pDev->bl_config->level_min) / (pDev->bl_config->level_max - pDev->bl_config->level_min) + pDev->bl_config->pwm_min;
			if (pDev->bl_config->method == BL_CTL_PWM_POSITIVE) {
				pwm_hi = level;
				pwm_lo = pDev->bl_config->pwm_cnt - level;
			}
			else if (pDev->bl_config->method == BL_CTL_PWM_NEGATIVE) {
				pwm_hi = pDev->bl_config->pwm_cnt - level;
				pwm_lo = level;
			}

			switch (pDev->bl_config->pwm_port) {
				case BL_PWM_A:
					WRITE_LCD_CBUS_REG(PWM_PWM_A, (pwm_hi << 16) | (pwm_lo));
					break;
				case BL_PWM_B:
					WRITE_LCD_CBUS_REG(PWM_PWM_B, (pwm_hi << 16) | (pwm_lo));
					break;
				case BL_PWM_C:
					WRITE_LCD_CBUS_REG(PWM_PWM_C, (pwm_hi << 16) | (pwm_lo));
					break;
				case BL_PWM_D:
					WRITE_LCD_CBUS_REG(PWM_PWM_D, (pwm_hi << 16) | (pwm_lo));
					break;
				default:
					break;
			}
			break;
		case BL_CTL_PWM_COMBO:
			if (level >= pDev->bl_config->combo_level_switch) {
				//pre_set combo_low duty max
				if (pDev->bl_config->combo_low_method == BL_CTL_PWM_NEGATIVE) {
					pwm_hi = pDev->bl_config->combo_low_cnt - pDev->bl_config->combo_low_duty_max;
					pwm_lo = pDev->bl_config->combo_low_duty_max;
				}
				else {
					pwm_hi = pDev->bl_config->combo_low_duty_max;
					pwm_lo = pDev->bl_config->combo_low_cnt - pDev->bl_config->combo_low_duty_max;
				}
				switch (pDev->bl_config->combo_low_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_PWM_A, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_PWM_B, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_PWM_C, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_PWM_D, (pwm_hi << 16) | (pwm_lo));
						break;
					default:
						break;
				}

				//set combo_high duty
				level = (pDev->bl_config->combo_high_duty_max - pDev->bl_config->combo_high_duty_min) * (level - pDev->bl_config->combo_level_switch) / (pDev->bl_config->level_max - pDev->bl_config->combo_level_switch) + pDev->bl_config->combo_high_duty_min;
				if (pDev->bl_config->combo_high_method == BL_CTL_PWM_NEGATIVE) {
					pwm_hi = pDev->bl_config->combo_high_cnt - level;
					pwm_lo = level;
				}
				else {
					pwm_hi = level;
					pwm_lo = pDev->bl_config->combo_high_cnt - level;
				}
				switch (pDev->bl_config->combo_high_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_PWM_A, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_PWM_B, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_PWM_C, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_PWM_D, (pwm_hi << 16) | (pwm_lo));
						break;
					default:
						break;
				}
			}
			else {
				//pre_set combo_high duty min
				if (pDev->bl_config->combo_high_method == BL_CTL_PWM_NEGATIVE) {
					pwm_hi = pDev->bl_config->combo_high_cnt - pDev->bl_config->combo_high_duty_min;
					pwm_lo = pDev->bl_config->combo_high_duty_min;
				}
				else {
					pwm_hi = pDev->bl_config->combo_high_duty_min;;
					pwm_lo = pDev->bl_config->combo_high_cnt - pDev->bl_config->combo_high_duty_min;
				}
				switch (pDev->bl_config->combo_high_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_PWM_A, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_PWM_B, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_PWM_C, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_PWM_D, (pwm_hi << 16) | (pwm_lo));
						break;
					default:
						break;
				}

				//set combo_low duty
				level = (pDev->bl_config->combo_low_duty_max - pDev->bl_config->combo_low_duty_min) * (level - pDev->bl_config->level_min) / (pDev->bl_config->combo_level_switch - pDev->bl_config->level_min) + pDev->bl_config->combo_low_duty_min;
				if (pDev->bl_config->combo_low_method == BL_CTL_PWM_NEGATIVE) {
					pwm_hi = pDev->bl_config->combo_low_cnt - level;
					pwm_lo = level;
				}
				else {
					pwm_hi = level;
					pwm_lo = pDev->bl_config->combo_low_cnt - level;
				}
				switch (pDev->bl_config->combo_low_port) {
					case BL_PWM_A:
						WRITE_LCD_CBUS_REG(PWM_PWM_A, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_B:
						WRITE_LCD_CBUS_REG(PWM_PWM_B, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_C:
						WRITE_LCD_CBUS_REG(PWM_PWM_C, (pwm_hi << 16) | (pwm_lo));
						break;
					case BL_PWM_D:
						WRITE_LCD_CBUS_REG(PWM_PWM_D, (pwm_hi << 16) | (pwm_lo));
						break;
					default:
						break;
				}
			}
			break;
		case BL_CTL_EXTERN:
#ifdef CONFIG_AML_BL_EXTERN
			bl_extern_driver = aml_bl_extern_get_driver();
			if (bl_extern_driver == NULL) {
				printf("no bl_extern driver\n");
			}
			else {
				if (bl_extern_driver->set_level) {
					ret = bl_extern_driver->set_level(level);
					if (ret) {
						printf("[bl_extern] set_level error\n");
					}
				}
				else {
					printf("[bl_extern] set_level is null\n");
				}
			}
#endif
			break;
		default:
			break;
	}
}

static unsigned get_lcd_backlight_level(void)
{
    DBG_PRINT("%s :%d\n", __FUNCTION__, bl_level);
    return bl_level;
}

static void lcd_power_ctrl(Bool_t status)
{
	int i;
#ifdef CONFIG_PLATFORM_HAS_PMU
	struct aml_pmu_driver *pmu_driver;
#endif
#ifdef CONFIG_AML_LCD_EXTERN
	struct aml_lcd_extern_driver_t *lcd_extern_driver;
#endif

	DBG_PRINT("%s(): %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
	if (status) {
		if (pDev->pConf->lcd_power_ctrl.power_on_uboot.type < LCD_POWER_TYPE_MAX) {
			DBG_PRINT("lcd_power_on_uboot\n");
			switch (pDev->pConf->lcd_power_ctrl.power_on_uboot.type) {
				case LCD_POWER_TYPE_CPU:
					aml_lcd_gpio_set(pDev->pConf->lcd_power_ctrl.power_on_uboot.gpio, pDev->pConf->lcd_power_ctrl.power_on_uboot.value);
					break;
				case LCD_POWER_TYPE_PMU:
#ifdef CONFIG_PLATFORM_HAS_PMU
					pmu_driver = aml_pmu_get_driver();
					if (pmu_driver == NULL) {
						printf("no pmu driver\n");
					}
					else if (pmu_driver->pmu_set_gpio) {
						if (pDev->pConf->lcd_power_ctrl.power_on_uboot.value == LCD_POWER_GPIO_OUTPUT_LOW) {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_on_uboot.gpio, 0);
						}
						else {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_on_uboot.gpio, 1);
						}
					}
#endif
					break;
				default:
					break;
			}
			if (pDev->pConf->lcd_power_ctrl.power_on_uboot.delay > 0)
				mdelay(pDev->pConf->lcd_power_ctrl.power_on_uboot.delay);
		}
		for (i=0; i<pDev->pConf->lcd_power_ctrl.power_on_step; i++) {
			DBG_PRINT("%s %s step %d\n", __FUNCTION__, (status ? "ON" : "OFF"), i+1);
			switch (pDev->pConf->lcd_power_ctrl.power_on_config[i].type) {
				case LCD_POWER_TYPE_CPU:
					aml_lcd_gpio_set(pDev->pConf->lcd_power_ctrl.power_on_config[i].gpio, pDev->pConf->lcd_power_ctrl.power_on_config[i].value);
					break;
				case LCD_POWER_TYPE_PMU:
#ifdef CONFIG_PLATFORM_HAS_PMU
					pmu_driver = aml_pmu_get_driver();
					if (pmu_driver == NULL) {
						printf("no pmu driver\n");
					}
					else if (pmu_driver->pmu_set_gpio) {
						if (pDev->pConf->lcd_power_ctrl.power_on_config[i].value == LCD_POWER_GPIO_OUTPUT_LOW) {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_on_config[i].gpio, 0);
						}
						else {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_on_config[i].gpio, 1);
						}
					}
#endif
					break;
				case LCD_POWER_TYPE_SIGNAL:
					lcd_ports_ctrl(ON);
					break;
				case LCD_POWER_TYPE_INITIAL:
#ifdef CONFIG_AML_LCD_EXTERN
					lcd_extern_driver = aml_lcd_extern_get_driver();
					if (lcd_extern_driver == NULL) {
						printf("no lcd_extern driver\n");
					}
					else {
						if (lcd_extern_driver->power_on)
							lcd_extern_driver->power_on();
						DBG_PRINT("%s power on\n", lcd_extern_driver->name);
					}
#endif
					break;
				default:
					printf("lcd power ctrl ON step %d is null.\n", i+1);
					break;
			}
			if (pDev->pConf->lcd_power_ctrl.power_on_config[i].delay > 0)
				mdelay(pDev->pConf->lcd_power_ctrl.power_on_config[i].delay);
		}
	}
	else {
		mdelay(30);
		for (i=0; i<pDev->pConf->lcd_power_ctrl.power_off_step; i++) {
			DBG_PRINT("%s %s step %d\n", __FUNCTION__, (status ? "ON" : "OFF"), i+1);
			switch (pDev->pConf->lcd_power_ctrl.power_off_config[i].type) {
				case LCD_POWER_TYPE_CPU:
					aml_lcd_gpio_set(pDev->pConf->lcd_power_ctrl.power_off_config[i].gpio, pDev->pConf->lcd_power_ctrl.power_off_config[i].value);
					break;
				case LCD_POWER_TYPE_PMU:
#ifdef CONFIG_PLATFORM_HAS_PMU
					pmu_driver = aml_pmu_get_driver();
					if (pmu_driver == NULL) {
						printf("no pmu driver\n");
					}
					else if (pmu_driver->pmu_set_gpio) {
						if (pDev->pConf->lcd_power_ctrl.power_off_config[i].value == LCD_POWER_GPIO_OUTPUT_LOW) {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_off_config[i].gpio, 0);
						}
						else {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_off_config[i].gpio, 1);
						}
					}
#endif
					break;
				case LCD_POWER_TYPE_SIGNAL:
					lcd_ports_ctrl(OFF);
					break;
				case LCD_POWER_TYPE_INITIAL:
#ifdef CONFIG_AML_LCD_EXTERN
					lcd_extern_driver = aml_lcd_extern_get_driver();
					if (lcd_extern_driver == NULL) {
						printf("no lcd_extern driver\n");
					}
					else {
						if (lcd_extern_driver->power_off)
							lcd_extern_driver->power_off();
						DBG_PRINT("%s power off\n", lcd_extern_driver->name);
					}
#endif
					break;
				default:
					printf("lcd power ctrl OFF step %d is null.\n", i+1);
					break;
			}
			if (pDev->pConf->lcd_power_ctrl.power_off_config[i].delay > 0)
				mdelay(pDev->pConf->lcd_power_ctrl.power_off_config[i].delay);
		}
		if (pDev->pConf->lcd_power_ctrl.power_off_uboot.type < LCD_POWER_TYPE_MAX) {
			DBG_PRINT("lcd_power_off_uboot\n");
			switch (pDev->pConf->lcd_power_ctrl.power_off_uboot.type) {
				case LCD_POWER_TYPE_CPU:
					aml_lcd_gpio_set(pDev->pConf->lcd_power_ctrl.power_off_uboot.gpio, pDev->pConf->lcd_power_ctrl.power_off_uboot.value);
					break;
				case LCD_POWER_TYPE_PMU:
#ifdef CONFIG_PLATFORM_HAS_PMU
					pmu_driver = aml_pmu_get_driver();
					if (pmu_driver == NULL) {
						printf("no pmu driver\n");
					}
					else if (pmu_driver->pmu_set_gpio) {
						if (pDev->pConf->lcd_power_ctrl.power_off_uboot.value == LCD_POWER_GPIO_OUTPUT_LOW) {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_off_uboot.gpio, 0);
						}
						else {
							pmu_driver->pmu_set_gpio(pDev->pConf->lcd_power_ctrl.power_off_uboot.gpio, 1);
						}
					}
#endif
					break;
				default:
					break;
			}
			if (pDev->pConf->lcd_power_ctrl.power_off_uboot.delay > 0)
				mdelay(pDev->pConf->lcd_power_ctrl.power_off_uboot.delay);
		}
	}
	printf("%s(): %s finished.\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static void set_lcd_gamma_table_ttl(u16 *data, u32 rgb_mask, u16 gamma_coeff)
{
	int i;

	while (!(READ_LCD_REG(GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
	WRITE_LCD_REG(GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) | (0x1 << rgb_mask) | (0x0 << LCD_HADR));
	for (i=0;i<256;i++) {
		while (!(READ_LCD_REG(GAMMA_CNTL_PORT) & (0x1 << LCD_WR_RDY)));
		WRITE_LCD_REG(GAMMA_DATA_PORT, (data[i] * gamma_coeff / 100));
	}
	while (!(READ_LCD_REG(GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
	WRITE_LCD_REG(GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) | (0x1 << rgb_mask) | (0x23 << LCD_HADR));
}

static void set_lcd_gamma_table_lvds(u16 *data, u32 rgb_mask, u16 gamma_coeff)
{
	int i;

	while (!(READ_LCD_REG(L_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
	WRITE_LCD_REG(L_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) | (0x1 << rgb_mask) | (0x0 << LCD_HADR));
	for (i=0;i<256;i++) {
		while (!(READ_LCD_REG(L_GAMMA_CNTL_PORT) & (0x1 << LCD_WR_RDY)));
		WRITE_LCD_REG(L_GAMMA_DATA_PORT, (data[i] * gamma_coeff / 100));
	}
	while (!(READ_LCD_REG(L_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
	WRITE_LCD_REG(L_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) | (0x1 << rgb_mask) | (0x23 << LCD_HADR));
}

static void write_tcon_double(MLVDS_Tcon_Config_t *mlvds_tcon)
{
    unsigned int tmp;
    int channel_num = mlvds_tcon->channel_num;
    int hv_sel = mlvds_tcon->hv_sel;
    int hstart_1 = mlvds_tcon->tcon_1st_hs_addr;
    int hend_1 = mlvds_tcon->tcon_1st_he_addr;
    int vstart_1 = mlvds_tcon->tcon_1st_vs_addr;
    int vend_1 = mlvds_tcon->tcon_1st_ve_addr;
    int hstart_2 = mlvds_tcon->tcon_2nd_hs_addr;
    int hend_2 = mlvds_tcon->tcon_2nd_he_addr;
    int vstart_2 = mlvds_tcon->tcon_2nd_vs_addr;
    int vend_2 = mlvds_tcon->tcon_2nd_ve_addr;

    tmp = READ_LCD_REG(L_TCON_MISC_SEL_ADDR);
    switch(channel_num)
    {
        case 0 :
            WRITE_LCD_REG(MTCON0_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON0_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON0_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON0_1ST_VE_ADDR, vend_1);
            WRITE_LCD_REG(MTCON0_2ND_HS_ADDR, hstart_2);
            WRITE_LCD_REG(MTCON0_2ND_HE_ADDR, hend_2);
            WRITE_LCD_REG(MTCON0_2ND_VS_ADDR, vstart_2);
            WRITE_LCD_REG(MTCON0_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_STH1_SEL)) | (hv_sel << LCD_STH1_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 1 :
            WRITE_LCD_REG(MTCON1_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON1_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON1_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON1_1ST_VE_ADDR, vend_1);
            WRITE_LCD_REG(MTCON1_2ND_HS_ADDR, hstart_2);
            WRITE_LCD_REG(MTCON1_2ND_HE_ADDR, hend_2);
            WRITE_LCD_REG(MTCON1_2ND_VS_ADDR, vstart_2);
            WRITE_LCD_REG(MTCON1_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_CPV1_SEL)) | (hv_sel << LCD_CPV1_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 2 :
            WRITE_LCD_REG(MTCON2_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON2_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON2_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON2_1ST_VE_ADDR, vend_1);
            WRITE_LCD_REG(MTCON2_2ND_HS_ADDR, hstart_2);
            WRITE_LCD_REG(MTCON2_2ND_HE_ADDR, hend_2);
            WRITE_LCD_REG(MTCON2_2ND_VS_ADDR, vstart_2);
            WRITE_LCD_REG(MTCON2_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_STV1_SEL)) | (hv_sel << LCD_STV1_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 3 :
            WRITE_LCD_REG(MTCON3_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON3_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON3_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON3_1ST_VE_ADDR, vend_1);
            WRITE_LCD_REG(MTCON3_2ND_HS_ADDR, hstart_2);
            WRITE_LCD_REG(MTCON3_2ND_HE_ADDR, hend_2);
            WRITE_LCD_REG(MTCON3_2ND_VS_ADDR, vstart_2);
            WRITE_LCD_REG(MTCON3_2ND_VE_ADDR, vend_2);
            tmp &= (~(1 << LCD_OEV1_SEL)) | (hv_sel << LCD_OEV1_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 4 :
            WRITE_LCD_REG(MTCON4_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON4_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON4_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON4_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_STH2_SEL)) | (hv_sel << LCD_STH2_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 5 :
            WRITE_LCD_REG(MTCON5_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON5_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON5_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON5_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_CPV2_SEL)) | (hv_sel << LCD_CPV2_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 6 :
            WRITE_LCD_REG(MTCON6_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON6_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON6_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON6_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_OEH_SEL)) | (hv_sel << LCD_OEH_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        case 7 :
            WRITE_LCD_REG(MTCON7_1ST_HS_ADDR, hstart_1);
            WRITE_LCD_REG(MTCON7_1ST_HE_ADDR, hend_1);
            WRITE_LCD_REG(MTCON7_1ST_VS_ADDR, vstart_1);
            WRITE_LCD_REG(MTCON7_1ST_VE_ADDR, vend_1);
            tmp &= (~(1 << LCD_OEV3_SEL)) | (hv_sel << LCD_OEV3_SEL);
            WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tmp);
            break;
        default:
            break;
    }
}

static void set_tcon_lvds(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s.\n",__FUNCTION__);

    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableR, H_SEL_R, pConf->lcd_effect.gamma_r_coeff);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableG, H_SEL_G, pConf->lcd_effect.gamma_g_coeff);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableB, H_SEL_B, pConf->lcd_effect.gamma_b_coeff);

	WRITE_LCD_REG_BITS(L_GAMMA_CNTL_PORT, ((pConf->lcd_effect.gamma_cntl_port >> LCD_GAMMA_EN) & 1), 0, 1);
	//WRITE_LCD_REG(L_GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

	WRITE_LCD_REG(L_RGB_BASE_ADDR,   pConf->lcd_effect.rgb_base_addr);
	WRITE_LCD_REG(L_RGB_COEFF_ADDR,  pConf->lcd_effect.rgb_coeff_addr);

	if (pConf->lcd_effect.dith_user) {
		WRITE_LCD_REG(L_DITH_CNTL_ADDR,  pConf->lcd_effect.dith_cntl_addr);
	}
	else {
		if(pConf->lcd_basic.lcd_bits == 8)
			WRITE_LCD_REG(L_DITH_CNTL_ADDR,  0x400);
		else
			WRITE_LCD_REG(L_DITH_CNTL_ADDR,  0x600);
	}

	WRITE_LCD_REG(VPP_MISC, READ_LCD_REG(VPP_MISC) & ~(VPP_OUT_SATURATE));
}

static void set_tcon_ttl(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s.\n",__FUNCTION__);
	Lcd_Timing_t *tcon_adr = &(pConf->lcd_timing);
	unsigned hs_pol, vs_pol;

    set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableR, LCD_H_SEL_R, pConf->lcd_effect.gamma_r_coeff);
    set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableG, LCD_H_SEL_G, pConf->lcd_effect.gamma_g_coeff);
    set_lcd_gamma_table_ttl(pConf->lcd_effect.GammaTableB, LCD_H_SEL_B, pConf->lcd_effect.gamma_b_coeff);

	WRITE_LCD_REG_BITS(GAMMA_CNTL_PORT, ((pConf->lcd_effect.gamma_cntl_port >> LCD_GAMMA_EN) & 1), 0, 1);
	//WRITE_LCD_REG(GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

    WRITE_LCD_REG(RGB_BASE_ADDR,   pConf->lcd_effect.rgb_base_addr);
    WRITE_LCD_REG(RGB_COEFF_ADDR,  pConf->lcd_effect.rgb_coeff_addr);
	WRITE_LCD_REG(POL_CNTL_ADDR,   (pConf->lcd_timing.pol_cntl_addr) & ((1 << LCD_CPH1_POL) | (1 << LCD_CPH2_POL) | (1 << LCD_CPH3_POL)));

	if (pConf->lcd_effect.dith_user) {
		WRITE_LCD_REG(DITH_CNTL_ADDR,  pConf->lcd_effect.dith_cntl_addr);
	}
	else {
		if(pConf->lcd_basic.lcd_bits == 8)
			WRITE_LCD_REG(DITH_CNTL_ADDR,  0x400);
		else
			WRITE_LCD_REG(DITH_CNTL_ADDR,  0x600);
	}

	hs_pol = (pConf->lcd_timing.pol_cntl_addr >> LCD_HS_POL) & 1;
	vs_pol = (pConf->lcd_timing.pol_cntl_addr >> LCD_VS_POL) & 1;

	if (hs_pol) {
		WRITE_LCD_REG(STH1_HS_ADDR,    tcon_adr->sth1_hs_addr);
		WRITE_LCD_REG(STH1_HE_ADDR,    tcon_adr->sth1_he_addr);
	}
	else {
		WRITE_LCD_REG(STH1_HS_ADDR,    tcon_adr->sth1_he_addr);
		WRITE_LCD_REG(STH1_HE_ADDR,    tcon_adr->sth1_hs_addr);
	}
	WRITE_LCD_REG(STH1_VS_ADDR,    tcon_adr->sth1_vs_addr);
	WRITE_LCD_REG(STH1_VE_ADDR,    tcon_adr->sth1_ve_addr);

	WRITE_LCD_REG(OEH_HS_ADDR,     tcon_adr->oeh_hs_addr);
	WRITE_LCD_REG(OEH_HE_ADDR,     tcon_adr->oeh_he_addr);
	WRITE_LCD_REG(OEH_VS_ADDR,     tcon_adr->oeh_vs_addr);
	WRITE_LCD_REG(OEH_VE_ADDR,     tcon_adr->oeh_ve_addr);

	WRITE_LCD_REG(STV1_HS_ADDR,    tcon_adr->stv1_hs_addr);
	WRITE_LCD_REG(STV1_HE_ADDR,    tcon_adr->stv1_he_addr);
	if (vs_pol) {
		WRITE_LCD_REG(STV1_VS_ADDR,    tcon_adr->stv1_vs_addr);
		WRITE_LCD_REG(STV1_VE_ADDR,    tcon_adr->stv1_ve_addr);
	}
	else {
		WRITE_LCD_REG(STV1_VS_ADDR,    tcon_adr->stv1_ve_addr);
		WRITE_LCD_REG(STV1_VE_ADDR,    tcon_adr->stv1_vs_addr);
	}

	WRITE_LCD_REG(L_INV_CNT_ADDR,    tcon_adr->inv_cnt_addr);
	WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tcon_adr->tcon_misc_sel_addr);

	WRITE_LCD_REG(VPP_MISC, READ_LCD_REG(VPP_MISC) & ~(VPP_OUT_SATURATE));
}

// Set the mlvds TCON
// this function should support dual gate or singal gate TCON setting.
// singal gate TCON, Scan Function TO DO.
// scan_function   // 0 - Z1, 1 - Z2, 2- Gong
static void set_tcon_mlvds(Lcd_Config_t *pConf)
{
    printf("%s.\n",__FUNCTION__);
	MLVDS_Tcon_Config_t *mlvds_tconfig_l = pConf->lcd_control.mlvds_tcon_config;
    int dual_gate = pConf->lcd_control.mlvds_config->test_dual_gate;
    int bit_num = pConf->lcd_basic.lcd_bits;
    int pair_num = pConf->lcd_control.mlvds_config->test_pair_num;

    unsigned int data32;

    int pclk_div;
    int ext_pixel = dual_gate ? pConf->lcd_control.mlvds_config->total_line_clk : 0;
    int dual_wr_rd_start;
    int i = 0;

//    debug(" Notice: Setting VENC_DVI_SETTING[0x%4x] and GAMMA_CNTL_PORT[0x%4x].LCD_GAMMA_EN as 0 temporary\n", VENC_DVI_SETTING, GAMMA_CNTL_PORT);
//    debug(" Otherwise, the panel will display color abnormal.\n");
//    WRITE_MPEG_REG(VENC_DVI_SETTING, 0);

    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableR, H_SEL_R, pConf->lcd_effect.gamma_r_coeff);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableG, H_SEL_G, pConf->lcd_effect.gamma_g_coeff);
    set_lcd_gamma_table_lvds(pConf->lcd_effect.GammaTableB, H_SEL_B, pConf->lcd_effect.gamma_b_coeff);

    WRITE_MPEG_REG(L_GAMMA_CNTL_PORT, pConf->lcd_effect.gamma_cntl_port);
	WRITE_MPEG_REG(L_GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

    WRITE_MPEG_REG(L_RGB_BASE_ADDR, pConf->lcd_effect.rgb_base_addr);
    WRITE_MPEG_REG(L_RGB_COEFF_ADDR, pConf->lcd_effect.rgb_coeff_addr);
    //WRITE_MPEG_REG(L_POL_CNTL_ADDR, pConf->lcd_timing.pol_cntl_addr);
    if(pConf->lcd_basic.lcd_bits == 8)
		WRITE_MPEG_REG(L_DITH_CNTL_ADDR,  0x400);
	else
		WRITE_MPEG_REG(L_DITH_CNTL_ADDR,  0x600);

//    WRITE_MPEG_REG(L_INV_CNT_ADDR, pConf->lcd_timing.inv_cnt_addr);
//    WRITE_MPEG_REG(L_TCON_MISC_SEL_ADDR, pConf->lcd_timing.tcon_misc_sel_addr);
//    WRITE_MPEG_REG(L_DUAL_PORT_CNTL_ADDR, pConf->lcd_timing.dual_port_cntl_addr);
//
//    CLEAR_MPEG_REG_MASK(VPP_MISC, VPP_OUT_SATURATE);

    data32 = (0x9867 << tcon_pattern_loop_data) |
             (1 << tcon_pattern_loop_start) |
             (4 << tcon_pattern_loop_end) |
             (1 << ((mlvds_tconfig_l[6].channel_num)+tcon_pattern_enable)); // POL_CHANNEL use pattern generate

    WRITE_MPEG_REG(L_TCON_PATTERN_HI,  (data32 >> 16));
    WRITE_MPEG_REG(L_TCON_PATTERN_LO, (data32 & 0xffff));

    pclk_div = (bit_num == 8) ? 3 : // phy_clk / 8
                                2 ; // phy_clk / 6
   data32 = (1 << ((mlvds_tconfig_l[7].channel_num)-2+tcon_pclk_enable)) |  // enable PCLK_CHANNEL
            (pclk_div << tcon_pclk_div) |
            (
              (pair_num == 6) ?
              (
              ((bit_num == 8) & dual_gate) ?
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              ) :
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              )
              ) :
              (
              ((bit_num == 8) & dual_gate) ?
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              ) :
              (bit_num == 8) ?
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              ) :
              (
                (0 << (tcon_delay + 0*3)) |
                (0 << (tcon_delay + 1*3)) |
                (0 << (tcon_delay + 2*3)) |
                (0 << (tcon_delay + 3*3)) |
                (0 << (tcon_delay + 4*3)) |
                (0 << (tcon_delay + 5*3)) |
                (0 << (tcon_delay + 6*3)) |
                (0 << (tcon_delay + 7*3))
              )
              )
            );

    WRITE_MPEG_REG(TCON_CONTROL_HI,  (data32 >> 16));
    WRITE_MPEG_REG(TCON_CONTROL_LO, (data32 & 0xffff));

    WRITE_MPEG_REG( L_TCON_DOUBLE_CTL,
                   (1<<(mlvds_tconfig_l[3].channel_num))   // invert CPV
                  );

    // for channel 4-7, set second setting same as first
    WRITE_MPEG_REG(L_DE_HS_ADDR, (0x3 << 14) | ext_pixel);   // 0x3 -- enable double_tcon fir channel7:6
    WRITE_MPEG_REG(L_DE_HE_ADDR, (0x3 << 14) | ext_pixel);   // 0x3 -- enable double_tcon fir channel5:4
    WRITE_MPEG_REG(L_DE_VS_ADDR, (0x3 << 14) | 0);	// 0x3 -- enable double_tcon fir channel3:2
    WRITE_MPEG_REG(L_DE_VE_ADDR, (0x3 << 14) | 0);	// 0x3 -- enable double_tcon fir channel1:0

    dual_wr_rd_start = 0x5d;
    WRITE_MPEG_REG(MLVDS_DUAL_GATE_WR_START, dual_wr_rd_start);
    WRITE_MPEG_REG(MLVDS_DUAL_GATE_WR_END, dual_wr_rd_start + 1280);
    WRITE_MPEG_REG(MLVDS_DUAL_GATE_RD_START, dual_wr_rd_start + ext_pixel - 2);
    WRITE_MPEG_REG(MLVDS_DUAL_GATE_RD_END, dual_wr_rd_start + 1280 + ext_pixel - 2);

    WRITE_MPEG_REG(MLVDS_SECOND_RESET_CTL, (pConf->lcd_control.mlvds_config->mlvds_insert_start + ext_pixel));

    data32 = (0 << ((mlvds_tconfig_l[5].channel_num)+mlvds_tcon_field_en)) |  // enable EVEN_F on TCON channel 6
             ( (0x0 << mlvds_scan_mode_odd) | (0x0 << mlvds_scan_mode_even)
             ) | (0 << mlvds_scan_mode_start_line);

    WRITE_MPEG_REG(MLVDS_DUAL_GATE_CTL_HI,  (data32 >> 16));
    WRITE_MPEG_REG(MLVDS_DUAL_GATE_CTL_LO, (data32 & 0xffff));

    DBG_PRINT("write minilvds tcon 0~7.\n");
    for(i = 0; i < 8; i++)
    {
		write_tcon_double(&mlvds_tconfig_l[i]);
    }
}

static void set_lcd_spread_spectrum(int ss_level)
{
	DBG_PRINT("%s.\n", __FUNCTION__);

	switch (ss_level) {
		case 1:  // about 0.5%
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL2, 0x16110696);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL3, 0x6d625012);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL4, 0x130);
			break;
		case 2:  //about 1%
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL2, 0x16110696);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL3, 0x4d625012);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL4, 0x130);
			break;
		case 3:  //about 2%
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL2, 0x16110696);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL3, 0x2d425012);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL4, 0x130);
			break;
		case 4:  //about 3%
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL2, 0x16110696);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL3, 0x1d425012);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL4, 0x130);
			break;
		case 5:  //about 4%
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL2, 0x16110696);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL3, 0x0d125012);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL4, 0x130);
			break;
		case 6:  //about 5%
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL2, 0x16110696);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL3, 0x0e425012);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL4, 0x130);
			break;
		case 0:
		default:  //disable ss
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL2, 0x814d3928);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL3, 0x6b425012);
			WRITE_LCD_CBUS_REG(HHI_VIID_PLL_CNTL4, 0x110 );
	}
}

static void vclk_set_lcd(int lcd_type, int vclk_sel, unsigned long pll_reg, unsigned long vid_div_reg, unsigned long clk_ctrl_reg)
{
	unsigned xd;
	int wait_loop = PLL_WAIT_LOCK_CNT;
	unsigned pll_lock = 0;

	DBG_PRINT("setup lcd clk.\n");
	vid_div_reg = ((vid_div_reg & 0x1ffff) | (1 << 16) | (1 << 15) | (0x3 << 0));	//select vid2_pll and enable clk
	xd = (clk_ctrl_reg >> CLK_CTRL_XD) & 0xf;

	if(vclk_sel)
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 0, 19, 1);	//disable vclk2_en
	else
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 0, 19, 2);	//disable vclk1_en1, en0
	udelay(2);

    WRITE_LCD_CBUS_REG( HHI_VIID_PLL_CNTL, pll_reg | (1<<29));
    WRITE_LCD_CBUS_REG( HHI_VIID_PLL_CNTL2, 0x814d3928);
	WRITE_LCD_CBUS_REG( HHI_VIID_PLL_CNTL3, 0x6b425012);
	WRITE_LCD_CBUS_REG( HHI_VIID_PLL_CNTL4, 0x110);
	WRITE_LCD_CBUS_REG( HHI_VIID_PLL_CNTL, pll_reg);
	do{
		udelay(100);
		pll_lock = (READ_LCD_CBUS_REG(HHI_VIID_PLL_CNTL) >> PLL_CTRL_LOCK) & 0x1;
		if (wait_loop < 20)
			printf("vid2_pll_locked=%u, wait_lock_loop=%d\n", pll_lock, (PLL_WAIT_LOCK_CNT - wait_loop + 1));
		wait_loop--;
	}while((pll_lock == 0) && (wait_loop > 0));

	WRITE_LCD_CBUS_REG(HHI_VIID_DIVIDER_CNTL, vid_div_reg);
	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 1, 7, 1);	//0x104c[7]:SOFT_RESET_POST
	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 1, 3, 1);	//0x104c[3]:SOFT_RESET_PRE
	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 0, 0, 2);	//0x104c[1:0]:RESET_N_PRE, RESET_N_POST
	udelay(10);
	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 0, 3, 1);	//release SOFT_RESET_PRE
	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 0, 7, 1);	//release SOFT_RESET_POST
	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 3, 0, 2);	//release RESET_N_PRE, RESET_N_POST
	udelay(5);

	if(vclk_sel) {
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_DIV, (xd-1), 0, 8);	// setup XD divider
		udelay(5);
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 1, 19, 1);	//vclk2_en0
	}
	else {
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_DIV, (xd-1), 0, 8);	// setup XD divider
		udelay(5);
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 3, 19, 2);     //[19]vclk_en0, [20]vclk_en1 (en1 is for tcon_clko???)

	}
	udelay(2);

	if(lcd_type == LCD_DIGITAL_TTL){
		if(vclk_sel)
			WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_DIV, 8, 20, 4); // [23:20] enct_clk_sel
		else
			WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_DIV, 0, 20, 4); // [23:20] enct_clk_sel
	}
	else {
		if(vclk_sel)
			WRITE_MPEG_REG_BITS (HHI_VIID_CLK_DIV, 8, 12, 4); // [23:20] encl_clk_sel
		else
			WRITE_MPEG_REG_BITS (HHI_VIID_CLK_DIV, 0, 12, 4); // [23:20] encl_clk_sel
	}

	if(vclk_sel) {
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 1, 0, 1);	//enable v2_clk_div1
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 1, 15, 1);	//soft reset
		udelay(5);
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 0, 15, 1);	//release soft reset
	}
	else {
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 1, 0, 1);		//enable v1_clk_div1
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 1, 15, 1);	//soft reset
		udelay(5);
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 0, 15, 1);	//release soft reset
	}
}

static void clk_util_lvds_set_clk_div(  unsigned long divn_sel, unsigned long divn_tcnt, unsigned long div2_en)
{
    // assign          lvds_div_phy_clk_en     = tst_lvds_tmode ? 1'b1         : phy_clk_cntl[10];
    // assign          lvds_div_div2_sel       = tst_lvds_tmode ? atest_i[5]   : phy_clk_cntl[9];
    // assign          lvds_div_sel            = tst_lvds_tmode ? atest_i[7:6] : phy_clk_cntl[8:7];
    // assign          lvds_div_tcnt           = tst_lvds_tmode ? 3'd6         : phy_clk_cntl[6:4];
    // If dividing by 1, just select the divide by 1 path
    if( divn_tcnt == 1 ) {
        divn_sel = 0;
    }
    WRITE_LCD_REG(LVDS_PHY_CLK_CNTL, ((READ_LCD_REG(LVDS_PHY_CLK_CNTL) & ~((0x3 << 7) | (1 << 9) | (0x7 << 4))) | ((1 << 10) | (divn_sel << 7) | (div2_en << 9) | (((divn_tcnt-1)&0x7) << 4))));
}

static void set_pll_lcd(Lcd_Config_t *pConf)
{
	unsigned pll_reg, div_reg, clk_reg;
	int vclk_sel, xd;
	int lcd_type, ss_level;
	unsigned pll_div_post = 0, phy_clk_div2 = 0;

	DBG_PRINT("%s\n", __FUNCTION__);

	pll_reg = pConf->lcd_timing.pll_ctrl;
	div_reg = pConf->lcd_timing.div_ctrl;
	clk_reg = pConf->lcd_timing.clk_ctrl;
	ss_level = (clk_reg >> CLK_CTRL_SS) & 0xf;
	vclk_sel = (clk_reg >> CLK_CTRL_VCLK_SEL) & 0x1;
	xd = (clk_reg >> CLK_CTRL_XD) & 0xf;

	lcd_type = pConf->lcd_basic.lcd_type;

	switch(lcd_type){
		case LCD_DIGITAL_LVDS:
			xd = 1;
			pll_div_post = 7;
			phy_clk_div2 = 0;
			div_reg = (div_reg | (1 << DIV_CTRL_POST_SEL) | (1 << DIV_CTRL_LVDS_CLK_EN) | ((pll_div_post-1) << DIV_CTRL_DIV_POST) | (phy_clk_div2 << DIV_CTRL_PHY_CLK_DIV2));
			break;
		case LCD_DIGITAL_TTL:
			break;
		default:
			break;
	}
	clk_reg = (pConf->lcd_timing.clk_ctrl & ~(0xf << CLK_CTRL_XD)) | (xd << CLK_CTRL_XD);

	DBG_PRINT("ss_level=%u(%s), pll_reg=0x%x, div_reg=0x%x, xd=%d.\n", ss_level, lcd_ss_level_table[ss_level], pll_reg, div_reg, xd);
	vclk_set_lcd(lcd_type, vclk_sel, pll_reg, div_reg, clk_reg);
	set_lcd_spread_spectrum(ss_level);

	switch(lcd_type){
		case LCD_DIGITAL_LVDS:
			clk_util_lvds_set_clk_div(1, pll_div_post, phy_clk_div2);
			//    lvds_gen_cntl       <= {10'h0,      // [15:4] unused
			//                            2'h1,       // [5:4] divide by 7 in the PHY
			//                            1'b0,       // [3] fifo_en
			//                            1'b0,       // [2] wr_bist_gate
			//                            2'b00};     // [1:0] fifo_wr mode
			//FIFO_CLK_SEL = 1; // div7
			WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 1, 4, 2);	//lvds fifo clk div 7
			WRITE_LCD_REG(LVDS_PHY_CNTL0, 0xffff);
			WRITE_LCD_REG_BITS(LVDS_PHY_CLK_CNTL, 0, 15, 1);	// lvds div reset
			udelay(5);
			WRITE_LCD_REG_BITS(LVDS_PHY_CLK_CNTL, 1, 15, 1);	// Release lvds div reset
			break;
		case LCD_DIGITAL_TTL:
			break;
		default:
			break;
	}
}

static void set_pll_mlvds(Lcd_Config_t *pConf)
{
    debug("%s\n", __FUNCTION__);

    int test_bit_num = pConf->lcd_basic.lcd_bits;
    int test_dual_gate = pConf->lcd_control.mlvds_config->test_dual_gate;
    int test_pair_num= pConf->lcd_control.mlvds_config->test_pair_num;
    int pll_div_post, phy_clk_div2, FIFO_CLK_SEL, MPCLK_DELAY, MCLK_half, MCLK_half_delay;
    unsigned int data32;
    unsigned long mclk_pattern_dual_6_6;
    int test_high_phase = (test_bit_num != 8) | test_dual_gate;
    unsigned long rd_data;

    unsigned pll_reg, div_reg, clk_reg, xd;
    int vclk_sel;
	int lcd_type, ss_level;

	pll_reg = pConf->lcd_timing.pll_ctrl;
	div_reg = pConf->lcd_timing.div_ctrl;
	clk_reg = pConf->lcd_timing.clk_ctrl;
	ss_level = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_SS) & 0xf;
    vclk_sel = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_VCLK_SEL) & 0x1;
	xd = 1;

	lcd_type = pConf->lcd_basic.lcd_type;

    switch(pConf->lcd_control.mlvds_config->TL080_phase) {
      case 0 :
        mclk_pattern_dual_6_6 = 0xc3c3c3;
        MCLK_half = 1;
        break;
      case 1 :
        mclk_pattern_dual_6_6 = 0xc3c3c3;
        MCLK_half = 0;
        break;
      case 2 :
        mclk_pattern_dual_6_6 = 0x878787;
        MCLK_half = 1;
        break;
      case 3 :
        mclk_pattern_dual_6_6 = 0x878787;
        MCLK_half = 0;
        break;
      case 4 :
        mclk_pattern_dual_6_6 = 0x3c3c3c;
        MCLK_half = 1;
        break;
       case 5 :
        mclk_pattern_dual_6_6 = 0x3c3c3c;
        MCLK_half = 0;
        break;
       case 6 :
        mclk_pattern_dual_6_6 = 0x787878;
        MCLK_half = 1;
        break;
      default : // case 7
        mclk_pattern_dual_6_6 = 0x787878;
        MCLK_half = 0;
        break;
    }

    pll_div_post = (test_bit_num == 8) ?
                      (test_dual_gate ? 4 : 8) :
                      (test_dual_gate ? 3 : 6) ;

    phy_clk_div2 = (test_pair_num != 3);

	div_reg = (div_reg | (1 << DIV_CTRL_POST_SEL) | (1 << DIV_CTRL_LVDS_CLK_EN) | ((pll_div_post-1) << DIV_CTRL_DIV_POST) | (phy_clk_div2 << DIV_CTRL_PHY_CLK_DIV2));
	clk_reg = (pConf->lcd_timing.clk_ctrl & ~(0xf << CLK_CTRL_XD)) | (xd << CLK_CTRL_XD);
	DBG_PRINT("ss_level=%d, pll_reg=0x%x, div_reg=0x%x, xd=%d.\n", ss_level, pll_reg, div_reg, xd);
	vclk_set_lcd(lcd_type, vclk_sel, pll_reg, div_reg, clk_reg);
	set_lcd_spread_spectrum(ss_level);

	clk_util_lvds_set_clk_div(1, pll_div_post, phy_clk_div2);

	//enable v2_clk div
    // WRITE_MPEG_REG( HHI_VIID_CLK_CNTL, READ_MPEG_REG(HHI_VIID_CLK_CNTL) | (0xF << 0) );
    // WRITE_MPEG_REG( HHI_VID_CLK_CNTL, READ_MPEG_REG(HHI_VID_CLK_CNTL) | (0xF << 0) );

    WRITE_MPEG_REG( LVDS_PHY_CNTL0, 0xffff );

    //    lvds_gen_cntl       <= {10'h0,      // [15:4] unused
    //                            2'h1,       // [5:4] divide by 7 in the PHY
    //                            1'b0,       // [3] fifo_en
    //                            1'b0,       // [2] wr_bist_gate
    //                            2'b00};     // [1:0] fifo_wr mode

    FIFO_CLK_SEL = (test_bit_num == 8) ? 2 : // div8
                                    0 ; // div6
    rd_data = READ_MPEG_REG(LVDS_GEN_CNTL);
    rd_data = (rd_data & 0xffcf) | (FIFO_CLK_SEL<< 4);
    WRITE_MPEG_REG( LVDS_GEN_CNTL, rd_data);

    MPCLK_DELAY = (test_pair_num == 6) ?
                  ((test_bit_num == 8) ? (test_dual_gate ? 5 : 3) : 2) :
                  ((test_bit_num == 8) ? 3 : 3) ;

    MCLK_half_delay = pConf->lcd_control.mlvds_config->phase_select ? MCLK_half :
                      (test_dual_gate & (test_bit_num == 8) & (test_pair_num != 6));

    if(test_high_phase)
    {
        if(test_dual_gate)
        data32 = (MPCLK_DELAY << mpclk_dly) |
                 (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                 (1 << use_mpclk) |
                 (MCLK_half_delay << mlvds_clk_half_delay) |
                 (((test_bit_num == 8) ? (
                                           (test_pair_num == 6) ? 0x999999 : // DIV4
                                                                  0x555555   // DIV2
                                         ) :
                                         (
                                           (test_pair_num == 6) ? mclk_pattern_dual_6_6 : // DIV8
                                                                  0x999999   // DIV4
                                         )
                 ) << mlvds_clk_pattern);
        else if(test_bit_num == 8)
            data32 = (MPCLK_DELAY << mpclk_dly) |
                     (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                     (1 << use_mpclk) |
                     (0 << mlvds_clk_half_delay) |
                     (0xc3c3c3 << mlvds_clk_pattern);      // DIV 8
        else
            data32 = (MPCLK_DELAY << mpclk_dly) |
                     (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                     (1 << use_mpclk) |
                     (0 << mlvds_clk_half_delay) |
                     (
                       (
                         (test_pair_num == 6) ? 0xc3c3c3 : // DIV8
                                                0x999999   // DIV4
                       ) << mlvds_clk_pattern
                     );
    }
    else {
        if(test_pair_num == 6) {
            data32 = (MPCLK_DELAY << mpclk_dly) |
                     (((test_bit_num == 8) ? 3 : 2) << mpclk_div) |
                     (1 << use_mpclk) |
                     (0 << mlvds_clk_half_delay) |
                     (
                       (
                         (test_pair_num == 6) ? 0x999999 : // DIV4
                                                0x555555   // DIV2
                       ) << mlvds_clk_pattern
                     );
        }
        else {
            data32 = (1 << mlvds_clk_half_delay) |
                   (0x555555 << mlvds_clk_pattern);      // DIV 2
        }
    }

    WRITE_MPEG_REG(MLVDS_CLK_CTL_HI,  (data32 >> 16));
    WRITE_MPEG_REG(MLVDS_CLK_CTL_LO, (data32 & 0xffff));

	//pll_div_sel
    if(1){
		// Set Soft Reset vid_pll_div_pre
		WRITE_MPEG_REG(HHI_VIID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VIID_DIVIDER_CNTL) | 0x00008);
		// Set Hard Reset vid_pll_div_post
		WRITE_MPEG_REG(HHI_VIID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VIID_DIVIDER_CNTL) & 0x1fffd);
		// Set Hard Reset lvds_phy_ser_top
		WRITE_MPEG_REG(LVDS_PHY_CLK_CNTL, READ_MPEG_REG(LVDS_PHY_CLK_CNTL) & 0x7fff);
		// Release Hard Reset lvds_phy_ser_top
		WRITE_MPEG_REG(LVDS_PHY_CLK_CNTL, READ_MPEG_REG(LVDS_PHY_CLK_CNTL) | 0x8000);
		// Release Hard Reset vid_pll_div_post
		WRITE_MPEG_REG(HHI_VIID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VIID_DIVIDER_CNTL) | 0x00002);
		// Release Soft Reset vid_pll_div_pre
		WRITE_MPEG_REG(HHI_VIID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VIID_DIVIDER_CNTL) & 0x1fff7);
	}
	else{
		// Set Soft Reset vid_pll_div_pre
		WRITE_MPEG_REG(HHI_VID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VID_DIVIDER_CNTL) | 0x00008);
		// Set Hard Reset vid_pll_div_post
		WRITE_MPEG_REG(HHI_VID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VID_DIVIDER_CNTL) & 0x1fffd);
		// Set Hard Reset lvds_phy_ser_top
		WRITE_MPEG_REG(LVDS_PHY_CLK_CNTL, READ_MPEG_REG(LVDS_PHY_CLK_CNTL) & 0x7fff);
		// Release Hard Reset lvds_phy_ser_top
		WRITE_MPEG_REG(LVDS_PHY_CLK_CNTL, READ_MPEG_REG(LVDS_PHY_CLK_CNTL) | 0x8000);
		// Release Hard Reset vid_pll_div_post
		WRITE_MPEG_REG(HHI_VID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VID_DIVIDER_CNTL) | 0x00002);
		// Release Soft Reset vid_pll_div_pre
		WRITE_MPEG_REG(HHI_VID_DIVIDER_CNTL, READ_MPEG_REG(HHI_VID_DIVIDER_CNTL) & 0x1fff7);
    }
}

static void venc_set_ttl(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s.\n",__FUNCTION__);

	WRITE_LCD_REG(ENCT_VIDEO_EN, 0);
	WRITE_LCD_REG(VPU_VIU_VENC_MUX_CTRL, (3<<0) | (3<<2));	//viu1, viu2 select enct

    WRITE_LCD_REG(ENCT_VIDEO_MODE,        0);
    WRITE_LCD_REG(ENCT_VIDEO_MODE_ADV,    0x0008);


    WRITE_LCD_REG(ENCT_VIDEO_FILT_CTRL,    0x1000);	// bypass filter

    WRITE_LCD_REG(ENCT_VIDEO_MAX_PXCNT,    pConf->lcd_basic.h_period - 1);
    WRITE_LCD_REG(ENCT_VIDEO_MAX_LNCNT,    pConf->lcd_basic.v_period - 1);

    WRITE_LCD_REG(ENCT_VIDEO_HAVON_BEGIN,  pConf->lcd_timing.video_on_pixel);
    WRITE_LCD_REG(ENCT_VIDEO_HAVON_END,    pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel );
    WRITE_LCD_REG(ENCT_VIDEO_VAVON_BLINE,  pConf->lcd_timing.video_on_line);
    WRITE_LCD_REG(ENCT_VIDEO_VAVON_ELINE,  pConf->lcd_basic.v_active - 1 + pConf->lcd_timing.video_on_line);

    WRITE_LCD_REG(ENCT_VIDEO_HSO_BEGIN,    15);
    WRITE_LCD_REG(ENCT_VIDEO_HSO_END,      31);
    WRITE_LCD_REG(ENCT_VIDEO_VSO_BEGIN,    15);
    WRITE_LCD_REG(ENCT_VIDEO_VSO_END,      31);
    WRITE_LCD_REG(ENCT_VIDEO_VSO_BLINE,    0);
    WRITE_LCD_REG(ENCT_VIDEO_VSO_ELINE,    2);

    WRITE_LCD_REG(ENCT_VIDEO_EN,           1);	// enable enct
}

static void venc_set_lvds(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s\n",__FUNCTION__);

	WRITE_LCD_REG(ENCL_VIDEO_EN, 0);

	WRITE_LCD_REG(VPU_VIU_VENC_MUX_CTRL, (0<<0) | (0<<2));	//viu1, viu2 select encl

	WRITE_LCD_REG(ENCL_VIDEO_MODE,			0);
	WRITE_LCD_REG(ENCL_VIDEO_MODE_ADV,		0x8); // Sampling rate: 1

    WRITE_LCD_REG(ENCL_VIDEO_FILT_CTRL, 0x1000);	// bypass filter

    WRITE_LCD_REG(ENCL_VIDEO_MAX_PXCNT,	pConf->lcd_basic.h_period - 1);
    WRITE_LCD_REG(ENCL_VIDEO_MAX_LNCNT,	pConf->lcd_basic.v_period - 1);

    WRITE_LCD_REG(ENCL_VIDEO_HAVON_BEGIN,	pConf->lcd_timing.video_on_pixel);
    WRITE_LCD_REG(ENCL_VIDEO_HAVON_END,	pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
    WRITE_LCD_REG(ENCL_VIDEO_VAVON_BLINE,	pConf->lcd_timing.video_on_line);
    WRITE_LCD_REG(ENCL_VIDEO_VAVON_ELINE,	pConf->lcd_basic.v_active - 1  + pConf->lcd_timing.video_on_line);

    WRITE_LCD_REG(ENCL_VIDEO_HSO_BEGIN,	pConf->lcd_timing.sth1_hs_addr);
    WRITE_LCD_REG(ENCL_VIDEO_HSO_END,		pConf->lcd_timing.sth1_he_addr);
    WRITE_LCD_REG(ENCL_VIDEO_VSO_BEGIN,	pConf->lcd_timing.stv1_hs_addr);
    WRITE_LCD_REG(ENCL_VIDEO_VSO_END,		pConf->lcd_timing.stv1_he_addr);
    WRITE_LCD_REG(ENCL_VIDEO_VSO_BLINE,	pConf->lcd_timing.stv1_vs_addr);
    WRITE_LCD_REG(ENCL_VIDEO_VSO_ELINE,	pConf->lcd_timing.stv1_ve_addr);

	WRITE_LCD_REG(ENCL_VIDEO_RGBIN_CTRL, 	(1 << 0));//(1 << 1) | (1 << 0));	//bit[0] 1:RGB, 0:YUV

    WRITE_LCD_REG(ENCL_VIDEO_EN,           1);	// enable encl
}

static void venc_set_mlvds(Lcd_Config_t *pConf)
{
	int ext_pixel = pConf->lcd_control.mlvds_config->test_dual_gate ? pConf->lcd_control.mlvds_config->total_line_clk : 0;
	int active_h_start = pConf->lcd_timing.video_on_pixel;
	int active_v_start = pConf->lcd_timing.video_on_line;
	int width = pConf->lcd_basic.h_active;
	int height = pConf->lcd_basic.v_active;
	int max_height = pConf->lcd_basic.v_period;

	DBG_PRINT("%s\n", __FUNCTION__);

	WRITE_MPEG_REG(ENCL_VIDEO_EN, 0);

	WRITE_MPEG_REG(VPU_VIU_VENC_MUX_CTRL, (0<<0) | (0<<2));	//viu1, viu2 select encl

	WRITE_LCD_REG(ENCL_VIDEO_MODE,             0x0040 | (1<<14)); // Enable Hsync and equalization pulse switch in center; bit[14] cfg_de_v = 1
	WRITE_LCD_REG(ENCL_VIDEO_MODE_ADV,         0x0008); // Sampling rate: 1

	WRITE_LCD_REG(ENCL_VIDEO_FILT_CTRL,		0x1000);	// bypass filter

	WRITE_LCD_REG(ENCL_VIDEO_YFP1_HTIME,       active_h_start);
	WRITE_LCD_REG(ENCL_VIDEO_YFP2_HTIME,       active_h_start + width);

	WRITE_LCD_REG(ENCL_VIDEO_MAX_PXCNT,        pConf->lcd_control.mlvds_config->total_line_clk - 1 + ext_pixel);
	WRITE_LCD_REG(ENCL_VIDEO_MAX_LNCNT,        max_height - 1);

	WRITE_LCD_REG(ENCL_VIDEO_HAVON_BEGIN,      active_h_start);
	WRITE_LCD_REG(ENCL_VIDEO_HAVON_END,        active_h_start + width - 1);  // for dual_gate mode still read 1408 pixel at first half of line
	WRITE_LCD_REG(ENCL_VIDEO_VAVON_BLINE,      active_v_start);
	WRITE_LCD_REG(ENCL_VIDEO_VAVON_ELINE,      active_v_start + height -1);  //15+768-1);

	WRITE_LCD_REG(ENCL_VIDEO_HSO_BEGIN,        24);
	WRITE_LCD_REG(ENCL_VIDEO_HSO_END,          1420 + ext_pixel);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_BEGIN,        1400 + ext_pixel);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_END,          1410 + ext_pixel);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_BLINE,        1);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_ELINE,        3);

	WRITE_LCD_REG(ENCL_VIDEO_RGBIN_CTRL, 	(1 << 0));//(1 << 1) | (1 << 0));	//bit[0] 1:RGB, 0:YUV

	// enable encl
    WRITE_LCD_REG( ENCL_VIDEO_EN,           1);
}

static void set_control_lvds(Lcd_Config_t *pConf)
{
	unsigned lvds_repack, pn_swap, bit_num;
	unsigned data32;

	DBG_PRINT("%s\n", __FUNCTION__);

	WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 0, 3, 1); // disable fifo

    data32 = (0x00 << LVDS_blank_data_r) |
             (0x00 << LVDS_blank_data_g) |
             (0x00 << LVDS_blank_data_b) ;
    WRITE_LCD_REG(LVDS_BLANK_DATA_HI,  (data32 >> 16));
    WRITE_LCD_REG(LVDS_BLANK_DATA_LO, (data32 & 0xffff));

    WRITE_LCD_REG(LVDS_PHY_CNTL0, 0xffff);
    WRITE_LCD_REG(LVDS_PHY_CNTL1, 0xff00);

    //WRITE_MPEG_REG(ENCL_VIDEO_EN, 1);
	lvds_repack = (pConf->lcd_control.lvds_config->lvds_repack) & 0x1;
	pn_swap = (pConf->lcd_control.lvds_config->pn_swap) & 0x1;

	switch(pConf->lcd_basic.lcd_bits) {
		case 10:
			bit_num=0;
			break;
		case 8:
			bit_num=1;
			break;
		case 6:
			bit_num=2;
			break;
		case 4:
			bit_num=3;
			break;
		default:
			bit_num=1;
			break;
	}

	WRITE_LCD_REG_BITS(MLVDS_CONTROL, 0, 0, 1);  //disable mlvds

	WRITE_LCD_REG(LVDS_PACK_CNTL_ADDR,
					( lvds_repack<<0 ) | // repack
					( 0<<2 ) | // odd_even
					( 0<<3 ) | // reserve
					( 0<<4 ) | // lsb first
					( pn_swap<<5 ) | // pn swap
					( 0<<6 ) | // dual port
					( 0<<7 ) | // use tcon control
					( bit_num<<8 ) | // 0:10bits, 1:8bits, 2:6bits, 3:4bits.
					( 0<<10 ) | //r_select  //0:R, 1:G, 2:B, 3:0
					( 1<<12 ) | //g_select  //0:R, 1:G, 2:B, 3:0
					( 2<<14 ));  //b_select  //0:R, 1:G, 2:B, 3:0;

    WRITE_LCD_REG(LVDS_GEN_CNTL, (READ_MPEG_REG(LVDS_GEN_CNTL) | (1 << 0)));  //fifo enable
    //WRITE_MPEG_REG(LVDS_GEN_CNTL, (READ_MPEG_REG(LVDS_GEN_CNTL) | (1 << 3))); // enable fifo

	DBG_PRINT("lvds fifo clk = %d\n", (int)clk_util_clk_msr(LVDS_FIFO_CLK));
}

static void set_control_ttl(Lcd_Config_t *pConf)
{
	unsigned rb_port_swap, rgb_bit_swap;

	rb_port_swap = (unsigned)(pConf->lcd_control.ttl_config->rb_swap);
	rgb_bit_swap = (unsigned)(pConf->lcd_control.ttl_config->bit_swap);

	WRITE_LCD_REG(DUAL_PORT_CNTL_ADDR, (rb_port_swap << LCD_RGB_SWP) | (rgb_bit_swap << LCD_BIT_SWP));
}

static void set_control_mlvds(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s\n", __FUNCTION__);

	int test_bit_num = pConf->lcd_basic.lcd_bits;
    int test_pair_num = pConf->lcd_control.mlvds_config->test_pair_num;
    int test_dual_gate = pConf->lcd_control.mlvds_config->test_dual_gate;
    int scan_function = pConf->lcd_control.mlvds_config->scan_function;     //0:U->D,L->R  //1:D->U,R->L
    int mlvds_insert_start;
    unsigned int reset_offset;
    unsigned int reset_length;

    unsigned long data32;

    mlvds_insert_start = test_dual_gate ?
                           ((test_bit_num == 8) ? ((test_pair_num == 6) ? 0x9f : 0xa9) :
                                                  ((test_pair_num == 6) ? pConf->lcd_control.mlvds_config->mlvds_insert_start : 0xa7)
                           ) :
                           (
                             (test_pair_num == 6) ? ((test_bit_num == 8) ? 0xa9 : 0xa7) :
                                                    ((test_bit_num == 8) ? 0xae : 0xad)
                           );

    // Enable the LVDS PHY (power down bits)
    WRITE_MPEG_REG(LVDS_PHY_CNTL1,READ_MPEG_REG(LVDS_PHY_CNTL1) | (0x7F << 8) );

    data32 = (0x00 << LVDS_blank_data_r) |
             (0x00 << LVDS_blank_data_g) |
             (0x00 << LVDS_blank_data_b) ;
    WRITE_MPEG_REG(LVDS_BLANK_DATA_HI,  (data32 >> 16));
    WRITE_MPEG_REG(LVDS_BLANK_DATA_LO, (data32 & 0xffff));

    data32 = 0x7fffffff; //  '0'x1 + '1'x32 + '0'x2
    WRITE_MPEG_REG(MLVDS_RESET_PATTERN_HI,  (data32 >> 16));
    WRITE_MPEG_REG(MLVDS_RESET_PATTERN_LO, (data32 & 0xffff));
    data32 = 0x8000; // '0'x1 + '1'x32 + '0'x2
    WRITE_MPEG_REG(MLVDS_RESET_PATTERN_EXT,  (data32 & 0xffff));

    reset_length = 1+32+2;
    reset_offset = test_bit_num - (reset_length%test_bit_num);

    data32 = (reset_offset << mLVDS_reset_offset) |
             (reset_length << mLVDS_reset_length) |
             ((test_pair_num == 6) << mLVDS_data_write_toggle) |
             ((test_pair_num != 6) << mLVDS_data_write_ini) |
             ((test_pair_num == 6) << mLVDS_data_latch_1_toggle) |
             (0 << mLVDS_data_latch_1_ini) |
             ((test_pair_num == 6) << mLVDS_data_latch_0_toggle) |
             (1 << mLVDS_data_latch_0_ini) |
             ((test_pair_num == 6) << mLVDS_reset_1_select) |
             (mlvds_insert_start << mLVDS_reset_start);
    WRITE_MPEG_REG(MLVDS_CONFIG_HI,  (data32 >> 16));
    WRITE_MPEG_REG(MLVDS_CONFIG_LO, (data32 & 0xffff));

    data32 = (1 << mLVDS_double_pattern) |  //POL double pattern
			 (0x3f << mLVDS_ins_reset) |
             (test_dual_gate << mLVDS_dual_gate) |
             ((test_bit_num == 8) << mLVDS_bit_num) |
             ((test_pair_num == 6) << mLVDS_pair_num) |
             (0 << mLVDS_msb_first) |
             (0 << mLVDS_PORT_SWAP) |
             ((scan_function==1 ? 1:0) << mLVDS_MLSB_SWAP) |
             (0 << mLVDS_PN_SWAP) |
             (1 << mLVDS_en);
    WRITE_MPEG_REG(MLVDS_CONTROL,  (data32 & 0xffff));

    WRITE_MPEG_REG(LVDS_PACK_CNTL_ADDR,
                   ( 0 ) | // repack
                   ( 0<<2 ) | // odd_even
                   ( 0<<3 ) | // reserve
                   ( 0<<4 ) | // lsb first
                   ( 0<<5 ) | // pn swap
                   ( 0<<6 ) | // dual port
                   ( 0<<7 ) | // use tcon control
                   ( 1<<8 ) | // 0:10bits, 1:8bits, 2:6bits, 3:4bits.
                   ( (scan_function==1 ? 2:0)<<10 ) |  //r_select // 0:R, 1:G, 2:B, 3:0
                   ( 1<<12 ) |                        //g_select
                   ( (scan_function==1 ? 0:2)<<14 ));  //b_select

    WRITE_MPEG_REG(L_POL_CNTL_ADDR,  (1 << LCD_DCLK_SEL) |
       //(0x1 << LCD_HS_POL) |
       (0x1 << LCD_VS_POL)
    );

    //WRITE_MPEG_REG( LVDS_GEN_CNTL, (READ_MPEG_REG(LVDS_GEN_CNTL) | (1 << 3))); // enable fifo
}

static void init_phy_lvds(Lcd_Config_t *pConf)
{
	unsigned swing_ctrl;
	DBG_PRINT("%s\n", __FUNCTION__);

    WRITE_MPEG_REG(LVDS_PHY_CNTL3, 0xee1);  //0xee0
	WRITE_MPEG_REG(LVDS_PHY_CNTL4, 0);

	switch (pConf->lcd_control.lvds_config->lvds_vswing) {
		case 0:
			swing_ctrl = 0xaf40;
			break;
		case 1:
			swing_ctrl = 0xaf40;
			break;
		case 2:
			swing_ctrl = 0xa840;
			break;
		case 3:
			swing_ctrl = 0xa880;
			break;
		case 4:
			swing_ctrl = 0xa8c0;
			break;
		default:
			swing_ctrl = 0xaf40;
			break;
	}
	WRITE_LCD_REG(LVDS_PHY_CNTL5, swing_ctrl);

	WRITE_LCD_REG(LVDS_PHY_CNTL0,0x1f);  //0xfff
	WRITE_LCD_REG(LVDS_PHY_CNTL1,0xffff);

	WRITE_LCD_REG(LVDS_PHY_CNTL6,0xcccc);
	WRITE_LCD_REG(LVDS_PHY_CNTL7,0xcccc);
	WRITE_LCD_REG(LVDS_PHY_CNTL8,0xcccc);
}

static void set_video_adjust(Lcd_Config_t *pConf)
{
	DBG_PRINT("vadj_brightness = 0x%x, vadj_contrast = 0x%x, vadj_saturation = 0x%x.\n", pConf->lcd_effect.vadj_brightness, pConf->lcd_effect.vadj_contrast, pConf->lcd_effect.vadj_saturation);
	WRITE_LCD_REG(VPP_VADJ2_Y, (pConf->lcd_effect.vadj_brightness << 8) | (pConf->lcd_effect.vadj_contrast << 0));
	WRITE_LCD_REG(VPP_VADJ2_MA_MB, (pConf->lcd_effect.vadj_saturation << 16));
	WRITE_LCD_REG(VPP_VADJ2_MC_MD, (pConf->lcd_effect.vadj_saturation << 0));
	WRITE_LCD_REG(VPP_VADJ_CTRL, 0xf);	//enable video adjust
}

static unsigned error_abs(unsigned num1, unsigned num2)
{
	if (num1 >= num2)
		return num1 - num2;
	else
		return num2 - num1;
}

static void generate_clk_parameter(Lcd_Config_t *pConf)
{
	unsigned pll_n = 0, pll_m = 0, pll_od = 0;
	unsigned vid_div_pre = 0;
	unsigned crt_xd = 0;

	unsigned m, n, od, div_pre, div_post, xd;
	unsigned od_sel, pre_div_sel;
	unsigned div_pre_sel_max, crt_xd_max;
	unsigned f_ref, pll_vco, fout_pll, div_pre_out, div_post_out, final_freq, iflogic_vid_clk_in_max;
	unsigned min_error = MAX_ERROR;
	unsigned error = MAX_ERROR;
	unsigned clk_num = 0;

	unsigned fin = FIN_FREQ;
	unsigned fout = pConf->lcd_timing.lcd_clk;

	if (fout >= 200) {//clk
		fout = fout / 1000;  //kHz
	}
	else {//frame_rate
		fout = (fout * pConf->lcd_basic.h_period * pConf->lcd_basic.v_period) / 1000;	//kHz
	}

	switch (pConf->lcd_basic.lcd_type) {
		case LCD_DIGITAL_LVDS:
			div_pre_sel_max = DIV_PRE_SEL_MAX;
			div_post = 7;
			crt_xd_max = 1;
			iflogic_vid_clk_in_max = LVDS_MAX_VID_CLK_IN;
			break;
		case LCD_DIGITAL_TTL:
			div_pre_sel_max = DIV_PRE_SEL_MAX;
			div_post = 1;
			crt_xd_max = CRT_VID_DIV_MAX;
			iflogic_vid_clk_in_max = TTL_MAX_VID_CLK_IN;
			break;
		case LCD_DIGITAL_MINILVDS:
			div_pre_sel_max = DIV_PRE_SEL_MAX;
			div_post = 6;
			crt_xd_max = 1;
			iflogic_vid_clk_in_max = MLVDS_MAX_VID_CLK_IN;
			break;
		default:
			div_pre_sel_max = DIV_PRE_SEL_MAX;
			div_post = 1;
			crt_xd_max = 1;
			iflogic_vid_clk_in_max = LCD_VENC_MAX_CLK_IN;
			break;
	}

	for (n = PLL_N_MIN; n <= PLL_N_MAX; n++) {
		f_ref = fin / n;
		if ((f_ref >= PLL_FREF_MIN) && (f_ref <= PLL_FREF_MAX)) {
			for (m = PLL_M_MIN; m <= PLL_M_MAX; m++) {
				pll_vco = f_ref * m;
				if ((pll_vco >= PLL_VCO_MIN) && (pll_vco <= PLL_VCO_MAX)) {
					for (od_sel = OD_SEL_MAX; od_sel > 0; od_sel--) {
						od = od_table[od_sel - 1];
						fout_pll = pll_vco / od;
						if (fout_pll <= DIV_PRE_MAX_CLK_IN) {
							for (pre_div_sel = 0; pre_div_sel < div_pre_sel_max; pre_div_sel++) {
								div_pre = div_pre_table[pre_div_sel];
								div_pre_out = fout_pll / div_pre;
								if (div_pre_out <= DIV_POST_MAX_CLK_IN) {
									div_post_out = div_pre_out / div_post;
									if (div_post_out <= CRT_VID_MAX_CLK_IN) {
										for (xd = 1; xd <= crt_xd_max; xd++) {
											final_freq = div_post_out / xd;
											if (final_freq < LCD_VENC_MAX_CLK_IN) {
												if (final_freq < iflogic_vid_clk_in_max) {
													error = error_abs(final_freq, fout);
													if (error < min_error) {
														min_error = error;
														pll_m = m;
														pll_n = n;
														pll_od = od_sel - 1;
														vid_div_pre = pre_div_sel;
														crt_xd = xd;
														clk_num++;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (clk_num > 0) {
		pConf->lcd_timing.pll_ctrl = (pll_od << PLL_CTRL_OD) | (pll_n << PLL_CTRL_N) | (pll_m << PLL_CTRL_M);
		pConf->lcd_timing.div_ctrl = 0x18803 | (vid_div_pre << DIV_CTRL_DIV_PRE);
		pConf->lcd_timing.clk_ctrl = (pConf->lcd_timing.clk_ctrl & ~(0xf << CLK_CTRL_XD)) | (crt_xd << CLK_CTRL_XD);
	}
	else {
		pConf->lcd_timing.pll_ctrl = (1 << PLL_CTRL_OD) | (1 << PLL_CTRL_N) | (32 << PLL_CTRL_M);
		pConf->lcd_timing.div_ctrl = 0x18803;
		pConf->lcd_timing.clk_ctrl = (pConf->lcd_timing.clk_ctrl & ~(0xf << CLK_CTRL_XD)) | (7 << CLK_CTRL_XD);
		printf("Out of clock range, reset to default setting!\n");
	}
}

static void lcd_sync_duration(Lcd_Config_t *pConf)
{
	unsigned m, n, od, pre_div, xd, post_div;
	unsigned h_period, v_period, sync_duration;
	unsigned lcd_clk;

	m = ((pConf->lcd_timing.pll_ctrl) >> PLL_CTRL_M) & 0x1ff;
	n = ((pConf->lcd_timing.pll_ctrl) >> PLL_CTRL_N) & 0x1f;
	od = ((pConf->lcd_timing.pll_ctrl) >> PLL_CTRL_OD) & 0x3;
	od = od_table[od];
	pre_div = ((pConf->lcd_timing.div_ctrl) >> DIV_CTRL_DIV_PRE) & 0x7;
	pre_div = div_pre_table[pre_div];

	h_period = pConf->lcd_basic.h_period;
	v_period = pConf->lcd_basic.v_period;

	switch(pConf->lcd_basic.lcd_type) {
		case LCD_DIGITAL_LVDS:
			xd = 1;
			post_div = 7;
			break;
		case LCD_DIGITAL_TTL:
			xd = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_XD) & 0xf;
			post_div = 1;
			break;
		case LCD_DIGITAL_MINILVDS:
			xd = 1;
			post_div = 6;
			break;
		default:
			xd = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_XD) & 0xf;
			post_div = 1;
			break;
	}

	lcd_clk = ((m * FIN_FREQ) / (n * od * pre_div * xd * post_div)) * 1000;
	pConf->lcd_timing.lcd_clk = lcd_clk;
	sync_duration = ((lcd_clk / h_period) * 100) / v_period;
	sync_duration = (sync_duration + 5) / 10;

	pConf->lcd_timing.sync_duration_num = sync_duration;
	pConf->lcd_timing.sync_duration_den = 10;
	printf("lcd_clk=%u.%03uMHz, frame_rate=%u.%uHz.\n\n", (lcd_clk / 1000000), ((lcd_clk / 1000) % 1000), (sync_duration / pConf->lcd_timing.sync_duration_den), ((sync_duration * 10 / pConf->lcd_timing.sync_duration_den) % 10));
}

static void lcd_tcon_config(Lcd_Config_t *pConf)
{
	unsigned short hstart, hend, vstart, vend;
	unsigned short h_delay = 0;
	unsigned short h_offset = 0, v_offset = 0;

	switch (pConf->lcd_basic.lcd_type) {
		case LCD_DIGITAL_LVDS:
			h_delay = LVDS_DELAY;
			break;
		case LCD_DIGITAL_TTL:
			h_delay = TTL_DELAY;
			break;
		case LCD_DIGITAL_MINILVDS:
			h_delay = MLVDS_DELAY;
			break;
		default:
			h_delay = 0;
			break;
	}
#if 0
	h_offset = (pConf->lcd_timing.h_offset & 0xffff);
	v_offset = (pConf->lcd_timing.v_offset & 0xffff);
	if ((pConf->lcd_timing.h_offset >> 31) & 1)
		pConf->lcd_timing.de_hstart = (pConf->lcd_timing.video_on_pixel + pConf->lcd_basic.h_period + h_delay + h_offset) % pConf->lcd_basic.h_period;
	else
		pConf->lcd_timing.de_hstart = (pConf->lcd_timing.video_on_pixel + pConf->lcd_basic.h_period + h_delay - h_offset) % pConf->lcd_basic.h_period;
	if ((pConf->lcd_timing.v_offset >> 31) & 1)
		pConf->lcd_timing.de_vstart = (pConf->lcd_timing.video_on_line + pConf->lcd_basic.v_period + v_offset) % pConf->lcd_basic.v_period;
	else
		pConf->lcd_timing.de_vstart = (pConf->lcd_timing.video_on_line + pConf->lcd_basic.v_period - v_offset) % pConf->lcd_basic.v_period;

	hstart = (pConf->lcd_timing.de_hstart + pConf->lcd_basic.h_period - pConf->lcd_timing.hsync_bp) % pConf->lcd_basic.h_period;
	hend = (pConf->lcd_timing.de_hstart + pConf->lcd_basic.h_period - pConf->lcd_timing.hsync_bp + pConf->lcd_timing.hsync_width) % pConf->lcd_basic.h_period;
	pConf->lcd_timing.sth1_hs_addr = hstart;
	pConf->lcd_timing.sth1_he_addr = hend;
	pConf->lcd_timing.sth1_vs_addr = 0;
	pConf->lcd_timing.sth1_ve_addr = pConf->lcd_basic.v_period - 1;

	vstart = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp) % pConf->lcd_basic.v_period;
	vend = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp + pConf->lcd_timing.vsync_width) % pConf->lcd_basic.v_period;
	pConf->lcd_timing.stv1_vs_addr = vstart;
	pConf->lcd_timing.stv1_ve_addr = vend;
	pConf->lcd_timing.stv1_hs_addr = hstart;
	pConf->lcd_timing.stv1_he_addr = hstart;

	pConf->lcd_timing.de_hstart = pConf->lcd_timing.de_hstart;
	pConf->lcd_timing.de_vstart = pConf->lcd_timing.de_vstart;

	pConf->lcd_timing.oeh_hs_addr = pConf->lcd_timing.de_hstart;
	pConf->lcd_timing.oeh_he_addr = (pConf->lcd_timing.de_hstart + pConf->lcd_basic.h_active) % pConf->lcd_basic.h_period;
	pConf->lcd_timing.oeh_vs_addr = pConf->lcd_timing.de_vstart;
	pConf->lcd_timing.oeh_ve_addr = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_active - 1) % pConf->lcd_basic.v_period;
#else
    pConf->lcd_timing.video_on_pixel = pConf->lcd_basic.h_period - pConf->lcd_basic.h_active - 1 -h_delay;
    pConf->lcd_timing.video_on_line = pConf->lcd_basic.v_period - pConf->lcd_basic.v_active;

    h_offset = (pConf->lcd_timing.h_offset & 0xffff);
    v_offset = (pConf->lcd_timing.v_offset & 0xffff);
    if ((pConf->lcd_timing.h_offset >> 31) & 1)
        pConf->lcd_timing.de_hstart = (pConf->lcd_basic.h_period - pConf->lcd_basic.h_active - 1 + pConf->lcd_basic.h_period - h_offset) % pConf->lcd_basic.h_period;
    else
        pConf->lcd_timing.de_hstart = (pConf->lcd_basic.h_period - pConf->lcd_basic.h_active - 1 + h_offset) % pConf->lcd_basic.h_period;
    if ((pConf->lcd_timing.v_offset >> 31) & 1)
        pConf->lcd_timing.de_vstart = (pConf->lcd_basic.v_period - pConf->lcd_basic.v_active + pConf->lcd_basic.v_period - v_offset) % pConf->lcd_basic.v_period;
    else
        pConf->lcd_timing.de_vstart = (pConf->lcd_basic.v_period - pConf->lcd_basic.v_active + v_offset) % pConf->lcd_basic.v_period;

    hstart = (pConf->lcd_timing.de_hstart + pConf->lcd_basic.h_period - pConf->lcd_timing.hsync_bp) % pConf->lcd_basic.h_period;
    hend = (pConf->lcd_timing.de_hstart + pConf->lcd_basic.h_period - pConf->lcd_timing.hsync_bp + pConf->lcd_timing.hsync_width) % pConf->lcd_basic.h_period;
    pConf->lcd_timing.sth1_hs_addr = hstart;
    pConf->lcd_timing.sth1_he_addr = hend;
    pConf->lcd_timing.sth1_vs_addr = 0;
    pConf->lcd_timing.sth1_ve_addr = pConf->lcd_basic.v_period - 1;

    pConf->lcd_timing.stv1_hs_addr = hstart;
    pConf->lcd_timing.stv1_he_addr = hstart;
    vstart = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp) % pConf->lcd_basic.v_period;
    vend = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp + pConf->lcd_timing.vsync_width) % pConf->lcd_basic.v_period;
    pConf->lcd_timing.stv1_vs_addr = vstart;
    pConf->lcd_timing.stv1_ve_addr = vend;

    pConf->lcd_timing.oeh_hs_addr = pConf->lcd_timing.de_hstart;
    pConf->lcd_timing.oeh_he_addr = (pConf->lcd_timing.de_hstart + pConf->lcd_basic.h_active) % pConf->lcd_basic.h_period;
    pConf->lcd_timing.oeh_vs_addr = pConf->lcd_timing.de_vstart;
    pConf->lcd_timing.oeh_ve_addr = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_active - 1) % pConf->lcd_basic.v_period;
#endif

	DBG_PRINT("sth1_hs_addr=%d, sth1_he_addr=%d, sth1_vs_addr=%d, sth1_ve_addr=%d\n", pConf->lcd_timing.sth1_hs_addr, pConf->lcd_timing.sth1_he_addr, pConf->lcd_timing.sth1_vs_addr, pConf->lcd_timing.sth1_ve_addr);
	DBG_PRINT("stv1_hs_addr=%d, stv1_he_addr=%d, stv1_vs_addr=%d, stv1_ve_addr=%d\n", pConf->lcd_timing.stv1_hs_addr, pConf->lcd_timing.stv1_he_addr, pConf->lcd_timing.stv1_vs_addr, pConf->lcd_timing.stv1_ve_addr);
	DBG_PRINT("oeh_hs_addr=%d, oeh_he_addr=%d, oeh_vs_addr=%d, oeh_ve_addr=%d\n", pConf->lcd_timing.oeh_hs_addr, pConf->lcd_timing.oeh_he_addr, pConf->lcd_timing.oeh_vs_addr, pConf->lcd_timing.oeh_ve_addr);
}

static void lcd_control_config(Lcd_Config_t *pConf)
{
	unsigned char ss_level = (pConf->lcd_timing.clk_ctrl >> CLK_CTRL_SS) & 0xf;

	ss_level = ((ss_level >= SS_LEVEL_MAX) ? (SS_LEVEL_MAX-1) : ss_level);

	switch (pConf->lcd_basic.lcd_type) {
		case LCD_DIGITAL_LVDS:
			if (pConf->lcd_control.lvds_config->lvds_repack_user == 0) {
				if (pConf->lcd_basic.lcd_bits == 6)
					pConf->lcd_control.lvds_config->lvds_repack = 0;
				else
					pConf->lcd_control.lvds_config->lvds_repack = 1;
			}
			break;
		default:
			break;
	}
    pConf->lcd_timing.clk_ctrl = ((pConf->lcd_timing.clk_ctrl & ~(0xf << CLK_CTRL_SS)) | (ss_level << CLK_CTRL_SS));
}

static void lcd_config_init(Lcd_Config_t *pConf)
{
#ifdef CONFIG_PLATFORM_HAS_PMU
	struct aml_pmu_driver *pmu_driver;
	int battery_percent;
#endif

	lcd_control_config(pConf);
	if (pConf->lcd_timing.clk_ctrl & (1 << CLK_CTRL_AUTO)) {
		printf("Auto generate clock parameters.\n");
		generate_clk_parameter(pConf);
		DBG_PRINT("pll_ctrl=0x%x, div_ctrl=0x%x, clk_ctrl=0x%x.\n", pConf->lcd_timing.pll_ctrl, pConf->lcd_timing.div_ctrl, pConf->lcd_timing.clk_ctrl);
	}
	else {
		printf("Custome clock parameters.\n");
		printf("pll_ctrl=0x%x, div_ctrl=0x%x, clk_ctrl=0x%x.\n", pConf->lcd_timing.pll_ctrl, pConf->lcd_timing.div_ctrl, pConf->lcd_timing.clk_ctrl);
	}

	lcd_sync_duration(pConf);
	lcd_tcon_config(pConf);

	if (pDev->bl_config->level_default == pDev->bl_config->level_min) {
		set_lcd_backlight_level(pDev->bl_config->level_min);
	}
	else {
#ifdef CONFIG_PLATFORM_HAS_PMU
		/* if battery percentage is very low, set backlight level as low as possible  */
		pmu_driver = aml_pmu_get_driver();
		if (pmu_driver && pmu_driver->pmu_get_battery_capacity) {
			battery_percent = pmu_driver->pmu_get_battery_capacity();
			if (battery_percent <= BATTERY_LOW_THRESHOLD) {
				set_lcd_backlight_level(pDev->bl_config->level_min + battery_percent + 10);
			} else {
				set_lcd_backlight_level(pDev->bl_config->level_default);
			}
		} else {
			set_lcd_backlight_level(pDev->bl_config->level_default);
		}
#else
		set_lcd_backlight_level(pDev->bl_config->level_default);
#endif
	}
}

static void print_lcd_clock(void)
{
	DBG_PRINT("viid pll clk = %d\n", clk_util_clk_msr(12));
	DBG_PRINT("cts enct clk = %d\n", clk_util_clk_msr(10));
	DBG_PRINT("cts encl clk = %d\n", clk_util_clk_msr(9));
	DBG_PRINT("lvds fifo clk = %d\n", clk_util_clk_msr(24));
}

static void _init_lcd_driver(Lcd_Config_t *pConf)	//before power on lcd
{
	int lcd_type = pConf->lcd_basic.lcd_type;
	unsigned char ss_level = (pConf->lcd_timing.clk_ctrl >> CLK_CTRL_SS) & 0xf;

	printf("\nInit LCD mode: %s(%u) %ubit, %ux%u@%u.%uHz, ss_level=%u(%s)\n", lcd_type_table[lcd_type], lcd_type, pConf->lcd_basic.lcd_bits, pConf->lcd_basic.h_active, pConf->lcd_basic.v_active, (pConf->lcd_timing.sync_duration_num / 10), (pConf->lcd_timing.sync_duration_num % 10), ss_level, lcd_ss_level_table[ss_level]);

	switch(lcd_type) {
		case LCD_DIGITAL_LVDS:
			set_pll_lcd(pConf);
			venc_set_lvds(pConf);
			set_tcon_lvds(pConf);
			set_control_lvds(pConf);
			init_phy_lvds(pConf);
			break;
		case LCD_DIGITAL_TTL:
			set_pll_lcd(pConf);
			venc_set_ttl(pConf);
			set_tcon_ttl(pConf);
			set_control_ttl(pConf);
			break;
		case LCD_DIGITAL_MINILVDS:
			set_pll_mlvds(pConf);
			venc_set_mlvds(pConf);
			set_tcon_mlvds(pConf);
			set_control_mlvds(pConf);
			init_phy_lvds(pConf);
			break;
		default:
			printf("Invalid LCD type.\n");
			break;
	}
	set_video_adjust(pConf);
	printf("%s finished.\n", __FUNCTION__);
}

static void _disable_lcd_driver(Lcd_Config_t *pConf)	//after power off lcd
{
	int vclk_sel;

	vclk_sel = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_VCLK_SEL) & 0x1;

	WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 0, 3, 1);	//disable lvds fifo
	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 0, 11, 1);	//close lvds phy clk gate: 0x104c[11]

	WRITE_LCD_REG(ENCT_VIDEO_EN, 0);	//disable enct
	WRITE_LCD_REG(ENCL_VIDEO_EN, 0);	//disable encl

	if (vclk_sel)
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 0, 0, 5);	//close vclk2 gate: 0x104b[4:0]
	else
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 0, 0, 5);		//close vclk1 gate: 0x105f[4:0]

	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 0, 16, 1);	//close vid2_pll gate: 0x104c[16]

	WRITE_LCD_CBUS_REG_BITS(HHI_VIID_PLL_CNTL, 1, 30, 1);	//power down vid2_pll: 0x1047[30]
	printf("disable lcd display driver.\n");
}

static inline void _enable_vsync_interrupt(void)
{
	if ((READ_LCD_REG(ENCT_VIDEO_EN) & 1) || (READ_LCD_REG(ENCL_VIDEO_EN) & 1)) {
		WRITE_LCD_REG(VENC_INTCTRL, 0x200);
	}
	else{
		WRITE_LCD_REG(VENC_INTCTRL, 0x2);
	}
}

static void _lcd_module_enable(void)
{
	BUG_ON(pDev==NULL);
	_init_lcd_driver(pDev->pConf);
	lcd_power_ctrl(ON);
	//lcd_backlight_power_ctrl(ON);	//disable backlight at pannel init
	_enable_vsync_interrupt();
}

static int lcd_set_current_vmode(vmode_t mode)	//set display width
{
	if (mode != VMODE_LCD)
		return -1;

	WRITE_LCD_REG(VPP_POSTBLEND_H_SIZE, pDev->lcd_info.width);
	_lcd_module_enable();
	if (VMODE_INIT_NULL == pDev->lcd_info.mode)
		pDev->lcd_info.mode = VMODE_LCD;
	else
		lcd_backlight_power_ctrl(ON);
	return 0;
}

static void _init_vout(lcd_dev_t *pDev)
{
    pDev->lcd_info.name = PANEL_NAME;
    pDev->lcd_info.mode = VMODE_INIT_NULL;
    pDev->lcd_info.width = pDev->pConf->lcd_basic.h_active;
    pDev->lcd_info.height = pDev->pConf->lcd_basic.v_active;
    pDev->lcd_info.field_height = pDev->pConf->lcd_basic.v_active;
    pDev->lcd_info.aspect_ratio_num = pDev->pConf->lcd_basic.screen_ratio_width;
    pDev->lcd_info.aspect_ratio_den = pDev->pConf->lcd_basic.screen_ratio_height;
    pDev->lcd_info.screen_real_width= pDev->pConf->lcd_basic.h_active_area;
    pDev->lcd_info.screen_real_height= pDev->pConf->lcd_basic.v_active_area;
    pDev->lcd_info.sync_duration_num = pDev->pConf->lcd_timing.sync_duration_num;
    pDev->lcd_info.sync_duration_den = pDev->pConf->lcd_timing.sync_duration_den;
}

static void _lcd_init(Lcd_Config_t *pConf)
{
	lcd_config_init(pConf);
	_init_vout(pDev);
	//_lcd_module_enable();	//remove repeatedly lcd_module_enable
	lcd_set_current_vmode(VMODE_LCD);
}

static int aml_lcd_pmu_gpio_name_map_num(const char *name)
{
	int index;

	for(index = 0; index < LCD_POWER_PMU_GPIO_MAX; index++) {
		if(!strcmp(name, lcd_power_pmu_gpio_table[index]))
			break;
	}
	return index;
}

#ifdef CONFIG_OF_LIBFDT
static inline int _get_lcd_model_timing(Lcd_Config_t *pConf)
{
	int ret=0;
	int nodeoffset;
	char* lcd_model;
	char* propdata;
	char propname[30];
	int i;

	nodeoffset = fdt_path_offset(dt_addr, "/lcd");
	if(nodeoffset < 0) {
		printf("dts: not find /lcd node %s.\n",fdt_strerror(nodeoffset));
		return ret;
	}

	lcd_model = fdt_getprop(dt_addr, nodeoffset, "lcd_model_name", NULL);
	sprintf(propname, "/%s", lcd_model);
	nodeoffset = fdt_path_offset(dt_addr, propname);
	if(nodeoffset < 0) {
		printf("dts: not find %s node %s.\n", propname, fdt_strerror(nodeoffset));
		return ret;
	}

	lcd_model = fdt_getprop(dt_addr, nodeoffset, "model_name", NULL);
	if (lcd_model == NULL) {
		printf("faild to get model_name\n");
		lcd_model = PANEL_MODEL_DEFAULT;
	}
	pConf->lcd_basic.model_name = lcd_model;
	printf("\nload lcd model in dtb: %s\n", pConf->lcd_basic.model_name);

	propdata = fdt_getprop(dt_addr, nodeoffset, "interface", NULL);
	if (propdata == NULL) {
		printf("faild to get lcd_type!\n");
		pConf->lcd_basic.lcd_type = LCD_TYPE_MAX;
	}
	else {
		for(i = 0; i < LCD_TYPE_MAX; i++) {
			if(!strncmp(propdata, lcd_type_table_match[i], 3))
				break;
		}
		pConf->lcd_basic.lcd_type = i;
	}
	DBG_PRINT("lcd_type = %s(%u),\n", lcd_type_table[pConf->lcd_basic.lcd_type], pConf->lcd_basic.lcd_type);
	propdata = fdt_getprop(dt_addr, nodeoffset, "active_area", NULL);
	if(propdata == NULL){
		printf("faild to get active_area\n");
	}
	else {
		pConf->lcd_basic.h_active_area = be32_to_cpup((u32*)propdata);
		pConf->lcd_basic.v_active_area = be32_to_cpup((((u32*)propdata)+1));
		pConf->lcd_basic.screen_ratio_width = be32_to_cpup((u32*)propdata);
		pConf->lcd_basic.screen_ratio_height = be32_to_cpup((((u32*)propdata)+1));
	}
	DBG_PRINT("h_active_area = %u, v_active_area = %u\n", pConf->lcd_basic.h_active_area, pConf->lcd_basic.v_active_area);
	propdata = fdt_getprop(dt_addr, nodeoffset, "lcd_bits_option", NULL);
	if(propdata == NULL){
		printf("faild to get lcd_bits_option\n");
	}
	else {
		pConf->lcd_basic.lcd_bits = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_basic.lcd_bits_option = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
	}
	DBG_PRINT("lcd_bits = %u, lcd_bits_option = %u\n", pConf->lcd_basic.lcd_bits, pConf->lcd_basic.lcd_bits_option);

	propdata = fdt_getprop(dt_addr, nodeoffset, "resolution", NULL);
	if(propdata == NULL){
		printf("faild to get resolution\n");
	}
	else {
		pConf->lcd_basic.h_active = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_basic.v_active = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "period", NULL);
	if(propdata == NULL){
		printf("faild to get period\n");
	}
	else {
		pConf->lcd_basic.h_period = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_basic.v_period = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
	}
	DBG_PRINT("h_active = %u, v_active =%u, h_period = %u, v_period = %u\n", pConf->lcd_basic.h_active, pConf->lcd_basic.v_active, pConf->lcd_basic.h_period, pConf->lcd_basic.v_period);

	propdata = fdt_getprop(dt_addr, nodeoffset, "clock_hz_pol", NULL);
	if(propdata == NULL){
		printf("faild to get clock_hz_pol\n");
	}
	else {
		pConf->lcd_timing.lcd_clk = be32_to_cpup((u32*)propdata);
		pConf->lcd_timing.pol_cntl_addr = (be32_to_cpup((((u32*)propdata)+1)) << LCD_CPH1_POL);
	}
	DBG_PRINT("pclk = %uHz, pol=%u\n", pConf->lcd_timing.lcd_clk, (pConf->lcd_timing.pol_cntl_addr >> LCD_CPH1_POL) & 1);
	propdata = fdt_getprop(dt_addr, nodeoffset, "hsync_width_backporch", NULL);
	if(propdata == NULL){
		printf("faild to get hsync_width_backporch\n");
	}
	else {
		pConf->lcd_timing.hsync_width = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_timing.hsync_bp = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
	}
	DBG_PRINT("hsync width = %u, backporch = %u\n", pConf->lcd_timing.hsync_width, pConf->lcd_timing.hsync_bp);
	propdata = fdt_getprop(dt_addr, nodeoffset, "vsync_width_backporch", NULL);
	if(propdata == NULL){
		printf("faild to get vsync_width_backporch\n");
	}
	else {
		pConf->lcd_timing.vsync_width = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_timing.vsync_bp = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
	}
	DBG_PRINT("vsync width = %u, backporch = %u\n", pConf->lcd_timing.vsync_width, pConf->lcd_timing.vsync_bp);
	propdata = fdt_getprop(dt_addr, nodeoffset, "pol_hsync_vsync", NULL);
	if(propdata == NULL){
		printf("faild to get pol_hsync_vsync\n");
	}
	else {
		pConf->lcd_timing.pol_cntl_addr = (pConf->lcd_timing.pol_cntl_addr & ~((1 << LCD_HS_POL) | (1 << LCD_VS_POL))) | ((be32_to_cpup((u32*)propdata) << LCD_HS_POL) | (be32_to_cpup((((u32*)propdata)+1)) << LCD_VS_POL));
	}
	DBG_PRINT("pol hsync = %u, vsync = %u\n", (pConf->lcd_timing.pol_cntl_addr >> LCD_HS_POL) & 1, (pConf->lcd_timing.pol_cntl_addr >> LCD_VS_POL) & 1);
	return ret;
}

static inline int _get_lcd_default_config(Lcd_Config_t *pConf)
{
	int ret=0;
	int nodeoffset;
	char * propdata;
	int i;

	nodeoffset = fdt_path_offset(dt_addr, "/lcd");
	if(nodeoffset < 0) {
		printf("lcd driver init: not find /lcd node %s.\n",fdt_strerror(nodeoffset));
		return ret;
	}

	if (pConf->lcd_basic.lcd_bits_option == 1) {
		propdata = fdt_getprop(dt_addr, nodeoffset, "lcd_bits_user", NULL);
		if(propdata == NULL){
			DBG_PRINT("don't find to match lcd_bits_user, use panel typical setting.\n");
		}
		else {
			pConf->lcd_basic.lcd_bits = (unsigned short)(be32_to_cpup((u32*)propdata));
			printf("lcd_bits = %u\n", pConf->lcd_basic.lcd_bits);
		}
	}

	//hardware design config
	propdata = fdt_getprop(dt_addr, nodeoffset, "ttl_rb_bit_swap", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match ttl_rb_bit_swap, use default setting.\n");
	}
	else {
		pConf->lcd_control.ttl_config->rb_swap = (unsigned char)(be32_to_cpup((u32*)propdata));
		pConf->lcd_control.ttl_config->bit_swap = (unsigned char)(be32_to_cpup((((u32*)propdata)+1)));
		DBG_PRINT("ttl rb_swap = %u, bit_swap = %u\n", pConf->lcd_control.ttl_config->rb_swap, pConf->lcd_control.ttl_config->bit_swap);
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "lvds_channel_pn_swap", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match lvds_channel_pn_swap, use default setting.\n");
	}
	else {
		pConf->lcd_control.lvds_config->pn_swap = be32_to_cpup((u32*)propdata);
		DBG_PRINT("lvds_pn_swap = %u\n", pConf->lcd_control.lvds_config->pn_swap);
	}

	//recommend setting
	propdata = fdt_getprop(dt_addr, nodeoffset, "valid_hvsync_de", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match valid_hvsync_de, use default setting.\n");
	}
	else {
		pConf->lcd_timing.hvsync_valid = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_timing.de_valid = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
		DBG_PRINT("valid hvsync = %u, de = %u\n", pConf->lcd_timing.hvsync_valid, pConf->lcd_timing.de_valid);
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "hsign_hoffset_vsign_voffset", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match hsign_hoffset_vsign_voffset, use default setting.\n");
		pDev->pConf->lcd_timing.h_offset = 0;
		pDev->pConf->lcd_timing.v_offset = 0;
	}
	else {
		pConf->lcd_timing.h_offset = ((be32_to_cpup((u32*)propdata) << 31) | ((be32_to_cpup((((u32*)propdata)+1)) & 0xffff) << 0));
		pConf->lcd_timing.v_offset = ((be32_to_cpup((((u32*)propdata)+2)) << 31) | ((be32_to_cpup((((u32*)propdata)+3)) & 0xffff) << 0));
		DBG_PRINT("h_offset = %s%u, ", (((pConf->lcd_timing.h_offset >> 31) & 1) ? "-" : ""), (pConf->lcd_timing.h_offset & 0xffff));
		DBG_PRINT("v_offset = %s%u\n", (((pConf->lcd_timing.v_offset >> 31) & 1) ? "-" : ""), (pConf->lcd_timing.v_offset & 0xffff));
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "dither_user_ctrl", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match dither_user_ctrl, use default setting.\n");
	}
	else {
		pConf->lcd_effect.dith_user = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_effect.dith_cntl_addr = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
		DBG_PRINT("dither_user = %u, dither_ctrl = 0x%x\n", pConf->lcd_effect.dith_user, pConf->lcd_effect.dith_cntl_addr);
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "vadj_brightness_contrast_saturation", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match vadj_brightness_contrast_saturation, use default setting.\n");
	}
	else {
		pConf->lcd_effect.vadj_brightness = be32_to_cpup((u32*)propdata);
		pConf->lcd_effect.vadj_contrast = be32_to_cpup((((u32*)propdata)+1));
		pConf->lcd_effect.vadj_saturation = be32_to_cpup((((u32*)propdata)+2));
		DBG_PRINT("vadj_brightness = 0x%x, vadj_contrast = 0x%x, vadj_saturation = 0x%x\n", pConf->lcd_effect.vadj_brightness, pConf->lcd_effect.vadj_contrast, pConf->lcd_effect.vadj_saturation);
	}

	propdata = fdt_getprop(dt_addr, nodeoffset, "gamma_en_revert", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match gamma_en_revert, use default setting.\n");
	}
	else {
		pConf->lcd_effect.gamma_cntl_port = (be32_to_cpup((u32*)propdata) << LCD_GAMMA_EN);
		pConf->lcd_effect.gamma_revert = (unsigned char)(be32_to_cpup((((u32*)propdata)+1)));
		//pConf->lcd_effect.gamma_vcom_hswitch_addr = 0;
		DBG_PRINT("gamma_en = %u, gamma_revert = %u\n", ((pConf->lcd_effect.gamma_cntl_port >> LCD_GAMMA_EN) & 1), pConf->lcd_effect.gamma_revert);
	}
	unsigned int lcd_gamma_multi = 0;
	propdata = fdt_getprop(dt_addr, nodeoffset, "gamma_multi_rgb_coeff", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match gamma_multi_rgb_coeff, use default setting.\n");
	}
	else {
		lcd_gamma_multi = be32_to_cpup((u32*)propdata);
		pConf->lcd_effect.gamma_r_coeff = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
		pConf->lcd_effect.gamma_g_coeff = (unsigned short)(be32_to_cpup((((u32*)propdata)+2)));
		pConf->lcd_effect.gamma_b_coeff = (unsigned short)(be32_to_cpup((((u32*)propdata)+3)));
		DBG_PRINT("gamma_multi = %u, gamma_r_coeff = %u, gamma_g_coeff = %u, gamma_b_coeff = %u\n", lcd_gamma_multi, pConf->lcd_effect.gamma_r_coeff, pConf->lcd_effect.gamma_g_coeff, pConf->lcd_effect.gamma_b_coeff);
	}
	if (lcd_gamma_multi == 1) {
		propdata = fdt_getprop(dt_addr, nodeoffset, "gamma_table_r", NULL);
		if(propdata == NULL){
			DBG_PRINT("don't find to match gamma_table_r, use default table.\n");
			lcd_setup_gamma_table(pConf, 0);
		}
		else {
			for (i=0; i<256; i++) {
				pConf->lcd_effect.GammaTableR[i] = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)) << 2);
			}
			DBG_PRINT("load gamma_table_r.\n");
		}
		propdata = fdt_getprop(dt_addr, nodeoffset, "gamma_table_g", NULL);
		if(propdata == NULL){
			DBG_PRINT("don't find to match gamma_table_g, use default table.\n");
			lcd_setup_gamma_table(pConf, 1);
		}
		else {
			for (i=0; i<256; i++) {
				pConf->lcd_effect.GammaTableG[i] = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)) << 2);
			}
			DBG_PRINT("load gamma_table_g.\n");
		}
		propdata = fdt_getprop(dt_addr, nodeoffset, "gamma_table_b", NULL);
		if(propdata == NULL){
			DBG_PRINT("don't find to match gamma_table_b, use default table.\n");
			lcd_setup_gamma_table(pConf, 2);
		}
		else {
			for (i=0; i<256; i++) {
				pConf->lcd_effect.GammaTableB[i] = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)) << 2);
			}
			DBG_PRINT("load gamma_table_b.\n");
		}
	}
	else {
		propdata = fdt_getprop(dt_addr, nodeoffset, "gamma_table", NULL);
		if(propdata == NULL){
			DBG_PRINT("don't find to match gamma_table, use default table.\n");
			lcd_setup_gamma_table(pConf, 3);
		}
		else {
			for (i=0; i<256; i++) {
				pConf->lcd_effect.GammaTableR[i] = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)) << 2);
				pConf->lcd_effect.GammaTableG[i] = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)) << 2);
				pConf->lcd_effect.GammaTableB[i] = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)) << 2);
			}
			DBG_PRINT("load gamma_table.\n");
		}
	}

	propdata = fdt_getprop(dt_addr, nodeoffset, "clock_spread_spectrum", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match clock_spread_spectrum, use default setting.\n");
	}
	else {
		pConf->lcd_timing.clk_ctrl = (pConf->lcd_timing.clk_ctrl & ~(0xf << CLK_CTRL_SS)) | (be32_to_cpup((u32*)propdata) << CLK_CTRL_SS);
		DBG_PRINT("lcd_clock spread_spectrum = %u\n", (pConf->lcd_timing.clk_ctrl >> CLK_CTRL_SS) & 0xf);
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "clock_auto_generation", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match clock_auto_generation, use default setting.\n");
	}
	else {
		pConf->lcd_timing.clk_ctrl = ((pConf->lcd_timing.clk_ctrl & ~(1 << CLK_CTRL_AUTO)) | (be32_to_cpup((u32*)propdata) << CLK_CTRL_AUTO));
		DBG_PRINT("lcd_clock auto_generation = %u\n", (pConf->lcd_timing.clk_ctrl >> CLK_CTRL_AUTO) & 1);
	}
	if (((pConf->lcd_timing.clk_ctrl >> CLK_CTRL_AUTO) & 1) == 0) {
		propdata = fdt_getprop(dt_addr, nodeoffset, "clk_pll_div_clk_ctrl", NULL);
		if(propdata == NULL){
			DBG_PRINT("don't find to match clk_pll_div_clk_ctrl, use default setting.\n");
		}
		else {
			pConf->lcd_timing.pll_ctrl = be32_to_cpup((u32*)propdata);
			pConf->lcd_timing.div_ctrl = be32_to_cpup((((u32*)propdata)+1));
			pConf->lcd_timing.clk_ctrl = (pConf->lcd_timing.clk_ctrl & ~(0xffff >> 0)) | (be32_to_cpup((((u32*)propdata)+2)) >> 0);
			printf("pll_ctrl = 0x%x, div_ctrl = 0x%x, clk_ctrl=0x%x\n", pConf->lcd_timing.pll_ctrl, pConf->lcd_timing.div_ctrl, (pConf->lcd_timing.clk_ctrl & 0xffff));
		}
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "lvds_vswing", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match lvds_vswing, use default setting.\n");
	}
	else {
		pConf->lcd_control.lvds_config->lvds_vswing = be32_to_cpup((u32*)propdata);
		if (pConf->lcd_basic.lcd_type == LCD_DIGITAL_LVDS)
			printf("lvds_vswing = 0x%x\n", pConf->lcd_control.lvds_config->lvds_vswing);
		else
			DBG_PRINT("lvds_vswing = 0x%x\n", pConf->lcd_control.lvds_config->lvds_vswing);
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "lvds_user_repack", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match lvds_user_repack, use default setting.\n");
		pConf->lcd_control.lvds_config->lvds_repack_user = 0;
		pConf->lcd_control.lvds_config->lvds_repack = 1;
	}
	else {
		pConf->lcd_control.lvds_config->lvds_repack_user = be32_to_cpup((u32*)propdata);
		pConf->lcd_control.lvds_config->lvds_repack = be32_to_cpup((((u32*)propdata)+1));
		if ((be32_to_cpup((u32*)propdata)) == 0) {
			DBG_PRINT("lvds_repack_user = %u, lvds_repack = %u\n", pConf->lcd_control.lvds_config->lvds_repack_user, pConf->lcd_control.lvds_config->lvds_repack);
		}
		else {
			if (pConf->lcd_basic.lcd_type == LCD_DIGITAL_LVDS)
				printf("lvds_repack = %u\n", pConf->lcd_control.lvds_config->lvds_repack);
			else
				DBG_PRINT("lvds_repack = %u\n", pConf->lcd_control.lvds_config->lvds_repack);
		}
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "rgb_base_coeff", NULL);
	if(propdata == NULL){
		DBG_PRINT("don't find to match rgb_base_coeff, use default setting.\n");
	}
	else {
		pConf->lcd_effect.rgb_base_addr = (unsigned short)(be32_to_cpup((u32*)propdata));
		pConf->lcd_effect.rgb_coeff_addr = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
		DBG_PRINT("rgb_base = 0x%x, rgb_coeff = 0x%x\n", pConf->lcd_effect.rgb_base_addr, pConf->lcd_effect.rgb_coeff_addr);
	}
	// propdata = fdt_getprop(dt_addr, nodeoffset, "video_on_pixel_line", NULL);
	// if(propdata == NULL){
		// DBG_PRINT("don't find to match video_on_pixel_line, use default setting.\n");
	// }
	// else {
		// pConf->lcd_timing.video_on_pixel = (unsigned short)(be32_to_cpup((u32*)propdata));
		// pConf->lcd_timing.video_on_line = (unsigned short)(be32_to_cpup((((u32*)propdata)+1)));
		// DBG_PRINT("video_on_pixel = %u, video_on_line = %u\n", pConf->lcd_timing.video_on_pixel, pConf->lcd_timing.video_on_line);
	// }

	return ret;
}

static inline int _get_lcd_power_config(Lcd_Config_t *pConf)
{
	int i;
	int index;
	int ret=0;
	int nodeoffset;
	char* propdata;
	char propname[20];
	struct fdt_property *prop;
	char *p;
	const char * str;

	nodeoffset = fdt_path_offset(dt_addr, "/lcd");
	if(nodeoffset < 0) {
		printf("lcd driver init: not find /lcd node %s.\n",fdt_strerror(nodeoffset));
		return ret;
	}

	//lcd power on/off only for uboot
	propdata = fdt_getprop(dt_addr, nodeoffset, "power_on_uboot", NULL);
	if (propdata == NULL) {
		DBG_PRINT("faild to get power_on_uboot\n");
		pConf->lcd_power_ctrl.power_on_uboot.type = LCD_POWER_TYPE_MAX;
		pConf->lcd_power_ctrl.power_on_uboot.gpio = GPIO_MAX;
		pConf->lcd_power_ctrl.power_on_uboot.value = LCD_POWER_GPIO_INPUT;
		pConf->lcd_power_ctrl.power_on_uboot.delay = 0;
	}
	else {
		prop = container_of(propdata, struct fdt_property, data);
		p = prop->data;
		str = p;
		if ((strcmp(str, "null") == 0) || ((strcmp(str, "n") == 0))) {
			DBG_PRINT("no power_on_uboot config\n");
			pConf->lcd_power_ctrl.power_on_uboot.type = LCD_POWER_TYPE_MAX;
			pConf->lcd_power_ctrl.power_on_uboot.gpio = GPIO_MAX;
			pConf->lcd_power_ctrl.power_on_uboot.value = LCD_POWER_GPIO_INPUT;
			pConf->lcd_power_ctrl.power_on_uboot.delay = 0;
		}
		else {
			for(index = 0; index < LCD_POWER_TYPE_MAX; index++) {
				if(!strcmp(str, lcd_power_type_table[index]))
					break;
			}
			pConf->lcd_power_ctrl.power_on_uboot.type = index;
			if (pConf->lcd_power_ctrl.power_on_uboot.type < LCD_POWER_TYPE_SIGNAL) {
				p += strlen(p) + 1;
				str = p;
				if (pConf->lcd_power_ctrl.power_on_uboot.type == LCD_POWER_TYPE_CPU) {
					pConf->lcd_power_ctrl.power_on_uboot.gpio = aml_lcd_gpio_name_map_num(str);
				}
				else if (pConf->lcd_power_ctrl.power_on_uboot.type == LCD_POWER_TYPE_PMU) {
					pConf->lcd_power_ctrl.power_on_uboot.gpio = aml_lcd_pmu_gpio_name_map_num(str);
				}

				p += strlen(p) + 1;
				str = p;
				if ((strcmp(str, "output_low") == 0) || (strcmp(str, "0") == 0)) {
					pConf->lcd_power_ctrl.power_on_uboot.value = LCD_POWER_GPIO_OUTPUT_LOW;
				}
				else if ((strcmp(str, "output_high") == 0) || (strcmp(str, "1") == 0)) {
					pConf->lcd_power_ctrl.power_on_uboot.value = LCD_POWER_GPIO_OUTPUT_HIGH;
				}
				else if ((strcmp(str, "input") == 0) || (strcmp(str, "2") == 0)) {
					pConf->lcd_power_ctrl.power_on_uboot.value = LCD_POWER_GPIO_INPUT;
				}
			}
			pConf->lcd_power_ctrl.power_on_uboot.delay = 50;
			DBG_PRINT("find power_on_uboot: type = %s(%d), ", lcd_power_type_table[pConf->lcd_power_ctrl.power_on_uboot.type], pConf->lcd_power_ctrl.power_on_uboot.type);
			if (pConf->lcd_power_ctrl.power_on_uboot.type == LCD_POWER_TYPE_CPU) {
				DBG_PRINT("gpio = %s(%d), ", aml_lcd_gpio_type_table[pConf->lcd_power_ctrl.power_on_uboot.gpio], pConf->lcd_power_ctrl.power_on_uboot.gpio);
				DBG_PRINT("value = %d\n", pConf->lcd_power_ctrl.power_on_uboot.value);
			}
			else if (pConf->lcd_power_ctrl.power_on_uboot.type == LCD_POWER_TYPE_PMU) {
				DBG_PRINT("gpio = %d, ", pConf->lcd_power_ctrl.power_on_uboot.gpio);
				DBG_PRINT("value = %d\n", pConf->lcd_power_ctrl.power_on_uboot.value);
			}
		}
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "power_off_uboot", NULL);
	if (propdata == NULL) {
		DBG_PRINT("faild to get power_off_uboot\n");
		pConf->lcd_power_ctrl.power_off_uboot.type = LCD_POWER_TYPE_MAX;
		pConf->lcd_power_ctrl.power_off_uboot.gpio = GPIO_MAX;
		pConf->lcd_power_ctrl.power_off_uboot.value = LCD_POWER_GPIO_INPUT;
		pConf->lcd_power_ctrl.power_off_uboot.delay = 0;
	}
	else {
		prop = container_of(propdata, struct fdt_property, data);
		p = prop->data;
		str = p;
		if ((strcmp(str, "null") == 0) || ((strcmp(str, "n") == 0))) {
			DBG_PRINT("no power_off_uboot config\n");
			pConf->lcd_power_ctrl.power_off_uboot.type = LCD_POWER_TYPE_MAX;
			pConf->lcd_power_ctrl.power_off_uboot.gpio = GPIO_MAX;
			pConf->lcd_power_ctrl.power_off_uboot.value = LCD_POWER_GPIO_INPUT;
			pConf->lcd_power_ctrl.power_off_uboot.delay = 0;
		}
		else {
			for(index = 0; index < LCD_POWER_TYPE_MAX; index++) {
				if(!strcmp(str, lcd_power_type_table[index]))
					break;
			}
			pConf->lcd_power_ctrl.power_off_uboot.type = index;
			if (pConf->lcd_power_ctrl.power_off_uboot.type < LCD_POWER_TYPE_SIGNAL) {
				p += strlen(p) + 1;
				str = p;
				if (pConf->lcd_power_ctrl.power_off_uboot.type == LCD_POWER_TYPE_CPU) {
					pConf->lcd_power_ctrl.power_off_uboot.gpio = aml_lcd_gpio_name_map_num(str);
				}
				else if (pConf->lcd_power_ctrl.power_off_uboot.type == LCD_POWER_TYPE_PMU) {
					pConf->lcd_power_ctrl.power_off_uboot.gpio = aml_lcd_pmu_gpio_name_map_num(str);
				}

				p += strlen(p) + 1;
				str = p;
				if ((strcmp(str, "output_low") == 0) || (strcmp(str, "0") == 0)) {
					pConf->lcd_power_ctrl.power_off_uboot.value = LCD_POWER_GPIO_OUTPUT_LOW;
				}
				else if ((strcmp(str, "output_high") == 0) || (strcmp(str, "1") == 0)) {
					pConf->lcd_power_ctrl.power_off_uboot.value = LCD_POWER_GPIO_OUTPUT_HIGH;
				}
				else if ((strcmp(str, "input") == 0) || (strcmp(str, "2") == 0)) {
					pConf->lcd_power_ctrl.power_off_uboot.value = LCD_POWER_GPIO_INPUT;
				}
			}
			pConf->lcd_power_ctrl.power_off_uboot.delay = 0;
			DBG_PRINT("find power_off_uboot: type = %s(%d), ", lcd_power_type_table[pConf->lcd_power_ctrl.power_off_uboot.type], pConf->lcd_power_ctrl.power_off_uboot.type);
			if (pConf->lcd_power_ctrl.power_off_uboot.type == LCD_POWER_TYPE_CPU) {
				DBG_PRINT("gpio = %s(%d), ", aml_lcd_gpio_type_table[pConf->lcd_power_ctrl.power_off_uboot.gpio], pConf->lcd_power_ctrl.power_off_uboot.gpio);
				DBG_PRINT("value = %d\n", pConf->lcd_power_ctrl.power_off_uboot.value);
			}
			else if (pConf->lcd_power_ctrl.power_off_uboot.type == LCD_POWER_TYPE_PMU) {
				DBG_PRINT("gpio = %d, ", pConf->lcd_power_ctrl.power_off_uboot.gpio);
				DBG_PRINT("value = %d\n", pConf->lcd_power_ctrl.power_off_uboot.value);
			}
		}
	}

	//lcd power on
	for (i=0; i < 10; i++) {
		sprintf(propname, "power_on_step_%d", i+1);
		propdata = fdt_getprop(dt_addr, nodeoffset, propname, NULL);
		if (propdata == NULL) {
			DBG_PRINT("faild to get %s\n", propname);
			break;
		}
		else {
			prop = container_of(propdata, struct fdt_property, data);
			p = prop->data;
			str = p;
			if ((strcmp(str, "null") == 0) || ((strcmp(str, "n") == 0))) {
				break;
			}
			for(index = 0; index < LCD_POWER_TYPE_MAX; index++) {
				if(!strcmp(str, lcd_power_type_table[index]))
					break;
			}
			pConf->lcd_power_ctrl.power_on_config[i].type = index;
			if (pConf->lcd_power_ctrl.power_on_config[i].type < LCD_POWER_TYPE_SIGNAL) {
				p += strlen(p) + 1;
				str = p;
				if (pConf->lcd_power_ctrl.power_on_config[i].type == LCD_POWER_TYPE_CPU) {
					pConf->lcd_power_ctrl.power_on_config[i].gpio = aml_lcd_gpio_name_map_num(str);
				}
				else if (pConf->lcd_power_ctrl.power_on_config[i].type == LCD_POWER_TYPE_PMU) {
					pConf->lcd_power_ctrl.power_on_config[i].gpio = aml_lcd_pmu_gpio_name_map_num(str);
				}

				p += strlen(p) + 1;
				str = p;
				if ((strcmp(str, "output_low") == 0) || (strcmp(str, "0") == 0)) {
					pConf->lcd_power_ctrl.power_on_config[i].value = LCD_POWER_GPIO_OUTPUT_LOW;
				}
				else if ((strcmp(str, "output_high") == 0) || (strcmp(str, "1") == 0)) {
					pConf->lcd_power_ctrl.power_on_config[i].value = LCD_POWER_GPIO_OUTPUT_HIGH;
				}
				else if ((strcmp(str, "input") == 0) || (strcmp(str, "2") == 0)) {
					pConf->lcd_power_ctrl.power_on_config[i].value = LCD_POWER_GPIO_INPUT;
				}
			}
		}
	}
	pConf->lcd_power_ctrl.power_on_step = i;
	DBG_PRINT("lcd_power_on_step = %d\n", pConf->lcd_power_ctrl.power_on_step);

	propdata = fdt_getprop(dt_addr, nodeoffset, "power_on_delay", NULL);
	if (propdata == NULL) {
		printf("faild to get power_on_delay\n");
	}
	else {
		for (i=0; i<pConf->lcd_power_ctrl.power_on_step; i++) {
			pConf->lcd_power_ctrl.power_on_config[i].delay = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)));
		}
	}

	//lcd power off
	for (i=0; i < 10; i++) {
		sprintf(propname, "power_off_step_%d", i+1);
		propdata = fdt_getprop(dt_addr, nodeoffset, propname, NULL);
		if (propdata == NULL) {
			DBG_PRINT("faild to get %s\n", propname);
			break;
		}
		else {
			prop = container_of(propdata, struct fdt_property, data);
			p = prop->data;
			str = p;
			if ((strcmp(str, "null") == 0) || ((strcmp(str, "n") == 0))) {
				break;
			}
			for(index = 0; index < LCD_POWER_TYPE_MAX; index++) {
				if(!strcmp(str, lcd_power_type_table[index]))
					break;
			}
			pConf->lcd_power_ctrl.power_off_config[i].type = index;
			if (pConf->lcd_power_ctrl.power_off_config[i].type < LCD_POWER_TYPE_SIGNAL) {
				p += strlen(p) + 1;
				str = p;
				if (pConf->lcd_power_ctrl.power_off_config[i].type == LCD_POWER_TYPE_CPU) {
					pConf->lcd_power_ctrl.power_off_config[i].gpio = aml_lcd_gpio_name_map_num(str);
				}
				else if (pConf->lcd_power_ctrl.power_off_config[i].type == LCD_POWER_TYPE_PMU) {
					pConf->lcd_power_ctrl.power_off_config[i].gpio = aml_lcd_pmu_gpio_name_map_num(str);
				}

				p += strlen(p) + 1;
				str = p;
				if ((strcmp(str, "output_low") == 0) || (strcmp(str, "0") == 0)) {
					pConf->lcd_power_ctrl.power_off_config[i].value = LCD_POWER_GPIO_OUTPUT_LOW;
				}
				else if ((strcmp(str, "output_high") == 0) || (strcmp(str, "1") == 0)) {
					pConf->lcd_power_ctrl.power_off_config[i].value = LCD_POWER_GPIO_OUTPUT_HIGH;
				}
				else if ((strcmp(str, "input") == 0) || (strcmp(str, "2") == 0)) {
					pConf->lcd_power_ctrl.power_off_config[i].value = LCD_POWER_GPIO_INPUT;
				}
			}
		}
	}
	pConf->lcd_power_ctrl.power_off_step = i;
	DBG_PRINT("lcd_power_off_step = %d\n", pConf->lcd_power_ctrl.power_off_step);

	propdata = fdt_getprop(dt_addr, nodeoffset, "power_off_delay", NULL);
	if (propdata == NULL) {
		printf("faild to get power_off_delay\n");
	}
	else {
		for (i=0; i<pConf->lcd_power_ctrl.power_off_step; i++) {
			pConf->lcd_power_ctrl.power_off_config[i].delay = (unsigned short)(be32_to_cpup((((u32*)propdata)+i)));
		}
	}

	for (i=0; i<pConf->lcd_power_ctrl.power_on_step; i++) {
		DBG_PRINT("power on step %d: type = %s(%d)\n", i+1, lcd_power_type_table[pConf->lcd_power_ctrl.power_on_config[i].type], pConf->lcd_power_ctrl.power_on_config[i].type);
		if (pConf->lcd_power_ctrl.power_on_config[i].type == LCD_POWER_TYPE_CPU) {
			DBG_PRINT("power on step %d: gpio = %s(%d)\n", i+1, aml_lcd_gpio_type_table[pConf->lcd_power_ctrl.power_on_config[i].gpio], pConf->lcd_power_ctrl.power_on_config[i].gpio);
			DBG_PRINT("power on step %d: value = %d\n", i+1, pConf->lcd_power_ctrl.power_on_config[i].value);
		}
		else if (pConf->lcd_power_ctrl.power_on_config[i].type == LCD_POWER_TYPE_PMU) {
			DBG_PRINT("power on step %d: gpio = %d\n", i+1, pConf->lcd_power_ctrl.power_on_config[i].gpio);
			DBG_PRINT("power on step %d: value = %d\n", i+1, pConf->lcd_power_ctrl.power_on_config[i].value);
		}
		DBG_PRINT("power on step %d: delay = %d\n", i+1, pConf->lcd_power_ctrl.power_on_config[i].delay);
	}

	for (i=0; i<pConf->lcd_power_ctrl.power_off_step; i++) {
		DBG_PRINT("power off step %d: type = %s(%d)\n", i+1, lcd_power_type_table[pConf->lcd_power_ctrl.power_off_config[i].type], pConf->lcd_power_ctrl.power_off_config[i].type);
		if (pConf->lcd_power_ctrl.power_off_config[i].type == LCD_POWER_TYPE_CPU) {
			DBG_PRINT("power off step %d: gpio = %s(%d)\n", i+1, aml_lcd_gpio_type_table[pConf->lcd_power_ctrl.power_off_config[i].gpio], pConf->lcd_power_ctrl.power_off_config[i].gpio);
			DBG_PRINT("power off step %d: value = %d\n", i+1, pConf->lcd_power_ctrl.power_off_config[i].value);
		}
		else if (pConf->lcd_power_ctrl.power_off_config[i].type == LCD_POWER_TYPE_PMU) {
			DBG_PRINT("power off step %d: gpio = %d\n", i+1, pConf->lcd_power_ctrl.power_off_config[i].gpio);
			DBG_PRINT("power off step %d: value = %d\n", i+1, pConf->lcd_power_ctrl.power_off_config[i].value);
		}
		DBG_PRINT("power off step %d: delay = %d\n", i+1, pConf->lcd_power_ctrl.power_off_config[i].delay);
	}
	return ret;
}

static inline int _get_lcd_backlight_config(Lcd_Bl_Config_t *bl_conf)
{
	int ret=0;
	int nodeoffset;
	char * propdata;
	unsigned int bl_para[3];
	int i;
	struct fdt_property *prop;
	char *p;
	const char * str;
	unsigned pwm_freq, pwm_cnt, pwm_pre_div, tmp;
	int len;

	nodeoffset = fdt_path_offset(dt_addr, "/backlight");
	if(nodeoffset < 0) {
		printf("backlight init: not find /backlight node %s.\n",fdt_strerror(nodeoffset));
		return ret;
	}

	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_level_default_uboot_kernel", NULL);
	if(propdata == NULL){
		printf("faild to get bl_level_default_uboot_kernel\n");
		bl_conf->level_default = BL_LEVEL_DFT;
	}
	else {
		bl_conf->level_default = (be32_to_cpup((u32*)propdata));
	}
	DBG_PRINT("bl level default uboot=%u\n", bl_conf->level_default);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_level_middle_mapping", NULL);
	if(propdata == NULL){
		printf("faild to get bl_level_middle_mapping\n");
		bl_conf->level_mid = BL_LEVEL_MID_DFT;
		bl_conf->level_mid_mapping = BL_LEVEL_MID_MAPPED_DFT;
	}
	else {
		bl_conf->level_mid = (be32_to_cpup((u32*)propdata));
		bl_conf->level_mid_mapping = (be32_to_cpup((((u32*)propdata)+1)));
	}
	DBG_PRINT("bl level mid=%u, mid_mapping=%u\n", bl_conf->level_mid, bl_conf->level_mid_mapping);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_level_max_min", NULL);
	if(propdata == NULL){
		printf("faild to get bl_level_max_min\n");
		bl_conf->level_min = BL_LEVEL_MIN_DFT;
		bl_conf->level_max = BL_LEVEL_MAX_DFT;
	}
	else {
		bl_conf->level_max = (be32_to_cpup((u32*)propdata));
		bl_conf->level_min = (be32_to_cpup((((u32*)propdata)+1)));
	}
	DBG_PRINT("bl level max=%u, min=%u\n", bl_conf->level_max, bl_conf->level_min);

	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_power_on_delay", NULL);
	if(propdata == NULL){
		printf("faild to get bl_power_on_delay\n");
		bl_conf->power_on_delay = 100;
	}
	else {
		bl_conf->power_on_delay = (unsigned short)(be32_to_cpup((u32*)propdata));
	}
	DBG_PRINT("bl power_on_delay: %ums\n", bl_conf->power_on_delay);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_ctrl_method", NULL);
	if(propdata == NULL){
		printf("faild to get bl_ctrl_method\n");
		bl_conf->method = BL_CTL_GPIO;
	}
	else {
		bl_conf->method = (be32_to_cpup((u32*)propdata) > BL_CTL_MAX) ? (BL_CTL_MAX-1) : (unsigned char)(be32_to_cpup((u32*)propdata));
	}
	DBG_PRINT("bl control_method: %s(%u)\n", bl_ctrl_method_table[bl_conf->method], bl_conf->method);

	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_port_gpio_used", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_port_gpio_used\n");
		str = "PWM_D";
		bl_conf->pwm_port = BL_PWM_D;
		bl_conf->pwm_gpio_used = 0;
	}
	else {
		prop = container_of(propdata, struct fdt_property, data);
		p = prop->data;
		str = p;
		if (strcmp(str, "PWM_A") == 0)
			bl_conf->pwm_port = BL_PWM_A;
		else if (strcmp(str, "PWM_B") == 0)
			bl_conf->pwm_port = BL_PWM_B;
		else if (strcmp(str, "PWM_C") == 0)
			bl_conf->pwm_port = BL_PWM_C;
		else if (strcmp(str, "PWM_D") == 0)
			bl_conf->pwm_port = BL_PWM_D;

		p += strlen(p) + 1;
		str = p;
		if (strncmp(str, "1", 1) == 0)
			bl_conf->pwm_gpio_used = 1;
		else
			bl_conf->pwm_gpio_used = 0;
	}
	DBG_PRINT("bl pwm_port: %s(%u)\n", propdata, bl_conf->pwm_port);
	DBG_PRINT("bl pwm gpio_used: %u\n", bl_conf->pwm_gpio_used);
	if ((bl_conf->method == BL_CTL_GPIO) || ((bl_conf->pwm_gpio_used == 1) && ((bl_conf->method == BL_CTL_PWM_NEGATIVE) || (bl_conf->method == BL_CTL_PWM_POSITIVE)))) {
		propdata = fdt_getprop(dt_addr, nodeoffset, "bl_gpio_port", NULL);
		if (propdata == NULL) {
			printf("faild to get bl_gpio_port\n");
			bl_conf->gpio = GPIOD_1;
		}
		else {
			bl_conf->gpio = aml_lcd_gpio_name_map_num(propdata);
		}
		DBG_PRINT("bl gpio = %s(%d)\n", propdata, bl_conf->gpio);
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_gpio_dim_max_min", NULL);
	if(propdata == NULL){
		printf("faild to get bl_gpio_dim_max_min\n");
		bl_conf->dim_max = 0x0;
		bl_conf->dim_min = 0xf;
	}
	else {
		bl_conf->dim_max = (be32_to_cpup((u32*)propdata));
		bl_conf->dim_min = (be32_to_cpup((((u32*)propdata)+1)));
	}
	DBG_PRINT("bl dim max = 0x%x, min = 0x%x\n", bl_conf->dim_max, bl_conf->dim_min);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_freq", NULL);
	if(propdata == NULL){
		pwm_freq = 1000;
		printf("faild to get bl_pwm_freq, default set to %uHz\n", pwm_freq);
	}
	else {
		pwm_freq = be32_to_cpup((u32*)propdata);
		pwm_freq = ((pwm_freq >= (FIN_FREQ * 500)) ? (FIN_FREQ * 500) : pwm_freq);
	}
	for (i=0; i<0x7f; i++) {
		pwm_pre_div = i;
		pwm_cnt = FIN_FREQ * 1000 / (pwm_freq * (pwm_pre_div + 1)) - 2;
		if (pwm_cnt <= 0xffff)
			break;
	}
	bl_conf->pwm_cnt = pwm_cnt;
	bl_conf->pwm_pre_div = pwm_pre_div;
	DBG_PRINT("bl pwm_frequency = %uHz, pwm_cnt = %u, pre_div = %u\n", pwm_freq, bl_conf->pwm_cnt, bl_conf->pwm_pre_div);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_duty_max_min", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_duty_max_min\n");
		bl_para[0] = 100;
		bl_para[1] = 20;
	}
	else {
		bl_para[0] = be32_to_cpup((u32*)propdata);
		bl_para[1] = be32_to_cpup((((u32*)propdata)+1));
	}
	bl_conf->pwm_max = (bl_conf->pwm_cnt * bl_para[0] / 100);
	bl_conf->pwm_min = (bl_conf->pwm_cnt * bl_para[1] / 100);
	DBG_PRINT("bl pwm_duty max = %u\%, min = %u\%\n", bl_para[0], bl_para[1]);

	//pwm combo
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_combo_high_low_level_switch", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_combo_high_low_level_switch\n");
		tmp = bl_conf->level_mid;
	}
	else {
		tmp = be32_to_cpup((u32*)propdata);
	}
	if (tmp > bl_conf->level_mid)
		tmp = ((tmp - bl_conf->level_mid) * (bl_conf->level_max - bl_conf->level_mid_mapping)) / (bl_conf->level_max - bl_conf->level_mid) + bl_conf->level_mid_mapping;
	else
		tmp = ((tmp - bl_conf->level_min) * (bl_conf->level_mid_mapping - bl_conf->level_min)) / (bl_conf->level_mid - bl_conf->level_min) + bl_conf->level_min;
	bl_conf->combo_level_switch = tmp;
	DBG_PRINT("bl pwm_combo level switch =%u\n", bl_conf->combo_level_switch);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_combo_high_port_method", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_combo_high_port_method\n");
		str = "PWM_C";
		bl_conf->combo_high_port = BL_PWM_C;
		bl_conf->combo_high_method = BL_CTL_PWM_NEGATIVE;
	}
	else {
		prop = container_of(propdata, struct fdt_property, data);
		p = prop->data;
		str = p;
		if (strcmp(str, "PWM_A") == 0)
			bl_conf->combo_high_port = BL_PWM_A;
		else if (strcmp(str, "PWM_B") == 0)
			bl_conf->combo_high_port = BL_PWM_B;
		else if (strcmp(str, "PWM_C") == 0)
			bl_conf->combo_high_port = BL_PWM_C;
		else if (strcmp(str, "PWM_D") == 0)
			bl_conf->combo_high_port = BL_PWM_D;

		p += strlen(p) + 1;
		str = p;
		if (strncmp(str, "1", 1) == 0)
			bl_conf->combo_high_method = BL_CTL_PWM_NEGATIVE;
		else
			bl_conf->combo_high_method = BL_CTL_PWM_POSITIVE;
	}
	DBG_PRINT("bl pwm_combo high port: %s(%u)\n", str, bl_conf->combo_high_port);
	DBG_PRINT("bl pwm_combo high method: %s(%u)\n", bl_ctrl_method_table[bl_conf->combo_high_method], bl_conf->combo_high_method);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_combo_low_port_method", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_combo_low_port_method\n");
		str = "PWM_D";
		bl_conf->combo_low_port = BL_PWM_D;
		bl_conf->combo_high_method = BL_CTL_PWM_POSITIVE;
	}
	else {
		prop = container_of(propdata, struct fdt_property, data);
		p = prop->data;
		str = p;
		if (strcmp(str, "PWM_A") == 0)
			bl_conf->combo_low_port = BL_PWM_A;
		else if (strcmp(str, "PWM_B") == 0)
			bl_conf->combo_low_port = BL_PWM_B;
		else if (strcmp(str, "PWM_C") == 0)
			bl_conf->combo_low_port = BL_PWM_C;
		else if (strcmp(str, "PWM_D") == 0)
			bl_conf->combo_low_port = BL_PWM_D;

		p += strlen(p) + 1;
		str = p;
		if (strncmp(str, "1", 1) == 0)
			bl_conf->combo_low_method = BL_CTL_PWM_NEGATIVE;
		else
			bl_conf->combo_low_method = BL_CTL_PWM_POSITIVE;
	}
	DBG_PRINT("bl pwm_combo low port: %s(%u)\n", str, bl_conf->combo_low_port);
	DBG_PRINT("bl pwm_combo low method: %s(%u)\n", bl_ctrl_method_table[bl_conf->combo_low_method], bl_conf->combo_low_method);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_combo_high_freq_duty_max_min", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_combo_high_freq_duty_max_min\n");
		bl_para[0] = 300000;	//freq=300k
		bl_para[1] = 100;
		bl_para[2] = 50;
	}
	else {
		bl_para[0] = be32_to_cpup((u32*)propdata);
		bl_para[1] = be32_to_cpup((((u32*)propdata)+1));
		bl_para[2] = be32_to_cpup((((u32*)propdata)+2));
	}
	pwm_freq = ((bl_para[0] >= (FIN_FREQ * 500)) ? (FIN_FREQ * 500) : bl_para[0]);
	for (i=0; i<0x7f; i++) {
		pwm_pre_div = i;
		pwm_cnt = FIN_FREQ * 1000 / (pwm_freq * (pwm_pre_div + 1)) - 2;
		if (pwm_cnt <= 0xffff)
			break;
	}
	bl_conf->combo_high_cnt = pwm_cnt;
	bl_conf->combo_high_pre_div = pwm_pre_div;
	bl_conf->combo_high_duty_max = (bl_conf->combo_high_cnt * bl_para[1] / 100);
	bl_conf->combo_high_duty_min = (bl_conf->combo_high_cnt * bl_para[2] / 100);
	DBG_PRINT("bl pwm_combo high freq=%uHz, duty_max=%u\%, duty_min=%u\%\n", pwm_freq, bl_para[1], bl_para[2]);
	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_combo_low_freq_duty_max_min", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_combo_low_freq_duty_max_min\n");
		bl_para[0] = 1000;	//freq=1k
		bl_para[1] = 100;
		bl_para[2] = 50;
	}
	else {
		bl_para[0] = be32_to_cpup((u32*)propdata);
		bl_para[1] = be32_to_cpup((((u32*)propdata)+1));
		bl_para[2] = be32_to_cpup((((u32*)propdata)+2));
	}
	pwm_freq = ((bl_para[0] >= (FIN_FREQ * 500)) ? (FIN_FREQ * 500) : bl_para[0]);
	for (i=0; i<0x7f; i++) {
		pwm_pre_div = i;
		pwm_cnt = FIN_FREQ * 1000 / (pwm_freq * (pwm_pre_div + 1)) - 2;
		if (pwm_cnt <= 0xffff)
			break;
	}
	bl_conf->combo_low_cnt = pwm_cnt;
	bl_conf->combo_low_pre_div = pwm_pre_div;
	bl_conf->combo_low_duty_max = (bl_conf->combo_low_cnt * bl_para[1] / 100);
	bl_conf->combo_low_duty_min = (bl_conf->combo_low_cnt * bl_para[2] / 100);
	DBG_PRINT("bl pwm_combo low freq=%uHz, duty_max=%u\%, duty_min=%u\%\n", pwm_freq, bl_para[1], bl_para[2]);

	//get backlight pinmux for pwm
	if (bl_conf->method == BL_CTL_PWM_COMBO) {
		nodeoffset = fdt_path_offset(dt_addr, "/pinmux/lcd_backlight_combo");
		if(nodeoffset < 0) {
			printf("backlight init: not find /pinmux/lcd_backlight_combo node %s.\n",fdt_strerror(nodeoffset));
			bl_conf->pinmux_set_num = 0;
			bl_conf->pinmux_clr_num = 0;
			return ret;
		}
	}
	else {
		nodeoffset = fdt_path_offset(dt_addr, "/pinmux/lcd_backlight");
		if(nodeoffset < 0) {
			printf("backlight init: not find /pinmux/lcd_backlight node %s.\n",fdt_strerror(nodeoffset));
			bl_conf->pinmux_set_num = 0;
			bl_conf->pinmux_clr_num = 0;
			return ret;
		}
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "amlogic,setmask", &len);
	if(propdata == NULL){
		printf("faild to get amlogic,setmask\n");
		bl_conf->pinmux_set_num = 0;
	}
	else {
		bl_conf->pinmux_set_num = len / 8;
		for (i=0; i<bl_conf->pinmux_set_num; i++) {
			bl_conf->pinmux_set[i][0] = be32_to_cpup((((u32*)propdata)+2*i));
			bl_conf->pinmux_set[i][1] = be32_to_cpup((((u32*)propdata)+2*i+1));
		}
	}
	propdata = fdt_getprop(dt_addr, nodeoffset, "amlogic,clrmask", &len);
	if(propdata == NULL){
		printf("faild to get amlogic,clrmask\n");
		bl_conf->pinmux_clr_num = 0;
	}
	else {
		bl_conf->pinmux_clr_num = len / 8;
		for (i=0; i<bl_conf->pinmux_clr_num; i++) {
			bl_conf->pinmux_clr[i][0] = be32_to_cpup((((u32*)propdata)+2*i));
			bl_conf->pinmux_clr[i][1] = be32_to_cpup((((u32*)propdata)+2*i+1));
		}
	}

	for (i=0; i<bl_conf->pinmux_set_num; i++) {
		DBG_PRINT("backlight pinmux set %d: mux_num = %d, mux_mask = 0x%x\n", i+1, bl_conf->pinmux_set[i][0], bl_conf->pinmux_set[i][1]);
	}
	for (i=0; i<bl_conf->pinmux_clr_num; i++) {
		DBG_PRINT("backlight pinmux clr %d: mux_num = %d, mux_mask = 0x%x\n", i+1, bl_conf->pinmux_clr[i][0], bl_conf->pinmux_clr[i][1]);
	}

	return ret;
}
#endif

static inline void _set_panel_info(void)
{
	panel_info.vd_base = simple_strtoul(getenv("fb_addr"), NULL, NULL);
	panel_info.vl_col = simple_strtoul(getenv("display_width"), NULL, NULL);
	panel_info.vl_row = simple_strtoul(getenv("display_height"), NULL, NULL);
	panel_info.vl_bpix = simple_strtoul(getenv("display_bpp"), NULL, NULL);
	panel_info.vd_color_fg = simple_strtoul(getenv("display_color_fg"), NULL, NULL);
	panel_info.vd_color_bg = simple_strtoul(getenv("display_color_bg"), NULL, NULL);

	DBG_PRINT("panel_info: resolution = %ux%u\n", panel_info.vl_col, panel_info.vl_row);
	DBG_PRINT("panel_info: vl_bpix = %u\n", panel_info.vl_bpix);
}

int lcd_probe(void)
{
    pDev = (lcd_dev_t *)malloc(sizeof(lcd_dev_t));
    if (!pDev) {
        printf("[tcon]: Not enough memory.\n");
        return -1;
    }
	printf("lcd driver version: %s%s\n", DRIVER_DATE, DRV_TYPE);

	dts_ready = 0;	//prepare dts_ready flag, default no dts
#ifdef CONFIG_OF_LIBFDT
#ifdef CONFIG_DT_PRELOAD
	int ret;

#ifdef CONFIG_DTB_LOAD_ADDR
	dt_addr = CONFIG_DTB_LOAD_ADDR;
#else
	dt_addr = 0x83000000;
#endif
	ret = fdt_check_header(dt_addr);
	if(ret < 0) {
		dts_ready = 0;
		printf("check dts: %s, load default lcd parameters\n", fdt_strerror(ret));
	}
	else {
		dts_ready = 1;
	}
#endif
#endif

	if(dts_ready == 0) {
		pDev->pConf = &lcd_config_dft;
		pDev->bl_config = &bl_config_dft;
		if (pDev->pConf->lcd_basic.model_name == NULL) {
			pDev->pConf->lcd_basic.model_name = PANEL_MODEL_DEFAULT;
		}
		else if (strncmp(pDev->pConf->lcd_basic.model_name, "none", 4) == 0) {
			printf("no lcd exist!\n");
			return 1;
		}
		lcd_default_config_init(pDev->pConf);
		backlight_default_config_init(pDev->bl_config);
		printf("load default lcd model: %s\n", pDev->pConf->lcd_basic.model_name);
	}
	else {
#ifdef CONFIG_OF_LIBFDT
		pDev->pConf = &lcd_config;
		pDev->bl_config = &bl_config;
		_get_lcd_model_timing(pDev->pConf);
		_get_lcd_default_config(pDev->pConf);
		_get_lcd_power_config(pDev->pConf);
		_get_lcd_backlight_config(pDev->bl_config);
#endif
	}

	_set_panel_info();
	_lcd_init(pDev->pConf);
	print_lcd_clock();
	return 0;
}

int lcd_remove(void)
{
	lcd_backlight_power_ctrl(OFF);
	lcd_power_ctrl(OFF);
	_disable_lcd_driver(pDev->pConf);

	free(pDev);
	pDev = NULL;

	return 0;
}

//***************************************************//
//for lcd_function_call by other module, compatible dts
//***************************************************//
static void _enable_backlight(void)
{
	if (pDev != NULL)
		lcd_backlight_power_ctrl(ON);
}
static void _disable_backlight(void)
{
	if (pDev != NULL)
		lcd_backlight_power_ctrl(OFF);
}
static void _set_backlight_level(unsigned level)
{
	if (pDev != NULL)
		set_lcd_backlight_level(level);
}
static unsigned _get_backlight_level(void)
{
	if (pDev != NULL)
		return get_lcd_backlight_level();
	else
		return 0;
}
static void _lcd_enable(void)
{
	lcd_probe();
}
static void _lcd_disable(void)
{
	if (pDev != NULL)
		lcd_remove();
}
static void _lcd_power_on(void)
{
	if (pDev != NULL)
		lcd_power_ctrl(ON);
}
static void _lcd_power_off(void)
{
	if (pDev != NULL)
		lcd_power_ctrl(OFF);
}
static void _lcd_test(unsigned num)
{
	unsigned venc_video_mode, venc_test_base;

	if (pDev != NULL) {
		if (pDev->pConf->lcd_basic.lcd_type == LCD_DIGITAL_TTL) {
			venc_video_mode = ENCT_VIDEO_MODE_ADV;
			venc_test_base = ENCT_TST_EN;
		}
		else {
			venc_video_mode = ENCL_VIDEO_MODE_ADV;
			venc_test_base = ENCL_TST_EN;
		}

		switch (num) {
			case 0:
				WRITE_LCD_REG(venc_video_mode, 0x8);
				printf("disable bist pattern\n");
				printf("video dev test 1/2/3: show different test pattern\n");
				break;
			case 1:
				WRITE_LCD_REG(venc_video_mode, 0);
				WRITE_LCD_REG(venc_test_base+1, 1);
				WRITE_LCD_REG(venc_test_base+5, pDev->pConf->lcd_basic.h_active / 8);
				WRITE_LCD_REG(venc_test_base+6, pDev->pConf->lcd_basic.h_active / 8);
				WRITE_LCD_REG(venc_test_base, 1);
				printf("show test pattern 1\n");
				printf("video dev test 0: disable test pattern\n");
				break;
			case 2:
				WRITE_LCD_REG(venc_video_mode, 0);
				WRITE_LCD_REG(venc_test_base+1, 2);
				WRITE_LCD_REG(venc_test_base, 1);
				printf("show test pattern 2\n");
				printf("video dev test 0: disable test pattern\n");
				break;
			case 3:
				WRITE_LCD_REG(venc_video_mode, 0);
				WRITE_LCD_REG(venc_test_base+1, 3);
				WRITE_LCD_REG(venc_test_base, 1);
				printf("show test pattern 3\n");
				printf("video dev test 0: disable test pattern\n");
				break;
			default:
				printf("un-support pattern num\n");
				printf("video dev test 1/2/3: show different test pattern\n");
				printf("video dev test 0: disable test pattern\n");
				break;
		}
	}
}

struct panel_operations panel_oper =
{
	.enable       =	_lcd_enable,
	.disable      =	_lcd_disable,
	.bl_on        =	_enable_backlight,
	.bl_off       =	_disable_backlight,
	.set_bl_level =	_set_backlight_level,
	.get_bl_level = _get_backlight_level,
	.power_on     = _lcd_power_on,
	.power_off    = _lcd_power_off,
	.test         = _lcd_test,
};
//****************************************