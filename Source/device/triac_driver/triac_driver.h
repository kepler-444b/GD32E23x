#ifndef _TRIAC_DRIVER_H_
#define _TRIAC_DRIVER_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t led_group_0[4];
    uint8_t led_group_1[4];
    uint8_t led_group_2[4];
    uint8_t led_group_3[4];
    uint8_t led_group_4[4];
    uint8_t led_group_5[4];
    uint8_t led_group_6[4];
    uint8_t led_group_7[4];
} all_led_status;

typedef struct
{
    bool blink;
    uint16_t blink_count;

    bool test;
    uint16_t tset_count;

    uint8_t addr;          // 设备地址
    uint8_t led_status[4]; // 每路状态

    uint16_t get_switch_count; // 定期获取
} triac_status_t;

void triac_driver_init(void);
#endif