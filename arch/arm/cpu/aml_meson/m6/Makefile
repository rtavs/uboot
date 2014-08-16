#
# (C) Copyright 2000-2011
# Jerry Yu , Amlogic  Engineering, jerry.yu@amlogic.com.
#
# (C) Copyright 2011 Amlogic Semiconductor, Inc.
#
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(SOC).o

COBJS-$(CONFIG_AML_SDIO)+=sdio.o
COBJS	= clock.o gpio.o pinmux.o $(COBJS-y)
SOBJS := mmutable.o

COBJS-$(CONFIG_AML_ETHERNET) += aml_eth_clk_set.o aml_eth_pinmux.o

COBJS-$(CONFIG_AML_RTC)+=aml_rtc_setting.o

COBJS-$(CONFIG_USB_DWC_OTG_HCD) += usb.o

COBJS-$(CONFIG_VIDEO_AMLLCD) += aml_lcd_gpio.o
COBJS-$(CONFIG_VIDEO_AMLLCD) += lcdoutc.o
COBJS += hdmi_tx_hdcp.o
COBJS-$(CONFIG_AML_HDMI_TX) += hdmi_tx_set.o enc_clk_config.o


SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(SOBJS) $(COBJS))

all:	$(obj).depend $(LIB)

$(LIB):	$(OBJS)
	$(call cmd_link_o_target, $(OBJS))


#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
