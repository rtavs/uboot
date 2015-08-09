#include <common.h>
#include <asm/io.h>
#include <asm/arch/regaddr.h>
#include <asm/arch/clock.h>
#include <asm/arch/uart.h>
#include <serial.h>

DECLARE_GLOBAL_DATA_PTR;

static void serial_putc_port(unsigned port_base, const char c)
{
	if (c == '\n')
		serial_putc_port(port_base, '\r');

	while ((readl(P_UART_STATUS(port_base)) & UART_STAT_MASK_TFIFO_FULL))
		;

	writel(c, P_UART_WFIFO(port_base));

	while(!(readl(P_UART_STATUS(port_base)) & UART_STAT_MASK_TFIFO_EMPTY))
		;
}

static void serial_setbrg_port(unsigned port_base)
{
	unsigned long baud_para;
	int clk81 = clk_get_rate(UART_CLK_SRC);

	if(clk81 < 0)
	    return;

	baud_para = clk81 / (gd->baudrate * 4) - 1;
	baud_para &= UART_CNTL_MASK_BAUD_RATE;

	writel((readl(P_UART_CONTROL(port_base)) & ~UART_CNTL_MASK_BAUD_RATE) | baud_para, P_UART_CONTROL(port_base));
}

static void serial_set_stop_port(unsigned port_base, int stop_bits)
{
	unsigned long uart_config;

	uart_config = readl(P_UART_CONTROL(port_base)) & ~UART_CNTL_MASK_STP_BITS;

	switch(stop_bits)
	{
	    case 2:
	        uart_config |= UART_CNTL_MASK_STP_2BIT;
	        break;
	    case 1:
	    default:
	        uart_config |= UART_CNTL_MASK_STP_1BIT;
	        break;
	}

	writel(uart_config, P_UART_CONTROL(port_base));
}

static void serial_set_parity_port(unsigned port_base, int type)
{
	unsigned long uart_config;

	uart_config = readl(P_UART_CONTROL(port_base)) & ~(UART_CNTL_MASK_PRTY_TYPE | UART_CNTL_MASK_PRTY_EN);
	writel(uart_config, P_UART_CONTROL(port_base));
}

static void serial_set_dlen_port(unsigned port_base, int data_len)
{
	unsigned long uart_config;

	uart_config = readl(P_UART_CONTROL(port_base)) & ~UART_CNTL_MASK_CHAR_LEN;

	switch(data_len)
	{
	    case 5:
	        uart_config |= UART_CNTL_MASK_CHAR_5BIT;
	        break;
	    case 6:
	        uart_config |= UART_CNTL_MASK_CHAR_6BIT;
	        break;
	    case 7:
	        uart_config |= UART_CNTL_MASK_CHAR_7BIT;
	        break;
	    case 8:
	    default:
	        uart_config |= UART_CNTL_MASK_CHAR_8BIT;
	        break;
	}

	writel(uart_config, P_UART_CONTROL(port_base));
}

static void serial_reset_port(unsigned port_base)
{
	writel(readl(P_UART_CONTROL(port_base)) | UART_CNTL_MASK_RST_TX | UART_CNTL_MASK_RST_RX | UART_CNTL_MASK_CLR_ERR, P_UART_CONTROL(port_base));
	writel(readl(P_UART_CONTROL(port_base)) & ~(UART_CNTL_MASK_RST_TX | UART_CNTL_MASK_RST_RX | UART_CNTL_MASK_CLR_ERR), P_UART_CONTROL(port_base));
}

static int serial_init_port(unsigned port_base)
{
	int ret;

	writel(0, P_UART_CONTROL(port_base));
	if (ret < 0)
		return -1;

	serial_setbrg_port(port_base);

#ifndef CONFIG_SERIAL_STP_BITS
#define CONFIG_SERIAL_STP_BITS 1
#endif
	serial_set_stop_port(port_base, CONFIG_SERIAL_STP_BITS);

#ifndef CONFIG_SERIAL_PRTY_TYPE
#define CONFIG_SERIAL_PRTY_TYPE 0
#endif
	serial_set_parity_port(port_base, CONFIG_SERIAL_PRTY_TYPE);

#ifndef CONFIG_SERIAL_CHAR_LEN
#define CONFIG_SERIAL_CHAR_LEN 8
#endif
	serial_set_dlen_port(port_base, CONFIG_SERIAL_CHAR_LEN);

	writel(readl(P_UART_CONTROL(port_base)) | UART_CNTL_MASK_TX_EN | UART_CNTL_MASK_RX_EN, P_UART_CONTROL(port_base));
	while (!(readl(P_UART_STATUS(port_base)) & UART_STAT_MASK_TFIFO_EMPTY))
		;

	serial_reset_port(port_base);
	serial_putc_port(port_base,'\n');

	return 0;
}

static int serial_tstc_port(unsigned port_base)
{
	return (readl(P_UART_STATUS(port_base)) & UART_STAT_MASK_RFIFO_CNT);
}

static int serial_getc_port(unsigned port_base)
{
	unsigned char ch;

	while((readl(P_UART_STATUS(port_base)) & UART_STAT_MASK_RFIFO_CNT)==0)
		;
	ch = readl(P_UART_RFIFO(port_base)) & 0x00ff;

	if (readl(P_UART_STATUS(port_base)) & (UART_STAT_MASK_PRTY_ERR | UART_STAT_MASK_FRAM_ERR)) {
		writel(readl(P_UART_CONTROL(port_base)) |UART_CNTL_MASK_CLR_ERR, P_UART_CONTROL(port_base));
		writel(readl(P_UART_CONTROL(port_base)) & (~UART_CNTL_MASK_CLR_ERR), P_UART_CONTROL(port_base));
	}

	return ((int) ch);
}

static void serial_puts_port(unsigned port_base, const char *s)
{
	while (*s) {
		serial_putc_port(port_base, *s++);
	}
}

#define DECL_MESON_UART(n, port)		\
						\
static int uart_##n##_init(void)		\
{						\
	serial_init_port(port); 		\
	return 0;				\
}						\
						\
static void uart_##n##_setbrg(void)		\
{						\
	serial_setbrg_port(port);		\
}						\
						\
static int uart_##n##_getc(void)		\
{						\
	return serial_getc_port(port);		\
}						\
						\
static int uart_##n##_tstc(void)		\
{						\
	return serial_tstc_port(port);		\
}						\
						\
static void uart_##n##_putc(const char c)	\
{						\
	serial_putc_port(port, c);		\
}						\
						\
static void uart_##n##_puts(const char *s)	\
{						\
	serial_puts_port(port, s);		\
}						\
						\
struct serial_device serial_device_##n = {	\
	.name   = "meson_uart"#n,		\
	.start  = uart_##n##_init,		\
	.setbrg = uart_##n##_setbrg,		\
	.getc   = uart_##n##_getc,		\
	.tstc   = uart_##n##_tstc,		\
	.putc   = uart_##n##_putc,		\
	.puts   = uart_##n##_puts,		\
};

DECL_MESON_UART(0, UART_PORT_0)
DECL_MESON_UART(1, UART_PORT_1)
#ifdef UART_PORT_AO
DECL_MESON_UART(ao, UART_PORT_AO)
#endif

__weak struct serial_device *default_serial_console(void)
{
#if CONFIG_CONS_INDEX == 0
	return &serial_device_0;
#elif CONFIG_CONS_INDEX == 1
	return &serial_device_1;
#elif CONFIG_CONS_INDEX == 2
	return &serial_device_ao;
#endif
}

void meson_serial_initialize(void)
{
	serial_register(&serial_device_0);
	serial_register(&serial_device_1);
#ifdef UART_PORT_AO
	serial_register(&serial_device_ao);
#endif
}
