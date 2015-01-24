//copy from efuse_hw.c

#define WRITE_EFUSE_REG(reg, val)  __raw_writel(val, reg)
#define READ_EFUSE_REG(reg)  (__raw_readl(reg))
#define WRITE_EFUSE_REG_BITS(reg, val, start, len) \
	WRITE_EFUSE_REG(reg,	(READ_EFUSE_REG(reg) & ~(((1L<<(len))-1)<<(start)) )| ((unsigned)((val)&((1L<<(len))-1)) << (start)))
#define READ_EFUSE_REG_BITS(reg, start, len) \
	((READ_EFUSE_REG(reg) >> (start)) & ((1L<<(len))-1))
//EFUSE_CNTL1
#define CNTL1_PD_ENABLE_BIT					27
#define CNTL1_PD_ENABLE_SIZE					1
#define CNTL1_PD_ENABLE_ON	1
#define CNTL1_PD_ENABLE_OFF   0

#define CNTL1_AUTO_RD_BUSY_BIT              26
#define CNTL1_AUTO_RD_BUSY_SIZE             1

#define CNTL1_AUTO_RD_START_BIT             25
#define CNTL1_AUTO_RD_START_SIZE            1

#define CNTL1_AUTO_RD_ENABLE_BIT            24
#define CNTL1_AUTO_RD_ENABLE_SIZE           1
#define CNTL1_AUTO_RD_ENABLE_ON             1
#define CNTL1_AUTO_RD_ENABLE_OFF            0

#define CNTL1_BYTE_WR_DATA_BIT              16
#define CNTL1_BYTE_WR_DATA_SIZE             8

#define CNTL1_AUTO_WR_BUSY_BIT              14
#define CNTL1_AUTO_WR_BUSY_SIZE             1

#define CNTL1_AUTO_WR_START_BIT             13
#define CNTL1_AUTO_WR_START_SIZE            1
#define CNTL1_AUTO_WR_START_ON              1
#define CNTL1_AUTO_WR_START_OFF             0

#define CNTL1_AUTO_WR_ENABLE_BIT            12
#define CNTL1_AUTO_WR_ENABLE_SIZE           1
#define CNTL1_AUTO_WR_ENABLE_ON             1
#define CNTL1_AUTO_WR_ENABLE_OFF            0

#define CNTL1_BYTE_ADDR_SET_BIT             11
#define CNTL1_BYTE_ADDR_SET_SIZE            1
#define CNTL1_BYTE_ADDR_SET_ON              1
#define CNTL1_BYTE_ADDR_SET_OFF             0

#define CNTL1_BYTE_ADDR_BIT                 0
#define CNTL1_BYTE_ADDR_SIZE                10

//EFUSE_CNTL4
#define CNTL4_ENCRYPT_ENABLE_BIT            10
#define CNTL4_ENCRYPT_ENABLE_SIZE           1
#define CNTL4_ENCRYPT_ENABLE_ON             1
#define CNTL4_ENCRYPT_ENABLE_OFF            0

#define CNTL4_ENCRYPT_RESET_BIT             9
#define CNTL4_ENCRYPT_RESET_SIZE            1
#define CNTL4_ENCRYPT_RESET_ON              1
#define CNTL4_ENCRYPT_RESET_OFF             0


#define CNTL4_XOR_ROTATE_BIT                8
#define CNTL4_XOR_ROTATE_SIZE               1

#define CNTL4_XOR_BIT                       0
#define CNTL4_XOR_SIZE                      8

static void __efuse_write_byte( unsigned long addr, unsigned long data )
{
#ifndef CONFIG_MESON_TRUSTZONE
#ifdef EFUSE_DEBUG
	char *p = (char*)debug_buf;
	p[addr] = data;
	return;
#endif
	//printf("addr=%d, data=%x\n", addr, data);
	unsigned long auto_wr_is_enabled = 0;

	//set efuse PD=0
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 0, 27, 1);

	if ( READ_EFUSE_REG( P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_ENABLE_BIT ) )
	{
		auto_wr_is_enabled = 1;
	}
	else
	{
		/* temporarily enable Write mode */
		WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_ON,
			CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
	}

#if defined(CONFIG_M8) || defined(CONFIG_M8B)
	unsigned int byte_sel = addr % 4;
	addr = addr / 4;

	/* write the address */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, addr,
		CNTL1_BYTE_ADDR_BIT, CNTL1_BYTE_ADDR_SIZE );

	//auto write byte select (0-3), for m8
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL3, byte_sel,
		CNTL1_AUTO_WR_START_BIT, 2 );
#else
	/* write the address */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, addr,
		CNTL1_BYTE_ADDR_BIT, CNTL1_BYTE_ADDR_SIZE );
#endif

	/* set starting byte address */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
		CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
		CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );

	/* write the byte */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, data,
		CNTL1_BYTE_WR_DATA_BIT, CNTL1_BYTE_WR_DATA_SIZE );
	/* start the write process */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
		CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
		CNTL1_AUTO_WR_START_BIT, CNTL1_AUTO_WR_START_SIZE );
	/* dummy read */
	READ_EFUSE_REG(P_EFUSE_CNTL1 );

	while ( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_WR_BUSY_BIT ))
	{
		__udelay(1);
	}

	/* if auto write wasn't enabled and we enabled it, then disable it upon exit */
	if (auto_wr_is_enabled == 0 )
	{
		WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_ENABLE_OFF,
			CNTL1_AUTO_WR_ENABLE_BIT, CNTL1_AUTO_WR_ENABLE_SIZE );
	}

	//set efuse PD=1
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 1, 27, 1);

#endif   // endif trustzone
}

static void __efuse_read_dword( unsigned long addr, unsigned long *data )
{
#ifndef CONFIG_MESON_TRUSTZONE
#ifdef EFUSE_DEBUG
	unsigned *p =debug_buf;
	*data = p[addr>>2];
	return;
#endif

#if defined(CONFIG_M8) || defined(CONFIG_M8B)
	addr = addr / 4;	//each address have 4 bytes in m8
#endif
	unsigned long auto_rd_is_enabled = 0;

	//set efuse PD=0
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 0, 27, 1);

	if( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_ENABLE_BIT ))
	{
		auto_rd_is_enabled = 1;
	}
	else
	{
		/* temporarily enable Read mode */
		WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_ON,
			CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	}

	/* write the address */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, addr,
		CNTL1_BYTE_ADDR_BIT,  CNTL1_BYTE_ADDR_SIZE );

	/* set starting byte address */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_ON,
		CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_BYTE_ADDR_SET_OFF,
		CNTL1_BYTE_ADDR_SET_BIT, CNTL1_BYTE_ADDR_SET_SIZE );
   /* start the read process */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_ON,
		CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_WR_START_OFF,
		CNTL1_AUTO_RD_START_BIT, CNTL1_AUTO_RD_START_SIZE );

	/* dummy read */
	READ_EFUSE_REG( P_EFUSE_CNTL1 );
	while ( READ_EFUSE_REG(P_EFUSE_CNTL1) & ( 1 << CNTL1_AUTO_RD_BUSY_BIT ) )
	{
		__udelay(1);
	}
	/* read the 32-bits value */
	( *data ) = READ_EFUSE_REG( P_EFUSE_CNTL2 );
	/* if auto read wasn't enabled and we enabled it, then disable it upon exit */
	if ( auto_rd_is_enabled == 0 )
	{
		WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, CNTL1_AUTO_RD_ENABLE_OFF,
			CNTL1_AUTO_RD_ENABLE_BIT, CNTL1_AUTO_RD_ENABLE_SIZE );
	}

	//set efuse PD=1
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL1, 1, 27, 1);

	//printf("__efuse_read_dword: addr=%ld, data=0x%lx\n", addr, *data);
#endif   // end if trustzone
}

static int efuse_init(void)
{
#ifndef CONFIG_MESON_TRUSTZONE
	/* disable efuse encryption */
	WRITE_EFUSE_REG_BITS( P_EFUSE_CNTL4, CNTL4_ENCRYPT_ENABLE_OFF,
		CNTL4_ENCRYPT_ENABLE_BIT, CNTL4_ENCRYPT_ENABLE_SIZE );
#if defined(CONFIG_M6) || defined(CONFIG_M8) || defined(CONFIG_M8B)
	// clear power down bit
	WRITE_EFUSE_REG_BITS(P_EFUSE_CNTL1, CNTL1_PD_ENABLE_OFF,
			CNTL1_PD_ENABLE_BIT, CNTL1_PD_ENABLE_SIZE);
#endif
#endif   // endif trustzone
	return 0;
}

static void efuse_write_byte(unsigned long addr, unsigned char data){
	efuse_init();
	unsigned int int_addr = addr;
	__efuse_write_byte(int_addr, (unsigned long)data);
}

static unsigned int efuse_read_byte(unsigned long addr){
	efuse_init();
	unsigned int int_addr = addr;
	unsigned int off_addr = addr % 4;
	unsigned int r_data = 0;
	__efuse_read_dword(int_addr, &r_data);
	//serial_put_hex(r_data, 32);
	//serial_put_hex(((r_data >> (off_addr*8)) & 0xff), 8);
	return ((r_data >> (off_addr*8)) & 0xff);
}

static void efuse_dump(void){
	int loo = 0;
	for(loo=0; loo<512; loo++){
		if(loo % 16 == 0){
			serial_puts("\n");
			serial_put_hex(loo, 12);
			serial_puts(": ");
		}
		//efuse_read_byte(loo);
		unsigned int tmp_data = efuse_read_byte(loo);
		serial_put_hex(tmp_data, 8);
		serial_puts(" ");
	}
	serial_puts("\n");
}