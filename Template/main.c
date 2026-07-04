#include "main.h"
#include "../../Source/protocol/protocol.h"
#include "../Source/device/device.h"
#include "../Source/eventbus/eventbus.h"
#include "../Source/timer/timer.h"
#include "../Source/usart/usart.h"
#include "../Source/watchdog/watchdog.h"
#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>

int main(void)
{
    systick_config();
    SystemCoreClockUpdate();
    delay_1ms(1000);

    app_usart_init(USART1, 115200); // 初始化调试串口 
    app_eventbus_init(); // 初始化事件总线

    app_timer_init(); // 初始化软定时器
    
    app_watchdog_init(); // 初始化看门狗
    app_portocol_init(); // 初始化协议层
    jump_to_device_init();

    while (1) {
        app_timer_poll();
        app_eventbus_poll();
    }
}
