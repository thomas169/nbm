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

#include <stdint.h>
#include <stdbool.h>

/* todo: implent use of these, currently makes no difference */
enum NbmDevices { 
    NBM5100A = 1,
    NBM5100B = 2,
    NBM7100A = 4,
    NBM7100B = 8
};

/* todo: add more and actually use them */
enum NbmErrors { 
    NBM_ERROR_NO_ERROR = 0,
    NBM_ERROR_IO_ERROR = 1, // must be 1, as IO call return bool with 1 as failure
    NBM_ERROR_INVALID_VALUE = 2,
    NBM_ERROR_NOT_WRITEABLE = 4,
    NMB_ERROR_NOT_INITALISED = 8,
    NMB_ERROR_INVALID_REGISTER = 16,
    NMB_ERROR_INVALID_FIELD = 32
};

enum NbmI2cAddr { 
    NBM_I2C_ADDR_SPI = 0,
    NBM_I2C_ADDR_0x2E = 0x2E,
    NBM_I2C_ADDR_0x2F = 0x2F
};

#define NBM_FORM_FIELD_VALUE(reg_no, dev_mask, msb_bit_pos, lsb_bit_pos, solo_in_reg, writeable)  \
    ((((reg_no) & 0xF) << 12) | (((msb_bit_pos) & 0xF) << 8) | (((msb_bit_pos) & 0x7) << 5) | \
    (((lsb_bit_pos) & 0x7) << 2) | (((solo_in_reg) & 0x1) << 1) | (((writeable) & 0x1) << 0))

#define DEVICE_NBM_ALL (NBM5100A | NBM5100B | NBM7100A | NBM7100B)
#define DEVICE_5100_SERIES  (NBM5100A | NBM5100B)
#define DEVICE_7100_SERIES (NBM7100A | NBM7100B)
#define DEVICE_I2C_SERIES (NBM5100A | NBM7100A)
#define DEVICE_SPI_SERIES (NBM5100B | NBM7100B)

/* will be 1 byte */
enum NbmRegisters {
    nbm_reg_status = 0,
    nbm_reg_chenergy1 = 1,
    nbm_reg_chenergy2 = 2,
    nbm_reg_chenergy3 = 3,
    nbm_reg_chenergy4 = 4,
    nbm_reg_vcap = 5,
    nbm_reg_vchend = 6,
    nbm_reg_profile_msb = 7,
    nbm_reg_command = 8,
    nbm_reg_set1 = 9,
    nbm_reg_set2 = 10,
    nbm_reg_set3 = 11,
    nbm_reg_set4 = 12,
    nbm_reg_set5 = 13
};

/* will be 2 bytes */
enum NbmFields {
    nbm_field_lowbat = NBM_FORM_FIELD_VALUE(nbm_reg_status, DEVICE_NBM_ALL, 7, 7, 0, 0),
    nbm_field_ew = NBM_FORM_FIELD_VALUE(nbm_reg_status, DEVICE_NBM_ALL, 6, 6, 0, 0),
    nbm_field_alrm = NBM_FORM_FIELD_VALUE(nbm_reg_status, DEVICE_NBM_ALL, 5, 5, 0, 0),
    nbm_field_rdy = NBM_FORM_FIELD_VALUE(nbm_reg_status, DEVICE_NBM_ALL, 0, 0, 0, 0),
    nbm_field_chengy = NBM_FORM_FIELD_VALUE(nbm_reg_chenergy1, DEVICE_NBM_ALL, 7, 0, 1, 0),
    nbm_field_vcap = NBM_FORM_FIELD_VALUE(nbm_reg_vcap, DEVICE_NBM_ALL, 4, 0, 1, 0),
    nbm_field_vchend = NBM_FORM_FIELD_VALUE(nbm_reg_vchend, DEVICE_NBM_ALL, 4, 0, 1, 0),
    nbm_field_prof = NBM_FORM_FIELD_VALUE(nbm_reg_command, DEVICE_NBM_ALL, 7, 4, 0, 1), /* using command reg part */
    nbm_field_rstpf = NBM_FORM_FIELD_VALUE(nbm_reg_command, DEVICE_NBM_ALL, 3, 3, 0, 1),
    nbm_field_act = NBM_FORM_FIELD_VALUE(nbm_reg_command, DEVICE_NBM_ALL, 2, 2, 0, 1),
    nbm_field_ecm = NBM_FORM_FIELD_VALUE(nbm_reg_command, DEVICE_NBM_ALL, 1, 1, 0, 1),
    nbm_field_eod = NBM_FORM_FIELD_VALUE(nbm_reg_command, DEVICE_NBM_ALL, 0, 0, 0, 1),
    nbm_field_vfix = NBM_FORM_FIELD_VALUE(nbm_reg_set1, DEVICE_NBM_ALL, 7, 4, 0, 1),
    nbm_field_vset = NBM_FORM_FIELD_VALUE(nbm_reg_set1, DEVICE_NBM_ALL, 3, 0, 0, 1),
    nbm_field_ich = NBM_FORM_FIELD_VALUE(nbm_reg_set2, DEVICE_NBM_ALL, 7, 5, 0, 1),
    nbm_field_vdhhiz = NBM_FORM_FIELD_VALUE(nbm_reg_set2, DEVICE_NBM_ALL, 4, 4, 0, 1),
    nbm_field_vmin = NBM_FORM_FIELD_VALUE(nbm_reg_set2, DEVICE_NBM_ALL, 2, 0, 0, 1),
    nbm_field_automode = NBM_FORM_FIELD_VALUE(nbm_reg_set3, DEVICE_NBM_ALL, 7, 7, 0, 1),
    nbm_field_eew = NBM_FORM_FIELD_VALUE(nbm_reg_set3, DEVICE_NBM_ALL, 4, 4, 0, 1),
    nbm_field_vew = NBM_FORM_FIELD_VALUE(nbm_reg_set3, DEVICE_NBM_ALL, 3, 0, 0, 1),
    nbm_field_balmode = NBM_FORM_FIELD_VALUE(nbm_reg_set4, DEVICE_NBM_ALL, 7, 6, 0, 1),
    nbm_field_enbal = NBM_FORM_FIELD_VALUE(nbm_reg_set4, DEVICE_NBM_ALL, 5, 5, 0, 1),
    nbm_field_vcapmax = NBM_FORM_FIELD_VALUE(nbm_reg_set4, DEVICE_NBM_ALL, 4, 4, 0, 1),
    nbm_field_opt_marg = NBM_FORM_FIELD_VALUE(nbm_reg_set5, DEVICE_NBM_ALL, 1, 0, 1, 1)
};

/* clear this macro from namespace as wont be needed anymore */
#undef NBM_FORM_FIELD_VALUE

/* defines for all of the values we can set, the form of each term
 * is comprised of: NBM_{REG_NAME}_VAL_{DESC} where:
 *  REG_NAME: shorthand reigster name
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
#define NBM_VSET_VAL_3V0 9
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

#define NBM_VCAP_VAL_1mA10 27
#define NBM_VCAP_VAL_2mA30 28
#define NBM_VCAP_VAL_3mA15 29
#define NBM_VCAP_VAL_4mA90 30

#define NBM_ENBAL_VAL_INACTIVE 0
#define NBM_ENBAL_VAL_ACTIVE 1

/* main user faceing datatype NbmDevice */
struct NbmDevice {
    enum NbmDevices device_type;
    enum NbmErrors error_code;
    enum NbmI2cAddr i2c_addr;
    /* user defined so will be SPI or I2C calls to MCU */
    bool (*write_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len);
    bool (*read_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len);
    /* user defined if null will not be used */
    void (*on_error_callback)(uint8_t error_code);
};

/* init fcn for the nbm, user passes the two callbacks and devices type */
void nbm_init(struct NbmDevice *dev, enum NbmDevices device_type, enum NbmI2cAddr i2c_addr,
    bool (*write_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len),
    bool (*read_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len),
    void (*on_error_callback)(uint8_t error_code));

/* now the useful functions */
void nbm_write_field(struct NbmDevice *dev, enum NbmFields field, uint8_t value);
void nbm_read_field(struct NbmDevice *dev, enum NbmFields field, void *value);
void nbm_read_reg(struct NbmDevice *dev, enum NbmRegisters, uint8_t *value, uint8_t size);
void nbm_write_reg(struct NbmDevice *dev, enum NbmRegisters, const uint8_t *value, uint8_t size);
