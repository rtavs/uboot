#ifndef _MESON_IOBUS_M8B_H
#define _MESON_IOBUS_M8B_H

#define IO_CBUS_BASE			0xc1100000
#define IO_AXI_BUS_BASE			0xc1300000
#define IO_AHB_BUS_BASE			0xc9000000
#define IO_APB_BUS_BASE			0xc8000000
#define IO_APB_HDMI_BUS_BASE		0xd0040000
#define IO_VPU_BUS_BASE			0xd0100000

#define MESON_PERIPHS1_VIRT_BASE	0xc1108400
#define MESON_PERIPHS1_PHYS_BASE	0xc1108400

#define MESON_PERIPHS1_VIRT_BASE	0xc1108400
#define MESON_PERIPHS1_PHYS_BASE	0xc1108400

#define CBUS_REG_OFFSET(reg)		((reg) << 2)
#define CBUS_REG_ADDR(reg)		(IO_CBUS_BASE + CBUS_REG_OFFSET(reg))

#define AXI_REG_OFFSET(reg)		((reg) << 2)
#define AXI_REG_ADDR(reg)		(IO_AXI_BUS_BASE + AXI_REG_OFFSET(reg))

#define AHB_REG_OFFSET(reg)		((reg) << 2)
#define AHB_REG_ADDR(reg)		(IO_AHB_BUS_BASE + AHB_REG_OFFSET(reg))

#define VPU_REG_OFFSET(reg)		((reg) << 2)
#define VPU_REG_ADDR(reg)		(IO_VPU_BUS_BASE + VPU_REG_OFFSET(reg))

#define APB_REG_OFFSET(reg)		(reg)
#define APB_REG_ADDR(reg)		(IO_APB_BUS_BASE + APB_REG_OFFSET(reg))
#define APB_REG_ADDR_VALID(reg)		(((unsigned long)(reg) & 3) == 0)

#define APB_HDMI_REG_OFFSET(reg)	(reg)
#define APB_HDMI_REG_ADDR(reg)		(IO_APB_HDMI_BUS_BASE + APB_HDMI_REG_OFFSET(reg))
#define APB_HDMI_REG_ADDR_VALID(reg)	(((unsigned long)(reg) & 3) == 0)

#endif /* _MESON_IOBUS_M8B_H */
