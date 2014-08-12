#include <config.h>
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/reg_addr.h>
#include <asm/arch/uart.h>
#include <amlogic/aml_pmu_common.h>

extern void hard_i2c_init(void);
extern unsigned char hard_i2c_read8(unsigned char SlaveAddr, unsigned char RegAddr);
extern void hard_i2c_write8(unsigned char SlaveAddr, unsigned char RegAddr, unsigned char Data);
extern unsigned char hard_i2c_read168(unsigned char SlaveAddr, unsigned short RegAddr);
extern void hard_i2c_write168(unsigned char SlaveAddr, unsigned short RegAddr, unsigned char Data);

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifndef CONFIG_VDDAO_VOLTAGE
#define CONFIG_VDDAO_VOLTAGE 1200
#endif


#define DEVID 0x64


static char format_buf[12] = {};
static char *format_dec_value(uint32_t val)
{
    uint32_t tmp, i = 11;

    memset(format_buf, 0, 12);
    while (val) {
        tmp = val % 10;
        val /= 10;
        format_buf[--i] = tmp + '0';
    }
    return format_buf + i;
}

static void print_voltage_info(char *voltage_prefix, int voltage_idx, uint32_t voltage, uint32_t reg_from, uint32_t reg_to, uint32_t addr)
{
    serial_puts(voltage_prefix);
    if (voltage_idx >= 0) {
        serial_put_hex(voltage_idx, 8);
    }
    serial_puts(" set to ");
    serial_puts(format_dec_value(voltage));
    serial_puts(", register from 0x");
    serial_put_hex(reg_from, 16);
    serial_puts(" to 0x");
    serial_put_hex(reg_to, 16);
    serial_puts(", addr:0x");
    serial_put_hex(addr, 16);
    serial_puts("\n");
}

#define PWM_PRE_DIV 0
struct vcck_pwm {
    int          vcck_voltage;
    unsigned int pwm_value;
};

static struct vcck_pwm vcck_pwm_table[] = {
    {1401, 0x0000001c},
    {1384, 0x0001001b},
    {1367, 0x0002001a},
    {1350, 0x00030019},
    {1333, 0x00040018},
    {1316, 0x00050017},
    {1299, 0x00060016},
    {1282, 0x00070015},
    {1265, 0x00080014},
    {1248, 0x00090013},
    {1232, 0x000a0012},
    {1215, 0x000b0011},
    {1198, 0x000c0010},
    {1181, 0x000d000f},
    {1164, 0x000e000e},
    {1147, 0x000f000d},
    {1130, 0x0010000c},
    {1113, 0x0011000b},
    {1096, 0x0012000a},
    {1079, 0x00130009},
    {1062, 0x00140008},
    {1045, 0x00150007},
    {1028, 0x00160006},
    {1011, 0x00170005},
    { 994, 0x00180004},
    { 977, 0x00190003},
    { 960, 0x001a0002},
    { 943, 0x001b0001},
    { 926, 0x001c0000}
};

static int vcck_set_default_voltage(int voltage)
{
    int i = 0;
    unsigned int misc_cd;

    // set PWM_C pinmux
    setbits_le32(P_PERIPHS_PIN_MUX_2, (1 << 2));
    clrbits_le32(P_PERIPHS_PIN_MUX_1, (1 << 29));
    misc_cd = readl(P_PWM_MISC_REG_CD);

    if (voltage > vcck_pwm_table[0].vcck_voltage) {
        serial_puts("vcck voltage too large:");
        serial_puts(format_dec_value(voltage));
        return -1;
    }
    for (i = 0; i < ARRAY_SIZE(vcck_pwm_table) - 1; i++) {
        if (vcck_pwm_table[i].vcck_voltage     >= voltage &&
            vcck_pwm_table[i + 1].vcck_voltage <  voltage) {
            break;
        }
    }
    serial_puts("set vcck voltage to ");
    serial_puts(format_dec_value(voltage));
    serial_puts(", pwm_value from 0x");
    serial_put_hex(readl(P_PWM_PWM_C), 32);
    serial_puts(" to 0x");
    serial_put_hex(vcck_pwm_table[i].pwm_value, 32);
    serial_puts("\n");
    writel(vcck_pwm_table[i].pwm_value, P_PWM_PWM_C);
    writel(((misc_cd & ~(0x7f << 8)) | ((1 << 15) | (PWM_PRE_DIV << 8) | (1 << 0))), P_PWM_MISC_REG_CD);
    return 0;
}

#ifdef CONFIG_RN5T618
void rn5t618_set_bits(uint32_t add, uint8_t bits, uint8_t mask)
{
    uint8_t val;
    val = hard_i2c_read8(DEVID, add);
    val &= ~(mask);                                         // clear bits;
    val |=  (bits & mask);                                  // set bits;
    hard_i2c_write8(DEVID, add, val);
}

int find_idx(int start, int target, int step, int size)
{
    int i = 0;
    do {
        if (start >= target) {
            break;
        }
        start += step;
        i++;
    } while (i < size);
    return i;
}

int rn5t618_set_dcdc_voltage(int dcdc, int voltage)
{
    int addr;
    int idx_to, idx_cur;
    addr = 0x35 + dcdc;
    idx_to = find_idx(6000, voltage * 10, 125, 256);            // step is 12.5mV
    idx_cur = hard_i2c_read8(DEVID, addr);
    print_voltage_info("DCDC", dcdc, voltage, idx_cur, idx_to, addr);
    hard_i2c_write8(DEVID, addr, idx_to);
    __udelay(5 * 1000);
}

#define LDO_RTC1        10
#define LDO_RTC2        11
int rn5t618_set_ldo_voltage(int ldo, int voltage)
{
    int addr;
    int idx_to, idx_cur;
    int start = 900;

    switch (ldo) {
    case LDO_RTC1:
        addr  = 0x56;
        start = 1700;
        break;
    case LDO_RTC2:
        addr  = 0x57;
        start = 900;
        break;
    case 1:
    case 2:
    case 4:
    case 5:
        start = 900;
        addr  = 0x4b + ldo;
        break;
    case 3:
        start = 600;
        addr  = 0x4b + ldo;
        break;
    default:
        serial_puts("wrong LDO value\n");
        break;
    }
    idx_to = find_idx(start, voltage, 25, 128);                 // step is 25mV
    idx_cur = hard_i2c_read8(DEVID, addr);
    print_voltage_info("LDO", ldo, voltage, idx_cur, idx_to, addr);
    hard_i2c_write8(DEVID, addr, idx_to);
    __udelay(5 * 100);
}

void rn5t618_power_init(int init_mode)
{
    unsigned char val;

    hard_i2c_read8(DEVID, 0x0b);                                // clear watch dog
    rn5t618_set_bits(0xB3, 0x40, 0x40);
    rn5t618_set_bits(0xB8, 0x02, 0x1f);                         // set charge current to 300mA

    if (init_mode == POWER_INIT_MODE_NORMAL) {
    #ifdef CONFIG_VCCK_VOLTAGE
        rn5t618_set_dcdc_voltage(1, CONFIG_VCCK_VOLTAGE);       // set cpu voltage
    #endif
    #ifdef CONFIG_VDDAO_VOLTAGE
        rn5t618_set_dcdc_voltage(2, CONFIG_VDDAO_VOLTAGE);      // set VDDAO voltage
    #endif
    } else if (init_mode == POWER_INIT_MODE_USB_BURNING) {
        /*
         * if under usb burning mode, keep VCCK and VDDEE
         * as low as possible for power saving and stable issue
         */
        rn5t618_set_dcdc_voltage(1, 900);                       // set cpu voltage
        rn5t618_set_dcdc_voltage(2, 950);                       // set VDDAO voltage
    }
#ifdef CONFIG_DDR_VOLTAGE
    rn5t618_set_dcdc_voltage(3, CONFIG_DDR_VOLTAGE);            // set DDR voltage
#endif
    val = hard_i2c_read8(DEVID, 0x30);
    val |= 0x01;
    hard_i2c_write8(DEVID, 0x30, val);                          // Enable DCDC3--DDR
#ifdef CONFIG_VDDIO_AO28
    rn5t618_set_ldo_voltage(1, CONFIG_VDDIO_AO28);              // VDDIO_AO28
#endif
#ifdef CONFIG_VDDIO_AO18
    rn5t618_set_ldo_voltage(2, CONFIG_VDDIO_AO18);              // VDDIO_AO18
#endif
#ifdef CONFIG_VCC1V8
    rn5t618_set_ldo_voltage(3, CONFIG_VCC1V8);                  // VCC1.8V
#endif
#ifdef CONFIG_VCC2V8
    rn5t618_set_ldo_voltage(4, CONFIG_VCC2V8);                  // VCC2.8V
#endif
#ifdef CONFIG_AVDD1V8
    rn5t618_set_ldo_voltage(5, CONFIG_AVDD1V8);                 // AVDD1.8V
#endif
#ifdef CONFIG_VDD_LDO
    rn5t618_set_ldo_voltage(LDO_RTC1, CONFIG_VDD_LDO);          // VDD_LDO
#endif
#ifdef CONFIG_RTC_0V9
    rn5t618_set_ldo_voltage(LDO_RTC2, CONFIG_RTC_0V9);          // RTC_0V9
#endif
}
#endif



void power_init(int init_mode)
{
    hard_i2c_init();

    __udelay(1000);
#ifdef CONFIG_RN5T618
    rn5t618_power_init(init_mode);
#elif defined (CONFIG_PWM_DEFAULT_VCCK_VOLTAGE) && defined (CONFIG_VCCK_VOLTAGE)
    vcck_set_default_voltage(CONFIG_VCCK_VOLTAGE);
#endif
    __udelay(1000);
}
