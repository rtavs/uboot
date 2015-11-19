#include <common.h>



DECLARE_GLOBAL_DATA_PTR;

/* add board specific code here */
int board_init(void)
{
    return 0;
}

int dram_init(void)
{
//	gd->ram_size = get_ram_size((long *)PHYS_SDRAM_0, PHYS_SDRAM_0_SIZE);

	return 0;
}
