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

#include "nbm.h"

#define GET_REG_FROM_FIELD(field) ((field) >> 12 & 0xF)
#define GET_MSB_POS_FROM_FIELD(field) ((field) >> 5 & 0x7)
#define GET_LSB_POS_FROM_FIELD(field) ((field) >> 2 & 0x7)
#define GET_LENGTH_FROM_FIELD(field) (GET_MSB_POS_FROM_FIELD(field) - GET_LSB_POS_FROM_FIELD(field) + 1)
#define GET_VALUE_MASK_FROM_FIELD(field) ((1 << GET_LENGTH_FROM_FIELD(field)) - 1)
#define GET_MASK_FROM_FIELD(field) (GET_VALUE_MASK_FROM_FIELD(field) << GET_LSB_POS_FROM_FIELD(field))
#define GET_DEVICE_FROM_FIELD(field) ((field) >> 2 & 0x7)
#define GET_SOLO_IN_REG_FROM_FIELD(field) ((field) >> 1 & 0x1)
#define GET_WRITEABLE_FIELD(field) ((field) >> 0 & 0x1)

#define GET_ADDR(dev) \
    (__builtin_parity((dev)->device_type & DEVICE_I2C_SERIES) ? \
            (dev)->addr.i2c_addr : (dev)->addr.spi_ss_gpio)

#define CHECK_DEVICE_FIELD(dev, field) \
    if (!(GET_DEVICE_FROM_FIELD(field) & (dev)->device_type)) \
         (dev)->error_code |= NBM_ERROR_INVALID_DEVICE

#define ERROR_CHECK(dev) \
    if ((dev)->error_code && (dev)->on_error_callback) \
        (dev)->on_error_callback((dev)->error_code)

#define SET_ERROR_AND_RUN_CALLBACK(dev, erno) \
    do { \
        (dev)->error_code |= (erno); \
        ERROR_CHECK(dev); \
    } while(0)


/* local only fuctions, not exposed on api */
static bool nbm_can_write_reg(enum nbm_registers reg);
static bool nbm_is_valid_reg(enum nbm_registers reg);
static bool nbm_is_valid_field(enum nbm_fields field);

void nbm_init(struct nbm_device *dev, enum nbm_types device_type, uint8_t addr,
            bool (*write_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len),
            bool (*read_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len),
            void (*on_error_callback)(uint8_t error_code)) {

    dev->error_code = NBM_ERROR_NO_ERROR;
    
    if (device_type == NBM5100A || device_type == NBM5100B || device_type == NBM7100A || device_type == NBM7100B)
        dev->device_type = device_type;
    else 
        dev->error_code |= NBM_ERROR_INVALID_VALUE;

    if (device_type == NBM5100A || device_type == NBM7100A)
        dev->addr.i2c_addr = addr;
    else
        dev->addr.spi_ss_gpio = addr;

    /* write_bytes_fcn and read_bytes_fcn are required */
    if (write_bytes_fcn)
        dev->write_bytes_fcn = write_bytes_fcn;
    else
        dev->error_code |= NBM_ERROR_NOT_INITALISED;

    if (read_bytes_fcn)
        dev->read_bytes_fcn = read_bytes_fcn;
    else
        dev->error_code |= NBM_ERROR_NOT_INITALISED;
    
    /* on_error_callback is optional, pass NULL to not use */
    dev->on_error_callback = on_error_callback;

    ERROR_CHECK(dev);

}

void nbm_write(struct nbm_device *dev, enum nbm_fields field, uint8_t value) {
    /* as we cannot write chenergy register all writes to a field will be 1 byte
     * long, however the prof field is split over two registers (grrr) */
    
    enum nbm_registers reg;
    uint8_t tmp;
    uint8_t masked_value;

    reg = GET_REG_FROM_FIELD(field);
    
    if (!nbm_can_write_reg(reg)){
        SET_ERROR_AND_RUN_CALLBACK(dev, NBM_ERROR_NOT_WRITEABLE);
        return;
    }

    if (!nbm_is_valid_field(field)) {
        SET_ERROR_AND_RUN_CALLBACK(dev, NBM_ERROR_INVALID_FIELD);
        return;
    }

    /* sanitise the value and return error if value not valid, dont do it for the
     * the prof field however as mask will be wrong if upper 2 bits are set */
    masked_value = value & GET_VALUE_MASK_FROM_FIELD(field);

    if (field == NBM_PROF) {
        value = value >> 0x4 & 0x3;
        /* no validity checks for value of prof, value has the two msb bits, these
         * are in the profile_msb register and solo in reg so no pre read needed */
    } else if (value != masked_value) {
        SET_ERROR_AND_RUN_CALLBACK(dev, NBM_ERROR_INVALID_VALUE);
        return;
    }
    volatile uint8_t c;
    /* if the field is alone in the given register we avoid the need to read it
     * before any writes, and subsquent faffing around with bit shifting. */
    if (!GET_SOLO_IN_REG_FROM_FIELD(field)) {
        nbm_read_reg(dev, reg, &tmp, 1);
        c  = ~GET_MASK_FROM_FIELD(field);
        tmp &= ~GET_MASK_FROM_FIELD(field);
        masked_value <<= GET_LSB_POS_FROM_FIELD(field);
        masked_value |= tmp;
    }

    /* Note there is no special handling of chenergy as its not writable */
    switch (field) {
        case NBM_PROF:
            dev->error_code |= dev->write_bytes_fcn(GET_ADDR(dev), NBM_REG_PROFILE_MSB, &value, 1);
            dev->error_code |= dev->write_bytes_fcn(GET_ADDR(dev), reg, &masked_value, 1);
            break;
        default:
            dev->error_code |= dev->write_bytes_fcn(GET_ADDR(dev), reg, &masked_value, 1);
    }
    ERROR_CHECK(dev);
}

void nbm_read(struct nbm_device *dev, enum nbm_fields field, void *value) {
    /* value 1 byte long unless reading nbm_chengy in which case 4 */

    uint8_t tmp;

    if (!nbm_is_valid_field(field)) {
        SET_ERROR_AND_RUN_CALLBACK(dev, NBM_ERROR_INVALID_FIELD);
        return;
    }

    switch (field) {
        case NBM_PROF:
            dev->error_code |= dev->read_bytes_fcn(GET_ADDR(dev), NBM_REG_PROFILE_MSB, &tmp, 1);
            dev->error_code |= dev->read_bytes_fcn(GET_ADDR(dev), GET_REG_FROM_FIELD(field), (uint8_t*) value, 1);
            /* read bottom 2 bits of tmp and shift above top 4 bits of value */
            (*(uint8_t*)value) = ((tmp & 0x3) << 0x4) |
                ((*(uint8_t*)value) >> GET_LSB_POS_FROM_FIELD(field) & GET_MASK_FROM_FIELD(field));
            break;
        case NBM_CHENGY:
            dev->error_code |= dev->read_bytes_fcn(GET_ADDR(dev), NBM_REG_CHENERGY1, (uint8_t*) value, 4);
            break;
        default:
            dev->error_code |= dev->read_bytes_fcn(GET_ADDR(dev), GET_REG_FROM_FIELD(field), (uint8_t*) value, 1);
            /* shift the lsb to 0 index and mask anything above top bit of field */
            (*(uint8_t*)value) = 
                (*(uint8_t*)value) >> GET_LSB_POS_FROM_FIELD(field) & GET_VALUE_MASK_FROM_FIELD(field);
    }
    ERROR_CHECK(dev);
}

void nbm_read_reg(struct nbm_device *dev, enum nbm_registers reg, uint8_t *value, uint8_t size) {
    
    if (!nbm_is_valid_reg(reg)) {
        SET_ERROR_AND_RUN_CALLBACK(dev, NBM_ERROR_INVALID_REGISTER);
        return;
    }

    dev->error_code |= dev->read_bytes_fcn(GET_ADDR(dev), reg, value, size);
    ERROR_CHECK(dev);
}


void nbm_write_reg(struct nbm_device *dev, enum nbm_registers reg, const uint8_t *value, uint8_t size) {
        
    if (!nbm_is_valid_reg(reg)) {
        SET_ERROR_AND_RUN_CALLBACK(dev, NBM_ERROR_INVALID_REGISTER);
        return;
    }

    if (!nbm_can_write_reg(reg)) {
        SET_ERROR_AND_RUN_CALLBACK(dev, NBM_ERROR_NOT_WRITEABLE);
        return;
    }

    dev->error_code |= dev->write_bytes_fcn(GET_ADDR(dev), reg, value, size);
    ERROR_CHECK(dev);
}

void nbm_read_ready(struct nbm_device *dev, bool *value) {
    dev->error_code |= dev->read_ready_pin_fcn(0, value);
}

void nbm_write_start(struct nbm_device *dev, bool *value) {
    dev->error_code |= dev->write_start_pin_fcn(0, value);
}

/* NOTE: in following do not use default when switching on enums. If we avoid
 * it's use, missing enum values in the switch block will be flagged. */

static bool nbm_can_write_reg(enum nbm_registers reg) {
    switch (reg) {
        case NBM_REG_STATUS:
        case NBM_REG_CHENERGY1:
        case NBM_REG_CHENERGY2:
        case NBM_REG_CHENERGY3:
        case NBM_REG_CHENERGY4:
        case NBM_REG_VCAP:
        case NBM_REG_VCHEND:
            return 0;
        case NBM_REG_PROFILE_MSB:
        case NBM_REG_COMMAND:
        case NBM_REG_SET1:
        case NBM_REG_SET2:
        case NBM_REG_SET3:
        case NBM_REG_SET4:
        case NBM_REG_SET5:
            return 1;
    }
    return 0;
}

static bool nbm_is_valid_reg(enum nbm_registers reg) {
    switch (reg) {
        case NBM_REG_STATUS:
        case NBM_REG_CHENERGY1:
        case NBM_REG_CHENERGY2:
        case NBM_REG_CHENERGY3:
        case NBM_REG_CHENERGY4:
        case NBM_REG_VCAP:
        case NBM_REG_VCHEND:
        case NBM_REG_PROFILE_MSB:
        case NBM_REG_COMMAND:
        case NBM_REG_SET1:
        case NBM_REG_SET2:
        case NBM_REG_SET3:
        case NBM_REG_SET4:
        case NBM_REG_SET5:
            return 1;
    }
    return 0;
}

static bool nbm_is_valid_field(enum nbm_fields field) {
    switch (field) {
        case NBM_LOWBAT:
        case NBM_EW:
        case NBM_ALRM:
        case NBM_RDY:
        case NBM_CHENGY:
        case NBM_VCAP:
        case NBM_VCHEND:
        case NBM_PROF:
        case NBM_RSTPF:
        case NBM_ACT:
        case NBM_ECM:
        case NBM_EOD:
        case NBM_VFIX:
        case NBM_VSET:
        case NBM_ICH:
        case NBM_VDHHIZ:
        case NBM_VMIN:
        case NBM_AUTOMODE:
        case NBM_EEW:
        case NBM_VEW:
        case NBM_BALMODE:
        case NBM_ENBAL:
        case NBM_VCAPMAX:
        case NBM_OPT_MARG:
            return 1;
    }
    return 0;
}

uint16_t nbm_vfix_to_mv(uint8_t vfix) {
    switch (vfix) {
        case NBM_VFIX_VAL_2V60:
            return 2600;
        case NBM_VFIX_VAL_2V95:
            return 2950;
        case NBM_VFIX_VAL_3V27:
            return 3270;
        case NBM_VFIX_VAL_3V57:
            return 3570;
        case NBM_VFIX_VAL_3V84:
            return 3840;
        case NBM_VFIX_VAL_4V10:
            return 4100;
        case NBM_VFIX_VAL_4V33:
            return 4330;
        case NBM_VFIX_VAL_4V55:
            return 4550;
        case NBM_VFIX_VAL_4V76:
            return 4760;
        case NBM_VFIX_VAL_4V96:
            return 4960;
        case NBM_VFIX_VAL_5V16:
            return 5160;
        case NBM_VFIX_VAL_5V34:
            return 5340;
        case NBM_VFIX_VAL_5V54:
            return 5540;
    }
    return 0;
}

uint16_t nbm_vcap_to_mv(uint8_t vcap) {
    switch (vcap) {
        case NBM_VCAP_VAL_SUB_1V1_A:
        case NBM_VCAP_VAL_SUB_1V1_B:
        case NBM_VCAP_VAL_SUB_1V1_C:
        case NBM_VCAP_VAL_1V10:
            return 1100;
        case NBM_VCAP_VAL_1V20:
            return 1200;
        case NBM_VCAP_VAL_1V30:
            return 1300;
        case NBM_VCAP_VAL_1V40:
            return 1400;
        case NBM_VCAP_VAL_1V51:
            return 1510;
        case NBM_VCAP_VAL_1V60:
            return 1600;
        case NBM_VCAP_VAL_1V71:
            return 1710;
        case NBM_VCAP_VAL_1V81:
            return 1810;
        case NBM_VCAP_VAL_1V99:
            return 1990;
        case NBM_VCAP_VAL_2V19:
            return 2190;
        case NBM_VCAP_VAL_2V40:
            return 2400;
        case NBM_VCAP_VAL_2V60:
            return 2600;
        case NBM_VCAP_VAL_2V79:
            return 2790;
        case NBM_VCAP_VAL_2V95:
            return 2950;
        case NBM_VCAP_VAL_3V01:
            return 3010;
        case NBM_VCAP_VAL_3V20:
            return 3200;
        case NBM_VCAP_VAL_3V27:
            return 3270;
        case NBM_VCAP_VAL_3V41:
            return 3410;
        case NBM_VCAP_VAL_3V57:
            return 3570;
        case NBM_VCAP_VAL_3V61:
            return 3610;
        case NBM_VCAP_VAL_3V84:
            return 3840;
        case NBM_VCAP_VAL_4V10:
            return 4100;
        case NBM_VCAP_VAL_4V33:
            return 4330;
        case NBM_VCAP_VAL_4V55:
            return 4550;
        case NBM_VCAP_VAL_4V76:
            return 4760;
        case NBM_VCAP_VAL_4V95:
            return 4950;
        case NBM_VCAP_VAL_5V16:
            return 5160;
        case NBM_VCAP_VAL_5V34:
            return 5340;
        case NBM_VCAP_VAL_5V54:
            return 5540;
    }
    return 0;
}


uint16_t nbm_vcapmax_to_mv(uint8_t vcapmax) {
    switch (vcapmax) {
        case NBM_VCAPMAX_VAL_4V95:
            return 4950;
        case NBM_VCAPMAX_VAL_5V54:
            return 5540;
    }
    return 0;
}

