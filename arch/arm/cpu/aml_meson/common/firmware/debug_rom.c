#include <asm/arch/romboot.h>
#include <asm/arch/timing.h>
#include <memtest.h>
#include <config.h>
#include <asm/arch/io.h>
#include <asm/arch/nand.h>
#ifndef STATIC_PREFIX
#define STATIC_PREFIX
#endif
extern void ipl_memcpy(void*, const void *, __kernel_size_t);
#define memcpy ipl_memcpy
#define get_timer get_utimer
static  char cmd_buf[DEBUGROM_CMD_BUF_SIZE];

void restart_arm(void);


STATIC_PREFIX char * get_cmd(void)
{
	return 0;
}
STATIC_PREFIX int run_cmd(char * cmd)
{
	return 0;
}


STATIC_PREFIX void debug_rom(char * file, int line)
{
    //int c;
    serial_puts("Enter Debugrom mode at ");
    serial_puts(file);
    serial_putc(':');
    serial_put_dword(line);

    while(run_cmd(get_cmd()))
    {
        ;
    }
}

void restart_arm(void)
{
	  writel(0x1234abcd,P_AO_RTI_STATUS_REG2);
	  //reset A9
	  writel(0x2,P_RESET4_REGISTER);
		while(1){
		}
}
