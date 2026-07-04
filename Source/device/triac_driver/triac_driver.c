#include "triac_driver.h"
#include "../Source/gpio/gpio.h"
#include "../Source/pcb/pcb.h"
#include "../Source/timer/timer.h"
#include "../Source/usart/usart.h"
#include "../eventbus/eventbus.h"

// 函数声明
static void triac_event_handler(event_type_e event, void *params);
static void triac_timer(void *arg);

// 全局变量
triac_status_t my_triac_status = {};

void triac_driver_init(void)
{
    APP_PRINTF("triac_driver_init\n");
    pcb_init(); // 初始化pcb所用引脚

    APP_SET_GPIO(PB6, true); // 开启电源指示灯
    app_timer_start(10, triac_timer, true, NULL, "triac_timer");
    app_eventbus_subscribe(triac_event_handler);
}

void EXTI4_15_IRQHandler(void) // 过零点触发中断服务函数
{
    if (exti_interrupt_flag_get(EXTI_11)) {
        exti_interrupt_flag_clear(EXTI_11);

        APP_PRINTF("1\n");
    }
}

static void triac_event_handler(event_type_e event, void *params)
{
    switch (event) {
    case EVENT_USART0_RECV: {
        all_led_status *data = (all_led_status *)params;
        switch (my_triac_status.addr) {
        case 0:
            memcpy(my_triac_status.led_status, data->led_group_0, 4);
            break;
        case 1:
            memcpy(my_triac_status.led_status, data->led_group_1, 4);
            break;
        case 2:
            memcpy(my_triac_status.led_status, data->led_group_2, 4);
            break;
        case 3:
            memcpy(my_triac_status.led_status, data->led_group_3, 4);
            break;
        case 4:
            memcpy(my_triac_status.led_status, data->led_group_4, 4);
            break;
        case 5:
            memcpy(my_triac_status.led_status, data->led_group_5, 4);
            break;
        case 6:
            memcpy(my_triac_status.led_status, data->led_group_6, 4);
            break;
        case 7:
            memcpy(my_triac_status.led_status, data->led_group_7, 4);
            break;
        default:
            break;
        }

        APP_PRINTF_BUF("data", data->led_group_0, sizeof(data->led_group_0));

        my_triac_status.blink = true;
        my_triac_status.blink_count = 0;
    } break;
    default:
        break;
    }
}

static void triac_timer(void *arg)
{
    if (my_triac_status.blink) {
        my_triac_status.blink_count++;
        if (my_triac_status.blink_count == 1) {
            APP_SET_GPIO(PB7, true);
        }
        if (my_triac_status.blink_count >= 10) {
            APP_SET_GPIO(PB7, false);
            my_triac_status.blink = false;
            my_triac_status.blink_count = 0;
        }
    }

    my_triac_status.tset_count++;
    if (my_triac_status.tset_count >= 500) {
        my_triac_status.test = !my_triac_status.test;
        my_triac_status.tset_count = 0;
    }
    if (my_triac_status.test == true) {
        APP_SET_GPIO(PB0, true);
        APP_SET_GPIO(PB1, true);
        APP_SET_GPIO(PB4, true);
        APP_SET_GPIO(PB5, true);
        APP_PRINTF("1\n");
    } else {
        APP_SET_GPIO(PB0, false);
        APP_SET_GPIO(PB1, false);
        APP_SET_GPIO(PB4, false);
        APP_SET_GPIO(PB5, false);
        APP_PRINTF("2\n");
    }

    my_triac_status.get_switch_count++; // 1s 检查一次拨码状态
    if (my_triac_status.get_switch_count >= 100) {
        bool bit_1 = APP_GET_GPIO(PB14);
        bool bit_2 = APP_GET_GPIO(PB13);
        bool bit_3 = APP_GET_GPIO(PB12);
        my_triac_status.addr = (bit_1 << 2) | (bit_2 << 1) | (bit_3 << 0);
        my_triac_status.get_switch_count = 0;
    }
}