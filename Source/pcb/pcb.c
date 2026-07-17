#include "pcb.h"
#include "../Source/device/devicemanager.h"
#include "gd32e23x.h"
#include "gd32e23x_gpio.h"
void pcb_init(void)
{
#if defined TRIAC_DRIVER
    rcu_periph_clock_enable(RCU_GPIOB);

    // 电源指示灯
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    gpio_bit_reset(GPIOB, GPIO_PIN_6); // 默认输出低电平

    // 过零信号
    rcu_periph_clock_enable(RCU_CFGCMP);
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_11);

    // 数据指示灯
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_bit_reset(GPIOB, GPIO_PIN_7); // 默认输出低电平

    // 4路调光引脚
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);
    gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5); // 默认输出低电平

    // 拨码
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14);

    // 按键
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_0);

#endif
}