#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/romboot.h>


#ifdef  CONFIG_AML_SPL_L1_CACHE_ON
#include <aml_a9_cache.c>
#endif  //CONFIG_AML_SPL_L1_CACHE_ON

#include <romboot.c>
#ifndef CONFIG_AML_UBOOT_MAGIC
#define CONFIG_AML_UBOOT_MAGIC 0x12345678
#endif

#if defined(CONFIG_AML_SMP)
#include <smp.dat>
SPL_STATIC_FUNC int load_smp_code()
{
	serial_puts("Start load SMP code!\n");
	unsigned * paddr = (unsigned*)PHYS_MEMORY_START;
	unsigned size = sizeof(smp_code)/sizeof(unsigned);
	int i;
	for(i = 0; i < size; i++){
		*paddr = smp_code[i];
		paddr++;
	}
	serial_puts("Load SMP code finished!\n");
}
#endif

SPL_STATIC_FUNC int load_uboot(unsigned __TEXT_BASE,unsigned __TEXT_SIZE)
{
	unsigned por_cfg=romboot_info->por_cfg;
	unsigned boot_id=romboot_info->boot_id;
	unsigned size;
	int rc=0;

    serial_puts("\nHHH\n");

#if defined(CONFIG_AML_SMP)
	load_smp_code();
#endif

#ifdef  CONFIG_AML_SPL_L1_CACHE_ON
	aml_cache_enable();
	//serial_puts("\nSPL log : ICACHE & DCACHE ON\n");
#endif	//CONFIG_AML_SPL_L1_CACHE_ON

	size=__TEXT_SIZE;

#if defined(AML_UBOOT_SINFO_OFFSET)
	unsigned int *pAUINF = (unsigned int *)(((unsigned int)load_uboot & 0xFFFF8000)+(AML_UBOOT_SINFO_OFFSET));
	size = *pAUINF++; //SPL
	size = *pAUINF++ - size; //TPL - SPL
	size += *pAUINF;  // + Secure OS
	size += 0x200;  //for secure boot, just add 512 without check, for simple
	if(size < (100<<10) || size > (1<<20)) //illegal size, restore to default from rom_spl.s
		size = __TEXT_SIZE;
#endif //AML_UBOOT_SINFO_OFFSET

	//boot_id = 1;

    if (boot_id != 0) {
	    serial_puts("\nboot_id is ");
	    serial_put_dec(boot_id);
	    serial_puts("\n\n");
    }

	if(boot_id > 1)
        boot_id=0;

	if(boot_id==0) {
       rc=fw_load_intl(por_cfg,__TEXT_BASE,size);
	} else {
	   rc=fw_load_extl(por_cfg,__TEXT_BASE,size);
	}

    if (rc == 0) {
	    serial_puts("\nfw_load rc is 0\n");
    }

	//here no need to flush I/D cache?
#if CONFIG_AML_SPL_L1_CACHE_ON
	aml_cache_disable();
	//dcache_flush();
#endif	//CONFIG_AML_SPL_L1_CACHE_ON


    if(rc==0) {
	    fw_print_info(por_cfg,boot_id);
        return rc;
    }

	if(rc){
	serial_puts(__FILE__);
		serial_puts(__FUNCTION__);
		serial_put_dword(__LINE__);
		AML_WATCH_DOG_START();
	}

#if CONFIG_ENABLE_EXT_DEVICE_RETRY
	while(rc)
	{
     debug_rom(__FILE__,__LINE__);

	    rc=fw_init_extl(por_cfg);//INTL device  BOOT FAIL
	    if(rc)
	        continue;
	    rc=fw_load_extl(por_cfg,__TEXT_BASE,size);
	    if(rc)
	        continue;
	}
#endif
	fw_print_info(por_cfg,1);

    return rc;
}
