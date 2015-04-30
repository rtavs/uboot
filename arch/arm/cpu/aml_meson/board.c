#include <common.h>
#include <asm/arch/io.h>
#include <asm/system.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	gd->ram_size = PHYS_MEMORY_SIZE;
	return 0;
}
