#ifndef _TRIAC_DRIVER_H_
#define _TRIAC_DRIVER_H_

#include "../Source/gpio/gpio.h"
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

    uint8_t addr;          // 设备地址
    uint8_t led_status[4]; // 每路状态

    uint16_t get_switch_count; // 定期获取拨码开关状态

    volatile uint16_t open_time[4]; // 每路 led 导通时间

    volatile gpio_pin_t led_ctrl_pins[4]; // 每路 led 的控制引脚

} triac_status_t;

typedef struct
{
    uint16_t fade_time_start[4];   // 开始渐变导通时间
    uint16_t fade_time_current[4]; // 当前渐变导通时间
    uint16_t fade_time_target[4];  // 目标渐变导通时间

    uint32_t count_ms[4]; // 每路已经渐变时间
    uint32_t time_ms[4];  // 每路渐变总时间
    bool running[4];      // 每路是否正在渐变

} triac_fade_t;

// 设备信息
typedef struct
{
    uint8_t fade_time;   // 渐变时间
    uint8_t death_value; // 死区值
    uint8_t reserve[2];  // 保留,补齐4字节
} triac_info_t;

void triac_driver_init(void);

#endif