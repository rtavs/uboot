#ifndef __SECURE_STORAGE__H__
#define __SECURE_STORAGE__H__

#define SECURE_STORAGE_NAND_TYPE	1
#define SECURE_STORAGE_SPI_TYPE		2
#define SECURE_STORAGE_EMMC_TYPE	3



static inline int secure_storage_nand_read(char *buf,unsigned int len)
{
	return -1;
}
static inline int secure_storage_nand_write(char *buf,unsigned int len)
{
	return -1;
}


static inline int secure_storage_spi_write(char *buf,unsigned int len)
{
	return -1;
}
static inline int secure_storage_spi_read(char *buf,unsigned int len)
{
	return -1;
}


static inline int secure_storage_emmc_write(char *buf,unsigned int len)
{
	return -1;
}
static inline int secure_storage_emmc_read(char *buf,unsigned int len)
{
	return -1;
}


static inline int securestore_key_init( char *seed,int len)
{
	return -1;
}
static inline int securestore_key_read(char *keyname,char *keybuf,unsigned int keylen,unsigned int *reallen)
{
	return -1;
}
static inline int securestore_key_write(char *keyname, char *keybuf,unsigned int keylen,int keytype)
{
	return -1;
}
static inline int securestore_key_query(char *keyname, unsigned int *query_return)
{
	return -1;
}
static inline int securestore_key_uninit()
{
	return -1;
}


#endif
