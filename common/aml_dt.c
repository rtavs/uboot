#define AML_DT_IND_LENGTH	4	/*fixed*/

#define AML_DT_TOTAL_DTB_OFFSET	8
#define AML_DT_FIRST_DTB_OFFSET	12
#define AML_DT_DTB_HEADER_SIZE	20
#define AML_DT_DTB_SOC_OFFSET	0
#define AML_DT_DTB_PLAT_OFFSET	4
#define AML_DT_DTB_VARI_OFFSET	8
#define AML_DT_DTB_OFFSET_OFFSET	12
#define AML_DT_DTB_SIZE_OFFSET	16

#define AML_DT_UBOOT_ENV	"aml_dt"
#define DT_HEADER_MAGIC		0xedfe0dd0	/*header of dtb file*/
#define AML_DT_HEADER_MAGIC	0x5f4c4d41	/*"AML_", multi dtbs supported*/

#define readl(addr) (*(volatile unsigned int*)(addr))

unsigned int get_multi_dt_entry(unsigned int fdt_addr){
	unsigned int dt_magic = readl(fdt_addr);
	unsigned int dt_total = 0;
	/*printf("      Process device tree. dt magic: %x\n", dt_magic);*/
	if(dt_magic == DT_HEADER_MAGIC){/*normal dtb*/
		/*printf("      One dtb detected\n");*/
		return fdt_addr;
	}
	if(dt_magic == AML_DT_HEADER_MAGIC){/*multi dtb*/
		/*fdt_addr + 0x8: num of dtbs*/
		dt_total = readl(fdt_addr + AML_DT_TOTAL_DTB_OFFSET);
		printf("      Multi dtb detected, support %d dtbs.\n", dt_total);
		int i = 0;
		unsigned char *aml_dt_buf = getenv(AML_DT_UBOOT_ENV);
		unsigned int aml_dt_len = aml_dt_buf ? strlen(aml_dt_buf) : 0;
		if(aml_dt_len <= 0){
			printf("      Get env aml_dt failed!\n");
			return fdt_addr;
		}
		unsigned char aml_dt_soc[AML_DT_IND_LENGTH+1];
		unsigned char aml_dt_plat[AML_DT_IND_LENGTH+1];
		unsigned char aml_dt_vari[AML_DT_IND_LENGTH+1];
		unsigned int dt_soc_loop = 0;
		unsigned int dt_plat_loop = 0;
		unsigned int dt_vari_loop = 0;
		unsigned int dt_get_soc = 0;
		unsigned int dt_get_plat = 0;
		unsigned int dt_get_vari = 0;

		/*eg: aml_dt[]="m6_g33_1g" -> aml_dt_soc[]="m6  ", aml_dt_plat[]="g33 ", aml_dt_vari[]="1g  "*/
		/*eg: aml_dt[]="m8_k200" -> aml_dt_soc[]="m8  ", aml_dt_plat[]="k200", aml_dt_vari[]="    "*/
		/*aml_dt process start*/
		for(i = 0; i < aml_dt_len; i++){
			if(aml_dt_buf[i] == '_'){
				if((dt_get_soc) && (dt_get_plat) && (!dt_get_vari)){
					dt_get_vari = 1;
					continue;
				}
				if((dt_get_soc) && (!dt_get_plat)){
					dt_get_plat = 1;
					continue;
				}
				if(!dt_get_soc){
					dt_get_soc = 1;
					continue;
				}
			}
			if((!dt_get_soc) && (dt_soc_loop < AML_DT_IND_LENGTH)){
				aml_dt_soc[dt_soc_loop] = aml_dt_buf[i];
				dt_soc_loop++;
			}
			if((dt_get_soc) && (!dt_get_plat) && (dt_plat_loop < AML_DT_IND_LENGTH)){
				aml_dt_plat[dt_plat_loop] = aml_dt_buf[i];
				dt_plat_loop++;
			}
			if((dt_get_soc) && (dt_get_plat) && (!dt_get_vari) && (dt_vari_loop < AML_DT_IND_LENGTH)){
				aml_dt_vari[dt_vari_loop] = aml_dt_buf[i];
				dt_vari_loop++;
			}
		}
		if(dt_soc_loop < AML_DT_IND_LENGTH){
			for(i = dt_soc_loop; i < AML_DT_IND_LENGTH; i++)
				aml_dt_soc[i] = ' ';
		}
		if(dt_plat_loop < AML_DT_IND_LENGTH){
			for(i = dt_plat_loop; i < AML_DT_IND_LENGTH; i++)
				aml_dt_plat[i] = ' ';
		}
		if(dt_vari_loop < AML_DT_IND_LENGTH){
			for(i = dt_vari_loop; i < AML_DT_IND_LENGTH; i++)
				aml_dt_vari[i] = ' ';
		}
		aml_dt_soc[AML_DT_IND_LENGTH] = '\0';
		aml_dt_plat[AML_DT_IND_LENGTH] = '\0';
		aml_dt_vari[AML_DT_IND_LENGTH] = '\0';
		/*aml_dt process end*/

		printf("        soc: \"%s\", platform: \"%s\", variant: \"%s\"\n", aml_dt_soc, aml_dt_plat, aml_dt_vari);

		/*process big endian and little endian*/
		/*process endian start*/
		unsigned int int_tmp = readl(aml_dt_soc);
		unsigned int aml_dt_soc_int = (int_tmp << 24) | ((int_tmp << 8) & 0xff0000) | \
			((int_tmp >> 8) & 0xff00) | (int_tmp >> 24);
		int_tmp = readl(aml_dt_plat);
		unsigned int aml_dt_plat_int = (int_tmp << 24) | ((int_tmp << 8) & 0xff0000) | \
			((int_tmp >> 8) & 0xff00) | (int_tmp >> 24);
		int_tmp = readl(aml_dt_vari);
		unsigned int aml_dt_vari_int = (int_tmp << 24) | ((int_tmp << 8) & 0xff0000) | \
			((int_tmp >> 8) & 0xff00) | (int_tmp >> 24);
		printf("        soc_int: %x, platform_int: %x, variant_int: %x\n", aml_dt_soc_int, aml_dt_plat_int, aml_dt_vari_int);
		/*process endian end*/

		/*match and print result start*/
		unsigned int soc_tmp = 0;
		unsigned int plat_tmp = 0;
		unsigned int vari_tmp = 0;
		unsigned int dtb_match_num = 0xffff;
		for(i = 0; i < dt_total; i++){
			soc_tmp = readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
				i * AML_DT_DTB_HEADER_SIZE + AML_DT_DTB_SOC_OFFSET);
			plat_tmp = readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
				i * AML_DT_DTB_HEADER_SIZE + AML_DT_DTB_PLAT_OFFSET);
			vari_tmp = readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
				i * AML_DT_DTB_HEADER_SIZE + AML_DT_DTB_VARI_OFFSET);
			printf("        %d dtb: soc %x plat %x vari %x \"", i, soc_tmp, plat_tmp, vari_tmp);
			unsigned char ch_tmp;
			unsigned int j = 0;
			for(j = 0; j < AML_DT_IND_LENGTH; j++){
				ch_tmp = (unsigned char)(soc_tmp>>(8*(AML_DT_IND_LENGTH-j-1)));
				if(ch_tmp != ' ')
					printf("%c", ch_tmp);
			}
			if(0x20202020 != plat_tmp)
				printf("_");
			for(j = 0; j < AML_DT_IND_LENGTH; j++){
				ch_tmp = (unsigned char)(plat_tmp>>(8*(AML_DT_IND_LENGTH-j-1)));
				if(ch_tmp != ' ')
					printf("%c", ch_tmp);
			}
			if(0x20202020 != vari_tmp)
				printf("_");
			for(j = 0; j < AML_DT_IND_LENGTH; j++){
				ch_tmp = (unsigned char)(vari_tmp>>(8*(AML_DT_IND_LENGTH-j-1)));
				if(ch_tmp != ' ')
					printf("%c", ch_tmp);
			}
			printf("\"\n");
			if((soc_tmp == aml_dt_soc_int) && (plat_tmp == aml_dt_plat_int) && (vari_tmp == aml_dt_vari_int)){
				//printf("Find match dtb\n");
				dtb_match_num = i;
			}
		}
		/*match and print result end*/

		if(0xffff != dtb_match_num){
			printf("      Find match dtb: %d\n", dtb_match_num);
			/*this offset is based on dtb image package, so should add on base address*/
			return (fdt_addr + readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
				dtb_match_num * AML_DT_DTB_HEADER_SIZE + AML_DT_DTB_OFFSET_OFFSET));
		}
		else{
			printf("      Not match any dtb.\n");
			return fdt_addr;
		}
	}
}
