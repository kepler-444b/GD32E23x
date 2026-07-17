#include "protocol.h"
#include "../Source/device/devicemanager.h"
#include "../Source/device/triac_driver/triac_driver.h"
#include "../Source/eventbus/eventbus.h"
#include "../Source/usart/usart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 函数声明
static void app_usart0_check(usart0_rx_buf_t *buf);
static void protocol_event_handler(event_type_e event, void *params);
static void protocol_build_frame_to_soft(uint8_t *data, uint8_t len);

// 协议层初始化
void app_portocol_init(void)
{
#if defined TRIAC_DRIVER
    app_usart_init(USART0, 9600);
#endif
    app_usart0_rx_callback(app_usart0_check); // 注册业务串口回调函数
    app_eventbus_subscribe(protocol_event_handler);
}

static void app_usart0_check(usart0_rx_buf_t *buf)
{
    if (buf->buffer[0] == SOFT_R_FH_1 && buf->buffer[1] == SOFT_R_FH_2) { // 接收上位机软件发送的数据

        static uint8_t soft_data[SOFT_DATA_MAX_LEN];
        memset(soft_data, 0, sizeof(soft_data));
        uint8_t data_len = buf->buffer[2]; // 有效数据长度

        memcpy(soft_data, &buf->buffer[2], data_len + 2); // 只拷贝有效数据 + type(命令类型) + X(设备类型)
        app_eventbus_publish(EVENT_SOFT_RECV, soft_data);
    }

#if defined TRIAC_DRIVER
    if (buf->buffer[0] == EXTEND_FH_1 && buf->buffer[1] == EXTEND_FH_2) { // 接收主机发送的数据
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
    }
#endif
    // APP_PRINTF_BUF("buf", buf->buffer, buf->length);
    // app_usart_tx_buf(buf->buffer, buf->length, USART0);
}

static void protocol_event_handler(event_type_e event, void *params)
{
    switch (event) {
    case EVENT_SOFT_SEND: {
        uint8_t *data = (uint8_t *)params;
        uint8_t length = data[0];
        protocol_build_frame_to_soft(data, length);
    }

    break;
    default:
        break;
    }
}

static void protocol_build_frame_to_soft(uint8_t *data, uint8_t len)
{
    static uint8_t buf[SOFT_DATA_MAX_LEN];
    memset(buf, 0, sizeof(buf));
    uint8_t index = 0;

    buf[index++] = SOFT_T_FH_1;
    buf[index++] = SOFT_T_FH_2;

    // 数据
    memcpy(&buf[index], data, len + 2); // + type(命令类型) + X(设备类型)
    index += len + 2;

    buf[index++] = SOFT_T_FT_1;
    buf[index++] = SOFT_T_FT_2;
    app_usart_tx_buf(buf, index, USART0);
}