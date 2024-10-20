/* 
 * a platform agnostic library for the lovely nbmx100x battery managment/booster 
 * devices from nexperia, written in ANSI C.
 * by thomas169
 *
 * provided as is blah blah blah... if your battery blows up dont come looking 
 * for me!
 * 
 * SPDX-License-Identifier: Apache-2.0 
 */

#ifndef NBM_H_
#define NBM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* todo: implent use of these, currently makes no difference */
enum nbm_types { 
    NBM5100A = 1,
    NBM5100B = 2,
    NBM7100A = 4,
    NBM7100B = 8
};

/* todo: add more and actually use them */
enum nbm_errors { 
    NBM_ERROR_NO_ERROR = 0,
    NBM_ERROR_IO_ERROR = 1, /* must be 1, as IO call return bool with 1 as failure */
    NBM_ERROR_INVALID_VALUE = 2,
    NBM_ERROR_NOT_WRITEABLE = 4,
    NBM_ERROR_NOT_INITALISED = 8,
    NBM_ERROR_INVALID_REGISTER = 16,
    NBM_ERROR_INVALID_FIELD = 32,
    NBM_ERROR_INVALID_DEVICE = 64
};

union nbm_addr {
    enum nbm_i2c_addr { 
        NBM_I2C_ADDR_0x2E = 0x2E,
        NBM_I2C_ADDR_0x2F = 0x2F
    } i2c_addr;
    uint8_t spi_ss_gpio;
};

#define NBM_FORM_FIELD_VALUE(reg_no, dev_mask, msb_bit_pos, lsb_bit_pos, solo_in_reg, writeable)  \
    ((((reg_no) & 0xF) << 12) | (((msb_bit_pos) & 0xF) << 8) | (((msb_bit_pos) & 0x7) << 5) | \
    (((lsb_bit_pos) & 0x7) << 2) | (((solo_in_reg) & 0x1) << 1) | (((writeable) & 0x1) << 0))

#define DEVICE_NBM_ALL (NBM5100A | NBM5100B | NBM7100A | NBM7100B)
#define DEVICE_5100_SERIES (NBM5100A | NBM5100B)
#define DEVICE_7100_SERIES (NBM7100A | NBM7100B)
#define DEVICE_I2C_SERIES (NBM5100A | NBM7100A)
#define DEVICE_SPI_SERIES (NBM5100B | NBM7100B)

/* will be 1 byte */
enum nbm_registers {
    NBM_REG_STATUS = 0,
    NBM_REG_CHENERGY1 = 1,
    NBM_REG_CHENERGY2 = 2,
    NBM_REG_CHENERGY3 = 3,
    NBM_REG_CHENERGY4 = 4,
    NBM_REG_VCAP = 5,
    NBM_REG_VCHEND = 6,
    NBM_REG_PROFILE_MSB = 7,
    NBM_REG_COMMAND = 8,
    NBM_REG_SET1 = 9,
    NBM_REG_SET2 = 10,
    NBM_REG_SET3 = 11,
    NBM_REG_SET4 = 12,
    NBM_REG_SET5 = 13
};

/* Will be 2 bytes */
enum nbm_fields {
    NBM_LOWBAT = NBM_FORM_FIELD_VALUE(NBM_REG_STATUS, DEVICE_NBM_ALL, 7, 7, 0, 0),
    NBM_EW = NBM_FORM_FIELD_VALUE(NBM_REG_STATUS, DEVICE_NBM_ALL, 6, 6, 0, 0),
    NBM_ALRM = NBM_FORM_FIELD_VALUE(NBM_REG_STATUS, DEVICE_NBM_ALL, 5, 5, 0, 0),
    NBM_RDY = NBM_FORM_FIELD_VALUE(NBM_REG_STATUS, DEVICE_NBM_ALL, 0, 0, 0, 0),
    NBM_CHENGY = NBM_FORM_FIELD_VALUE(NBM_REG_CHENERGY1, DEVICE_NBM_ALL, 7, 0, 1, 0),
    NBM_VCAP = NBM_FORM_FIELD_VALUE(NBM_REG_VCAP, DEVICE_NBM_ALL, 4, 0, 1, 0),
    NBM_VCHEND = NBM_FORM_FIELD_VALUE(NBM_REG_VCHEND, DEVICE_NBM_ALL, 4, 0, 1, 0),
    NBM_PROF = NBM_FORM_FIELD_VALUE(NBM_REG_COMMAND, DEVICE_NBM_ALL, 7, 4, 0, 1), /* Using command reg part */
    NBM_RSTPF = NBM_FORM_FIELD_VALUE(NBM_REG_COMMAND, DEVICE_NBM_ALL, 3, 3, 0, 1),
    NBM_ACT = NBM_FORM_FIELD_VALUE(NBM_REG_COMMAND, DEVICE_NBM_ALL, 2, 2, 0, 1),
    NBM_ECM = NBM_FORM_FIELD_VALUE(NBM_REG_COMMAND, DEVICE_NBM_ALL, 1, 1, 0, 1),
    NBM_EOD = NBM_FORM_FIELD_VALUE(NBM_REG_COMMAND, DEVICE_NBM_ALL, 0, 0, 0, 1),
    NBM_VFIX = NBM_FORM_FIELD_VALUE(NBM_REG_SET1, DEVICE_NBM_ALL, 7, 4, 0, 1),
    NBM_VSET = NBM_FORM_FIELD_VALUE(NBM_REG_SET1, DEVICE_NBM_ALL, 3, 0, 0, 1),
    NBM_ICH = NBM_FORM_FIELD_VALUE(NBM_REG_SET2, DEVICE_NBM_ALL, 7, 5, 0, 1),
    NBM_VDHHIZ = NBM_FORM_FIELD_VALUE(NBM_REG_SET2, DEVICE_NBM_ALL, 4, 4, 0, 1),
    NBM_VMIN = NBM_FORM_FIELD_VALUE(NBM_REG_SET2, DEVICE_NBM_ALL, 2, 0, 0, 1),
    NBM_AUTOMODE = NBM_FORM_FIELD_VALUE(NBM_REG_SET3, DEVICE_I2C_SERIES, 7, 7, 0, 1),
    NBM_EEW = NBM_FORM_FIELD_VALUE(NBM_REG_SET3, DEVICE_NBM_ALL, 4, 4, 0, 1),
    NBM_VEW = NBM_FORM_FIELD_VALUE(NBM_REG_SET3, DEVICE_NBM_ALL, 3, 0, 0, 1),
    NBM_BALMODE = NBM_FORM_FIELD_VALUE(NBM_REG_SET4, DEVICE_5100_SERIES, 7, 6, 0, 1),
    NBM_ENBAL = NBM_FORM_FIELD_VALUE(NBM_REG_SET4, DEVICE_5100_SERIES, 5, 5, 0, 1),
    NBM_VCAPMAX = NBM_FORM_FIELD_VALUE(NBM_REG_SET4, DEVICE_NBM_ALL, 4, 4, 0, 1),
    NBM_OPT_MARG = NBM_FORM_FIELD_VALUE(NBM_REG_SET5, DEVICE_NBM_ALL, 1, 0, 1, 1)
};

/* clear this macro from namespace as wont be needed anymore */
#undef NBM_FORM_FIELD_VALUE

/* defines for all of the values we can set, the form of each term
 * is comprised of: NBM_{FIELD_NAME}_VAL_{DESC} where:
 *  FIELD_NAME: shorthand field name
 *  DESC: helpful (?) description of value 
 * use of these inplace of actual values is strongly encouraged. */
#define NBM_LOWBAT_VAL_VBAT_LOW 1
#define NBM_LOWBAT_VAL_VBAT_GOOD 0

#define NBM_EW_VAL_VCAP_LOW 1
#define NBM_EW_VAL_VCAP_GOOD 0

#define NBM_ALRM_VAL_ILOAD_GOOD 0
#define NBM_ALRM_VAL_ILOAD_TO_HIGH 1

#define NBM_RDY_VAL_CAP_CHARGED 1
#define NBM_RDY_VAL_CAP_NOT_CHARGED_OR_RESET 0

#define NBM_EOD_VAL_ON_DEMAND_ENABLE 1
#define NBM_EOD_VAL_ON_DEMAND_INACTIVE 0

#define NBM_ECM_VAL_CONTINUOUS_MODE_ENABLE 1
#define NBM_ECM_VAL_CONTINUOUS_MODE_INACTIVE 0

#define NBM_ACT_VAL_FORCE_ACTIVE_ENABLE 1
#define NBM_ACT_VAL_FORCE_ACTIVE_INACTIVE 0

#define NBM_RSTPF_VAL_RESET_PROFILER_INACTIVE 0
#define NBM_RSTPF_VAL_RESET_PROFILER_ACTIVE 1

#define NBM_AUTOMODE_VAL_AUTOMODE_INACTIVE 0
#define NBM_AUTOMODE_VAL_AUTOMODE_ACTIVE 1

#define NBM_VSET_VAL_1V8 0
#define NBM_VSET_VAL_2V0 1
#define NBM_VSET_VAL_2V2 2
#define NBM_VSET_VAL_2V4 3
#define NBM_VSET_VAL_2V5 4
#define NBM_VSET_VAL_2V6 5
#define NBM_VSET_VAL_2V7 6
#define NBM_VSET_VAL_2V8 7
#define NBM_VSET_VAL_2V9 8
#define NBM_VSET_VAL_3V0 9 /* default */
#define NBM_VSET_VAL_3V1 10
#define NBM_VSET_VAL_3V2 11
#define NBM_VSET_VAL_3V3 12
#define NBM_VSET_VAL_3V4 13
#define NBM_VSET_VAL_3V5 14
#define NBM_VSET_VAL_3V6 15

#define NBM_VFIX_VAL_2V60 3
#define NBM_VFIX_VAL_2V95 4
#define NBM_VFIX_VAL_3V27 5
#define NBM_VFIX_VAL_3V57 6
#define NBM_VFIX_VAL_3V84 7
#define NBM_VFIX_VAL_4V10 8
#define NBM_VFIX_VAL_4V33 9
#define NBM_VFIX_VAL_4V55 10
#define NBM_VFIX_VAL_4V76 11
#define NBM_VFIX_VAL_4V96 12
#define NBM_VFIX_VAL_5V16 13
#define NBM_VFIX_VAL_5V34 14
#define NBM_VFIX_VAL_5V54 15

#define NBM_VCAPMAX_VAL_4V95 0
#define NBM_VCAPMAX_VAL_5V54 1

#define NBM_VMIN_VAL_2V4 0
#define NBM_VMIN_VAL_2V6 1
#define NBM_VMIN_VAL_2V8 2
#define NBM_VMIN_VAL_3V0 3
#define NBM_VMIN_VAL_3V2 4

#define NBM_ICH_VAL_2mA 0
#define NBM_ICH_VAL_4mA 1
#define NBM_ICH_VAL_8mA 2
#define NBM_ICH_VAL_16mA 3
#define NBM_ICH_VAL_50mA 4

#define NBM_VEW_VAL_2V4 0
#define NBM_VEW_VAL_2V6 1
#define NBM_VEW_VAL_2V8 2
#define NBM_VEW_VAL_3V0 3
#define NBM_VEW_VAL_3V2 4
#define NBM_VEW_VAL_3V4 5
#define NBM_VEW_VAL_3V6 6
#define NBM_VEW_VAL_3V84 7
#define NBM_VEW_VAL_4V1 8
#define NBM_VEW_VAL_4V3 9

#define NBM_VDH_VAL_VDH_ALWAYS_ON 0
#define NBM_VDH_VAL_VDH_HIZ 1

#define NBM_PROF_VAL_NO_OPTIMISER 0
/* cba writing out 64 terms for the profile */
#define NBM_PROF_VAL_PROFILE(x) \
    ((uint8_t)(((uint8_t)(x)) > 63 ? 63 : ((uint8_t)(x)) < 1 ? 1 : (x)))

#define NBM_OPT_MARG_VAL_INACITVE 0
#define NBM_OPT_MARG_VAL_2V19 1
#define NBM_OPT_MARG_VAL_2V60 2
#define NBM_OPT_MARG_VAL_2V95 3

#define NBM_VCAP_VAL_SUB_1V1_A 0
#define NBM_VCAP_VAL_SUB_1V1_B 1
#define NBM_VCAP_VAL_SUB_1V1_C 2
#define NBM_VCAP_VAL_1V10 3
#define NBM_VCAP_VAL_1V20 4
#define NBM_VCAP_VAL_1V30 5
#define NBM_VCAP_VAL_1V40 6
#define NBM_VCAP_VAL_1V51 7
#define NBM_VCAP_VAL_1V60 8
#define NBM_VCAP_VAL_1V71 9
#define NBM_VCAP_VAL_1V81 10
#define NBM_VCAP_VAL_1V99 11
#define NBM_VCAP_VAL_2V19 12
#define NBM_VCAP_VAL_2V40 13
#define NBM_VCAP_VAL_2V60 14
#define NBM_VCAP_VAL_2V79 15
#define NBM_VCAP_VAL_2V95 16
#define NBM_VCAP_VAL_3V01 17
#define NBM_VCAP_VAL_3V20 18
#define NBM_VCAP_VAL_3V27 19
#define NBM_VCAP_VAL_3V41 20
#define NBM_VCAP_VAL_3V57 21
#define NBM_VCAP_VAL_3V61 22
#define NBM_VCAP_VAL_3V84 23
#define NBM_VCAP_VAL_4V10 24
#define NBM_VCAP_VAL_4V33 25
#define NBM_VCAP_VAL_4V55 26
#define NBM_VCAP_VAL_4V76 27
#define NBM_VCAP_VAL_4V95 28
#define NBM_VCAP_VAL_5V16 29
#define NBM_VCAP_VAL_5V34 30
#define NBM_VCAP_VAL_5V54 31

#define NBM_BALMODE_VAL_1mA10 0
#define NBM_BALMODE_VAL_2mA30 1
#define NBM_BALMODE_VAL_3mA15 2
#define NBM_BALMODE_VAL_4mA90 3

#define NBM_ENBAL_VAL_INACTIVE 0
#define NBM_ENBAL_VAL_ACTIVE 1

/* main user facing datatype NbmDevice */
struct nbm_device {
    enum nbm_types device_type;
    enum nbm_errors error_code;
    union nbm_addr addr;
    /* user defined so will be SPI or I2C calls to MCU */
    bool (*write_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len);
    bool (*read_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len);
    /* user defined functions to read/write the two gpio pins */
    bool (*read_ready_pin_fcn)(void* pin, bool *state);
    bool (*write_start_pin_fcn)(void* pin, bool state);
    /* user defined if null will not be used */
    void (*on_error_callback)(uint8_t error_code);
};

/* init fcn for the nbm, user passes the callbacks and devices settings */
void nbm_init(struct nbm_device *dev, enum nbm_types device_type, uint8_t addr,
    bool (*write_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len),
    bool (*read_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len),
    void (*on_error_callback)(uint8_t error_code));

/* now the useful functions */
void nbm_write(struct nbm_device *dev, enum nbm_fields field, uint8_t value);
void nbm_read(struct nbm_device *dev, enum nbm_fields field, void *value);
void nbm_read_reg(struct nbm_device *dev, enum nbm_registers, uint8_t *value, uint8_t size);
void nbm_write_reg(struct nbm_device *dev, enum nbm_registers, const uint8_t *value, uint8_t size);
void nbm_read_ready(struct nbm_device *dev, bool *value);
void nbm_write_start(struct nbm_device *dev, bool *value);

/* some helpers for voltage comparisons you are likely to use */
uint16_t nbm_vfix_to_mv(uint8_t vfix);
uint16_t nbm_vcap_to_mv(uint8_t vcap);
uint16_t nbm_vcapmax_to_mv(uint8_t vcapmax);

#ifdef __cplusplus
}
#endif

#endif /* include guard */

