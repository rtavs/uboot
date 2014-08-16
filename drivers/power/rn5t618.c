#include <common.h>
#include <div64.h>
#include <asm/setup.h>
#include <asm/arch/i2c.h>
#include <asm/arch/gpio.h>
#include <aml_i2c.h>
#include <amlogic/aml_lcd.h>
#include <amlogic/rn5t618.h>

#include <amlogic/aml_pmu_common.h>

#define ABS(x)          ((x) >0 ? (x) : -(x) )
#define MAX_BUF         100
#define RN5T618_ADDR    0x32

#define DBG(format, args...) printf("[RN5T618]"format,##args)
static int rn5t618_curr_dir = 0;
static int rn5t618_battery_test = 0;

int rn5t618_write(int add, uint8_t val)
{
    int ret;
    uint8_t buf[2] = {};
    buf[0] = add & 0xff;
    buf[1] = val & 0xff;
    struct i2c_msg msg[] = {
        {
            .addr  = RN5T618_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        }
    };
    ret = aml_i2c_xfer_slow(msg, 1);
    if (ret < 0) {
        DBG("%s: i2c transfer failed, ret:%d\n", __func__, ret);
        return ret;
    }
    return 0;
}

int rn5t618_writes(int add, uint8_t *buff, int len)
{
    int ret;
    uint8_t buf[MAX_BUF] = {};
    buf[0] = add & 0xff;
    memcpy(buf + 1, buff, len);
    struct i2c_msg msg[] = {
        {
            .addr  = RN5T618_ADDR,
            .flags = 0,
            .len   = len + 1,
            .buf   = buf,
        }
    };
    ret = aml_i2c_xfer_slow(msg, 1);
    if (ret < 0) {
        DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}

int rn5t618_read(int add, uint8_t *val)
{
    int ret;
    uint8_t buf[1] = {};
    buf[0] = add & 0xff;
    struct i2c_msg msg[] = {
        {
            .addr  = RN5T618_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        },
        {
            .addr  = RN5T618_ADDR,
            .flags = I2C_M_RD,
            .len   = 1,
            .buf   = val,
        }
    };
    ret = aml_i2c_xfer_slow(msg, 2);
    if (ret < 0) {
        DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}

int rn5t618_reads(int add, uint8_t *buff, int len)
{
    int ret;
    uint8_t buf[1] = {};
    buf[0] = add & 0xff;
    struct i2c_msg msg[] = {
        {
            .addr  = RN5T618_ADDR,
            .flags = 0,
            .len   = sizeof(buf),
            .buf   = buf,
        },
        {
            .addr  = RN5T618_ADDR,
            .flags = I2C_M_RD,
            .len   = len,
            .buf   = buff,
        }
    };
    ret = aml_i2c_xfer_slow(msg, 2);
    if (ret < 0) {
        DBG("%s: i2c transfer failed, ret:%d\n", __FUNCTION__, ret);
        return ret;
    }
    return 0;
}

int rn5t618_set_bits(int add, uint8_t bits, uint8_t mask)
{
    uint8_t val;
    rn5t618_read(add, &val);
    val &= ~(mask);                                         // clear bits;
    val |=  (bits & mask);                                  // set bits;
    return rn5t618_write(add, val);
}

int rn5t618_get_battery_voltage(void)
{
    uint8_t val[2];
    int result = 0;
    int tmp;
    int i;
    for (i = 0; i < 8; i++) {
        rn5t618_set_bits(0x0066, 0x01, 0x07);                   // select vbat channel
        udelay(200);
        rn5t618_reads(0x006A, val, 2);
        tmp = (val[0] << 4) | (val[1] & 0x0f);
        tmp = (tmp * 5000) / 4096;                              // resolution: 1.221mV
        result += tmp;
    }

    return result / 8;
}

int rn5t618_get_charge_status(int print)
{
    uint16_t val = 0;
    uint8_t status;

    rn5t618_read(0x00BD, &val);
    if (rn5t618_curr_dir < 0) {
        val |= 0x8000;
    }
    if (print) {
        printf("-- charge status:0x%04x ", val);
    }
    status = (val & 0x1f);
    if ((val & 0xc0) && (rn5t618_curr_dir == 1)) {
        if (status == 2 || status ==3) {
            return 1;                                       // charging
        } else {
            return 0;
        }
    } else {
        return 2;
    }
    return 0;
}

static int rn5t618_get_coulomber(void)
{
    uint8_t val[4];
    int result;

    result = rn5t618_reads(0x00F3, val, 4);
    if (result) {
        printf("%s, failed: %d\n", __func__, __LINE__);
        return result;
    }

    result = val[3] | (val[2] << 8) | (val[1] << 16) | (val[0] << 24);
    result = result / (3600);                                           // to mAh
    return result;
}

int rn5t618_get_battery_current(void)
{
    uint8_t val[2];
    int result;

    rn5t618_reads(0x00FB, val, 2);
    result = ((val[0] & 0x3f) << 8) | (val[1]);
    if (result & 0x2000) {                                  // discharging, complement code
        result = result ^ 0x3fff;
        rn5t618_curr_dir = -1;
    } else {
        rn5t618_curr_dir = 1;
    }

    return result;
}

int rn5t618_set_gpio(int gpio, int output)
{
    int val = output ? 1 : 0;
//    DBG("%s, gpio:%d, output:%d\n", __func__, gpio, output);
    if (gpio < 0 || gpio > 3) {
        DBG("%s, wrong input of GPIO:%d\n", __func__, gpio);
        return -1;
    }
    rn5t618_set_bits(0x0091, val << gpio, 1 << gpio);       // set out put value
    rn5t618_set_bits(0x0090, 1 << gpio, 1 << gpio);         // set pin to output mode
    return 0;
}

int rn5t618_get_gpio(int gpio, int *val)
{
    int value;
    if (gpio < 0 || gpio > 3) {
        DBG("%s, wrong input of GPIO:%d\n", __func__, gpio);
        return -1;
    }
    rn5t618_read(0x0097, &value);                           // read status
    *val = (value & (1 << gpio)) ? 1 : 0;
    return 0;
}

void rn5t618_power_off()
{
    rn5t618_set_gpio(0, 1);
    rn5t618_set_gpio(1, 1);
    udelay(100 * 1000);
    rn5t618_set_bits(0x00EF, 0x00, 0x10);                     // disable coulomb counter
    rn5t618_set_bits(0x00E0, 0x00, 0x01);                     // disable fuel gauge
    DBG("%s, send power off command\n", __func__);
    rn5t618_set_bits(0x000f, 0x00, 0x01);                     // do not re-power-on system
    rn5t618_set_bits(0x000E, 0x01, 0x01);                     // software power off PMU
    udelay(100);
    while (1) {
        udelay(1000 * 1000);
        DBG("%s, error\n", __func__);
    }
}

int rn5t618_set_usb_current_limit(int limit)
{
    int val;
    if ((limit < 0 || limit > 1500) && (limit != -1)) {
       DBG("%s, wrong usb current limit:%d\n", __func__, limit);
       return -1;
    }
    if (limit == -1) {                                       // -1 means not limit, so set limit to max
        limit = 1500;
    }
    val = (limit / 100) - 1;
    DBG("%s, set usb current limit to %d\n", __func__, limit);
    return rn5t618_set_bits(0x00B7, val, 0x1f);
}

int rn5t618_get_charging_percent()
{
    int rest_vol;
    /*
     * TODO, need add code to calculate battery capability when cannot get battery parameter
     */
    return 50;           // test now
}

int rn5t618_set_charging_current(int current)
{
    return 0;
}

int rn5t618_charger_online(void)
{
    int val;
    rn5t618_read(0x00BD, &val);
    if (val & 0xc0) {
        return 1;
    } else {
        return 0;
    }
}

int rn5t618_set_charge_enable(int enable)
{
    int bits = enable ? 0x03 : 0x00;

    return rn5t618_set_bits(0x00B3, bits, 0x03);
}

int rn5t618_set_full_charge_voltage(int voltage)
{
    int bits;

    if (voltage > 4350 * 1000 || voltage < 4050 * 1000) {
        printf("%s, invalid target charge voltage:%d\n", __func__, voltage);
        return -1;
    }
    if (voltage == 4350000) {
        /*
         * If target charge voltage is 4.35v, should close charger first.
         */
        bits = 0x40;
        rn5t618_set_charge_enable(0);
        udelay(50 * 1000);
        rn5t618_set_bits(0x00BB, bits, 0x70);
        return 0;
    } else {
        bits = ((voltage - 4050000) / 50000) << 4;
    }
    return rn5t618_set_bits(0x00BB, bits, 0x70);
}

int rn5t618_set_charge_end_current(int curr)
{
    int bits;

    if (curr < 50000 || curr > 200000) {
        printf("%s, invalid charge end current:%d\n", __func__, curr);
        return -1;
    }
    bits = (curr / 50000 - 1) << 6;
    return rn5t618_set_bits(0x00B8, bits, 0xc0);
}

int rn5t618_set_trickle_time(int minutes)
{
    int bits;

    if (minutes != 40 && minutes != 80) {
        printf("%s, invalid trickle time:%d\n", __func__, minutes);
        return -1;
    }
    bits = (minutes == 40) ? 0x00 : 0x10;
    return rn5t618_set_bits(0x00B9, bits, 0x30);
}

int rn5t618_set_rapid_time(int minutes)
{
    int bits;

    if (minutes > 300 || minutes < 120) {
        printf("%s, invalid rapid charge time:%d\n", __func__, minutes);
        return -1;
    }
    bits = (minutes - 120) / 60;
    return rn5t618_set_bits(0x00B9, bits, 0x03);
}

int rn5t618_set_long_press_time(int ms)
{
    int bits;

    if (ms < 1000 || ms > 12000) {
        printf("%s, invalid long press time:%d\n", __func__, ms);
        return -1;
    }
    switch(ms) {
    case  1000: bits = 0x10; break;
    case  2000: bits = 0x20; break;
    case  4000: bits = 0x30; break;
    case  6000: bits = 0x40; break;
    case  8000: bits = 0x50; break;
    case 10000: bits = 0x60; break;
    case 12000: bits = 0x70; break;
    default : return -1;
    }
    return rn5t618_set_bits(0x0010, bits, 0x70);
}

int rn5t618_set_recharge_voltage(int voltage)
{
    int bits;

    if (voltage < 3850 || voltage > 4100) {
        printf("%s, invalid recharge volatage:%d\n", __func__, voltage);
        return -1;
    }
    if (voltage == 4100) {
        bits = 0x04;
    } else {
        bits = ((voltage - 3850) / 50);
    }
    return rn5t618_set_bits(0x00BB, bits, 0x07);
}

int rn5t618_inti_para(struct battery_parameter *battery)
{
    if (battery) {
        rn5t618_set_full_charge_voltage(battery->pmu_init_chgvol);
        rn5t618_set_charge_end_current (150000);
        rn5t618_set_charging_current   (battery->pmu_init_chgcur);
        rn5t618_set_trickle_time       (battery->pmu_init_chg_pretime);
        rn5t618_set_rapid_time         (battery->pmu_init_chg_csttime);
        rn5t618_set_charge_enable      (battery->pmu_init_chg_enabled);
        rn5t618_set_long_press_time    (battery->pmu_pekoff_time);
        rn5t618_set_recharge_voltage   (4100);
        return 0;
    } else {
        return -1;
    }
}

void dump_pmu_register(void)
{
    uint8_t val[16];
    int     i;
    uint8_t idx_table[] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0xb};

    printf("[RN5T618] DUMP ALL REGISTERS\n");
    for (i = 0; i < sizeof(idx_table); i++) {
        rn5t618_reads(idx_table[i] * 16, val, 16);
        printf("0x%02x - %02x: ", idx_table[i] * 16, idx_table[i] * 16 + 15);
        printf("%02x %02x %02x %02x ",   val[0],  val[1],  val[2],  val[3]);
        printf("%02x %02x %02x %02x   ", val[4],  val[5],  val[6],  val[7]);
        printf("%02x %02x %02x %02x ",   val[8],  val[9],  val[10], val[11]);
        printf("%02x %02x %02x %02x\n",  val[12], val[13], val[14], val[15]);
    }
}

uint8_t mov_hex(uint8_t input)
{
    uint8_t tmp = input & 0xf0;
    tmp = tmp | (tmp >> 4);
    return tmp;
}

int rn5t618_check_fault(void)
{
    uint8_t val;
    int i = 0;;
    char *fault_reason[] = {
        "PWRONPOFF",                        // bit 0
        "abnormal temperature",             // bit 1
        "low power condition in VINDET",    // bit 2
        "IODETPOFF",                        // bit 3
        "CPUPOFF",                          // bit 4
        "watchdog power off",               // bit 5
        "overcurrent of DCDC1-3",           // bit 6
        "N_OEPOF"                           // bit 7
    };

    printf("PMU fault status:\n");
    rn5t618_read(0x000A, &val);
    while (val) {
        if (val & 0x01) {
            printf("-- %s\n", fault_reason[i]);
        }
        i++;
        val >>= 1;
    }
    return 0;
}

int rn5t618_init(void)
{
    uint8_t buf[10];

#ifdef CONFIG_DISABLE_POWER_KEY_OFF
    rn5t618_set_bits(0x0010, 0x80, 0x80);                       // disable power off PMU by long press power key
#endif
    rn5t618_check_fault();
    rn5t618_write(0x04, 0xA5);
    rn5t618_read(0x009A, buf);
    printf("reg[0x9A] = 0x%02x\n", buf[0]);
    if ((buf[0] &0xf0) != 0xf0) {
        rn5t618_set_bits(0x9A, 0x00, 0x10);                     // change GPIO3 from PSO to GPIO function
    }

    rn5t618_write(0x0066, 0x00);                                // clear ADRQ to stop ADC
    rn5t618_set_bits(0x0064, 0x2f, 0x2f);                       // open VBAT and IBAT adc to auto-mode
    rn5t618_set_bits(0x0065, 0x00, 0x07);                       // set AUTO_TIMER to 250 ms
    rn5t618_set_bits(0x0066, 0x28, 0x38);                       // auto mode of ADC, 4 average of each sample
    rn5t618_set_bits(0x00B3, 0x03, 0x03);                       // enable charge of vbus/dcin
    rn5t618_set_bits(0x00BB, 0x34, 0x77);                       // charge target voltage to 4.2v, recharge-voltage to 4.1v
    rn5t618_set_bits(0x002C, 0x10, 0x30);                       // DCDC1 set to PWM mode
    rn5t618_set_bits(0x002E, 0x10, 0x30);                       // DCDC2 set to PWM mode
    rn5t618_set_bits(0x0030, 0x10, 0x30);                       // DCDC3 set to PWM mode
    rn5t618_set_bits(0x002F, 0x40, 0xc0);                       // DCDC2 frequent set to 2.2MHz
    rn5t618_set_gpio(3, 0);                                     // open GPIO 2, DCDC 3.3
    rn5t618_set_gpio(1, 0);                                     // close GPIO 1, boost 5V
    rn5t618_set_bits(0x00EF, 0x10, 0x10);                       // enable coulomb counter
    rn5t618_set_bits(0x0010, 0x60, 0x70);                       // set long-press timeout to 10s
//  rn5t618_set_bits(0x00E0, 0x01, 0x01);                       // enable Fuel gauge

    /*
     * configure sleep sequence
     */
    rn5t618_reads(0x0016, buf, 3);
    buf[0] = mov_hex(buf[0]);                                   // sleep sequence for DC1
    buf[1] = mov_hex(buf[1]);                                   // sleep sequence for DC2
    buf[2] = mov_hex(buf[2]);                                   // sleep sequence for DC3
    rn5t618_writes(0x0016, buf, 3);
    rn5t618_reads(0x001c, buf, 4);
    buf[0] = mov_hex(buf[0]);                                   // sleep sequence for LDO1
    buf[1] = mov_hex(buf[1]);                                   // sleep sequence for LDO2
    buf[2] = mov_hex(buf[2]);                                   // sleep sequence for LDO3
    buf[3] = mov_hex(buf[3]);                                   // sleep sequence for LDO4
    rn5t618_writes(0x001c, buf, 4);
    rn5t618_reads(0x002A, buf, 1);
    buf[0] = mov_hex(buf[0]);                                   // sleep sequence for LDORTC1
    rn5t618_writes(0x002A, buf, 1);

    rn5t618_write(0x0013, 0x00);                                // clear watch dog IRQ

    rn5t618_set_bits(0x00B3, 0x00, 0x40);                       // enable rapid-charge to charge end

    rn5t618_set_bits(0x00BC, 0x01, 0x03);                       // set DIE shut temp to 120 Celsius
    rn5t618_set_bits(0x00BA, 0x00, 0x0c);                       // set VWEAK to 3.0V
    rn5t618_set_bits(0x00BB, 0x00, 0x80);                       // set VWEAK to 3.0v
    udelay(100 * 1000);                                         // delay a short time

//    dump_pmu_register();

    return 0;
}

struct aml_pmu_driver g_aml_pmu_driver = {
    .pmu_init                    = rn5t618_init,
    .pmu_get_battery_capacity    = rn5t618_get_charging_percent,
    .pmu_get_extern_power_status = rn5t618_charger_online,
    .pmu_set_gpio                = rn5t618_set_gpio,
    .pmu_get_gpio                = rn5t618_get_gpio,
    .pmu_reg_read                = rn5t618_read,
    .pmu_reg_write               = rn5t618_write,
    .pmu_reg_reads               = rn5t618_reads,
    .pmu_reg_writes              = rn5t618_writes,
    .pmu_set_bits                = rn5t618_set_bits,
    .pmu_set_usb_current_limit   = rn5t618_set_usb_current_limit,
    .pmu_set_charge_current      = rn5t618_set_charging_current,
    .pmu_power_off               = rn5t618_power_off,
    .pmu_init_para               = rn5t618_inti_para,
};

struct aml_pmu_driver* aml_pmu_get_driver(void)
{
    return &g_aml_pmu_driver;
}
