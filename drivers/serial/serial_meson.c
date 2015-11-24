
#include <common.h>
#include <asm/io.h>
#include <serial.h>
#include <asm/arch/clock.h>

DECLARE_GLOBAL_DATA_PTR;


struct uart_meson {
    u32 wfifo;      //
    u32 rfifo;      //
    u32 cntl;       //
    u32 stat;       //
    u32 misc;       //
    u32 reg5;       //
};


/* MESON_UART_CONTROL bits */
#define UART_CNTL_MASK_BAUD_RATE                (0xfff)
#define UART_CNTL_MASK_TX_EN                    (1<<12)
#define UART_CNTL_MASK_RX_EN                    (1<<13)
#define UART_CNTL_MASK_2WIRE                    (1<<15)
#define UART_CNTL_MASK_STP_BITS                 (3<<16)
#define UART_CNTL_MASK_STP_1BIT                 (0<<16)
#define UART_CNTL_MASK_STP_2BIT                 (1<<16)
#define UART_CNTL_MASK_PRTY_EVEN                (0<<18)
#define UART_CNTL_MASK_PRTY_ODD                 (1<<18)
#define UART_CNTL_MASK_PRTY_TYPE                (1<<18)
#define UART_CNTL_MASK_PRTY_EN                  (1<<19)
#define UART_CNTL_MASK_CHAR_LEN                 (3<<20)
#define UART_CNTL_MASK_CHAR_8BIT                (0<<20)
#define UART_CNTL_MASK_CHAR_7BIT                (1<<20)
#define UART_CNTL_MASK_CHAR_6BIT                (2<<20)
#define UART_CNTL_MASK_CHAR_5BIT                (3<<20)
#define UART_CNTL_MASK_RST_TX                   (1<<22)
#define UART_CNTL_MASK_RST_RX                   (1<<23)
#define UART_CNTL_MASK_CLR_ERR                  (1<<24)
#define UART_CNTL_MASK_INV_RX                   (1<<25)
#define UART_CNTL_MASK_INV_TX                   (1<<26)
#define UART_CNTL_MASK_RINT_EN                  (1<<27)
#define UART_CNTL_MASK_TINT_EN                  (1<<28)
#define UART_CNTL_MASK_INV_CTS                  (1<<29)
#define UART_CNTL_MASK_MASK_ERR                 (1<<30)
#define UART_CNTL_MASK_INV_RTS                  (1<<31)



/* MESON_UART_STATUS bits */
#define UART_STAT_MASK_RFIFO_CNT                (0x7f<<0)
#define UART_STAT_MASK_TFIFO_CNT                (0x7f<<8)
#define UART_STAT_MASK_PRTY_ERR                 (1<<16)
#define UART_STAT_MASK_FRAM_ERR                 (1<<17)
#define UART_STAT_MASK_WFULL_ERR                (1<<18)
#define UART_STAT_MASK_RFIFO_FULL               (1<<19)
#define UART_STAT_MASK_RFIFO_EMPTY              (1<<20)
#define UART_STAT_MASK_TFIFO_FULL               (1<<21)
#define UART_STAT_MASK_TFIFO_EMPTY              (1<<22)
#define UART_STAT_MASK_XMIT_BUSY                (1<<25)
#define UART_STAT_MASK_RECV_BUSY                (1<<26)


#define UART_STAT_ERR                      (UART_STAT_MASK_PRTY_ERR | \
                                            UART_STAT_MASK_FRAM_ERR  | \
                                            UART_STAT_MASK_WFULL_ERR)



#define UART_STAT_MASK_RFIFO_CNT    (0x7f<<0)
#define UART_STAT_MASK_TFIFO_CNT    (0x7f<<8)


/* MESON_UART_MISC bits */
#define MESON_UART_XMIT_IRQ(c)		(((c) & 0xff) << 8)
#define MESON_UART_RECV_IRQ(c)		((c) & 0xff)

/* MESON_UART_REG5 bits */
#define MESON_UART_BAUD_MASK		0x7fffff
#define MESON_UART_BAUD_USE		    BIT(23)



#define P_AO_UART_WFIFO 		AOBUS_REG_ADDR(AO_UART_WFIFO)



#define MESON_UART_AO_BASE  0xc81004c0
//
static struct uart_meson *uart_ports[6] = {
//	[0] = (struct uart_meson *)MESON_SERIAL_BASEADDR0,
//	[1] = (struct uart_meson *)MESON_SERIAL_BASEADDR1,
	[2] = (struct uart_meson *)MESON_UART_AO_BASE,
};




static void meson_uart_dev_setbrg(const int port)
{
    struct uart_meson *regs = uart_ports[port];

    u32 clk81 = meson_get_clk_rate(CLK81);
    if (clk81 < 0)
        return;

    u32 baud = clk81/(gd->baudrate*4) - 1;
    baud &= UART_CNTL_MASK_BAUD_RATE;

    u32 val = (readl(&regs->cntl) & ~UART_CNTL_MASK_BAUD_RATE) | baud;
    /* write to the register */
    writel(val, &regs->cntl);
}

static void meson_uart_set_stopbit(unsigned port)
{
    unsigned long uart_config;
    struct uart_meson *regs = uart_ports[port];
    int stop_bits = 1;

#ifdef CONFIG_SERIAL_STP_BITS
    stop_bits = CONFIG_SERIAL_STP_BITS;
#endif

    uart_config = readl(&regs->cntl) & ~UART_CNTL_MASK_STP_BITS;
    /* stop bits */
    switch(stop_bits) {
        case 2:
            uart_config |= UART_CNTL_MASK_STP_2BIT;
            break;
        case 1:
        default:
            uart_config |= UART_CNTL_MASK_STP_1BIT;
            break;
    }

    /* write to the register */
    writel(uart_config, &regs->cntl);
}


static void meson_uart_set_parity(unsigned port)
{
    unsigned long uart_config;
    struct uart_meson *regs = uart_ports[port];
    uart_config = readl(&regs->cntl) & ~(UART_CNTL_MASK_PRTY_TYPE | UART_CNTL_MASK_PRTY_EN);
    writel(uart_config, &regs->cntl);
}


static void meson_uart_set_dlen(unsigned port)
{
    int data_len = 8;
    unsigned long uart_config;
	struct uart_meson *regs = uart_ports[port];

    uart_config = readl(&regs->cntl) & ~UART_CNTL_MASK_CHAR_LEN;
    /* data bits */
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

    /* write to the register */
    writel(uart_config, &regs->cntl);
}


/*
 * reset the uart state machine
 */
static void meson_uart_reset_port(unsigned port)
{
    struct uart_meson *regs = uart_ports[port];
    /* write to the register */
    writel(readl(&regs->cntl) | UART_CNTL_MASK_RST_TX | UART_CNTL_MASK_RST_RX | UART_CNTL_MASK_CLR_ERR, &regs->cntl);
    writel(readl(&regs->cntl) & ~(UART_CNTL_MASK_RST_TX | UART_CNTL_MASK_RST_RX | UART_CNTL_MASK_CLR_ERR), &regs->cntl);
}


static int meson_uart_dev_init(const int port)
{

	struct uart_meson *regs = uart_ports[port];

    writel(0, &regs->cntl);

    /* init baudrate/stopbit/parity/*/
    meson_uart_dev_setbrg(port);
    meson_uart_set_stopbit(port);
    meson_uart_set_parity(port);
    meson_uart_set_dlen(port);

    /* enable TX RX */
    writel(readl(&regs->cntl) | UART_CNTL_MASK_TX_EN | UART_CNTL_MASK_RX_EN, &regs->cntl);

    /* clear TX FIFO */
    while (!(readl(&regs->stat) & UART_STAT_MASK_TFIFO_EMPTY));

    meson_uart_reset_port(port);
//       serial_putc_port(port_base,'\n');

    return 0;
}

static int meson_uart_dev_stop(const int port)
{
    return 0;
}


static int meson_uart_dev_getc(const int port)
{
	struct uart_meson *regs = uart_ports[port];

    // use UART_STAT_MASK_RFIFO_EMPTY ?
    //while((readl(&regs->stat) & UART_STAT_MASK_RFIFO_EMPTY));

    while((readl(&regs->stat) & UART_STAT_MASK_RFIFO_CNT) == 0);

    if (readl(&regs->stat) & UART_STAT_ERR) {
        // clear error, it doesn't clear to 0 automatically
        writel(readl(&regs->cntl) | UART_CNTL_MASK_CLR_ERR, &regs->cntl);
        writel(readl(&regs->cntl) & (~UART_CNTL_MASK_CLR_ERR), &regs->cntl);
    }
    return (int)(readl(&regs->rfifo) & 0xFF);
}


static int uart_meson_dev_tstc(const int port)
{
	struct uart_meson *regs = uart_ports[port];
    return (readl(&regs->stat) & UART_STAT_MASK_RFIFO_CNT);
}

static void meson_uart_dev_putc(const char c, const int port)
{
    struct uart_meson *regs = uart_ports[port];

    if (c == '\n')
        meson_uart_dev_putc(port,'\r');

    // wait TX FIFO not full
    while (readl(&regs->stat) & UART_STAT_MASK_TFIFO_FULL);
    writel(c,&regs->wfifo);
    while(!(readl(&regs->stat) & UART_STAT_MASK_TFIFO_EMPTY));
}

static void meson_uart_dev_puts(const char *s, const int port)
{
    while (*s) {
        meson_uart_dev_putc(port,*s++);
    }
}




/* Multi serial device functions */
#define DECLARE_MESON_SERIAL(port) \
static int uart##port##_init(void) { return meson_uart_dev_init(port); } \
static int uart##port##_stop(void) { return meson_uart_dev_stop(port); } \
static void uart##port##_setbrg(void) { meson_uart_dev_setbrg(port); } \
static int uart##port##_getc(void) { return meson_uart_dev_getc(port); } \
static int uart##port##_tstc(void) { return uart_meson_dev_tstc(port); } \
static void uart##port##_putc(const char c) { meson_uart_dev_putc(c, port); } \
static void uart##port##_puts(const char *s) { meson_uart_dev_puts(s, port); } \
struct serial_device serial_device##port = {	\
	.name	= "meson_serial"#port,		\
	.start	= uart##port##_init,		\
	.stop	= uart##port##_stop,		\
	.setbrg	= uart##port##_setbrg,		\
	.getc	= uart##port##_getc,		\
	.tstc	= uart##port##_tstc,		\
	.putc	= uart##port##_putc,		\
	.puts	= uart##port##_puts,		\
}


DECLARE_MESON_SERIAL(0);
DECLARE_MESON_SERIAL(1);
DECLARE_MESON_SERIAL(2);
DECLARE_MESON_SERIAL(3);




void meson_serial_initialize(void)
{
	serial_register(&serial_device0);
	serial_register(&serial_device1);
	serial_register(&serial_device2);
	serial_register(&serial_device3);
}

__weak struct serial_device *default_serial_console(void)
{
#if CONFIG_CONS_INDEX == 0
	return &serial_device0;
#elif CONFIG_CONS_INDEX == 1
	return &serial_device1;
#elif CONFIG_CONS_INDEX == 2
	return &serial_device2;
#elif CONFIG_CONS_INDEX == 3
	return &serial_device3;
#else
#error "CONFIG_CONS_INDEX? missing."
#endif
}
