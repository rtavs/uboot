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
 * Author:  Tim Yao <timyao@amlogic.com>
 * Modify:  Evoke Zhang <evoke.zhang@amlogic.com>
 */

#ifndef LCDOUTC_H
#define LCDOUTC_H

#include <common.h>
#include <linux/list.h>
#include <amlogic/aml_lcd.h>
#include <asm/arch/aml_lcd_gpio.h>

/* for GAMMA_CNTL_PORT */
   /// GAMMA VCOM POL
   #define LCD_GAMMA_VCOM_POL       7
   /// GAMMA DATA REVERSE OUTPUT FOLLOWING VCOM
   #define LCD_GAMMA_RVS_OUT        6
   /// GAMMA ADDR PORT IS RDY
   #define LCD_ADR_RDY              5
   /// GAMMA DATA PORT IS RDY to Write
   #define LCD_WR_RDY               4
   /// GAMMA DATA PORT IS RDY to Read
   #define LCD_RD_RDY               3
   /// RGB10-->RGB8 using Trancate or Round off
   #define LCD_GAMMA_TR             2
   #define LCD_GAMMA_SET            1
   #define LCD_GAMMA_EN             0

/* for GAMMA_ADDR_PORT */
   /// Host Read/Write
   #define LCD_H_RD                 12
   /// Burst Mode
   #define LCD_H_AUTO_INC           11
   #define LCD_H_SEL_R              10
   #define LCD_H_SEL_G              9
   #define LCD_H_SEL_B              8
   /// 7:0
   #define LCD_HADR_MSB             7
   /// 7:0
   #define LCD_HADR                 0

/* for POL_CNTL_ADDR */
   #define LCD_DCLK_SEL             14    //FOR DCLK OUTPUT
   #define LCD_TCON_VSYNC_SEL_DVI   11	 // FOR RGB format DVI output
   #define LCD_TCON_HSYNC_SEL_DVI   10	 // FOR RGB format DVI output
   #define LCD_TCON_DE_SEL_DVI      9	 // FOR RGB format DVI output
   #define LCD_CPH3_POL             8
   #define LCD_CPH2_POL             7
   #define LCD_CPH1_POL             6
   #define LCD_TCON_DE_SEL          5
   #define LCD_TCON_VS_SEL          4
   #define LCD_TCON_HS_SEL          3
   #define LCD_DE_POL               2
   #define LCD_VS_POL               1
   #define LCD_HS_POL               0

/* for DITH_CNTL_ADDR */
   #define LCD_DITH10_EN            10
   #define LCD_DITH8_EN             9
   #define LCD_DITH_MD              8
   /// 7:4
   #define LCD_DITH10_CNTL_MSB      7
   /// 7:4
   #define LCD_DITH10_CNTL          4
   /// 3:0
   #define LCD_DITH8_CNTL_MSB       3
   /// 3:0
   #define LCD_DITH8_CNTL           0

/* for INV_CNT_ADDR */
   #define LCD_INV_EN               4
   #define LCD_INV_CNT_MSB          3
   #define LCD_INV_CNT              0

/* for TCON_MISC_SEL_ADDR */
   #define LCD_STH2_SEL             12
   #define LCD_STH1_SEL             11
   #define LCD_OEH_SEL              10
   #define LCD_VCOM_SEL             9
   #define LCD_DB_LINE_SW           8
   #define LCD_CPV2_SEL             7
   #define LCD_CPV1_SEL             6
   #define LCD_STV2_SEL             5
   #define LCD_STV1_SEL             4
   #define LCD_OEV_UNITE            3
   #define LCD_OEV3_SEL             2
   #define LCD_OEV2_SEL             1
   #define LCD_OEV1_SEL             0

/* for DUAL_PORT_CNTL_ADDR */
   #define LCD_ANALOG_SEL_CPH3      8
   #define LCD_ANALOG_3PHI_CLK_SEL  7
   #define LCD_LVDS_SEL54           6
   #define LCD_LVDS_SEL27           5
   #define LCD_TTL_SEL              4
   #define LCD_DUAL_PIXEL           3
   #define LCD_PORT_SWP             2
   #define LCD_RGB_SWP              1
   #define LCD_BIT_SWP              0

/* for LVDS_PACK_CNTL_ADDR */
   #define LCD_LD_CNT_MSB           7
   #define LCD_LD_CNT               5
   #define LCD_PN_SWP               4
   #define LCD_RES                  3
   #define LCD_LVDS_PORT_SWP        2
   #define LCD_PACK_RVS             1
   #define LCD_PACK_LITTLE          0

/* for video encoder */
	#define MIPI_DELAY				8
	#define LVDS_DELAY				8
	#define EDP_DELAY				8
	#define TTL_DELAY				19

//********************************************//
// for clk parameter auto generation
//********************************************//
//**** clk parameters bit ***/
	#define PLL_CTRL_LOCK			31
	#define PLL_CTRL_EN				30
	#define PLL_CTRL_RST			29
	#define PLL_CTRL_OD				16	//[17:16]
	#define PLL_CTRL_N				10	//[14:10]
	#define PLL_CTRL_M				0	//[8:0]

	#define DIV_CTRL_DIV_POST		12	//[14:12]
	#define DIV_CTRL_LVDS_CLK_EN	11
	#define DIV_CTRL_PHY_CLK_DIV2	10
	#define DIV_CTRL_POST_SEL		8	//[9:8]
	#define	DIV_CTRL_DIV_PRE		4	//[6:4]

	#define CLK_CTRL_AUTO			31
	#define CLK_TEST_FLAG			30
	#define CLK_CTRL_FRAC			16	//[27:16]
	#define CLK_CTRL_LEVEL			12	//[14:12]
	//#define CLK_CTRL_PLL_SEL		10
	//#define CLK_CTRL_DIV_SEL		9
	#define CLK_CTRL_VCLK_SEL		8
	#define CLK_CTRL_SS				4	//[7:4]
	#define CLK_CTRL_XD				0	//[3:0]

	#define PLL_WAIT_LOCK_CNT		200

/**** clk frequency limit ***/
	/* PLL */
	#define FIN_FREQ				(24 * 1000)
	#define PLL_M_MIN				2
	#define PLL_M_MAX				511
	#define PLL_N_MIN				1
	#define PLL_N_MAX				1

	#define PLL_FREF_MIN			(5 * 1000)
	#define PLL_FREF_MAX			(25 * 1000)
	#define PLL_VCO_MIN				(1200 * 1000)
	#define PLL_VCO_MAX				(3000 * 1000)
	/* MIPI-DSI PHY */
	#define MIPI_PHY_MAX_CLK_IN		(1000 * 1000)
	/* VID_DIV */
	#define DIV_PRE_MAX_CLK_IN		(1500 * 1000)
	#define DIV_POST_MAX_CLK_IN		(1000 * 1000)
	/* CRT_VIDEO */
	#define CRT_VID_MAX_CLK_IN		(1300 * 1000)
	/* ENCL */
	#define ENCL_MAX_CLK_IN			(333 * 1000)
	/* lcd interface video clk */
	#define MIPI_MAX_VID_CLK_IN		ENCL_MAX_CLK_IN
	#define LVDS_MAX_VID_CLK_IN		ENCL_MAX_CLK_IN
	#define TTL_MAX_VID_CLK_IN		ENCL_MAX_CLK_IN
	/* clk max error */
	#define MAX_ERROR				(2 * 1000)

#define CRT_VID_DIV_MAX				15
#define OD_SEL_MAX					3
#define DIV_PRE_SEL_MAX				6

static const unsigned od_table[4] = {1,2,4,8};
static const unsigned div_pre_table[6] = {1,2,3,4,5,6};
//********************************************//

/* for lcd power on/off config */
typedef enum
{
    LCD_POWER_TYPE_CPU = 0,
    LCD_POWER_TYPE_PMU,
	LCD_POWER_TYPE_SIGNAL,
	LCD_POWER_TYPE_INITIAL,
    LCD_POWER_TYPE_MAX,
} Lcd_Power_Type_t;
#define LCD_POWER_TYPE_NULL			LCD_POWER_TYPE_MAX

typedef enum
{
	LCD_POWER_PMU_GPIO0 = 0,
	LCD_POWER_PMU_GPIO1,
	LCD_POWER_PMU_GPIO2,
	LCD_POWER_PMU_GPIO3,
	LCD_POWER_PMU_GPIO4,
	LCD_POWER_PMU_GPIO_MAX,
} Lcd_Power_Pmu_Gpio_t;

#define	LCD_POWER_GPIO_OUTPUT_LOW	0
#define	LCD_POWER_GPIO_OUTPUT_HIGH	1
#define	LCD_POWER_GPIO_INPUT		2

static const char* lcd_power_type_table[]={
	"cpu",
	"pmu",
	"signal",
	"init",
	"null",
};

static const char* lcd_power_pmu_gpio_table[]={
	"GPIO0",
	"GPIO1",
	"GPIO2",
	"GPIO3",
	"GPIO4",
	"null",
};

typedef enum
{
	LCD_DIGITAL_MIPI = 0,
	LCD_DIGITAL_LVDS = 1,
	//LCD_DIGITAL_EDP = 2,
	LCD_DIGITAL_TTL = 3,
	//LCD_DIGITAL_MINILVDS = 4,
	LCD_TYPE_MAX,
} Lcd_Type_t;

static const char* lcd_type_table[]={
	"MIPI",
	"LVDS",
	"eDP",
	"TTL",
	"miniLVDS",
	"invalid",
};

static const char* lcd_type_table_match[]={
	"mipi",
	"lvds",
	"edp",
	"ttl",
	"minilvds",
	"invalid",
};

#define SS_LEVEL_MAX	5
static const char *lcd_ss_level_table[]={
	"0",
	"0.5%",
	"1%",
	"1.5%",
	"2%",
};

#define PANEL_MODEL_DEFAULT	"Panel_Default"
typedef struct {
	char *model_name;
    u16 h_active;   	// Horizontal display area
    u16 v_active;     	// Vertical display area
    u16 h_period;       // Horizontal total period time
    u16 v_period;       // Vertical total period time
    u32 screen_ratio_width;      // screen aspect ratio width
    u32 screen_ratio_height;     // screen aspect ratio height
    u32 h_active_area;/* screen physical width in "mm" unit */
    u32 v_active_area;/* screen physical height in "mm" unit */

    Lcd_Type_t lcd_type;
    u16 lcd_bits;         // 6 or 8 bits
	u16 lcd_bits_option;  //option=0, means the panel only support one lcd_bits option
}Lcd_Basic_t;

typedef struct {
    u32 pll_ctrl;        /* video PLL settings */
    u32 div_ctrl;        /* video pll div settings */
	u32 clk_ctrl;        /* video clock settings */  //[20]clk_auto, [19:16]ss_ctrl, [12]pll_sel, [8]div_sel, [4]vclk_sel, [3:0]xd
	u32 lcd_clk;		/* lcd clock*/
    u16 sync_duration_num;
    u16 sync_duration_den;

	u16 video_on_pixel;
    u16 video_on_line;

	u16 hsync_width;
	u16 hsync_bp;
	u16 vsync_width;
	u16 vsync_bp;
	u16 hvsync_valid;
	u16 de_hstart;
	u16 de_vstart;
	u16 de_valid;
	u32 vsync_h_phase; //[31]sign [15:0]value
	u32 h_offset;
	u32 v_offset;

    u16 sth1_hs_addr;
    u16 sth1_he_addr;
    u16 sth1_vs_addr;
    u16 sth1_ve_addr;

    u16 oeh_hs_addr;
    u16 oeh_he_addr;
    u16 oeh_vs_addr;
    u16 oeh_ve_addr;

    //u16 vcom_hswitch_addr;
    //u16 vcom_vs_addr;
    //u16 vcom_ve_addr;

    //u16 cpv1_hs_addr;
    //u16 cpv1_he_addr;
    //u16 cpv1_vs_addr;
    //u16 cpv1_ve_addr;

    u16 stv1_hs_addr;
    u16 stv1_he_addr;
    u16 stv1_vs_addr;
    u16 stv1_ve_addr;

    //u16 oev1_hs_addr;
    //u16 oev1_he_addr;
    //u16 oev1_vs_addr;
    //u16 oev1_ve_addr;

    u16 pol_cntl_addr;
    u16 inv_cnt_addr;
    u16 tcon_misc_sel_addr;
} Lcd_Timing_t;

// Fine Effect Tune
typedef struct {
    u16 gamma_cntl_port;
    u16 gamma_vcom_hswitch_addr;

    u16 rgb_base_addr;
    u16 rgb_coeff_addr;
	u16 dith_user;
    u16 dith_cntl_addr;

	u32 vadj_brightness;
	u32 vadj_contrast;
	u32 vadj_saturation;

	unsigned char gamma_revert;
	u16 gamma_r_coeff;
	u16 gamma_g_coeff;
	u16 gamma_b_coeff;
    u16 GammaTableR[256];
    u16 GammaTableG[256];
    u16 GammaTableB[256];
} Lcd_Effect_t;

//mipi dsi config
#define DSI_INIT_ON_MAX              100
#define DSI_INIT_OFF_MAX             30

#define BIT_OPERATION_MODE_INIT      0
#define BIT_OPERATION_MODE_DISP      4
#define BIT_TRANS_CTRL_CLK           0
#define BIT_TRANS_CTRL_SWITCH        4 //[5:4]
typedef struct DSI_Config_s{
    unsigned char lane_num;
    unsigned int bit_rate_max;
    unsigned int bit_rate_min;
    unsigned int bit_rate;
    unsigned int factor_denominator;
    unsigned int factor_numerator;
    unsigned int hline;
    unsigned int hsa;
    unsigned int hbp;
    unsigned int vsa;
    unsigned int vbp;
    unsigned int vfp;
    unsigned int vact;

    unsigned int venc_data_width;
    unsigned int dpi_data_format;
    unsigned int venc_fmt;
    unsigned int operation_mode;  //mipi-dsi operation mode: video, command. [4]display , [0]init
    unsigned int transfer_ctrl;  //[0]LP mode auto stop clk lane, [5:4]phy switch between LP and HS
    unsigned char video_mode_type;  //burst, non-burst(sync pulse, sync event)

    unsigned char *dsi_init_on;
    unsigned char *dsi_init_off;
    unsigned char lcd_extern_init;
}DSI_Config_t;

typedef struct {
	unsigned char link_user;
	unsigned char lane_count;
	unsigned char link_rate;
	unsigned char link_adaptive;
	unsigned char vswing;
	unsigned char preemphasis;
	unsigned int bit_rate;
} EDP_Config_t;

typedef struct {
	unsigned lvds_vswing;
	unsigned lvds_repack_user;
	unsigned lvds_repack;
	unsigned pn_swap;
} LVDS_Config_t;

typedef struct {
	unsigned char rb_swap;
	unsigned char bit_swap;
} TTL_Config_t;

typedef struct {
	DSI_Config_t *mipi_config;
	LVDS_Config_t *lvds_config;
	EDP_Config_t *edp_config;
	TTL_Config_t *ttl_config;
} Lcd_Control_Config_t;

typedef enum {
    OFF = 0,
    ON = 1,
} Bool_t;

// Power Control
typedef struct {
	unsigned char type;
	int gpio;
	unsigned short value;
	unsigned short delay;
} Lcd_Power_Config_t;

#define LCD_POWER_CTRL_STEP_MAX		15
typedef struct {
	Lcd_Power_Config_t power_on_uboot;
	Lcd_Power_Config_t power_off_uboot;
	Lcd_Power_Config_t power_on_config[LCD_POWER_CTRL_STEP_MAX];
	Lcd_Power_Config_t power_off_config[LCD_POWER_CTRL_STEP_MAX];
	int power_on_step;
	int power_off_step;
} Lcd_Power_Ctrl_t;

typedef struct {
    Lcd_Basic_t lcd_basic;
    Lcd_Timing_t lcd_timing;
    Lcd_Effect_t lcd_effect;
	Lcd_Control_Config_t lcd_control;
	Lcd_Power_Ctrl_t lcd_power_ctrl;
} Lcd_Config_t;

Lcd_Config_t lcd_config_dft;

//****************************************//
// backlight control
//****************************************//
#define BL_LEVEL_MAX_DFT   			255
#define BL_LEVEL_MIN_DFT   			10
#define BL_LEVEL_OFF				1

#define BL_LEVEL_MID_DFT    		128
#define BL_LEVEL_MID_MAPPED_DFT		102

#define BL_LEVEL_DFT				128

typedef enum {
	BL_CTL_GPIO = 0,
	BL_CTL_PWM_NEGATIVE = 1,
	BL_CTL_PWM_POSITIVE = 2,
	BL_CTL_PWM_COMBO = 3,
	BL_CTL_EXTERN = 4,
	BL_CTL_MAX = 5,
} BL_Ctrl_Method_t;

static const char* bl_ctrl_method_table[]={
	"gpio",
	"pwm_negative",
	"pwm_positive",
	"pwm_combo",
	"extern",
	"null",
};

typedef enum {
	BL_PWM_A = 0,
	BL_PWM_B,
	BL_PWM_C,
	BL_PWM_D,
	BL_PWM_MAX,
} BL_PWM_t;

typedef struct {
	unsigned level_default;
	unsigned level_mid;
	unsigned level_mid_mapping;
	unsigned level_min;
	unsigned level_max;
	unsigned short power_on_delay;
	unsigned char method;
	int gpio;
	unsigned dim_max;
	unsigned dim_min;
	unsigned char pwm_port;
	unsigned char pwm_gpio_used;
	unsigned pwm_cnt;
	unsigned pwm_pre_div;
	unsigned pwm_max;
	unsigned pwm_min;

	unsigned combo_level_switch;
	unsigned char combo_high_port;
	unsigned char combo_high_method;
	unsigned char combo_low_port;
	unsigned char combo_low_method;
	unsigned combo_high_cnt;
	unsigned combo_high_pre_div;
	unsigned combo_high_duty_max;
	unsigned combo_high_duty_min;
	unsigned combo_low_cnt;
	unsigned combo_low_pre_div;
	unsigned combo_low_duty_max;
	unsigned combo_low_duty_min;

	unsigned pinmux_set_num;
	unsigned pinmux_set[5][2];
	unsigned pinmux_clr_num;
	unsigned pinmux_clr[5][2];
} Lcd_Bl_Config_t;

Lcd_Bl_Config_t bl_config_dft;
//*************************************//

extern void mdelay(unsigned long msec);

#endif /* LCDOUTC_H */
