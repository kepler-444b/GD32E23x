#ifndef _UART_H_
#define _UART_H_

#include <stdbool.h>
#include <stdint.h>
#include "gd32e23x_usart.h"
#include "../Source/device/devicemanager.h"

#define UART0_RECV_SIZE 512
#define UART1_RECV_SIZE 256

#if defined TRIAC_DRIVER
#define RS_485 // 是否使用 RS485 通讯

#endif

#define RD_SET_H gpio_bit_set(GPIOA, GPIO_PIN_8)
#define RD_SET_L gpio_bit_reset(GPIOA, GPIO_PIN_8)

// #define RTT_ENABLE // 使用RTT调试

#define APP_DEBUG // 此宏用来管理整个工程的 debug 信息

#if defined APP_DEBUG
#define APP_PRINTF(...) printf(__VA_ARGS__)
#define APP_PRINTF_BUF(name, buf, len)                           \
    do {                                                         \
        APP_PRINTF("%s: ", (name));                              \
        for (size_t _idx = 0; _idx < (len); _idx++) {            \
            APP_PRINTF("%02X ", ((const uint8_t *)(buf))[_idx]); \
        }                                                        \
        APP_PRINTF("\n");                                        \
    } while (0)

#define APP_PRINTF_BUF16(name, buf, len)                                           \
    do {                                                                           \
        APP_PRINTF("%s: ", (name));                                                \
        for (size_t _idx = 0; _idx < (len); _idx++) {                              \
            uint16_t val = ((uint16_t)(buf)[_idx * 2] << 8) | (buf)[_idx * 2 + 1]; \
            APP_PRINTF("%04X ", val);                                              \
        }                                                                          \
        APP_PRINTF("\n");                                                          \
    } while (0)

#define APP_ERROR(fmt, ...) \
    APP_PRINTF("[#%s#] \"" fmt "\" ERROR!\n", __func__, ##__VA_ARGS__)

#else

#define APP_PRINTF(...)
#define APP_PRINTF_BUF(name, buf, len)
#define APP_ERROR(fmt, ...)
#endif

// ------------------------- 接收缓冲结构 -------------------------
typedef struct {
    uint8_t buffer[UART0_RECV_SIZE];
    uint16_t length;
} usart0_rx_buf_t;

typedef struct {
    const uint8_t *tx_ptr;
    uint16_t tx_len;
} usart0_tx_buf_t;

typedef struct {
    uint8_t buffer[UART1_RECV_SIZE];
    uint16_t length;
} usart1_rx_buf_t;

// ------------------------- 回调类型 -------------------------
typedef void (*usart_rx0_callback_t)(usart0_rx_buf_t *);
typedef void (*usart_rx1_callback_t)(usart1_rx_buf_t *);

// ------------------------- 公开接口 -------------------------
void app_usart0_rx_callback(usart_rx0_callback_t callback);

void app_usart_init(uint32_t usart_com, uint32_t baudrate);

void app_usart_tx_buf(const uint8_t *data, uint16_t length, uint32_t usart_periph);

#endif
