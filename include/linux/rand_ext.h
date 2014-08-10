#ifndef __RAND_EXT__H
#define __RAND_EXT__H



static int random_generate(unsigned int seed_ext,unsigned char *buf,unsigned len)
{
	return -1;
}
static unsigned int random_u32(unsigned int seed)
{
	return seed;
}
static unsigned short random_u16(unsigned int seed)
{
	return seed;
}
static unsigned char random_u8(unsigned int seed)
{
	return seed;
}

#endif
