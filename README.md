# nbm
Library for Nexperia NBM series of battery booster ICs (NBM5100, NBM7100)

# Useing
Typically you would compile `nbm.c` serperatly and include `nbm.h` in you main application sources.

You then need to define in your source the to functions to read and write via I2C or SPI. Also you may use an optional error callback otherwise you  can just set it to `NULL`. For exmaple with STM32 and I2C you would have something like:

    bool user_impl_write_bytes_fcn(uint8_t i2c_addr, uint8_t reg, const uint8_t *value, uint8_t len) {
        // prototype:  HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) 	
        return (bool) HAL_I2C_Mem_Write(*hi2c, i2c_addr, MemAddSize, value, len, HAL_MAX_DELAY);
    }
    
    bool user_impl_read_bytes_fcn(uint8_t i2c_addr, uint8_t reg, uint8_t *value, uint8_t len) {
        // prototype:  HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) 	
        return (bool) HAL_I2C_Mem_Read(*hi2c, i2c_addr, reg, MemAddSize, value, len, HAL_MAX_DELAY);
    }
    
    void error_handler(uint8_t nbm_errno) {
        while(1) {
          asm("nop");
        }
        // as you wish, its optional anyway
    }

Then you must instialse the device with `nbm_init()`. Thereafter just call whatever IO operations you want. 

You can reference `nbm_fake.c` for a demo and test of the library.
