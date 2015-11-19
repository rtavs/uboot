
#include <common.h>
#include <div64.h>
#include <asm/io.h>
#include <serial.h>

DECLARE_GLOBAL_DATA_PTR;


struct uart_meson {
    u32 wfifo;      //
    u32 rfifo;      //
    u32 ctrl;       //
    u32 stat;       //
    u32 misc;       //
    u32 reg5;       //
};


/* MESON_UART_CONTROL bits */
#define UART_CTRL_TX_EN			BIT(12)
#define UART_CTRL_RX_EN			BIT(13)
#define UART_CTRL_2WIRE_EN		BIT(15)

#define UART_CTRL_STOP_BIN_LEN_MASK	(0x03 << 16)
#define UART_CTRL_STOP_BIN_1SB		(0x00 << 16)
#define UART_CTRL_STOP_BIN_2SB		(0x01 << 16)

#define UART_CTRL_PARITY_TYPE		BIT(18)
#define UART_CTRL_PARITY_EN		BIT(19)

#define UART_CTRL_TX_RST			BIT(22)
#define UART_CTRL_RX_RST			BIT(23)
#define UART_CTRL_CLR_ERR          BIT(24)
#define UART_CTRL_RX_INT_EN		BIT(27)
#define UART_CTRL_TX_INT_EN		BIT(28)
#define UART_CTRL_DATA_LEN_MASK    (0x03 << 20)
#define UART_CTRL_DATA_LEN_8BIT	(0x00 << 20)
#define UART_CTRL_DATA_LEN_7BIT    (0x01 << 20)
#define UART_CTRL_DATA_LEN_6BIT    (0x02 << 20)
#define UART_CTRL_DATA_LEN_5BIT    (0x03 << 20)







/* MESON_UART_STATUS bits */
#define UART_STAT_PARITY_ERR		BIT(16)
#define UART_STAT_FRAME_ERR		BIT(17)
#define UART_STAT_TX_FIFO_WERR		BIT(18)
#define UART_STAT_RX_EMPTY		    BIT(20)
#define UART_STAT_TX_FULL		    BIT(21)
#define UART_STAT_TX_EMPTY		    BIT(22)
#define UART_STAT_ERR			    (UART_STAT_PARITY_ERR | \
					                 UART_STAT_FRAME_ERR  | \
					                 UART_STAT_TX_FIFO_WERR)


#define UART_STAT_RFIFO_CNT    (0x7f<<0)
#define UART_STAT_TFIFO_CNT    (0x7f<<8)


/* MESON_UART_MISC bits */
#define MESON_UART_XMIT_IRQ(c)		(((c) & 0xff) << 8)
#define MESON_UART_RECV_IRQ(c)		((c) & 0xff)

/* MESON_UART_REG5 bits */
#define MESON_UART_BAUD_MASK		0x7fffff
#define MESON_UART_BAUD_USE		    BIT(23)




//
static struct uart_meson *uart_ports[6] = {
//	[0] = (struct uart_meson *)MESON_SERIAL_BASEADDR0,
//	[1] = (struct uart_meson *)MESON_SERIAL_BASEADDR1,
};


static int meson_uart_dev_init(const int port)
{

    return 0;
}

static int meson_uart_dev_stop(const int port)
{
    return 0;
}

static void meson_uart_dev_setbrg(const int port)
{
#if 0
	struct uart_meson *regs = uart_ports[port];

	u32 baudrate = gd->baudrate;

	u32 uclk = get_uart_clk(dev_index);

	u32 val;

	while (!(readl(&regs->stat) & UART_STAT_TX_EMPTY));
#endif
}

static int meson_uart_dev_getc(const int port)
{
	struct uart_meson *regs = uart_ports[port];

    // use UART_STAT_RX_EMPTY ?
    //while((readl(&regs->stat) & UART_STAT_RX_EMPTY));

    while((readl(&regs->stat) & UART_STAT_RFIFO_CNT) == 0);

    if (readl(&regs->stat) & UART_STAT_ERR) {
        // clear error, it doesn't clear to 0 automatically
        writel(readl(&regs->ctrl) | UART_CTRL_CLR_ERR, &regs->ctrl);
        writel(readl(&regs->ctrl) & (~UART_CTRL_CLR_ERR), &regs->ctrl);
    }
    return (int)(readl(&regs->rfifo) & 0xFF);
}


static int uart_meson_dev_tstc(const int port)
{
	struct uart_meson *regs = uart_ports[port];
    return (readl(&regs->stat) & UART_STAT_RFIFO_CNT);
}

static void meson_uart_dev_putc(const char c, const int port)
{
	struct uart_meson *regs = uart_ports[port];

    // wait TX FIFO not full
    while (readl(&regs->stat) & UART_STAT_TX_FULL);
    writel(c,&regs->wfifo);

	if (c == '\n')
		meson_uart_dev_putc(port,'\r');
}

static void meson_uart_dev_puts(const char *s, const int port)
{
    while (*s) {
        meson_uart_dev_putc(port,*s++);
    }
}




/* Multi serial device functions */
#define DECLARE_MESON_SERIAL_FUNCTIONS(port) \
static int uart##port##_init(void) { return meson_uart_dev_init(port); } \
static void uart##port##_setbrg(void) { meson_uart_dev_stop(port); } \
static int uart##port##_getc(void) { return meson_uart_dev_getc(port); } \
static int uart##port##_tstc(void) { return uart_meson_dev_tstc(port); } \
static void uart##port##_putc(const char c) { meson_uart_dev_putc(c, port); } \
static void uart##port##_puts(const char *s) { meson_uart_dev_puts(s, port); }


#define INIT_MESON_SERIAL_STRUCTURE(port, __name) {	\
	.name	= __name,				\
	.start	= uart##port##_init,		\
	.stop	= NULL,					\
	.setbrg	= uart##port##_setbrg,		\
	.getc	= uart##port##_getc,		\
	.tstc	= uart##port##_tstc,		\
	.putc	= uart##port##_putc,		\
	.puts	= uart##port##_puts,		\
}


DECLARE_MESON_SERIAL_FUNCTIONS(0);
struct serial_device meson_serial0_device =
	INIT_MESON_SERIAL_STRUCTURE(0, "meson_serial_0");

DECLARE_MESON_SERIAL_FUNCTIONS(1);
struct serial_device meson_serial1_device =
	INIT_MESON_SERIAL_STRUCTURE(1, "meson_serial_1");

DECLARE_MESON_SERIAL_FUNCTIONS(2);
struct serial_device meson_serial2_device =
	INIT_MESON_SERIAL_STRUCTURE(2, "meson_serial_2");

DECLARE_MESON_SERIAL_FUNCTIONS(3);
struct serial_device meson_serial3_device =
	INIT_MESON_SERIAL_STRUCTURE(3, "meson_serial_3");


void meson_serial_initialize(void)
{
	serial_register(&meson_serial0_device);
	serial_register(&meson_serial1_device);
	serial_register(&meson_serial2_device);
	serial_register(&meson_serial3_device);
}

__weak struct serial_device *default_serial_console(void)
{
#if defined(CONFIG_SERIAL0)
	return &meson_serial0_device;
#elif defined(CONFIG_SERIAL1)
	return &meson_serial1_device;
#elif defined(CONFIG_SERIAL2)
	return &meson_serial2_device;
#elif defined(CONFIG_SERIAL3)
	return &meson_serial3_device;
#else
#error "CONFIG_SERIAL? missing."
#endif
}
