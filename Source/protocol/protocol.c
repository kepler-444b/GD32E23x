#include "protocol.h"
#include "../Source/device/devicemanager.h"
#include "../Source/device/triac_driver/triac_driver.h"
#include "../Source/eventbus/eventbus.h"
#include "../Source/usart/usart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void app_usart0_check(usart0_rx_buf_t *buf);
void app_portocol_init(void)
{
#if defined TRIAC_DRIVER
    app_usart_init(USART0, 9600);
#endif
    app_usart0_rx_callback(app_usart0_check); // 注册业务串口回调函数
}

static void app_usart0_check(usart0_rx_buf_t *buf)
{
#if defined TRIAC_DRIVER
    if (buf->buffer[0] != EXTEND_FH_1 || buf->buffer[1] != EXTEND_FH_2) {
        return;
    }
    static all_led_status temp;
    memset(&temp, 0, sizeof(temp));
    memcpy(temp.led_group_0, &buf->buffer[13], sizeof(temp.led_group_0));
    memcpy(temp.led_group_1, &buf->buffer[19], sizeof(temp.led_group_1));
    memcpy(temp.led_group_2, &buf->buffer[23], sizeof(temp.led_group_2));
    memcpy(temp.led_group_3, &buf->buffer[27], sizeof(temp.led_group_3));
    memcpy(temp.led_group_4, &buf->buffer[32], sizeof(temp.led_group_4));
    memcpy(temp.led_group_5, &buf->buffer[36], sizeof(temp.led_group_5));
    memcpy(temp.led_group_6, &buf->buffer[46], sizeof(temp.led_group_6));
    memcpy(temp.led_group_7, &buf->buffer[50], sizeof(temp.led_group_7));

    app_eventbus_publish(EVENT_USART0_RECV, &temp);
#endif
    // APP_PRINTF_BUF("buf", buf->buffer, buf->length);
    // app_usart_tx_buf(buf->buffer, buf->length, USART0);
}