#include "flash.h"
#include "gd32e23x.h"
#include "gd32e23x_fmc.h"
#include <stdio.h>
#include <string.h>

fmc_state_enum app_flash_read(uint32_t address, uint32_t *data, uint32_t length)
{
    if ((address % 4) || (length % 4) || !data) {
        return FMC_PGAERR; // 编程对齐错误
    }
    for (uint32_t i = 0; i < length / 4; i++) {
        data[i] = REG32(address + (i * 4)); // 直接读取
    }

    return FMC_READY;
}

fmc_state_enum app_flash_write_word(uint32_t address, uint32_t *data, uint32_t length)
{
    fmc_state_enum state = FMC_READY;

    if ((address % 4) || (length % 4) || !data) {
        return FMC_PGAERR;
    }

    fmc_unlock(); // 解锁Flash操作

    for (uint32_t i = 0; i < length / 4; i++) { // 逐字编程
        state = fmc_word_program(address + (i * 4), data[i]);
        if (state != FMC_READY) {
            fmc_lock(); // 出错时重新上锁
            return state;
        }
    }

    fmc_lock(); // 重新锁定Flash

    return FMC_READY;
}

// 以双字(64位)为单位写入Flash
fmc_state_enum app_flash_write_doubleword(uint32_t address, uint64_t *data, uint32_t length)
{
    fmc_state_enum state = FMC_READY;

    if ((address % 4) || (length % 4) || !data) {
        return FMC_PGAERR;
    }

    fmc_unlock(); // 解锁Flash操作

    for (uint32_t i = 0; i < length / 8; i++) { // 逐双字编程
        state = fmc_doubleword_program(address + (i * 8), data[i]);
        if (state != FMC_READY) {
            fmc_lock(); // 出错时重新上锁
            return state;
        }
    }

    fmc_lock(); // 重新锁定Flash

    return FMC_READY;
}

fmc_state_enum app_flash_erase_page(uint32_t page_address)
{
    fmc_state_enum state;

    fmc_unlock(); // 解锁Flash操作
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);

    state = fmc_page_erase(page_address); // 执行页擦除

    fmc_lock(); // 重新锁定Flash

    return state;
}

fmc_state_enum app_flash_mass_erase(void)
{
    fmc_state_enum state;

    fmc_unlock(); // 解锁Flash操作

    state = fmc_mass_erase(); // 执行整片擦除

    fmc_lock(); // 重新锁定Flash

    return state;
}

/*!
    \brief      验证Flash内容是否与预期数据匹配
    \param[in]  address: 验证起始地址
    \param[in]  data: 预期数据指针
    \param[in]  length: 要验证的字节数
    \retval     fmc_state: 验证状态(FMC_READY表示匹配，FMC_PGERR表示不匹配)
*/
fmc_state_enum app_flash_verify(uint32_t address, uint32_t *data, uint32_t length)
{
    fmc_state_enum state = FMC_READY;
    uint32_t read_data;

    if ((address % 4) || (length % 4) || !data) {
        return FMC_PGAERR;
    }

    for (uint32_t i = 0; i < length / 4; i++) { // 逐字读取并比较
        read_data = REG32(address + (i * 4));
        if (read_data != data[i]) {
            state = FMC_PGERR; // 数据不匹配
            break;
        }
    }

    return state;
}

fmc_state_enum app_flash_program(uint32_t address, uint32_t *data, uint32_t length, bool erase_first)
{
#if 0
    fmc_state_enum state;
    if (erase_first) { // 如果需要先擦除
        uint32_t page_address = address & ~(FLASH_PAGE_SIZE - 1);

        state = app_flash_erase_page(page_address); // 执行页擦除
        if (state != FMC_READY) {
            return state; // 擦除失败直接返回
        }
    }
    return app_flash_write_word(address, data, length); // 执行数据写入
#else
    // fmc_state_enum state = FMC_READY;

    // if ((address % 4) || (length % 4) || !data) {
    //     return FMC_PGAERR;
    // }

    // if (erase_first) {
    //     uint32_t page_address = address & ~(FLASH_PAGE_SIZE - 1);
    //     fmc_unlock();
    //     fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);
    //     state = fmc_page_erase(page_address);
    //     fmc_lock();
    //     if (state != FMC_READY)
    //         return state;
    // }

    // fmc_unlock();
    // uint32_t word_len = length / 4;
    // for (uint32_t i = 0; i < word_len; i++) {
    //     state = fmc_word_program(address + i * 4, data[i]);
    //     if (state != FMC_READY)
    //         break;
    // }
    // fmc_lock();

    // return state;
    if ((address % 4) || (length % 4) || (data == NULL)) {
        return FMC_PGAERR;
    }

    fmc_state_enum state = FMC_READY;

    uint32_t start_page = address & ~(FLASH_PAGE_SIZE - 1);
    uint32_t end_addr = address + length;
    uint32_t end_page = end_addr & ~(FLASH_PAGE_SIZE - 1);

    fmc_unlock();

    /* ===== 1. 清标志 ===== */
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);

    /* ===== 2. 等待Flash空闲 ===== */
    while (fmc_flag_get(FMC_FLAG_BUSY))
        ;

    /* ===== 3. 擦除（支持跨页） ===== */
    if (erase_first) {

        for (uint32_t page = start_page; page <= end_page; page += FLASH_PAGE_SIZE) {
            while (fmc_flag_get(FMC_FLAG_BUSY))
                ;

            fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);

            state = fmc_page_erase(page);

            if (state != FMC_READY) {
                fmc_lock();
                return state;
            }
        }
    }

    /* ===== 4. 写入 ===== */
    uint32_t word_len = length / 4;

    for (uint32_t i = 0; i < word_len; i++) {
        while (fmc_flag_get(FMC_FLAG_BUSY))
            ;

        state = fmc_word_program(address + i * 4, data[i]);

        if (state != FMC_READY) {
            fmc_lock();
            return state;
        }
    }

    /* ===== 5. 收尾 ===== */
    while (fmc_flag_get(FMC_FLAG_BUSY))
        ;
    fmc_lock();

    return FMC_READY;
#endif
}
