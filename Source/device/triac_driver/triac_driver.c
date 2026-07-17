#include "triac_driver.h"
#include "../Source/flash/flash.h"
#include "../Source/pcb/pcb.h"
#include "../Source/timer/timer.h"
#include "../Source/usart/usart.h"
#include "../eventbus/eventbus.h"
#include "triac_lum_table.h"

#define SYSTEM_CLOCK_FREQ 72000000 ///< 系统时钟频率
#define TIMER_PERIOD 9             ///< 定时器周期(10us中断)

#define AC_HALF_CYCLE_COUNTS 1000 ///< 50Hz半周期 = 10ms = 1000次(10us/次)中断

// 函数声明
static void triac_event_handler(event_type_e event, void *params);
static void triac_prase_soft_data(uint8_t *data, uint8_t len);
static void triac_timer(void *arg);
static void triac_update(void *arg);

static void triac_timer15_init(void);
static void triac_zero_init(void);
static void triac_key_init(void);
static void triac_lum_map(void);

static void triac_load_info(void);
static void triac_save_info(void);

// 全局变量
static triac_status_t my_triac_status = {
    .led_ctrl_pins[0] = PB0,
    .led_ctrl_pins[1] = PB1,
    .led_ctrl_pins[2] = PB4,
    .led_ctrl_pins[3] = PB5,
};

static triac_info_t my_triac_info = {0};

static volatile triac_fade_t my_triac_fade = {0};

volatile uint16_t sim_zero_count = 0; // 模拟零点计数器

volatile bool zero_is_fail = true;  // 零点失效标记
volatile uint16_t zero_check_count; // 定时检测零点是否失效

void triac_driver_init(void)
{
    APP_PRINTF("triac_driver_init\n");
    pcb_init();              // 初始化pcb所用引脚
    APP_SET_GPIO(PB6, true); // 开启电源指示灯
    triac_load_info();
    triac_zero_init();
    triac_key_init();
    triac_timer15_init();

    app_timer_start(10, triac_timer, true, NULL, "triac_timer");
    app_timer_start(1, triac_update, true, NULL, "update");
    app_eventbus_subscribe(triac_event_handler);
}

// 开启 timer15 10us 级定时器
static void triac_timer15_init(void)
{
    timer_parameter_struct timer_initpara;
    rcu_periph_clock_enable(RCU_TIMER15);
    timer_deinit(TIMER15);
    timer_struct_para_init(&timer_initpara); // 使用默认参数初始化该结构体

    timer_initpara.prescaler = (SYSTEM_CLOCK_FREQ / 1000000) - 1;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = TIMER_PERIOD; // 1000次计数 = 1ms
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_init(TIMER15, &timer_initpara); // 使能中断

    timer_interrupt_flag_clear(TIMER15, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER15, TIMER_INT_UP);

    timer_enable(TIMER15);
    nvic_irq_enable(TIMER15_IRQn, 1); // 配置NVIC,优先级1
}

// 过零中断初始化
static void triac_zero_init(void)
{
    syscfg_exti_line_config(EXTI_SOURCE_GPIOB, EXTI_SOURCE_PIN11);
    exti_init(EXTI_11, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_11);
    nvic_irq_enable(EXTI4_15_IRQn, 0);
}

// 按键中段初始化
static void triac_key_init(void)
{
    syscfg_exti_line_config(EXTI_SOURCE_GPIOA, EXTI_SOURCE_PIN0);
    exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_FALLING);
    exti_interrupt_flag_clear(EXTI_0);
    nvic_irq_enable(EXTI0_1_IRQn, 0);
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
        triac_lum_map();

        my_triac_status.blink = true;
        my_triac_status.blink_count = 0;
    } break;

    case EVENT_SOFT_RECV: {

        uint8_t *data = (uint8_t *)params;
        uint8_t len = data[0] + 2; // 有效数据 + type(命令类型) + X(设备类型)
        triac_prase_soft_data(data, len);
    } break;
    default:
        break;
    }
}

// 解析上位机软件数据
static void triac_prase_soft_data(uint8_t *data, uint8_t len)
{
    uint8_t cmd_type = data[1];
    uint8_t dev_type = data[2];
    uint8_t reserve = data[3];
    uint8_t dev_addr = data[4];

    if (dev_type != triac_driver) { // 如果不是该设备,直接退出
        APP_ERROR("triac_driver");
        return;
    }
    if (dev_addr != my_triac_status.addr) { // 如果设备的地址不对,直接退出
        APP_ERROR("dev_addr");
        return;
    }

    // 获取设备信息
    if (cmd_type == get_dev_info) {
        static uint8_t dev_info[7];
        memset(dev_info, 0, sizeof(dev_info));

        dev_info[0] = 0x05;         // 上报的有效数据长度
        dev_info[1] = get_dev_info; // 上报的命令类型

        dev_info[2] = triac_driver;         // 上报的设备类型
        dev_info[3] = TRIAC_DRIVER_VER;     // reserve 填写软件版本
        dev_info[4] = my_triac_status.addr; // addr

        dev_info[5] = my_triac_info.fade_time;
        dev_info[6] = my_triac_info.death_value;
        app_eventbus_publish(EVENT_SOFT_SEND, dev_info);
    }

    // 设置设备信息
    if (cmd_type == set_dev_info) {
        uint8_t set_type = data[5];
        // 调节死区
        if (set_type == 0x01) {

            uint8_t lum = reserve;
            if (lum > 100)
                lum = 100;
            APP_PRINTF("lum:%d\n", lum);
            uint16_t temp_lum = Linear_table[lum]; // 线性表
            for (uint8_t i = 0; i < 4; i++) {      // 立即调节亮度,不渐变
                my_triac_fade.fade_time_current[i] = temp_lum;
                my_triac_fade.fade_time_target[i] = temp_lum;
                my_triac_status.open_time[i] = temp_lum;
            }
        }
        // 调节亮度
        if (set_type == 0x02) {
            uint16_t value = reserve;
            if (value > AC_HALF_CYCLE_COUNTS) {
                return;
            }
            my_triac_status.led_status[0] = value;
            my_triac_status.led_status[1] = value;
            my_triac_status.led_status[2] = value;
            my_triac_status.led_status[3] = value;
            triac_lum_map();
        }
        // 保存配置
        if (set_type == 0x03) {
            uint8_t fade_time = data[6];
            uint8_t death_value = data[7];
            APP_PRINTF("fade_time:%d death_value:%d\n", fade_time, death_value);

            my_triac_info.fade_time = fade_time;
            my_triac_info.death_value = death_value;

            triac_save_info();
        }
    }
}

// 亮度值重新映射
static void triac_lum_map(void)
{
    for (uint8_t i = 0; i < 4; i++) {

        // 0~100 映射到 0~1000
        uint16_t lum = my_triac_status.led_status[i];

        if (lum > 100)
            lum = 100;

        if (lum > 0) {
            lum = my_triac_info.death_value + ((uint16_t)lum * (100 - my_triac_info.death_value)) / 100;
        }
        uint16_t temp_lum = Linear_table[lum]; // 线性表

        // 目标发生变化
        if (my_triac_fade.fade_time_target[i] != temp_lum) {

            if (zero_is_fail) { // 零点失效后,只在这里打开或关闭
                if (temp_lum) {
                    APP_SET_GPIO(my_triac_status.led_ctrl_pins[i], false); // 立即点灯
                } else {
                    APP_SET_GPIO(my_triac_status.led_ctrl_pins[i], true); // 立即点灯
                }
                my_triac_fade.running[i] = false;

                // 同步内部状态
                my_triac_fade.fade_time_current[i] = temp_lum;
                my_triac_fade.fade_time_target[i] = temp_lum;
                continue;
            }

            // 当前值作为新的起点
            my_triac_fade.fade_time_start[i] = my_triac_fade.fade_time_current[i];

            my_triac_fade.fade_time_target[i] = temp_lum; // 更新目标值

            // 每一路独立计时
            my_triac_fade.count_ms[i] = 0;

            // 默认1秒渐变
            my_triac_fade.time_ms[i] = my_triac_info.fade_time * 100;

            // 开始该路渐变
            my_triac_fade.running[i] = true;
        }
    }
}

static void triac_timer(void *arg)
{
    zero_check_count++;
    if (zero_check_count >= 1000) { // 标记零点已经失效
        zero_is_fail = true;
    }

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

    my_triac_status.get_switch_count++; // 1s 检查一次拨码状态
    if (my_triac_status.get_switch_count >= 100) {
        bool bit_1 = !APP_GET_GPIO(PB12);
        bool bit_2 = !APP_GET_GPIO(PB13);
        bool bit_3 = !APP_GET_GPIO(PB14);
        my_triac_status.addr = (bit_1 << 2) | (bit_2 << 1) | (bit_3 << 0);
        my_triac_status.get_switch_count = 0;
    }
}

// 10us 级定时器中断服务函数
void TIMER15_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER15, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(TIMER15, TIMER_INT_FLAG_UP);

        if (zero_is_fail) { // 如果零点已经失效,则直接退出
            return;
        }

        sim_zero_count++;
        // 说明在硬件过零中断丢失的情况下,软件自行到达了下一个虚拟零点
        if (sim_zero_count >= AC_HALF_CYCLE_COUNTS) {
            sim_zero_count = 0;
        }
        // 模拟零点触发
        if (sim_zero_count == 0) {
            for (uint8_t i = 0; i < 4; i++) {
                my_triac_status.open_time[i] = my_triac_fade.fade_time_current[i];

                if (my_triac_status.open_time[i]) { // 如果导通时间不为0,零点后立即打开
                    APP_SET_GPIO(my_triac_status.led_ctrl_pins[i], false);
                } else {
                    APP_SET_GPIO(my_triac_status.led_ctrl_pins[i], true);
                }
            }
        }
        // 到达设定时间后关闭
        for (uint8_t i = 0; i < 4; i++) {
            if (my_triac_status.open_time[i]) {
                if (sim_zero_count == my_triac_status.open_time[i]) {
                    APP_SET_GPIO(my_triac_status.led_ctrl_pins[i], true);
                }
            }
        }
    }
}

// 零点中断
void EXTI4_15_IRQHandler(void) // 过零点触发中断服务函数
{
    if (exti_interrupt_flag_get(EXTI_11)) {
        exti_interrupt_flag_clear(EXTI_11);

        // 当真实的零点到来时,修改模拟零点计数器,强行让模拟零点与实际零点对齐
        sim_zero_count = AC_HALF_CYCLE_COUNTS - 1;

        zero_is_fail = false;
        zero_check_count = 0;
    }
}

void EXTI0_1_IRQHandler(void)
{
    if (exti_interrupt_flag_get(EXTI_0)) {
        exti_interrupt_flag_clear(EXTI_0);
        NVIC_SystemReset();
    }
}

// 渐变 1ms 定时器
static void triac_update(void *arg)
{
    for (uint8_t i = 0; i < 4; i++) {

        if (!my_triac_fade.running[i])
            continue;

        if (my_triac_fade.count_ms[i] < my_triac_fade.time_ms[i]) {
            my_triac_fade.count_ms[i]++;
        }

        uint32_t diff;    // 起始值与目标值的差值
        uint16_t current; // 当前渐变计算出来的导通时间

        if (my_triac_fade.fade_time_target[i] >= my_triac_fade.fade_time_start[i]) { // 目标值大于起始值->变亮

            diff = my_triac_fade.fade_time_target[i] - my_triac_fade.fade_time_start[i];

            // 导通时间 = 渐变起始值 + 按时间比例计算出的当前变化量
            current = my_triac_fade.fade_time_start[i] + (diff * my_triac_fade.count_ms[i]) / my_triac_fade.time_ms[i];

        } else { // 目标值小于起始值->变暗

            diff = my_triac_fade.fade_time_start[i] - my_triac_fade.fade_time_target[i];
            current = my_triac_fade.fade_time_start[i] - (diff * my_triac_fade.count_ms[i]) / my_triac_fade.time_ms[i];
        }

        my_triac_fade.fade_time_current[i] = current; // 将计算的导通时间更新到结构体中

        if (my_triac_fade.count_ms[i] >= my_triac_fade.time_ms[i]) { // 渐变完成

            my_triac_fade.fade_time_current[i] = my_triac_fade.fade_time_target[i];
            my_triac_fade.running[i] = false;
        }
    }
}

static void triac_load_info(void)
{
    if (app_flash_read(FLASH_INFO_ADDR, (uint32_t *)&my_triac_info, sizeof(triac_info_t)) != FMC_READY) {
        APP_ERROR("triac_load_info");
        return;
    }
    APP_PRINTF("death_value:%d\n", my_triac_info.death_value);
    APP_PRINTF("fade_time:%d\n", my_triac_info.fade_time);
}

static void triac_save_info(void)
{
    if (app_flash_program(FLASH_INFO_ADDR, (uint32_t *)&my_triac_info, sizeof(triac_info_t), true) != FMC_READY) {
        APP_ERROR("triac_save_info");
        return;
    }
}