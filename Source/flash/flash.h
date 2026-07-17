#ifndef _FLASH_H_
#define _FLASH_H_
#include <stdbool.h>
#include "../gpio/gpio.h"

#define FLASH_PAGE_SIZE 0x400U // 1024 字节 1KB

#define FLASH_INFO_ADDR 0x0801FC00U // 最后一页的起始地址,一般用于存放设备的配置信息

// 以字(32位)为单位写入Flash
fmc_state_enum app_flash_write_word(uint32_t address, uint32_t *data, uint32_t length);
// 以字(32位)为单位写入Flash(带自动擦除选项)
fmc_state_enum app_flash_program(uint32_t address, uint32_t *data, uint32_t length, bool erase_first);
// 以字(32为)为单位读取Flash
fmc_state_enum app_flash_read(uint32_t address, uint32_t *data, uint32_t length);
// 擦除页flash
fmc_state_enum app_flash_erase_page(uint32_t page_address);
// 擦除整片flash
fmc_state_enum app_flash_mass_erase(void);

#endif