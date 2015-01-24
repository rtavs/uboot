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
#include <asm/arch/clock.h>
#include <asm/arch/timing.h>
#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif
#include "mipi_dsi_util.h"
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

#define FIQ_VSYNC

#define PANEL_NAME		"panel"
#define DRV_TYPE 		"c8b"
#define DRIVER_DATE		"20140512"

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
static void _enable_backlight(void);
static Lcd_Bl_Config_t bl_config = {
	.level_default = BL_LEVEL_DFT,
	.level_mid = BL_LEVEL_MID_DFT,
	.level_mid_mapping = BL_LEVEL_MID_MAPPED_DFT,
	.level_min = BL_LEVEL_MIN_DFT,
	.level_max = BL_LEVEL_MAX_DFT,
};

static DSI_Config_t lcd_mipi_config = {
    .lane_num = 4,
    .bit_rate_min = 0,
    .bit_rate_max = 0,
    .transfer_ctrl = 0,
    .dsi_init_on = NULL,
    .dsi_init_off = NULL,
    .lcd_extern_init = 0,
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
        .mipi_config = &lcd_mipi_config,
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
			WRITE_LCD_CBUS_REG_BITS(HHI_DIF_CSI_PHY_CNTL3, 0x1e, 11, 5);	//enable LVDS phy 3 channels
		else
			WRITE_LCD_CBUS_REG_BITS(HHI_DIF_CSI_PHY_CNTL3, 0x1f, 11, 5);	//enable LVDS phy 4 channels
	}else {
		WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL1, 0x0);
		WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL2, 0x00060000);
		WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL3, 0x00200000);
	}

	DBG_PRINT("%s: %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static void lcd_ports_ctrl_mipi(Bool_t status)
{
    if (status) {
        switch (pDev->pConf->lcd_control.mipi_config->lane_num) {
            case 1:
                WRITE_LCD_CBUS_REG_BITS(HHI_DIF_CSI_PHY_CNTL3, 0x11, 11, 5);
                break;
            case 2:
                WRITE_LCD_CBUS_REG_BITS(HHI_DIF_CSI_PHY_CNTL3, 0x19, 11, 5);
                break;
            case 3:
                WRITE_LCD_CBUS_REG_BITS(HHI_DIF_CSI_PHY_CNTL3, 0x1d, 11, 5);
                break;
            case 4:
                WRITE_LCD_CBUS_REG_BITS(HHI_DIF_CSI_PHY_CNTL3, 0x1f, 11, 5);
                break;
            default:
                break;
        }
    }
    else {
        WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL1, 0x0);
        WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL2, 0x00060000);
        WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL3, 0x00200000);
    }

    DBG_PRINT("%s: %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static unsigned pinmux_data_set[][2]={
    {PERIPHS_PIN_MUX_0,0x03c00000},//RGB[7:2]
    //{PERIPHS_PIN_MUX_8,0x00070000},//RGB[1:0]
};
static unsigned pinmux_data_clr[][2]={
    {PERIPHS_PIN_MUX_6,0x0000077f},//RGB[7:2]
    //{PERIPHS_PIN_MUX_4,0x0000000e},//RGB[1:0]
    //{PERIPHS_PIN_MUX_5,0x07000000},//RGB[1:0]
    //{PERIPHS_PIN_MUX_6,0x0000f800},//RGB[1:0]
    //{PERIPHS_PIN_MUX_7,0x00300000},//RGB[1:0]
    //{PERIPHS_PIN_MUX_9,0x00001c00},//RGB[1:0]
};
static unsigned pinmux_data_set_table[][3]={
    {0,0xf},  //6bit index
    //{0,1,0xf},//8bit index
};
static unsigned pinmux_data_clr_table[][7]={
    {0,0xf},          //6bit index
    //{0,1,2,3,4,5,0xf},//8bit index
};

static unsigned pinmux_tcon_set[][2]={
    {PERIPHS_PIN_MUX_1,0x00000004},//clk
    {PERIPHS_PIN_MUX_1,0x00000002},//DE
    {PERIPHS_PIN_MUX_1,0x00000011},//sync
};
static unsigned pinmux_tcon_clr[][2]={
    {PERIPHS_PIN_MUX_1,0x02000008},//clk
    {PERIPHS_PIN_MUX_8,0x00001000},//clk
    {PERIPHS_PIN_MUX_1,0x01000000},//DE
    {PERIPHS_PIN_MUX_1,0x04800000},//sync
    {PERIPHS_PIN_MUX_5,0x08000000},//sync
    {PERIPHS_PIN_MUX_8,0x00000800},//sync
    {PERIPHS_PIN_MUX_9,0x00020000},//sync
};
static unsigned pinmux_tcon_set_table[][4]={
    {0,1,0xf},  //DE index
    {0,2,0xf},  //Sync index
    {0,1,2,0xf},//DE+Sync index
};
static unsigned pinmux_tcon_clr_table[][8]={
    {0,1,2,0xf},        //DE index
    {0,1,3,4,5,6,0xf},  //Sync index
    {0,1,2,3,4,5,6,0xf},//DE+Sync index
};

static unsigned gpio_data_set[][2]={
    {PREG_PAD_GPIO4_EN_N, 0x3ffff000},//RGB[7:2]
    //{PREG_PAD_GPIO3_EN_N, 0x1f800000},//RGB[1:0]
};
static unsigned gpio_tcon_set[][2]={
    {PREG_PAD_GPIO3_EN_N, 0x00100000},//clk
    {PREG_PAD_GPIO3_EN_N, 0x00200000},//DE
    {PREG_PAD_GPIO3_EN_N, 0x00480000},//Sync
};
static unsigned gpio_data_set_table[][3]={
    {0,0xf},  //6bit index
    //{0,1,0xf},//8bit index
};
static unsigned gpio_tcon_set_table[][4]={
    {0,1,0xf},  //DE index
    {0,2,0xf},  //Sync index
    {0,1,2,0xf},//DE+Sync index
};

static void lcd_ports_ctrl_ttl(Bool_t status)
{
	unsigned *pin_tcon_set, *pin_data_set;
	unsigned *pin_tcon_clr, *pin_data_clr;
	unsigned *gpio_tcon, *gpio_data;
	unsigned pin_reg;
	int i;

	//tcon pin
	if ((pDev->pConf->lcd_timing.de_valid == 1) && (pDev->pConf->lcd_timing.hvsync_valid == 0)) {
		pin_tcon_set = &pinmux_tcon_set_table[0][0];
		pin_tcon_clr = &pinmux_tcon_clr_table[0][0];
		gpio_tcon = &gpio_tcon_set_table[0][0];
	}
	else if ((pDev->pConf->lcd_timing.de_valid == 0) && (pDev->pConf->lcd_timing.hvsync_valid == 1)) {
		pin_tcon_set = &pinmux_tcon_set_table[1][0];
		pin_tcon_clr = &pinmux_tcon_clr_table[1][0];
		gpio_tcon = &gpio_tcon_set_table[1][0];
	}
	else {
		pin_tcon_set = &pinmux_tcon_set_table[2][0];
		pin_tcon_clr = &pinmux_tcon_clr_table[2][0];
		gpio_tcon = &gpio_tcon_set_table[2][0];
	}
	//RGB data pin
	//if (pDev->pConf->lcd_basic.lcd_bits == 8) {
	//	pin_data_set = &pinmux_data_set_table[1][0];
	//	pin_data_clr = &pinmux_data_clr_table[1][0];
	//	gpio_data = &gpio_data_set_table[1][0];
	//}
	//else
	//{
		pin_data_set = &pinmux_data_set_table[0][0];
		pin_data_clr = &pinmux_data_clr_table[0][0];
		gpio_data = &gpio_data_set_table[0][0];
	//}
	if (status) {
		i = 0;
		while (i < 0xf) {//pinmux_tcon_set
			if (pin_tcon_set[i] == 0xf) {
				break;
			}
			else {
				pin_reg = pinmux_tcon_set[pin_tcon_set[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) | pinmux_tcon_set[pin_tcon_set[i]][1]));
				i++;
			}
		}
		i = 0;
		while (i < 0xf) {//pinmux_tcon_clr
			if (pin_tcon_clr[i] == 0xf) {
				break;
			}
			else {
				pin_reg = pinmux_tcon_clr[pin_tcon_clr[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) & ~(pinmux_tcon_clr[pin_tcon_clr[i]][1])));
				i++;
			}
		}
		i = 0;
		while (i < 0xf) {//pinmux_data_set
			if (pin_data_set[i] == 0xf) {
				break;
			}
			else {
				pin_reg = pinmux_data_set[pin_data_set[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) | pinmux_data_set[pin_data_set[i]][1]));
				i++;
			}
		}
		i = 0;
		while (i < 0xf) {//pinmux_data_clr
			if (pin_data_clr[i] == 0xf) {
				break;
			}
			else {
				pin_reg = pinmux_data_clr[pin_data_clr[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) & ~(pinmux_data_clr[pin_data_clr[i]][1])));
				i++;
			}
		}
	}else {
		i = 0;
		while (i < 0xf) {//pinmux_data_set
			if (pin_data_set[i] == 0xf) {
				break;
			}
			else {
				pin_reg = pinmux_data_set[pin_data_set[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) & ~(pinmux_data_set[pin_data_set[i]][1])));
				i++;
			}
		}
		i = 0;
		while (i < 0xf) {//pinmux_tcon_set
			if (pin_tcon_set[i] == 0xf) {
				break;
			}
			else {
				pin_reg = pinmux_tcon_set[pin_tcon_set[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) & ~(pinmux_tcon_set[pin_tcon_set[i]][1])));
				i++;
			}
		}
		i = 0;
		while (i < 0xf) {//gpio_data_set
			if (gpio_data[i] == 0xf) {
				break;
			}
			else {
				pin_reg = gpio_data_set[gpio_data[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) | (gpio_data_set[gpio_data[i]][1])));
				i++;
			}
		}
		i = 0;
		while (i < 0xf) {//gpio_tcon_set
			if (gpio_tcon[i] == 0xf) {
				break;
			}
			else {
				pin_reg = gpio_tcon_set[gpio_tcon[i]][0];
				WRITE_LCD_CBUS_REG(pin_reg, (READ_LCD_CBUS_REG(pin_reg) | (gpio_tcon_set[gpio_tcon[i]][1])));
				i++;
			}
		}
	}
	DBG_PRINT("%s: %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
}

static void lcd_ports_ctrl(Bool_t status)
{
	switch(pDev->pConf->lcd_basic.lcd_type){
		case LCD_DIGITAL_MIPI:
			lcd_ports_ctrl_mipi(status);
			break;
		case LCD_DIGITAL_LVDS:
			lcd_ports_ctrl_lvds(status);
			break;
		case LCD_DIGITAL_TTL:
			lcd_ports_ctrl_ttl(status);
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

static void set_control_mipi(Lcd_Config_t *pConf);
 //for special interface
static void lcd_power_ctrl_video(Bool_t status)
{
    if (status) {
        switch(pDev->pConf->lcd_basic.lcd_type) {
            case LCD_DIGITAL_MIPI:
                set_control_mipi(pDev->pConf);
                break;
            default:
                break;
        }
    }
    else {
        switch(pDev->pConf->lcd_basic.lcd_type) {
            case LCD_DIGITAL_MIPI:
                mipi_dsi_link_off(pDev->pConf);  //link off command
                break;
            default:
                break;
        }
    }
    DBG_PRINT("%s: %s\n", __FUNCTION__, (status ? "ON" : "OFF"));
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
		lcd_power_ctrl_video(ON);
	}
	else {
		mdelay(30);
		lcd_power_ctrl_video(OFF);
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

static void set_gamma_table_lcd(u16 *data, u32 rgb_mask, u16 gamma_coeff)
{
	int i;

	while (!(READ_LCD_REG(L_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
	WRITE_LCD_REG(L_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) | (0x1 << rgb_mask) | (0x0 << LCD_HADR));
	for (i=0; i<256; i++) {
		while (!(READ_LCD_REG(L_GAMMA_CNTL_PORT) & (0x1 << LCD_WR_RDY)));
		WRITE_LCD_REG(L_GAMMA_DATA_PORT, (data[i] * gamma_coeff / 100));
	}
	while (!(READ_LCD_REG(L_GAMMA_CNTL_PORT) & (0x1 << LCD_ADR_RDY)));
	WRITE_LCD_REG(L_GAMMA_ADDR_PORT, (0x1 << LCD_H_AUTO_INC) | (0x1 << rgb_mask) | (0x23 << LCD_HADR));
}

static void set_tcon_lcd(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s.\n",__FUNCTION__);
	Lcd_Timing_t *tcon_adr = &(pConf->lcd_timing);
	unsigned hs_pol, vs_pol;
	int lcd_type;
	lcd_type = pConf->lcd_basic.lcd_type;

	set_gamma_table_lcd(pConf->lcd_effect.GammaTableR, LCD_H_SEL_R, pConf->lcd_effect.gamma_r_coeff);
	set_gamma_table_lcd(pConf->lcd_effect.GammaTableG, LCD_H_SEL_G, pConf->lcd_effect.gamma_g_coeff);
	set_gamma_table_lcd(pConf->lcd_effect.GammaTableB, LCD_H_SEL_B, pConf->lcd_effect.gamma_b_coeff);

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
		else if(pConf->lcd_basic.lcd_bits == 6)
			WRITE_LCD_REG(L_DITH_CNTL_ADDR,  0x600);
		else
			WRITE_LCD_REG(L_DITH_CNTL_ADDR,  0);
	}

	hs_pol = ((pConf->lcd_timing.pol_cntl_addr >> LCD_HS_POL) & 1);	//0 for low active, 1 for high active
	vs_pol = ((pConf->lcd_timing.pol_cntl_addr >> LCD_VS_POL) & 1);	//0 for low active, 1 for high active

	WRITE_LCD_REG(L_POL_CNTL_ADDR, ((1 << LCD_TCON_DE_SEL) | (1 << LCD_TCON_VS_SEL) | (1 << LCD_TCON_HS_SEL))); //enable tcon DE, Hsync, Vsync
	WRITE_LCD_REG(L_POL_CNTL_ADDR, (READ_LCD_REG(L_POL_CNTL_ADDR) | ((0 << LCD_DE_POL) | ((vs_pol ? 0 : 1) << LCD_VS_POL) | ((hs_pol ? 0 : 1) << LCD_HS_POL)))); //adjust hvsync pol

	//DE signal
	WRITE_LCD_REG(L_DE_HS_ADDR,		tcon_adr->oeh_hs_addr);
	WRITE_LCD_REG(L_DE_HE_ADDR,		tcon_adr->oeh_he_addr);
	WRITE_LCD_REG(L_DE_VS_ADDR,		tcon_adr->oeh_vs_addr);
	WRITE_LCD_REG(L_DE_VE_ADDR,		tcon_adr->oeh_ve_addr);

	//Hsync signal
	WRITE_LCD_REG(L_HSYNC_HS_ADDR,	tcon_adr->sth1_hs_addr);
	WRITE_LCD_REG(L_HSYNC_HE_ADDR,	tcon_adr->sth1_he_addr);
	WRITE_LCD_REG(L_HSYNC_VS_ADDR,	tcon_adr->sth1_vs_addr);
	WRITE_LCD_REG(L_HSYNC_VE_ADDR,	tcon_adr->sth1_ve_addr);

	//Vsync signal
	WRITE_LCD_REG(L_VSYNC_HS_ADDR,	tcon_adr->stv1_hs_addr);
	WRITE_LCD_REG(L_VSYNC_HE_ADDR,	tcon_adr->stv1_he_addr);
	WRITE_LCD_REG(L_VSYNC_VS_ADDR,	tcon_adr->stv1_vs_addr);
	WRITE_LCD_REG(L_VSYNC_VE_ADDR,	tcon_adr->stv1_ve_addr);

	WRITE_LCD_REG(VPP_MISC, READ_LCD_REG(VPP_MISC) & ~(VPP_OUT_SATURATE));
}

static void set_tcon_ttl(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s.\n",__FUNCTION__);
	Lcd_Timing_t *tcon_adr = &(pConf->lcd_timing);
	unsigned hs_pol, vs_pol;

	set_gamma_table_lcd(pConf->lcd_effect.GammaTableR, LCD_H_SEL_R, pConf->lcd_effect.gamma_r_coeff);
	set_gamma_table_lcd(pConf->lcd_effect.GammaTableG, LCD_H_SEL_G, pConf->lcd_effect.gamma_g_coeff);
	set_gamma_table_lcd(pConf->lcd_effect.GammaTableB, LCD_H_SEL_B, pConf->lcd_effect.gamma_b_coeff);

	WRITE_LCD_REG_BITS(L_GAMMA_CNTL_PORT, ((pConf->lcd_effect.gamma_cntl_port >> LCD_GAMMA_EN) & 1), 0, 1);
	//WRITE_LCD_REG(L_GAMMA_VCOM_HSWITCH_ADDR, pConf->lcd_effect.gamma_vcom_hswitch_addr);

	WRITE_LCD_REG(L_RGB_BASE_ADDR, pConf->lcd_effect.rgb_base_addr);
	WRITE_LCD_REG(L_RGB_COEFF_ADDR, pConf->lcd_effect.rgb_coeff_addr);
	WRITE_LCD_REG(L_POL_CNTL_ADDR,   (pConf->lcd_timing.pol_cntl_addr) & ((1 << LCD_CPH1_POL) | (1 << LCD_CPH2_POL) | (1 << LCD_CPH3_POL)));

	if (pConf->lcd_effect.dith_user) {
		WRITE_LCD_REG(L_DITH_CNTL_ADDR, pConf->lcd_effect.dith_cntl_addr);
	}
	else {
		if(pConf->lcd_basic.lcd_bits == 8)
			WRITE_LCD_REG(L_DITH_CNTL_ADDR,  0x400);
		else
			WRITE_LCD_REG(L_DITH_CNTL_ADDR,  0x600);
	}

	hs_pol = (pConf->lcd_timing.pol_cntl_addr >> LCD_HS_POL) & 1;
	vs_pol = (pConf->lcd_timing.pol_cntl_addr >> LCD_VS_POL) & 1;

	if (hs_pol) {
		WRITE_LCD_REG(L_STH1_HS_ADDR,    tcon_adr->sth1_hs_addr);
		WRITE_LCD_REG(L_STH1_HE_ADDR,    tcon_adr->sth1_he_addr);
	}
	else {
		WRITE_LCD_REG(L_STH1_HS_ADDR,    tcon_adr->sth1_he_addr);
		WRITE_LCD_REG(L_STH1_HE_ADDR,    tcon_adr->sth1_hs_addr);
	}
	WRITE_LCD_REG(L_STH1_VS_ADDR,    tcon_adr->sth1_vs_addr);
	WRITE_LCD_REG(L_STH1_VE_ADDR,    tcon_adr->sth1_ve_addr);

	WRITE_LCD_REG(L_OEH_HS_ADDR,     tcon_adr->oeh_hs_addr);
	WRITE_LCD_REG(L_OEH_HE_ADDR,     tcon_adr->oeh_he_addr);
	WRITE_LCD_REG(L_OEH_VS_ADDR,     tcon_adr->oeh_vs_addr);
	WRITE_LCD_REG(L_OEH_VE_ADDR,     tcon_adr->oeh_ve_addr);

	WRITE_LCD_REG(L_STV1_HS_ADDR,    tcon_adr->stv1_hs_addr);
	WRITE_LCD_REG(L_STV1_HE_ADDR,    tcon_adr->stv1_he_addr);
	if (vs_pol) {
		WRITE_LCD_REG(L_STV1_VS_ADDR,    tcon_adr->stv1_vs_addr);
		WRITE_LCD_REG(L_STV1_VE_ADDR,    tcon_adr->stv1_ve_addr);
	}
	else {
		WRITE_LCD_REG(L_STV1_VS_ADDR,    tcon_adr->stv1_ve_addr);
		WRITE_LCD_REG(L_STV1_VE_ADDR,    tcon_adr->stv1_vs_addr);
	}

	WRITE_LCD_REG(L_POL_CNTL_ADDR,   ((1 << LCD_TCON_DE_SEL) | (1 << LCD_TCON_VS_SEL) | (1 << LCD_TCON_HS_SEL))); //enable tcon DE, Hsync, Vsync
	WRITE_LCD_REG(L_POL_CNTL_ADDR,   (READ_LCD_REG(L_POL_CNTL_ADDR) | ((0 << LCD_DE_POL) | ((vs_pol ? 0 : 1) << LCD_VS_POL) | ((hs_pol ? 0 : 1) << LCD_HS_POL))));	//adjust hvsync pol

	//DE signal
	WRITE_LCD_REG(L_DE_HS_ADDR,    tcon_adr->oeh_hs_addr);
	WRITE_LCD_REG(L_DE_HE_ADDR,    tcon_adr->oeh_he_addr);
	WRITE_LCD_REG(L_DE_VS_ADDR,    tcon_adr->oeh_vs_addr);
	WRITE_LCD_REG(L_DE_VE_ADDR,    tcon_adr->oeh_ve_addr);

	WRITE_LCD_REG(L_OEV1_HS_ADDR,  tcon_adr->oeh_hs_addr);
	WRITE_LCD_REG(L_OEV1_HE_ADDR,  tcon_adr->oeh_he_addr);
	WRITE_LCD_REG(L_OEV1_VS_ADDR,  tcon_adr->oeh_vs_addr);
	WRITE_LCD_REG(L_OEV1_VE_ADDR,  tcon_adr->oeh_ve_addr);

	//Hsync signal
	WRITE_LCD_REG(L_HSYNC_HS_ADDR, tcon_adr->sth1_hs_addr);
	WRITE_LCD_REG(L_HSYNC_HE_ADDR, tcon_adr->sth1_he_addr);
	WRITE_LCD_REG(L_HSYNC_VS_ADDR, tcon_adr->sth1_vs_addr);
	WRITE_LCD_REG(L_HSYNC_VE_ADDR, tcon_adr->sth1_ve_addr);

	//Vsync signal
	WRITE_LCD_REG(L_VSYNC_HS_ADDR, tcon_adr->stv1_hs_addr);
	WRITE_LCD_REG(L_VSYNC_HE_ADDR, tcon_adr->stv1_he_addr);
	WRITE_LCD_REG(L_VSYNC_VS_ADDR, tcon_adr->stv1_vs_addr);
	WRITE_LCD_REG(L_VSYNC_VE_ADDR, tcon_adr->stv1_ve_addr);

    WRITE_LCD_REG(L_INV_CNT_ADDR,       tcon_adr->inv_cnt_addr);
    WRITE_LCD_REG(L_TCON_MISC_SEL_ADDR, tcon_adr->tcon_misc_sel_addr);
    //WRITE_LCD_REG(DUAL_PORT_CNTL_ADDR, tcon_adr->dual_port_cntl_addr);

    WRITE_LCD_REG(VPP_MISC, READ_LCD_REG(VPP_MISC) & ~(VPP_OUT_SATURATE));
}

static void set_lcd_spread_spectrum(int ss_level)
{
#if 0
    return;
#endif
}

static void vclk_set_lcd(int lcd_type, int vclk_sel, unsigned long pll_reg, unsigned long vid_div_reg, unsigned long clk_ctrl_reg)
{
	unsigned xd = 0;
	unsigned pll_level = 0, pll_frac = 0, ss_level = 0;
	int wait_loop = PLL_WAIT_LOCK_CNT;
	unsigned pll_lock = 0;
	unsigned pll_ctrl2, pll_ctrl3, pll_ctrl4, od_fb;

	DBG_PRINT("%s.\n", __FUNCTION__);

	vid_div_reg = ((vid_div_reg & 0x1ffff) | (1 << 16) | (0 << 15) | (0x3 << 0));	//select vid_pll and enable clk
	xd = (clk_ctrl_reg >> CLK_CTRL_XD) & 0xf;
	pll_level = (clk_ctrl_reg >> CLK_CTRL_LEVEL) & 0x7;
	pll_frac = (clk_ctrl_reg >> CLK_CTRL_FRAC) & 0xfff;
	ss_level = (clk_ctrl_reg >> CLK_CTRL_SS) & 0xf;
	pll_reg |= (1 << PLL_CTRL_EN);

	if(vclk_sel)
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 0, 19, 1);	//disable vclk2_en
	else
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 0, 19, 2);	//disable vclk1_en1, en0
	udelay(2);

	if (pll_frac == 0)
		pll_ctrl2 = 0x59c88000;
	else
		pll_ctrl2 = (0x59c8c000 | pll_frac);

	pll_ctrl4 = (0x00238100 & ~((1<<9) | (0xf<<4) | (0xf<<0)));
	switch (ss_level) {
		case 1:	//0.5%
			pll_ctrl4 |= ((1<<9) | (2<<4) | (1<<0));
			break;
		case 2:	//1%
			pll_ctrl4 |= ((1<<9) | (1<<4) | (1<<0));
			break;
		case 3:	//1.5%
			pll_ctrl4 |= ((1<<9) | (8<<4) | (1<<0));
			break;
		case 4: //2%
			pll_ctrl4 |= ((1<<9) | (0<<4) | (1<<0));
			break;
		case 0:
		default:
			ss_level = 0;
			break;
	}

	switch (pll_level) {
		case 1: //<=1.7G
			pll_ctrl3 = (ss_level > 0) ? 0xca7e3823 : 0xca49b022;
			od_fb = 0;
			break;
		case 2: //1.7G~2.0G
			pll_ctrl2 |= (1<<13);//special adjust
			pll_ctrl3 = (ss_level > 0) ? 0xca7e3823 : 0xca493822;
			od_fb = 1;
			break;
		case 3: //2.0G~2.5G
			pll_ctrl3 = (ss_level > 0) ? 0xca7e3823 : 0xca493822;
			od_fb = 1;
			break;
		case 4: //>=2.5G
			pll_ctrl3 = (ss_level > 0) ? 0xca7e3823 : 0xce49c022;
			od_fb = 1;
			break;
		default:
			pll_ctrl3 = 0xca7e3823;
			od_fb = 0;
			break;
	}
	WRITE_LCD_CBUS_REG_BITS(HHI_VID2_PLL_CNTL2, 1, 16, 1);//enable ext LDO
	WRITE_LCD_CBUS_REG(HHI_VID_PLL_CNTL2, pll_ctrl2);
	WRITE_LCD_CBUS_REG(HHI_VID_PLL_CNTL3, pll_ctrl3);
	WRITE_LCD_CBUS_REG(HHI_VID_PLL_CNTL4, (pll_ctrl4 | (od_fb << 24))); //[24] od_fb
	WRITE_LCD_CBUS_REG(HHI_VID_PLL_CNTL5, 0x00012385);
	WRITE_LCD_CBUS_REG(HHI_VID_PLL_CNTL, pll_reg | (1 << PLL_CTRL_RST));
	WRITE_LCD_CBUS_REG(HHI_VID_PLL_CNTL, pll_reg);

	do{
		udelay(50);
		pll_lock = (READ_LCD_CBUS_REG(HHI_VID_PLL_CNTL) >> PLL_CTRL_LOCK) & 0x1;
		if (wait_loop == 100) {
			if (pll_level == 2) {
				//change setting if can't lock
				WRITE_LCD_CBUS_REG_BITS(HHI_VID_PLL_CNTL2, 1, 12, 1);
				WRITE_LCD_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 1, PLL_CTRL_RST, 1);
				WRITE_LCD_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 0, PLL_CTRL_RST, 1);
				printf("change setting for vid pll stability\n");
			}
		}
		wait_loop--;
	}while((pll_lock == 0) && (wait_loop > 0));
	if (wait_loop == 0)
		printf("[error]: vid_pll lock failed\n");

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
		udelay(2);
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_DIV, 8, 12, 4); // [15:12] encl_clk_sel, select vclk2_div1
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_DIV, 1, 16, 2); // release vclk2_div_reset and enable vclk2_div
	}
	else {
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_DIV, (xd-1), 0, 8);	// setup XD divider
		udelay(5);
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 4, 16, 3);  // Bit[18:16] - v2_cntl_clk_in_sel
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 3, 19, 2);     //[19]vclk_en0, [20]vclk_en1 (en1 is for tcon_clko???)
		udelay(2);
		WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_DIV, 0, 12, 4); // [15:12] encl_clk_sel, select vclk_div1
		WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_DIV, 1, 16, 2); // release vclk_div_reset and enable vclk_div
	}
	udelay(2);

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

	WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL2, 1, 3, 1);	//enable encl clk gate, new added in m8baby
}

static void clk_util_lvds_set_clk_div(unsigned long divn_sel, unsigned long divn_tcnt, unsigned long div2_en)
{
#if 0
    // assign          lvds_div_phy_clk_en     = tst_lvds_tmode ? 1'b1         : phy_clk_cntl[10];
    // assign          lvds_div_div2_sel       = tst_lvds_tmode ? atest_i[5]   : phy_clk_cntl[9];
    // assign          lvds_div_sel            = tst_lvds_tmode ? atest_i[7:6] : phy_clk_cntl[8:7];
    // assign          lvds_div_tcnt           = tst_lvds_tmode ? 3'd6         : phy_clk_cntl[6:4];
    // If dividing by 1, just select the divide by 1 path
    if( divn_tcnt == 1 ) {
        divn_sel = 0;
    }
    WRITE_LCD_REG(LVDS_PHY_CLK_CNTL, ((READ_LCD_REG(LVDS_PHY_CLK_CNTL) & ~((0x3 << 7) | (1 << 9) | (0x7 << 4))) | ((1 << 10) | (divn_sel << 7) | (div2_en << 9) | (((divn_tcnt-1)&0x7) << 4))));
#else
    // ---------------------------------------------
    // Configure the LVDS PHY
    // ---------------------------------------------
    // wire    [4:0]   cntl_ser_en         = control[20:16];
    // wire            cntl_prbs_en        = control[13];
    // wire            cntl_prbs_err_en    = control[12];
    // wire    [1:0]   cntl_mode_set_high  = control[11:10];
    // wire    [1:0]   cntl_mode_set_low   = control[9:8];
    //
    // wire    [1:0]   fifo_clk_sel        = control[7;6]
    //
    // wire            mode_port_rev       = control[4];
    // wire            mode_bit_rev        = control[3];
    // wire            mode_inv_p_n        = control[2];
    // wire            phy_clk_en          = control[1];
    // wire            soft_reset_int      = control[0];
    WRITE_LCD_CBUS_REG(HHI_LVDS_TX_PHY_CNTL0, (0x1f << 16) | (0x1 << 6) ); // enable all serializers, divide by 7
#endif
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
		case LCD_DIGITAL_MIPI:
			break;
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
		case LCD_DIGITAL_MIPI:
			WRITE_LCD_REG(MIPI_DSI_TOP_CNTL, (READ_LCD_REG(MIPI_DSI_TOP_CNTL) & ~(0x7<<4)) | (1  << 4) | (1  << 5) | (0  << 6));
			WRITE_LCD_REG(MIPI_DSI_TOP_SW_RESET, (READ_LCD_REG(MIPI_DSI_TOP_SW_RESET) | 0xf));         // Release mipi_dsi_host's reset
			WRITE_LCD_REG(MIPI_DSI_TOP_SW_RESET, (READ_LCD_REG(MIPI_DSI_TOP_SW_RESET) & 0xfffffff0));  // Release mipi_dsi_host's reset
			WRITE_LCD_REG(MIPI_DSI_TOP_CLK_CNTL, (READ_LCD_REG(MIPI_DSI_TOP_CLK_CNTL) | 0x3));         // Enable dwc mipi_dsi_host's clock
			break;
		case LCD_DIGITAL_LVDS:
			clk_util_lvds_set_clk_div(1, pll_div_post, phy_clk_div2);
			//    lvds_gen_cntl       <= {10'h0,      // [15:4] unused
			//                            2'h1,       // [5:4] divide by 7 in the PHY
			//                            1'b0,       // [3] fifo_en
			//                            1'b0,       // [2] wr_bist_gate
			//                            2'b00};     // [1:0] fifo_wr mode
			//FIFO_CLK_SEL = 1; // div7
			WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 1, 4, 2);	//lvds fifo clk div 7

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

static void set_venc_lcd(Lcd_Config_t *pConf)
{
	DBG_PRINT("%s\n",__FUNCTION__);

	WRITE_LCD_REG(ENCL_VIDEO_EN,          0);

	WRITE_LCD_REG(VPU_VIU_VENC_MUX_CTRL,  (0<<0) | (0<<2));	//viu1, viu2 select encl

	WRITE_LCD_REG(ENCL_VIDEO_MODE,        0);
	WRITE_LCD_REG(ENCL_VIDEO_MODE_ADV,    0x8); // Sampling rate: 1

	WRITE_LCD_REG(ENCL_VIDEO_FILT_CTRL,   0x1000);	// bypass filter

	WRITE_LCD_REG(ENCL_VIDEO_MAX_PXCNT,   pConf->lcd_basic.h_period - 1);
	WRITE_LCD_REG(ENCL_VIDEO_MAX_LNCNT,   pConf->lcd_basic.v_period - 1);

	WRITE_LCD_REG(ENCL_VIDEO_HAVON_BEGIN, pConf->lcd_timing.video_on_pixel);
	WRITE_LCD_REG(ENCL_VIDEO_HAVON_END,   pConf->lcd_basic.h_active - 1 + pConf->lcd_timing.video_on_pixel);
	WRITE_LCD_REG(ENCL_VIDEO_VAVON_BLINE, pConf->lcd_timing.video_on_line);
	WRITE_LCD_REG(ENCL_VIDEO_VAVON_ELINE, pConf->lcd_basic.v_active - 1  + pConf->lcd_timing.video_on_line);

	WRITE_LCD_REG(ENCL_VIDEO_HSO_BEGIN,   pConf->lcd_timing.sth1_hs_addr);
	WRITE_LCD_REG(ENCL_VIDEO_HSO_END,     pConf->lcd_timing.sth1_he_addr);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_BEGIN,   pConf->lcd_timing.stv1_hs_addr);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_END,     pConf->lcd_timing.stv1_he_addr);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_BLINE,   pConf->lcd_timing.stv1_vs_addr);
	WRITE_LCD_REG(ENCL_VIDEO_VSO_ELINE,   pConf->lcd_timing.stv1_ve_addr);

	WRITE_LCD_REG(ENCL_VIDEO_RGBIN_CTRL,  (1 << 0));//(1 << 1) | (1 << 0));	//bit[0] 1:RGB, 0:YUV

    WRITE_LCD_REG(ENCL_VIDEO_EN,          1);	// enable encl
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
    WRITE_LCD_REG(LVDS_BLANK_DATA_HI, (data32 >> 16));
    WRITE_LCD_REG(LVDS_BLANK_DATA_LO, (data32 & 0xffff));

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

	//no need to set in M8, no minilvds logic, always lvds
	//WRITE_LCD_REG_BITS(MLVDS_CONTROL, 0, 0, 1);  //disable mlvds

	WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 1, 0, 1);  //[1:0]set single clock write mode
	WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 1, 3, 1); // enable fifo

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

	//DBG_PRINT("lvds fifo clk = %d.\n", clk_util_clk_msr(LVDS_FIFO_CLK));
}

static void set_control_mipi(Lcd_Config_t *pConf)
{
    set_mipi_dsi_control(pConf);
}

static void set_control_ttl(Lcd_Config_t *pConf)
{
	unsigned rb_port_swap, rgb_bit_swap;

	rb_port_swap = (unsigned)(pConf->lcd_control.ttl_config->rb_swap);
	rgb_bit_swap = (unsigned)(pConf->lcd_control.ttl_config->bit_swap);

	WRITE_LCD_REG(L_DUAL_PORT_CNTL_ADDR, (rb_port_swap << LCD_RGB_SWP) | (rgb_bit_swap << LCD_BIT_SWP));
}

static void init_phy_lvds(Lcd_Config_t *pConf)
{
	unsigned swing_ctrl;
	DBG_PRINT("%s\n", __FUNCTION__);

	WRITE_LCD_REG(LVDS_SER_EN, 0xfff);	//Enable the serializers

    WRITE_LCD_REG(LVDS_PHY_CNTL0, 0xffff);
    WRITE_LCD_REG(LVDS_PHY_CNTL1, 0xff00);
	WRITE_LCD_REG(LVDS_PHY_CNTL4, 0x007f);

	//WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL1, 0x00000348);
	switch (pConf->lcd_control.lvds_config->lvds_vswing) {
		case 0:
			swing_ctrl = 0x028;
			break;
		case 1:
			swing_ctrl = 0x048;
			break;
		case 2:
			swing_ctrl = 0x088;
			break;
		case 3:
			swing_ctrl = 0x0c8;
			break;
		case 4:
			swing_ctrl = 0x0f8;
			break;
		default:
			swing_ctrl = 0x048;
			break;
	}
	WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL1, swing_ctrl);
	WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL2, 0x000665b7);
	WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL3, 0x84070000);
}

static void init_phy_mipi(Lcd_Config_t *pConf)
{
    DBG_PRINT("%s\n", __FUNCTION__);

    WRITE_LCD_CBUS_REG_BITS(HHI_DSI_LVDS_EDP_CNTL1, 1, 4, 1);//swap mipi channels
    WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL1, 0x8);//DIF_REF_CTL0
    WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL2, (0x3e << 16) | (0xa5b8 << 0));//DIF_REF_CTL2:31-16bit, DIF_REF_CTL1:15-0bit
    WRITE_LCD_CBUS_REG(HHI_DIF_CSI_PHY_CNTL3, (0x26e0 << 16) | (0x459 << 0));//DIF_TX_CTL1:31-16bit, DIF_TX_CTL0:15-0bit
}

static void init_dphy(Lcd_Config_t *pConf)
{
	unsigned lcd_type = (unsigned)(pConf->lcd_basic.lcd_type);

	switch (lcd_type) {
		case LCD_DIGITAL_MIPI:
			WRITE_LCD_CBUS_REG(HHI_DSI_LVDS_EDP_CNTL0, lcd_type);	//dphy select by interface
			init_phy_mipi(pConf);
			break;
		case LCD_DIGITAL_LVDS:
			WRITE_LCD_CBUS_REG(HHI_DSI_LVDS_EDP_CNTL0, lcd_type);	//dphy select by interface
			init_phy_lvds(pConf);
			break;
		default:
			break;
	}
}

static void set_video_adjust(Lcd_Config_t *pConf)
{
	DBG_PRINT("vadj_brightness = 0x%x, vadj_contrast = 0x%x, vadj_saturation = 0x%x.\n", pConf->lcd_effect.vadj_brightness, pConf->lcd_effect.vadj_contrast, pConf->lcd_effect.vadj_saturation);
	WRITE_LCD_REG(VPP_VADJ2_Y, (pConf->lcd_effect.vadj_brightness << 8) | (pConf->lcd_effect.vadj_contrast << 0));
	WRITE_LCD_REG(VPP_VADJ2_MA_MB, (pConf->lcd_effect.vadj_saturation << 16));
	WRITE_LCD_REG(VPP_VADJ2_MC_MD, (pConf->lcd_effect.vadj_saturation << 0));
	WRITE_LCD_REG(VPP_VADJ_CTRL, 0xf);	//enable video adjust
}

static void generate_clk_parameter(Lcd_Config_t *pConf)
{
    unsigned pll_n = 0, pll_m = 0, pll_od = 0, pll_frac = 0, pll_level = 0;
    unsigned vid_div_pre = 0, crt_xd = 0;

    unsigned m, n, od, od_fb=0, div_pre, div_post, xd;
    unsigned od_sel, pre_div_sel;
    unsigned div_pre_sel_max, crt_xd_max;
    unsigned pll_vco, fout_pll, edp_tx_phy_out, div_pre_out, div_post_out, final_freq, iflogic_vid_clk_in_max;
    unsigned min_error = MAX_ERROR;
    unsigned error = MAX_ERROR;
    unsigned clk_num = 0;
    unsigned tmp;
    unsigned int dsi_bit_rate_min=0, dsi_bit_rate_max=0;

    unsigned fin = FIN_FREQ;
    unsigned fout = pConf->lcd_timing.lcd_clk;

    if (fout >= 200) {//clk
        fout = fout / 1000;  //kHz
    }
    else {//frame_rate
        fout = (fout * pConf->lcd_basic.h_period * pConf->lcd_basic.v_period) / 1000;	//kHz
    }

    switch (pConf->lcd_basic.lcd_type) {
        case LCD_DIGITAL_MIPI:
            div_pre_sel_max = DIV_PRE_SEL_MAX;
            div_post = 1;
            crt_xd_max = 16;
            dsi_bit_rate_min = pConf->lcd_control.mipi_config->bit_rate_min;
            dsi_bit_rate_max = pConf->lcd_control.mipi_config->bit_rate_max;
            iflogic_vid_clk_in_max = MIPI_MAX_VID_CLK_IN;
            break;
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
        default:
            div_pre_sel_max = DIV_PRE_SEL_MAX;
            div_post = 1;
            crt_xd_max = 1;
            iflogic_vid_clk_in_max = ENCL_MAX_CLK_IN;
            break;
    }

    switch (pConf->lcd_basic.lcd_type) {
        case LCD_DIGITAL_MIPI:
            if (fout < ENCL_MAX_CLK_IN) {
                for (xd = 1; xd <= crt_xd_max; xd++) {
                    div_post_out = fout * xd;
                    DBG_PRINT("div_post_out=%d, xd=%d, fout=%d\n",div_post_out, xd, fout);
                    if (div_post_out <= CRT_VID_MAX_CLK_IN) {
                        div_pre_out = div_post_out * div_post;
                        if (div_pre_out <= DIV_POST_MAX_CLK_IN) {
                            for (pre_div_sel = 0; pre_div_sel < div_pre_sel_max; pre_div_sel++) {
                                div_pre = div_pre_table[pre_div_sel];
                                fout_pll = div_pre_out * div_pre;
                                DBG_PRINT("pre_div_sel=%d, div_pre=%d, fout_pll=%d\n", pre_div_sel, div_pre, fout_pll);
                                if ((fout_pll <= dsi_bit_rate_max) && (fout_pll >= dsi_bit_rate_min)){
                                    for (od_sel = OD_SEL_MAX; od_sel > 0; od_sel--) {
                                        od = od_table[od_sel - 1];
                                        pll_vco = fout_pll * od;
                                        DBG_PRINT("od_sel=%d, od=%d, pll_vco=%d\n", od_sel, od, pll_vco);
                                        if ((pll_vco >= PLL_VCO_MIN) && (pll_vco <= PLL_VCO_MAX)) {
                                            if ((pll_vco >= 2500000) && (pll_vco <= PLL_VCO_MAX)) {
                                                od_fb = 1;
                                                pll_level = 4;
                                            }
                                            else if ((pll_vco >= 2000000) && (pll_vco < 2500000)) {
                                                od_fb = 1;
                                                pll_level = 3;
                                            }
                                            else if ((pll_vco >= 1700000) && (pll_vco < 2000000)) {//special adjust
                                                od_fb = 1;
                                                pll_level = 2;
                                            }
                                            else if ((pll_vco >= PLL_VCO_MIN) && (pll_vco < 1700000)) {
                                                od_fb = 0;
                                                pll_level = 1;
                                            }
                                            n = 1;
                                            m = pll_vco / (fin * (od_fb + 1));
                                            pll_frac = (pll_vco % (fin * (od_fb + 1))) * 4096 / (fin * (od_fb + 1));
                                            pll_m = m;
                                            pll_n = n;
                                            pll_od = od_sel - 1;
                                            vid_div_pre = pre_div_sel;
                                            crt_xd = xd;
                                            clk_num = 1;
                                            DBG_PRINT("pll_m=0x%x, pll_n=0x%x, pll_od=0x%x, vid_div_pre=0x%x, crt_xd=0x%x, pll_frac=0x%x, pll_level=%d\n",
                                                       pll_m, pll_n, pll_od, vid_div_pre, crt_xd, pll_frac, pll_level);
                                        }
                                        if (clk_num > 0)
                                            break;
                                    }
                                }
                                if (clk_num > 0)
                                    break;
                            }
                        }
                    }
                    if (clk_num > 0)
                        break;
                }
            }
            break;
        case LCD_DIGITAL_LVDS:
        case LCD_DIGITAL_TTL:
            if (fout < ENCL_MAX_CLK_IN) {
                for (xd = 1; xd <= crt_xd_max; xd++) {
                    div_post_out = fout * xd;
                    DBG_PRINT("div_post_out=%d, xd=%d, fout=%d\n",div_post_out, xd, fout);
                    if (div_post_out <= CRT_VID_MAX_CLK_IN) {
                        div_pre_out = div_post_out * div_post;
                        if (div_pre_out <= DIV_POST_MAX_CLK_IN) {
                            for (pre_div_sel = 0; pre_div_sel < div_pre_sel_max; pre_div_sel++) {
                                div_pre = div_pre_table[pre_div_sel];
                                fout_pll = div_pre_out * div_pre;
                                DBG_PRINT("pre_div_sel=%d, div_pre=%d, fout_pll=%d\n", pre_div_sel, div_pre, fout_pll);
                                if (fout_pll <= DIV_PRE_MAX_CLK_IN) {
                                    for (od_sel = OD_SEL_MAX; od_sel > 0; od_sel--) {
                                        od = od_table[od_sel - 1];
                                        pll_vco = fout_pll * od;
                                        DBG_PRINT("od_sel=%d, od=%d, pll_vco=%d\n", od_sel, od, pll_vco);
                                        if ((pll_vco >= PLL_VCO_MIN) && (pll_vco <= PLL_VCO_MAX)) {
                                            if ((pll_vco >= 2500000) && (pll_vco <= PLL_VCO_MAX)) {
                                                od_fb = 1;
                                                pll_level = 4;
                                            }
                                            else if ((pll_vco >= 2000000) && (pll_vco < 2500000)) {
                                                od_fb = 1;
                                                pll_level = 3;
                                            }
                                            else if ((pll_vco >= 1700000) && (pll_vco < 2000000)) {
                                                od_fb = 1;
                                                pll_level = 2;
                                            }
                                            else if ((pll_vco >= PLL_VCO_MIN) && (pll_vco < 1700000)) {
                                                od_fb = 0;
                                                pll_level = 1;
                                            }
                                            n = 1;
                                            m = pll_vco / (fin * (od_fb + 1));
                                            pll_frac = (pll_vco % (fin * (od_fb + 1))) * 4096 / (fin * (od_fb + 1));

                                            pll_m = m;
                                            pll_n = n;
                                            pll_od = od_sel - 1;
                                            vid_div_pre = pre_div_sel;
                                            crt_xd = xd;

                                            clk_num = 1;
                                            DBG_PRINT("pll_m=0x%x, pll_n=0x%x, pll_od=0x%x, vid_div_pre=0x%x, crt_xd=0x%x, pll_frac=0x%x, pll_level=%d\n",
                                                       pll_m, pll_n, pll_od, vid_div_pre, crt_xd, pll_frac, pll_level);
                                        }
                                        if (clk_num > 0)
                                            break;
                                    }
                                }
                                if (clk_num > 0)
                                    break;
                            }
                        }
                    }
                    if (clk_num > 0)
                        break;
                }
            }
            break;
        default:
            break;
    }
    if (clk_num > 0) {
        pConf->lcd_timing.pll_ctrl = (pll_od << 16) | (pll_n << 10) | (pll_m << 0);
        pConf->lcd_timing.div_ctrl = 0x18803 | (vid_div_pre << DIV_CTRL_DIV_PRE);
        tmp = (pConf->lcd_timing.clk_ctrl & ~((0xf << CLK_CTRL_XD) | (0x7 << CLK_CTRL_LEVEL) | (0xfff << CLK_CTRL_FRAC)));
        pConf->lcd_timing.clk_ctrl = (tmp | ((crt_xd << CLK_CTRL_XD) | (pll_level << CLK_CTRL_LEVEL) | (pll_frac << CLK_CTRL_FRAC)));
    }
    else {
        pConf->lcd_timing.pll_ctrl = (1 << PLL_CTRL_OD) | (1 << PLL_CTRL_N) | (50 << PLL_CTRL_M);
        pConf->lcd_timing.div_ctrl = 0x18803 | (1 << DIV_CTRL_DIV_PRE);
        pConf->lcd_timing.clk_ctrl = (pConf->lcd_timing.clk_ctrl & ~(0xf << CLK_CTRL_XD)) | (7 << CLK_CTRL_XD);
        printf("Out of clock range, reset to default setting!\n");
    }
}

static void lcd_sync_duration(Lcd_Config_t *pConf)
{
	unsigned m, n, od, od_fb, frac, pre_div, xd, post_div;
	unsigned h_period, v_period, sync_duration;
	unsigned lcd_clk;

	m = ((pConf->lcd_timing.pll_ctrl) >> 0) & 0x1ff;
	n = ((pConf->lcd_timing.pll_ctrl) >> 10) & 0x1f;
	od = ((pConf->lcd_timing.pll_ctrl) >> 16) & 0x3;
	od = od_table[od];
	frac = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_FRAC) & 0xfff;
	od_fb = ((((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_LEVEL) & 0x7) > 1) ? 1 : 0;

	pre_div = ((pConf->lcd_timing.div_ctrl) >> DIV_CTRL_DIV_PRE) & 0x7;
	pre_div = div_pre_table[pre_div];

	h_period = pConf->lcd_basic.h_period;
	v_period = pConf->lcd_basic.v_period;

	switch(pConf->lcd_basic.lcd_type) {
		case LCD_DIGITAL_MIPI:
			xd = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_XD) & 0xf;
			post_div = 1;
			break;
		case LCD_DIGITAL_LVDS:
			xd = 1;
			post_div = 7;
			break;
		case LCD_DIGITAL_TTL:
			xd = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_XD) & 0xf;
			post_div = 1;
			break;
		default:
			xd = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_XD) & 0xf;
			post_div = 1;
			break;
	}

	lcd_clk = (frac * (od_fb + 1) * FIN_FREQ) / 4096;
	//printf("pll_clk = %uMHz, m=%u, frac=%u, od_fb=%u, n=%u, od=%u\n", lcd_clk, m, frac, od_fb, n, od);
	lcd_clk = ((m * (od_fb + 1) * FIN_FREQ + lcd_clk) / (n * od * pre_div * post_div * xd)) * 1000;
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
	unsigned short h_offset = 0, v_offset = 0, vsync_h_phase=0;

	switch (pConf->lcd_basic.lcd_type) {
		case LCD_DIGITAL_MIPI:
			h_delay = MIPI_DELAY;
			break;
		case LCD_DIGITAL_LVDS:
			h_delay = LVDS_DELAY;
			break;
		case LCD_DIGITAL_TTL:
			h_delay = TTL_DELAY;
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

	vsync_h_phase = (pConf->lcd_timing.vsync_h_phase & 0xffff);
	if ((pConf->lcd_timing.vsync_h_phase >> 31) & 1) //negative
		vsync_h_phase = (hstart + pConf->lcd_basic.h_period - vsync_h_phase) % pConf->lcd_basic.h_period;
	else	//positive
		vsync_h_phase = (hstart + pConf->lcd_basic.h_period + vsync_h_phase) % pConf->lcd_basic.h_period;
	pConf->lcd_timing.stv1_hs_addr = vsync_h_phase;
	pConf->lcd_timing.stv1_he_addr = vsync_h_phase;

	vstart = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp) % pConf->lcd_basic.v_period;
	vend = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp + pConf->lcd_timing.vsync_width) % pConf->lcd_basic.v_period;
	pConf->lcd_timing.stv1_vs_addr = vstart;
	pConf->lcd_timing.stv1_ve_addr = vend;

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

    vsync_h_phase = (pConf->lcd_timing.vsync_h_phase & 0xffff);
    if ((pConf->lcd_timing.vsync_h_phase >> 31) & 1) //negative
        vsync_h_phase = (hstart + pConf->lcd_basic.h_period - vsync_h_phase) % pConf->lcd_basic.h_period;
    else	//positive
        vsync_h_phase = (hstart + pConf->lcd_basic.h_period + vsync_h_phase) % pConf->lcd_basic.h_period;
    pConf->lcd_timing.stv1_hs_addr = vsync_h_phase;
    pConf->lcd_timing.stv1_he_addr = vsync_h_phase;
    vstart = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp) % pConf->lcd_basic.v_period;
    vend = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_period - pConf->lcd_timing.vsync_bp + pConf->lcd_timing.vsync_width) % pConf->lcd_basic.v_period;
    pConf->lcd_timing.stv1_vs_addr = vstart;
    pConf->lcd_timing.stv1_ve_addr = vend;

    pConf->lcd_timing.oeh_hs_addr = pConf->lcd_timing.de_hstart;
    pConf->lcd_timing.oeh_he_addr = (pConf->lcd_timing.de_hstart + pConf->lcd_basic.h_active) % pConf->lcd_basic.h_period;
    pConf->lcd_timing.oeh_vs_addr = pConf->lcd_timing.de_vstart;
    pConf->lcd_timing.oeh_ve_addr = (pConf->lcd_timing.de_vstart + pConf->lcd_basic.v_active - 1) % pConf->lcd_basic.v_period;
#endif

	//DBG_PRINT("sth1_hs_addr=%d, sth1_he_addr=%d, sth1_vs_addr=%d, sth1_ve_addr=%d\n", pConf->lcd_timing.sth1_hs_addr, pConf->lcd_timing.sth1_he_addr, pConf->lcd_timing.sth1_vs_addr, pConf->lcd_timing.sth1_ve_addr);
	//DBG_PRINT("stv1_hs_addr=%d, stv1_he_addr=%d, stv1_vs_addr=%d, stv1_ve_addr=%d\n", pConf->lcd_timing.stv1_hs_addr, pConf->lcd_timing.stv1_he_addr, pConf->lcd_timing.stv1_vs_addr, pConf->lcd_timing.stv1_ve_addr);
	//DBG_PRINT("oeh_hs_addr=%d, oeh_he_addr=%d, oeh_vs_addr=%d, oeh_ve_addr=%d\n", pConf->lcd_timing.oeh_hs_addr, pConf->lcd_timing.oeh_he_addr, pConf->lcd_timing.oeh_vs_addr, pConf->lcd_timing.oeh_ve_addr);
}

static void lcd_control_config_pre(Lcd_Config_t *pConf)
{
    unsigned vclk_sel, ss_level;

    vclk_sel = 1;
    ss_level = (pConf->lcd_timing.clk_ctrl >> CLK_CTRL_SS) & 0xf;

    switch (pConf->lcd_basic.lcd_type) {
        case LCD_DIGITAL_MIPI:
            ss_level = ((ss_level > 0) ? 1 : 0);
            set_mipi_dsi_control_config(pConf);
            break;
        case LCD_DIGITAL_LVDS:
            ss_level = ((ss_level >= SS_LEVEL_MAX) ? (SS_LEVEL_MAX-1) : ss_level);
            if (pConf->lcd_control.lvds_config->lvds_repack_user == 0) {
                if (pConf->lcd_basic.lcd_bits == 6)
                    pConf->lcd_control.lvds_config->lvds_repack = 0;
                else
                    pConf->lcd_control.lvds_config->lvds_repack = 1;
            }
            break;
        case LCD_DIGITAL_TTL:
            ss_level = ((ss_level >= SS_LEVEL_MAX) ? (SS_LEVEL_MAX-1) : ss_level);
            if (pConf->lcd_basic.lcd_bits != 6) {
                pConf->lcd_basic.lcd_bits = 6;
                printf("lcd change to 6bit for ttl support!\n");
            }
            break;
        default:
            ss_level = ((ss_level >= SS_LEVEL_MAX) ? (SS_LEVEL_MAX-1) : ss_level);
            break;
    }
    pConf->lcd_timing.clk_ctrl &= (~((1 << CLK_CTRL_VCLK_SEL) | (0xf << CLK_CTRL_SS)));
    pConf->lcd_timing.clk_ctrl |= ((vclk_sel << CLK_CTRL_VCLK_SEL) | (ss_level << CLK_CTRL_SS));
}

static void lcd_control_config_post(Lcd_Config_t *pConf)
{
    switch (pConf->lcd_basic.lcd_type) {
        case LCD_DIGITAL_MIPI:
            set_mipi_dsi_control_config_post(pConf);
            break;
        default:
            break;
    }
}

static void lcd_config_init(Lcd_Config_t *pConf)
{
#ifdef CONFIG_PLATFORM_HAS_PMU
	struct aml_pmu_driver *pmu_driver;
	int battery_percent;
#endif

	lcd_control_config_pre(pConf);
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
	lcd_control_config_post(pConf);

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

static void print_lcd_info(Lcd_Config_t *pConf)
{
#ifdef LCD_DEBUG_INFO
    unsigned lcd_clk;
    int h_adj, v_adj;

    lcd_clk = (pConf->lcd_timing.lcd_clk / 1000);
    h_adj = ((pConf->lcd_timing.h_offset >> 31) & 1);
    v_adj = ((pConf->lcd_timing.v_offset >> 31) & 1);

    printf("\nLCD mode: %s %ubit, %ux%u@%u.%uHz\n"
           "lcd_clk           %u.%03uMHz\n"
           "ss_level          %d\n"
           "clk_pol           %d\n\n",
           lcd_type_table[pConf->lcd_basic.lcd_type], pConf->lcd_basic.lcd_bits, pConf->lcd_basic.h_active, pConf->lcd_basic.v_active, (pConf->lcd_timing.sync_duration_num / 10), (pConf->lcd_timing.sync_duration_num % 10),
           (lcd_clk / 1000), (lcd_clk % 1000), ((pConf->lcd_timing.clk_ctrl >> CLK_CTRL_SS) & 0xf), ((pConf->lcd_timing.pol_cntl_addr >> LCD_CPH1_POL) & 1));

    printf("h_period          %d\n"
           "v_period          %d\n"
           "hs_width          %d\n"
           "hs_backporch      %d\n"
           "hs_pol            %d\n"
           "vs_width          %d\n"
           "vs_backporch      %d\n"
           "vs_pol            %d\n"
           "vs_h_phase        %s%d\n"
           "hvsync_valid      %d\n"
           "de_valid          %d\n"
           "h_offset          %s%d\n"
           "v_offset          %s%d\n\n",
           pConf->lcd_basic.h_period, pConf->lcd_basic.v_period,
           pConf->lcd_timing.hsync_width, pConf->lcd_timing.hsync_bp, ((pConf->lcd_timing.pol_cntl_addr >> LCD_HS_POL) & 1),
           pConf->lcd_timing.vsync_width, pConf->lcd_timing.vsync_bp, ((pConf->lcd_timing.pol_cntl_addr >> LCD_VS_POL) & 1),
           (((pConf->lcd_timing.vsync_h_phase >> 31) & 1) ? "-":""), (pConf->lcd_timing.vsync_h_phase & 0xffff), pConf->lcd_timing.hvsync_valid, pConf->lcd_timing.de_valid,
           (h_adj ? "-" : ""), (pConf->lcd_timing.h_offset & 0xffff), (v_adj ? "-" : ""), (pConf->lcd_timing.v_offset & 0xffff));

    switch (pConf->lcd_basic.lcd_type) {
        case LCD_DIGITAL_TTL:
            printf("rb_swap           %u\n"
                   "bit_swap          %u\n\n",
                   pConf->lcd_control.ttl_config->rb_swap, pConf->lcd_control.ttl_config->bit_swap);
            break;
        case LCD_DIGITAL_LVDS:
            printf("vswing_level      %u\n"
                   "lvds_repack       %u\n"
                   "pn_swap           %u\n\n",
                   pConf->lcd_control.lvds_config->lvds_vswing, pConf->lcd_control.lvds_config->lvds_repack, pConf->lcd_control.lvds_config->pn_swap);
            break;
        case LCD_DIGITAL_MIPI:
            printf("dsi_lane_num      %u\n"
                   "dsi_bit_rate      %u.%03uMHz\n"
                   "operation_mode    %u(%s), %u(%s)\n"
                   "transfer_ctrl     %u, %u\n\n",
                   pDev->pConf->lcd_control.mipi_config->lane_num,
                   (pDev->pConf->lcd_control.mipi_config->bit_rate / 1000000), ((pDev->pConf->lcd_control.mipi_config->bit_rate % 1000000) / 1000),
                   ((pDev->pConf->lcd_control.mipi_config->operation_mode>>BIT_OPERATION_MODE_INIT) &1), (((pDev->pConf->lcd_control.mipi_config->operation_mode>>BIT_OPERATION_MODE_INIT) & 1) ? "COMMAND" : "VIDEO"),
                   ((pDev->pConf->lcd_control.mipi_config->operation_mode>>BIT_OPERATION_MODE_DISP) & 1), (((pDev->pConf->lcd_control.mipi_config->operation_mode>>BIT_OPERATION_MODE_DISP) & 1) ? "COMMAND" : "VIDEO"),
                   ((pDev->pConf->lcd_control.mipi_config->transfer_ctrl>>BIT_TRANS_CTRL_CLK) & 1), ((pDev->pConf->lcd_control.mipi_config->transfer_ctrl>>BIT_TRANS_CTRL_SWITCH) & 3));
            break;
        default:
            break;
    }

    if (pConf->lcd_effect.dith_user)
        printf("dither_ctrl       0x%x\n", pConf->lcd_effect.dith_cntl_addr);

    printf("pll_ctrl          0x%08x\n"
           "div_ctrl          0x%08x\n"
           "clk_ctrl          0x%08x\n"
           "video_on_pixel    %d\n"
           "video_on_line     %d\n\n",
           pConf->lcd_timing.pll_ctrl, pConf->lcd_timing.div_ctrl, pConf->lcd_timing.clk_ctrl,
           pConf->lcd_timing.video_on_pixel, pConf->lcd_timing.video_on_line);

    printf("vid2 pll clk      %dMHz\n"
           "lvds fifo clk     %dMHz\n"
           "cts encl clk      %dMHz\n\n",
           clk_util_clk_msr(62), clk_util_clk_msr(24), clk_util_clk_msr(9));
#endif
}

static void _init_lcd_driver(Lcd_Config_t *pConf)
{
	int lcd_type = pConf->lcd_basic.lcd_type;
	unsigned char ss_level = (pConf->lcd_timing.clk_ctrl >> CLK_CTRL_SS) & 0xf;

	printf("Init LCD mode: %s(%u) %ubit, %ux%u@%u.%uHz, ss_level=%u(%s)\n", lcd_type_table[lcd_type], lcd_type, pConf->lcd_basic.lcd_bits, pConf->lcd_basic.h_active, pConf->lcd_basic.v_active, (pConf->lcd_timing.sync_duration_num / 10), (pConf->lcd_timing.sync_duration_num % 10), ss_level, lcd_ss_level_table[ss_level]);

	switch(lcd_type){
		case LCD_DIGITAL_MIPI:
			set_pll_lcd(pConf);
			set_venc_lcd(pConf);
			set_tcon_lcd(pConf);
			init_dphy(pConf);
			break;
		case LCD_DIGITAL_LVDS:
			set_pll_lcd(pConf);
			set_venc_lcd(pConf);
			set_tcon_lcd(pConf);
			set_control_lvds(pConf);
			init_dphy(pConf);
			break;
		case LCD_DIGITAL_TTL:
			set_pll_lcd(pConf);
			set_venc_lcd(pConf);
			set_tcon_ttl(pConf);
			set_control_ttl(pConf);
			break;
		default:
			printf("Invalid LCD type.\n");
			break;
	}
	set_video_adjust(pConf);
	printf("%s finished.\n", __FUNCTION__);
}

static void _disable_lcd_driver(Lcd_Config_t *pConf)
{
    int vclk_sel;

    switch(pConf->lcd_basic.lcd_type){
        case LCD_DIGITAL_MIPI:
            mipi_dsi_off();
            break;
        case LCD_DIGITAL_LVDS:
        case LCD_DIGITAL_TTL:
        default:
            break;
    }

    vclk_sel = ((pConf->lcd_timing.clk_ctrl) >> CLK_CTRL_VCLK_SEL) & 0x1;

    WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 0, 11, 1);	//close lvds phy clk gate: 0x104c[11]
    WRITE_LCD_REG_BITS(LVDS_GEN_CNTL, 0, 3, 1);	//disable lvds fifo

    WRITE_LCD_REG(ENCL_VIDEO_EN, 0);	//disable encl
    WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL2, 0, 3, 1);	//disable vid_pll: 0x10e0[30]

    if (vclk_sel)
        WRITE_LCD_CBUS_REG_BITS(HHI_VIID_CLK_CNTL, 0, 0, 5);	//close vclk2 gate: 0x104b[4:0]
    else
        WRITE_LCD_CBUS_REG_BITS(HHI_VID_CLK_CNTL, 0, 0, 5);		//close vclk1 gate: 0x105f[4:0]

    WRITE_LCD_CBUS_REG_BITS(HHI_VIID_DIVIDER_CNTL, 0, 16, 1);	//close vid2_pll gate: 0x104c[16]

    WRITE_LCD_CBUS_REG_BITS(HHI_VID_PLL_CNTL, 0, 30, 1);	//disable vid_pll: 0x10e0[30]
    printf("disable lcd display driver.\n");
}

static inline void _enable_vsync_interrupt(void)
{
	if (READ_LCD_REG(ENCL_VIDEO_EN) & 1) {
		WRITE_LCD_REG(VENC_INTCTRL, 0x200);
	}
	else{
		WRITE_LCD_REG(VENC_INTCTRL, 0x2);
	}
}

static void _lcd_module_enable(void)
{
	int ret = 0;

	BUG_ON(pDev==NULL);
	_init_lcd_driver(pDev->pConf);
	lcd_power_ctrl(ON);
	//_enable_backlight();
	_enable_vsync_interrupt();
}

static int lcd_set_current_vmode(vmode_t mode)
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
	int i, j;

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

	propdata = fdt_getprop(dt_addr, nodeoffset, "vsync_horizontal_phase", NULL);
		if(propdata == NULL){
		printf("faild to get vsync_horizontal_phase\n");
		pConf->lcd_timing.vsync_h_phase =0;
	}
	else {
		pConf->lcd_timing.vsync_h_phase = ((((be32_to_cpup((u32*)propdata)) & 0x1) << 31) | (((be32_to_cpup((((u32*)propdata)+1)))&0xffff) << 0));
	}
	DBG_PRINT("vsync_h_phase = %u, sign = %u\n", (pConf->lcd_timing.vsync_h_phase&0xffff),(pConf->lcd_timing.vsync_h_phase>>31));

	if (LCD_DIGITAL_MIPI == pDev->pConf->lcd_basic.lcd_type) {
		DSI_Config_t *cfg = pDev->pConf->lcd_control.mipi_config;
		propdata = fdt_getprop(dt_addr, nodeoffset, "dsi_lane_num", NULL);
		if(propdata == NULL){
			printf("faild to get dsi_lane_num\n");
			cfg->lane_num = 4;
		} else {
			cfg->lane_num = (unsigned char)(be32_to_cpup((u32*)propdata));
		}
		DBG_PRINT("dsi_lane_num= %d\n",  cfg->lane_num);
		propdata = fdt_getprop(dt_addr, nodeoffset, "dsi_bit_rate_min_max", NULL);
		if(propdata == NULL){
			printf("faild to get dsi_bit_rate_min_max\n");
			cfg->bit_rate_min = 0;
			cfg->bit_rate_max = 0;
		}
		else {
			cfg->bit_rate_min = be32_to_cpup((u32*)propdata);
			cfg->bit_rate_max = be32_to_cpup((((u32*)propdata)+1));
		}
		DBG_PRINT("dsi bit_rate min = %dMHz, max = %dMHz\n", cfg->bit_rate_min, cfg->bit_rate_max);
		propdata = fdt_getprop(dt_addr, nodeoffset, "pclk_lanebyteclk_factor", NULL);
		if(propdata == NULL){
			printf("faild to get pclk_lanebyteclk_factor\n");
			cfg->factor_numerator = 0;
		}
		else {
			 cfg->factor_numerator= (be32_to_cpup((u32*)propdata));
		}
		cfg->factor_denominator = 10;
		DBG_PRINT("pclk_lanebyteclk factor = %d\n", cfg->factor_numerator);
		propdata = fdt_getprop(dt_addr, nodeoffset, "dsi_operation_mode", NULL);
		if(propdata == NULL){
			printf("faild to get dsi_operation_mode\n");
			 cfg->operation_mode = ((OPERATION_COMMAND_MODE << BIT_OPERATION_MODE_INIT) | (OPERATION_VIDEO_MODE << BIT_OPERATION_MODE_DISP));
		}
		else {
			cfg->operation_mode= (((be32_to_cpup((u32*)propdata)) << BIT_OPERATION_MODE_INIT) | ((be32_to_cpup((((u32*)propdata)+1))) << BIT_OPERATION_MODE_DISP));
		}
		DBG_PRINT("dsi_operation_mode init=%d, display=%d\n", ((cfg->operation_mode>>BIT_OPERATION_MODE_INIT) & 1), ((cfg->operation_mode>>BIT_OPERATION_MODE_DISP) & 1));
		propdata = fdt_getprop(dt_addr, nodeoffset, "dsi_transfer_ctrl", NULL);
		if(propdata == NULL){
			printf("faild to get dsi_transfer_ctrl\n");
			 cfg->transfer_ctrl = ((0 << BIT_TRANS_CTRL_CLK) | (0 << BIT_TRANS_CTRL_SWITCH));
		}
		else {
			cfg->transfer_ctrl= (((be32_to_cpup((u32*)propdata)) << BIT_TRANS_CTRL_CLK) | ((be32_to_cpup((((u32*)propdata)+1))) << BIT_TRANS_CTRL_SWITCH));
		}
		DBG_PRINT("dsi_transfer_ctrl clk=%d, switch=%d\n", ((cfg->transfer_ctrl>>BIT_TRANS_CTRL_CLK) & 1), ((cfg->transfer_ctrl>>BIT_TRANS_CTRL_SWITCH) & 1));
		//detect dsi init on table
		cfg->dsi_init_on = get_dsi_init_table(1);//dsi_init_on
		propdata = fdt_getprop(dt_addr, nodeoffset, "dsi_init_on", NULL);
		if(propdata == NULL){
			printf("faild to get dsi_init_on\n");
		}
		else {
			i = 0;
			while(i < DSI_INIT_ON_MAX) {
				if ((be32_to_cpup((((u32*)propdata)+i))) == 0xff) {
					if ((be32_to_cpup((((u32*)propdata)+i+1))) == 0xff) {
						i += 2;
						break;
					}
					i += 2;
				}
				else {
					i = i + 3 + (be32_to_cpup((((u32*)propdata)+i+2)));
				}
			}
			DBG_PRINT("dsi_init_on: ");
			for (j=0; j<i; j++) {
				cfg->dsi_init_on[j] = (be32_to_cpup((((u32*)propdata)+j)));
				DBG_PRINT("0x%02x ", cfg->dsi_init_on[j]);
			}
			DBG_PRINT("\n");
		}
		//detect dsi init off table
		cfg->dsi_init_off = get_dsi_init_table(0);//dsi_init_off
		propdata = fdt_getprop(dt_addr, nodeoffset, "dsi_init_off", NULL);
		if(propdata == NULL){
			printf("faild to get dsi_init_off\n");
		}
		else {
			i = 0;
			while(i < DSI_INIT_OFF_MAX) {
				if ((be32_to_cpup((((u32*)propdata)+i))) == 0xff) {
					if ((be32_to_cpup((((u32*)propdata)+i+1))) == 0xff) {
						i += 2;
						break;
					}
					i += 2;
				}
				else {
					i = i + 3 + (be32_to_cpup((((u32*)propdata)+i+2)));
				}
			}
			DBG_PRINT("dsi_init_off: ");
			for (j=0; j<i; j++) {
				cfg->dsi_init_off[j] = (be32_to_cpup((((u32*)propdata)+j)));
				DBG_PRINT("0x%02x ", cfg->dsi_init_off[j]);
			}
			DBG_PRINT("\n");
		}
		propdata = fdt_getprop(dt_addr, nodeoffset, "lcd_extern_init", NULL);
		if(propdata == NULL){
			printf("faild to get lcd_extern_init\n");
			cfg->lcd_extern_init = 0;
		}
		else {
			cfg->lcd_extern_init = (unsigned char)(be32_to_cpup((u32*)propdata));
		}
		DBG_PRINT("lcd_extern_init = %d\n", cfg->lcd_extern_init);
	}

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
		bl_conf->method = BL_CTL_PWM_NEGATIVE;
	}
	else {
		bl_conf->method = (be32_to_cpup((u32*)propdata) > BL_CTL_MAX) ? (BL_CTL_MAX-1) : (unsigned char)(be32_to_cpup((u32*)propdata));
	}
	DBG_PRINT("bl control_method: %s(%u)\n", bl_ctrl_method_table[bl_conf->method], bl_conf->method);

	propdata = fdt_getprop(dt_addr, nodeoffset, "bl_pwm_port_gpio_used", NULL);
	if(propdata == NULL){
		printf("faild to get bl_pwm_port_gpio_used\n");
		str = "PWM_C";
		bl_conf->pwm_port = BL_PWM_C;
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
			bl_conf->gpio = GPIODV_28;
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
		pwm_freq = 300000;
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
	len = 0;
	switch (bl_conf->method) {
		case BL_CTL_PWM_NEGATIVE:
		case BL_CTL_PWM_POSITIVE:
			nodeoffset = fdt_path_offset(dt_addr, "/pinmux/lcd_backlight");
			if(nodeoffset < 0)
				printf("backlight init: not find /pinmux/lcd_backlight node %s.\n",fdt_strerror(nodeoffset));
			else
				len = 1;
			break;
		case BL_CTL_PWM_COMBO:
			nodeoffset = fdt_path_offset(dt_addr, "/pinmux/lcd_backlight_combo");
			if(nodeoffset < 0)
				printf("backlight init: not find /pinmux/lcd_backlight_combo node %s.\n",fdt_strerror(nodeoffset));
			else
				len = 1;
			break;
		default:
			break;
	}
	if (len > 0) {
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
	dt_addr = 0x0f000000;
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
	print_lcd_info(pDev->pConf);
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
	if (pDev != NULL) {
		switch (num) {
			case 0:
				WRITE_LCD_REG(ENCL_VIDEO_MODE_ADV, 0x8);
				printf("disable bist pattern (1/2/3: show different test pattern)\n");
				break;
			case 1:
				WRITE_LCD_REG(ENCL_VIDEO_MODE_ADV, 0);
				WRITE_LCD_REG(ENCL_TST_MDSEL, 1);
				WRITE_LCD_REG(ENCL_TST_CLRBAR_STRT, pDev->pConf->lcd_timing.video_on_pixel);
				WRITE_LCD_REG(ENCL_TST_CLRBAR_WIDTH, (pDev->pConf->lcd_basic.h_active / 9));
				WRITE_LCD_REG(ENCL_TST_EN, 1);
				printf("show test pattern 1 (0: disable test pattern)\n");
				break;
			case 2:
				WRITE_LCD_REG(ENCL_VIDEO_MODE_ADV, 0);
				WRITE_LCD_REG(ENCL_TST_MDSEL, 2);
				WRITE_LCD_REG(ENCL_TST_EN, 1);
				printf("show test pattern 2 (0: disable test pattern)\n");
				break;
			case 3:
				WRITE_LCD_REG(ENCL_VIDEO_MODE_ADV, 0);
				WRITE_LCD_REG(ENCL_TST_MDSEL, 3);
				WRITE_LCD_REG(ENCL_TST_EN, 1);
				printf("show test pattern 3 (0: disable test pattern)\n");
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
	.enable       = _lcd_enable,
	.disable      = _lcd_disable,
	.bl_on        = _enable_backlight,
	.bl_off       = _disable_backlight,
	.set_bl_level = _set_backlight_level,
	.get_bl_level = _get_backlight_level,
	.power_on     = _lcd_power_on,
	.power_off    = _lcd_power_off,
	.test         = _lcd_test,
};
//****************************************
