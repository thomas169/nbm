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
#define GET_MSB_POS_FROM_FIELD(field) ((field) >> 8 & 0xF)
#define GET_LSB_POS_FROM_FIELD(field) ((field) >> 5 & 0x7)
#define GET_MASK_FROM_FIELD(field) (1 << (GET_MSB_POS_FROM_FIELD(field) - GET_LSB_POS_FROM_FIELD(field)))
#define GET_LENGTH_FROM_FIELD(field) (GET_MSB_POS_FROM_FIELD(field) - GET_LSB_POS_FROM_FIELD(field) + 1)
#define GET_DEVICE_FROM_FIELD(field) ((field) >> 2 & 0x7)
#define GET_SOLO_IN_REG_FROM_FIELD(field) ((field) >> 1 & 0x1)
#define GET_WRITEABLE_FIELD(field) ((field) >> 0 & 0x1)

/* local only fuctions, not exposed on api */
static bool nbm_can_write_reg(enum NbmRegisters reg);
static bool nbm_is_valid_reg(enum NbmRegisters reg);
static bool nbm_is_valid_field(enum NbmFields field);

void nbm_init(struct NbmDevice *dev, enum NbmDevices device_type, enum NbmI2cAddr i2c_addr,
            bool (*write_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len),
            bool (*read_bytes_fcn)(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len),
            void (*on_error_callback)(uint8_t error_code)) {

    dev->device_type = device_type;
    dev->error_code = NBM_ERROR_NO_ERROR;
    dev->i2c_addr = i2c_addr;

    /* write_bytes_fcn and read_bytes_fcn are required whilst on_error_callback is optional */
    if (write_bytes_fcn)
        dev->write_bytes_fcn = write_bytes_fcn;
    else
        dev->error_code = NMB_ERROR_NOT_INITALISED;

    if (read_bytes_fcn)
        dev->read_bytes_fcn = read_bytes_fcn;
    else
        dev->error_code = NMB_ERROR_NOT_INITALISED;

    if (on_error_callback)
        dev->on_error_callback = on_error_callback;

}

void nbm_write_field(struct NbmDevice *dev, enum NbmFields field, uint8_t value) {
    /* as we cannot write chenergy register all writes to a field will be 1 byte
     * long, however the prof field is split over two registers (grrr) */
    
    enum NbmRegisters reg;
    uint8_t tmp;
    uint8_t masked_value;
    bool io_status;

    reg = GET_REG_FROM_FIELD(field);
    
    if (!nbm_can_write_reg(reg)){
        dev->error_code |= NBM_ERROR_NOT_WRITEABLE;
        return;
    }

    if (!nbm_is_valid_field(field)) {
        dev->error_code |= NMB_ERROR_INVALID_FIELD;
        return;
    }

    /* sanitise the value and return error if value not valid, dont do it for the
     * the prof field however as mask will be wrong if upper 2 bits are set */
    if (field == nbm_field_prof)
        masked_value = value;
    else
        masked_value = value & GET_MASK_FROM_FIELD(field);

    if (value != masked_value) {
        dev->error_code |= NBM_ERROR_INVALID_VALUE;
        return;
    } 

    /* if the field is alone in the given register we avoid the need to read it
     * before any writes, and subsquent faffing around with bit shifting. */
    if (!GET_SOLO_IN_REG_FROM_FIELD(field)) {
        nbm_read_reg(dev, reg, &tmp, 1);
        tmp &= ~GET_MASK_FROM_FIELD(field);
        masked_value <<= GET_LSB_POS_FROM_FIELD(field);
        masked_value |= tmp;
    }

    /* Note there is no special handling of chenergy as its not writable */
    switch (field) {
        case nbm_field_prof:
            value = value >> 0x4 & 0x3;
            dev->error_code |= dev->write_bytes_fcn(dev->i2c_addr, nbm_reg_profile_msb, &value, 1);
            dev->error_code |= dev->write_bytes_fcn(dev->i2c_addr, reg, &masked_value, 1);
            break;
        default:
            dev->error_code |= dev->write_bytes_fcn(dev->i2c_addr, reg, &masked_value, 1);
    }
}

void nbm_read_field(struct NbmDevice *dev, enum NbmFields field, void *value) {

    uint8_t tmp;

    if (!nbm_is_valid_field(field)) {
        dev->error_code |= NMB_ERROR_INVALID_FIELD;
        return;
    }

    switch (field) {
        case nbm_field_prof:
            dev->error_code |= dev->read_bytes_fcn(dev->i2c_addr, nbm_reg_profile_msb, &tmp, 1);
            dev->error_code |= dev->read_bytes_fcn(dev->i2c_addr, GET_REG_FROM_FIELD(field), (uint8_t*) value, 1);
            /* read bottom 2 bits of tmp and shift above top 4 bits of value */
            tmp = tmp & 0x3 << 0x4;
            (*(uint8_t*)value) = 
                tmp | ((*(uint8_t*)value) >> GET_LSB_POS_FROM_FIELD(field) & GET_MASK_FROM_FIELD(field));
            break;
        case nbm_field_chengy:
            dev->error_code |= dev->read_bytes_fcn(dev->i2c_addr, nbm_reg_chenergy1, (uint8_t*) value, 4);
            break;
        default:
            dev->error_code |= dev->read_bytes_fcn(dev->i2c_addr, GET_REG_FROM_FIELD(field), (uint8_t*) value, 1);
            /* shift the lsb to 0 index and mask anything above top bit of field */
            (*(uint8_t*)value) = 
                (*(uint8_t*)value) >> GET_LSB_POS_FROM_FIELD(field) & GET_MASK_FROM_FIELD(field);
    }
}

void nbm_read_reg(struct NbmDevice *dev, enum NbmRegisters reg, uint8_t *value, uint8_t size) {
    
    if (!nbm_is_valid_reg(reg)) {
        dev->error_code |= NMB_ERROR_INVALID_REGISTER;
        return;
    }

    dev->error_code |= dev->read_bytes_fcn(dev->i2c_addr, reg, value, size);
}


void nbm_write_reg(struct NbmDevice *dev, enum NbmRegisters reg, const uint8_t *value, uint8_t size) {
        
    if (!nbm_is_valid_reg(reg)) {
        dev->error_code |= NMB_ERROR_INVALID_REGISTER;
        return;
    }

    if (!nbm_can_write_reg(reg)){
        dev->error_code |= NBM_ERROR_NOT_WRITEABLE;
        return;
    }

    dev->error_code |= dev->write_bytes_fcn(dev->i2c_addr, reg, value, size);
}

static bool nbm_can_write_reg(enum NbmRegisters reg) {
    switch (reg) {
        case nbm_reg_status:
        case nbm_reg_chenergy1:
        case nbm_reg_chenergy2:
        case nbm_reg_chenergy3:
        case nbm_reg_chenergy4:
        case nbm_reg_vcap:
        case nbm_reg_vchend:
            return 0;
        case nbm_reg_profile_msb:
        case nbm_reg_command:
        case nbm_reg_set1:
        case nbm_reg_set2:
        case nbm_reg_set3:
        case nbm_reg_set4:
        case nbm_reg_set5:
            return 1;
    }
    return 0;
}

static bool nbm_is_valid_reg(enum NbmRegisters reg) {
    switch (reg) {
         case nbm_reg_status:
        case nbm_reg_chenergy1:
        case nbm_reg_chenergy2:
        case nbm_reg_chenergy3:
        case nbm_reg_chenergy4:
        case nbm_reg_vcap:
        case nbm_reg_vchend:
        case nbm_reg_profile_msb:
        case nbm_reg_command:
        case nbm_reg_set1:
        case nbm_reg_set2:
        case nbm_reg_set3:
        case nbm_reg_set4:
        case nbm_reg_set5:
            return 1;
        default:
            return 0;
    }
}

static bool nbm_is_valid_field(enum NbmFields field) {
    switch (field) {
        case nbm_field_lowbat:
        case nbm_field_ew:
        case nbm_field_alrm:
        case nbm_field_rdy:
        case nbm_field_chengy:
        case nbm_field_vcap:
        case nbm_field_vchend:
        case nbm_field_prof:
        case nbm_field_rstpf:
        case nbm_field_act:
        case nbm_field_ecm:
        case nbm_field_eod:
        case nbm_field_vfix:
        case nbm_field_vset:
        case nbm_field_ich:
        case nbm_field_vdhhiz:
        case nbm_field_vmin:
        case nbm_field_automode:
        case nbm_field_eew:
        case nbm_field_vew:
        case nbm_field_balmode:
        case nbm_field_enbal:
        case nbm_field_vcapmax:
        case nbm_field_opt_marg :
            return 1;
        default:
            return 0;
    }
}

