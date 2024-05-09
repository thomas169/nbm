# nbm
Library for Nexperia NBM series of battery booster ICs (NBM5100, NBM7100)

# Useing
Typically you would compile `nbm.c` serperatly and include `nbm.h` in you main application sources.

You then need to define in your source the to functions to read and write via I2C or SPI. Also you may use an optional error callback otherwise you  can just set it to `NULL`. For exmaple with STM32 and I2C you would have something like:

    #include "nbm.h"
    /* other includes */
    
    /* nbm global in this demo */
    struct NbmDevice nbm;
    extern I2C_HandleTypeDef hi2c;
    
    bool user_impl_write_bytes_fcn(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len) {
        // prototype:  HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) 	
        return (bool) HAL_I2C_Mem_Write(*hi2c, i2c_addr, MemAddSize, value, len, HAL_MAX_DELAY);
    }
    
    bool user_impl_read_bytes_fcn(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len) {
        // prototype:  HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) 	
        return (bool) HAL_I2C_Mem_Read(*hi2c, i2c_addr, reg, MemAddSize, value, len, HAL_MAX_DELAY);
    }
    
    void error_handler(uint8_t nbm_errno) {
        /* content as you wish, its optional anyway */
        while(1) {
          asm("nop");
        }
    }
    
    int main() {
        uint8 nbm_misc_val;
        uint32_t nbm_chenergy_val;
        ...
        /* setup your peripherals, specificically clocks, and hi2c */
        hal_init_peripherals(...);
        hal_init_i2c(&hi2c,...);
        ...
        nbm_init(&nbm, NBM5100A, NBM_I2C_ADDR_0x2E, user_impl_write_bytes_fcn, user_impl_read_bytes_fcn, error_handler);
        ...
        nbm_write_field(&nbm, nbm_field_vfix, NBM_VFIX_VAL_3V57); /* vfix now set to 3.57V */
        ...
        nbm_read_field(&nbm, nbm_field_vcap, &nbm_misc_val);
        /* can now lookup nbm_read_val against NBM_VCAP_VAL_xxx defines to get real world voltage */
        ...
        /* some time later */
        nbm_read_field(&nbm, nbm_field_chengy, &nbm_chenergy_val);
        /* save nbm_chenergy in NVM if system going down and you need to keep track of it */
        ...
    }

Then you must instialse the device with `nbm_init()`. Thereafter just call whatever IO operations you want. 

You can reference `nbm_fake.c` for a demo and test of the library.
