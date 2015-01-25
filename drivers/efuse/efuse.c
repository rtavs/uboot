#include <config.h>
#include <common.h>
#include <asm/arch/io.h>
#include <command.h>
#include <malloc.h>
#include <amlogic/efuse.h>
#include "efuse_regs.h"


char efuse_buf[EFUSE_BYTES] = {0};

extern int efuseinfo_num;
extern efuseinfo_t efuseinfo[];
extern int efuse_active_version;
extern int efuse_active_customerid;
extern pfn efuse_getinfoex;
extern pfn_byPos efuse_getinfoex_byPos;
extern int printf(const char *fmt, ...);
extern void __efuse_write_byte( unsigned long addr, unsigned long data );
extern void __efuse_read_dword( unsigned long addr, unsigned long *data);
extern void efuse_init(void);

ssize_t efuse_read(char *buf, size_t count, loff_t *ppos )
{
	return -1;
}

ssize_t efuse_write(const char *buf, size_t count, loff_t *ppos )
{
	return -1;
}

static int cpu_is_before_m6(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c0, c0, 5	@ get MPIDR" : "=r" (val) : : "cc");

	return ((val & 0x40000000) == 0x40000000);
}


#if 0
struct efuse_chip_info_t{
	unsigned int Id1;
	unsigned int Id2;
	efuse_socchip_type_e type;
};
static const struct efuse_chip_info_t efuse_chip_info[]={
	{.Id1=0x000027ed, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M8},   //M8 second version
	{.Id1=0x000025e2, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M8},   //M8 first version
	{.Id1=0xe2000003, .Id2=0x00000bbb, .type=EFUSE_SOC_CHIP_M6}, //M6 Rev-B
	{.Id1=0x00000d67, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M6}, //M6 Rev-D
	{.Id1=0x00001435, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M6TV}, //M6TV
	{.Id1=0x000005cb, .Id2=0xe3a01000, .type=EFUSE_SOC_CHIP_M6TVLITE}, //M6TVC,M6TVLITE(M6C)
};
#define EFUSE_CHIP_INFO_NUM		sizeof(efuse_chip_info)/sizeof(efuse_chip_info[0])
#endif

struct efuse_chip_identify_t{
	unsigned int chiphw_mver;
	unsigned int chiphw_subver;
	unsigned int chiphw_thirdver;
	efuse_socchip_type_e type;
};
static const struct efuse_chip_identify_t efuse_chip_hw_info[]={
	{.chiphw_mver=27, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M8BABY},      //M8BABY ok
	{.chiphw_mver=26, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6TVD},      //M6TVD ok
	{.chiphw_mver=25, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M8},      //M8 ok
	{.chiphw_mver=24, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6TVLITE},  //M6TVC,M6TVLITE(M6C)
	{.chiphw_mver=23, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6TV},    //M6TV  ok
	{.chiphw_mver=22, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M6},      //M6  ok
	{.chiphw_mver=21, .chiphw_subver=0, .chiphw_thirdver=0, .type=EFUSE_SOC_CHIP_M3},
};
#define EFUSE_CHIP_HW_INFO_NUM  sizeof(efuse_chip_hw_info)/sizeof(efuse_chip_hw_info[0])


efuse_socchip_type_e efuse_get_socchip_type(void)
{
	efuse_socchip_type_e type;
	unsigned int *pID1 =(unsigned int *)0xd9040004;
	unsigned int *pID2 =(unsigned int *)0xd904002c;
	type = EFUSE_SOC_CHIP_UNKNOW;
	if(cpu_is_before_m6()){
		type = EFUSE_SOC_CHIP_M3;
	}
	else{
		unsigned int regval;
		int i;
		struct efuse_chip_identify_t *pinfo = (struct efuse_chip_identify_t*)&efuse_chip_hw_info[0];
		regval = READ_CBUS_REG(ASSIST_HW_REV);
		//printf("chip ASSIST_HW_REV reg:%d \n",regval);
		for(i=0;i<EFUSE_CHIP_HW_INFO_NUM;i++){
			if(pinfo->chiphw_mver == regval){
				type = pinfo->type;
				break;
			}
			pinfo++;
		}
	}
	//printf("%s \n",soc_chip[type]);
	return type;
}
static int efuse_checkversion(char *buf)
{
	efuse_socchip_type_e soc_type;
	int i;
	int ver = buf[0];
	for(i=0; i<efuseinfo_num; i++){
		if(efuseinfo[i].version == ver){
			soc_type = efuse_get_socchip_type();
			switch(soc_type){
				case EFUSE_SOC_CHIP_M3:
					if(ver != 1){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M6:
					if((ver != 2) && ((ver != 4))){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M6TV:
				case EFUSE_SOC_CHIP_M6TVLITE:
					if(ver != 2){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M6TVD:
					if(ver != M6TVD_EFUSE_VERSION_SERIALNUM_V1){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_M8:
				case EFUSE_SOC_CHIP_M8BABY:
					if(ver != M8_EFUSE_VERSION_SERIALNUM_V1){
						ver = -1;
					}
					break;
				case EFUSE_SOC_CHIP_UNKNOW:
				default:
					printf("soc is unknow\n");
					ver = -1;
					break;
			}
			return ver;
		}
	}
	return -1;
}

static int efuse_readversion(void)
{
	return -1;
}

static int efuse_getinfo_byPOS(unsigned pos, efuseinfo_item_t *info)
{
	return -1;
}


int efuse_chk_written(loff_t pos, size_t count)
{
	return -1;
}

int efuse_read_usr(char *buf, size_t count, loff_t *ppos)
{
	return 0;
}

int efuse_write_usr(char* buf, size_t count, loff_t* ppos)
{
	return -1;
}

int efuse_set_versioninfo(efuseinfo_item_t *info)
{
	return -1;
}


int efuse_getinfo(char *title, efuseinfo_item_t *info)
{
	return -1;
}

unsigned efuse_readcustomerid(void)
{
	return 0;
}

/*void efuse_getinfo_version(efuseinfo_item_t *info)
{
	strcpy(info->title, "version");
	info->we = 1;
	info->bch_reverse = 0;

	if(cpu_is_before_m6()){
		info->offset = 380;
		info->enc_len = 4;
		info->data_len = 3;
		info->bch_en = 1;
	}
	else{
		info->offset = 3;
		info->enc_len = 1;
		info->data_len = 1;
		info->bch_en = 0;
	}
}*/

char* efuse_dump(void)
{
	return NULL;
}

/* function: efuse_read_intlItem
 * intl_item: item name,name is [temperature,cvbs_trimming,temper_cvbs]
 *            [temperature: 2byte]
 *            [cvbs_trimming: 2byte]
 *            [temper_cvbs: 4byte]
 * buf:  output para
 * size: buf size
 * */
int efuse_read_intlItem(char *intl_item,char *buf,int size)
{
	efuse_socchip_type_e soc_type;
	loff_t pos;
	int len;
	int ret=-1;
	soc_type = efuse_get_socchip_type();
	switch(soc_type){
		case EFUSE_SOC_CHIP_M3:
			//pos = ;
			break;
		case EFUSE_SOC_CHIP_M6:
		case EFUSE_SOC_CHIP_M6TV:
		case EFUSE_SOC_CHIP_M6TVLITE:
			//pos = ;
			break;
		case EFUSE_SOC_CHIP_M8:
		case EFUSE_SOC_CHIP_M8BABY:
			if(strcmp(intl_item,"temperature") == 0){
				pos = 502;
				len = 2;
				if(size <= 0){
					printf("%s input size:%d is error\n",intl_item,size);
					return -1;
				}
				if(len > size){
					len = size;
				}
				ret = efuse_read( buf, len, &pos );
				return ret;
			}
			if(strcmp(intl_item,"cvbs_trimming") == 0){
				/* cvbs note:
				 * cvbs has 2 bytes, position is 504 and 505, 504 is low byte,505 is high byte
				 * p504[bit2~0] is cvbs trimming CDAC_GSW<2:0>
				 * p505[bit7] : 1--wrote cvbs, 0-- not wrote cvbs
				 * */
				pos = 504;
				len = 2;
				if(size <= 0){
					printf("%s input size:%d is error\n",intl_item,size);
					return -1;
				}
				if(len > size){
					len = size;
				}
				ret = efuse_read( buf, len, &pos );
				return ret;
			}
			if(strcmp(intl_item,"temper_cvbs") == 0){
				pos = 502;
				len = 4;
				if(size <= 0){
					printf("%s input size:%d is error\n",intl_item,size);
					return -1;
				}
				if(len > size){
					len = size;
				}
				ret = efuse_read( buf, len, &pos );
				return ret;
			}
			break;
		case EFUSE_SOC_CHIP_M6TVD:
			break;
		case EFUSE_SOC_CHIP_UNKNOW:
		default:
			printf("%s:%d chip is unkow\n",__func__,__LINE__);
			//return -1;
			break;
	}
	return ret;
}
