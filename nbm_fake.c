/* 
 * test and exmaple script for the nbm library
 * by thomas169
 * 
 * SPDX-License-Identifier: Apache-2.0 
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "nbm.h"

#define N_REGISTERS 14

struct fake_i2c_nbm {
    uint8_t registers[N_REGISTERS];
    uint8_t addr;
    uint8_t *reg_ptr;
};

struct fake_i2c_nbm fake_nbm_device = {
    /* initialise as default */
    .registers = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x09, 0x80, 0x80, 0x00, 0x00},
    .addr = NBM_I2C_ADDR_0x2F,
    .reg_ptr = NULL
};

bool do_read(uint8_t addr, uint8_t reg_no, uint8_t *value, uint8_t size) {
    if ((addr != fake_nbm_device.addr) || (reg_no + size > N_REGISTERS))
        return 1;

    fake_nbm_device.reg_ptr = &fake_nbm_device.registers[reg_no];
    memcpy(value, fake_nbm_device.reg_ptr, size);
    return 0;
}

bool do_write(uint8_t addr, uint8_t reg_no, const uint8_t *value, uint8_t size) {
    if ((addr != fake_nbm_device.addr) || (reg_no + size > N_REGISTERS))
        return 1;
    fake_nbm_device.reg_ptr = &fake_nbm_device.registers[reg_no];
    memcpy(fake_nbm_device.reg_ptr, value, size);
    return 0;
}


/* following are also a demo of the 3 user supplied fucntions you meed to
 * provide so to actually use the library */
bool user_impl_write_bytes_fcn(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len) {
    /* would usually be HAL_I2C_MASTER_WRITE() on STM32 or simmilar */
    printf("do_write()\n");
    do_write(i2c_addr, reg, value, len);
}


bool user_impl_read_bytes_fcn(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len) {
    /* would usually be HAL_I2C_MASTER_WRITE() on STM32 or simmilar */
    printf("do_read()\n");
    do_read(i2c_addr, reg, value, len);
}

void hard_fault_handler(uint8_t errno) {
    while(1) {
        asm("nop");
    }
}

/* test out the library with the fake nbm device */
int main() {

    uint8_t misc_val;
    uint32_t chenergy;
    struct NbmDevice nbm;

    /* note we dont give a correct i2c address here */
    nbm_init(&nbm, NBM5100A, 0, 
            user_impl_write_bytes_fcn, user_impl_read_bytes_fcn, hard_fault_handler);

    printf("expect value error\n");
    nbm_write_field(&nbm, nbm_field_enbal, 7);
    nbm_read_field(&nbm, nbm_field_enbal, &misc_val);
    printf("value of misc_val is: %d\n", misc_val);
    printf("dev errno is: %d\n\n", nbm.error_code);
    nbm.error_code = 0;
    
    printf("expect io error\n");
    nbm_write_field(&nbm, nbm_field_enbal, 1);
    nbm_read_field(&nbm, nbm_field_enbal, &misc_val);
    printf("value of misc_val is: %d\n", misc_val);
    printf("dev errno is: %d\n\n", nbm.error_code);
    nbm.error_code = 0;

    /* fix the nbm_init() i2c_addr based fup */
    nbm.i2c_addr = NBM_I2C_ADDR_0x2F;

    printf("expect invlaid fields error\n");
    nbm_write_field(&nbm, 0x01, 1);
    printf("dev errno is: %d\n", nbm.error_code);
    nbm_read_field(&nbm, 0x01, &misc_val);
    printf("value of misc_val is: %d\n", misc_val);
    printf("dev errno is: %d\n\n", nbm.error_code);
    nbm.error_code = 0;

    printf("expect not writable error\n");
    nbm_write_field(&nbm, nbm_field_lowbat, 1);
    nbm_read_field(&nbm, nbm_field_lowbat, &misc_val);
    printf("value of misc_val is: %d\n", misc_val);
    printf("dev errno is: %d\n\n", nbm.error_code);
    nbm.error_code = 0;

    printf("expect not invalid reg error\n");
    misc_val = 169;
    nbm_write_reg(&nbm, 16, &misc_val, 1);
    misc_val = 0;
    printf("dev errno is: %d\n", nbm.error_code);
    nbm.error_code = 0;
    nbm_read_reg(&nbm, 16, &misc_val, 1);
    printf("value of misc_val is: %d\n", misc_val);
    printf("dev errno is: %d\n\n", nbm.error_code);
    nbm.error_code = 0;

    printf("expect no error and value of 1 for field and 32 for register\n");  
    nbm_write_field(&nbm, nbm_field_enbal, NBM_ENBAL_VAL_ACTIVE);
    nbm_read_field(&nbm, nbm_field_enbal, &misc_val);
    printf("value of misc_val is: %d\n", misc_val);
    nbm_read_reg(&nbm, nbm_reg_set4, &misc_val, 1);
    printf("value of misc_val reg is: %d [%d]\n", misc_val, misc_val == NBM_ENBAL_VAL_ACTIVE);
    printf("dev errno is: %d\n\n", nbm.error_code);

    printf("expect no error and read from the special case of cherngy as 67305985\n");
    /* fake it so its not zero first should be 67305985 == (1 << 24 | 2 << 16 | 3 << 8 | 4 << 0) */
    fake_nbm_device.registers[1] = 0x01;
    fake_nbm_device.registers[2] = 0x02;
    fake_nbm_device.registers[3] = 0x03;
    fake_nbm_device.registers[4] = 0x04;
    /* note using a uint8_t here would be likely cause segfault or nuke nbm */
    nbm_read_field(&nbm, nbm_field_chengy, &chenergy);
    printf("value of chenergy is: %d\n", chenergy);
    printf("dev errno is: %d\n\n", nbm.error_code);
    nbm.error_code = 0;

    printf("expect no error and correct handing of the special prof field so read back " \
        "37 and for the regs get 2 and 80 \n");
    fake_nbm_device.registers[nbm_reg_command] = 0x00;  
    fake_nbm_device.registers[nbm_reg_profile_msb] = 0x00; 
    nbm_write_field(&nbm, nbm_field_prof, NBM_PROF_VAL_PROFILE(37));
    nbm_read_field(&nbm, nbm_field_prof, &misc_val);
    printf("value of misc_val is: %d\n", misc_val);
    nbm_read_reg(&nbm, nbm_reg_profile_msb, &misc_val, 1);
    printf("value of misc_val reg is: %d\n", misc_val);
    nbm_read_reg(&nbm, nbm_reg_command, &misc_val, 1);
    printf("value of misc_val reg is: %d\n", misc_val);
    printf("dev errno is: %d\n\n", nbm.error_code);
}
