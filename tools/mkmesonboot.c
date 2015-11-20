/*
 * (C) Copyright 2007-2015
 *
 * a simple tool to generate bootable image for meson platform.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>




#define ALIGN(x, a) __ALIGN_MASK((x), (typeof(x))(a)-1)
#define __ALIGN_MASK(x, mask) (((x)+(mask))&~(mask))



#define READ_SIZE       32*1024     // Size for data reading
#define CHECK_SIZE      8*1024      // Size for data checking

static unsigned short buf[READ_SIZE/sizeof(short)];


static void m8_caculate(void)
{
    int i;
    unsigned short sum=0;
    //unsigned * magic;
    // Calculate sum
    for(i=0; i < 0x1b0/2; i++) {  // 0 - 216
        sum^=buf[i];
        //printf("step %d, sum = 0x%4x\n", i, sum);
    }

    printf("\n\n");
    for (i = 256; i < CHECK_SIZE / 2; i++) {
        sum^=buf[i];
        //printf("step %d sum = 0x%4x\n", i, sum);
    }

    buf[0x1b8/2] = sum;
    printf("\n\n......sum = 0x%x\n\n", sum);
}

int m8_write(FILE * fp_spl,FILE * fp_in ,FILE * fp_out)
{
    return 0;
}


int m8b_write(FILE * fp_spl,FILE * fp_uboot ,FILE * fp_out)
{
    int count;
    int num;

    memset(buf,0,sizeof(buf));

    printf("buff size = %ld, array num = %ld\n", sizeof(buf), (sizeof(buf)/sizeof(buf[0])));

    num = fread(buf, sizeof(buf[0]), sizeof(buf)/sizeof(buf[0]), fp_spl);

    printf("read size = %d\n",num);


    //note: 1. Following code is to improve the performance when load TPL (+ seucre OS)
    //             get the precise TPL size for SPL
    //      2. Refer SECURE_OS_SRAM_BASE from file arch\arm\include\asm\arch-m8\trustzone.h
    //             can get more detail info
    //      3. Here should care the READ_SIZE  is the SPL size which can be 32KB or 64KB
#define AML_UBOOT_SINFO_OFFSET (READ_SIZE-512-32)
#if defined(AML_UBOOT_SINFO_OFFSET)
    printf("xxxxxxxxxxxxxxxxx\n");
    fseek(fp_uboot,0,SEEK_END);
    unsigned int nINLen = ftell(fp_uboot);
    nINLen = (nINLen + 0xF ) & (~0xF);
    fseek(fp_uboot,0,SEEK_SET);
    unsigned int * pAUINF = (int *)(buf+(AML_UBOOT_SINFO_OFFSET>>1));
    *pAUINF++ = READ_SIZE; //32KB or 64KB
    *pAUINF   = nINLen+READ_SIZE;
#undef AML_UBOOT_SINFO_OFFSET //for env clean up
#endif //AML_UBOOT_SINFO_OFFSET

    m8_caculate();

    fwrite(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_out);

    printf("111111111111111\n");


    while(!feof(fp_spl)) {
        printf("22222222222222\n");
        count=fread(buf,sizeof(buf[0]),sizeof(buf)/sizeof(buf[0]),fp_spl);
        fwrite(buf,sizeof(buf[0]),count,fp_out);
    }

    while(!feof(fp_uboot)) {
        printf("333333333333333\n");
        count=fread(buf,sizeof(char),sizeof(buf),fp_uboot);
        fwrite(buf,sizeof(char),count,fp_out);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    FILE *fp_spl, *fp_uboot, *fp_out;

    if (argc < 4) {
        printf("\tThis program makes an input bin file to meson " \
                "bootable image.\n" \
                "\tUsage: %s <spl> <uboot> <out>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fp_spl = fopen(argv[1], "rb");
        if (!fp_spl) {
        perror("Open spl file");
        return EXIT_FAILURE;
    }

    fp_uboot = fopen(argv[2], "rb");
        if (!fp_uboot) {
        perror("Open uboot file");
        return EXIT_FAILURE;
    }

    fp_out = fopen(argv[3], "wb");
    if (fp_out < 0) {
        perror("Open output file");
        return EXIT_FAILURE;
    }


    m8b_write(fp_spl,fp_uboot,fp_out);


    fclose(fp_spl);
    fclose(fp_uboot);
    fclose(fp_out);

    return EXIT_SUCCESS;
}
