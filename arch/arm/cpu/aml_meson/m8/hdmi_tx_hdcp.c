/*
 * Amlogic MX
 * frame buffer driver-----------HDMI_TX
 * Copyright (C) 2010 Amlogic, Inc.
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
 */

#include <common.h>
#include <asm/arch-m8/reg_addr.h>
#include <asm/arch-m8/io.h>
#include <asm/io.h>
#include "hdmi_tx_reg.h"

unsigned long hdmi_hdcp_rd_reg(unsigned long addr)
{
    __raw_writel(addr, P_HDMI_ADDR_PORT);
    __raw_writel(addr, P_HDMI_ADDR_PORT);

    return __raw_readl(P_HDMI_DATA_PORT);
}

void hdmi_hdcp_wr_reg(unsigned long addr, unsigned long data)
{
    __raw_writel(addr, P_HDMI_ADDR_PORT);
    __raw_writel(addr, P_HDMI_ADDR_PORT);
    __raw_writel(data, P_HDMI_DATA_PORT);
}

static void set_reg32_bits_op(uint32_t _reg, const uint32_t _val, const uint32_t _start, const uint32_t _len)
{
    unsigned int tmp;
    tmp = (__raw_readl(_reg) & ~(((1L<<(_len))-1)<<(_start))) | ((unsigned int)(_val) << (_start));
    __raw_writel(tmp, _reg);
}

#define TX_HDCP_KSV_OFFSET          0x540
#define TX_HDCP_KSV_SIZE            5
// Must be done by system init
// In kenrel hdmi driver, it will get AKSV value
// If equals to 0, then kernel won't enable HDCP
extern int hdmi_hdcp_clear_ksv_ram(void);
int hdmi_hdcp_clear_ksv_ram(void)
{
    int i;

    __raw_writel(0x100, P_HHI_HDMI_CLK_CNTL);       // enable hdmi system clock
    __raw_writel(0xffff00ff, P_HHI_MEM_PD_REG0);    // power hdmi ram register

    for(i = 0; i < TX_HDCP_KSV_SIZE; i++) {
        hdmi_hdcp_wr_reg(TX_HDCP_KSV_OFFSET + i, 0x00);
    }
    printf("clr h-ram\n");
    return 0;
}
