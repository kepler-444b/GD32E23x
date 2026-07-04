#include "watchdog.h"
#include "../Source/timer/timer.h"
#include "../Source/usart/usart.h"
#include "gd32e23x.h"
#include <stdio.h>

#define SYSTEM_CLOCK_FREQ 72000000 // 系统时钟频率(72MHz)
#define TIMER_PERIOD 1999          // 2kHz下2000ms=1s中断

static void timer_feed_dog(void *arg);

void app_watchdog_init(void)
{
    // 看门狗配置
    rcu_osci_on(RCU_IRC40K);             // 使能看门狗独立时钟源
    rcu_osci_stab_wait(RCU_IRC40K);      // 等待时钟稳定
    fwdgt_config(3125, FWDGT_PSC_DIV64); // 5s 溢出一次(触发中断服务函数)
    fwdgt_enable();

    app_timer_start(1000, timer_feed_dog, true, NULL, "feed_dog"); // 1s 定时器喂狗
}

static void timer_feed_dog(void *arg)
{ 
    fwdgt_counter_reload();
}
