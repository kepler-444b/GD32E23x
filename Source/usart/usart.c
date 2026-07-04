#include "usart.h"
#include "../../Source/eventbus/eventbus.h"
#include "gd32e23x_dma.h"
#include <stdio.h>

#if defined RTT_ENABLE
#include "../rtt/SEGGER_RTT.h"
#endif

#define USART0_TX_DMA_CH DMA_CH1
#define USART0_RX_DMA_CH DMA_CH2

static usart0_rx_buf_t rx0_buf = {0};
static usart0_tx_buf_t tx0_buf = {0};
static usart_rx0_callback_t rx0_callback;

static volatile uint8_t tx_busy = 0;

void app_usart0_rx_callback(usart_rx0_callback_t callback)
{
    rx0_callback = callback;
}

// 函数声明
static void USART_DMA_send_data(const uint8_t *data, uint16_t len);

void app_usart_init(uint32_t usart_com, uint32_t baudrate)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    if (usart_com == USART0) { // 业务串口 (使用DMA收发)

#if defined RS_485
        rcu_periph_clock_enable(RCU_GPIOA);
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_8);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
        RD_SET_L; // 先拉低,用于接收
#endif
        rcu_periph_clock_enable(RCU_USART0);

        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_9 | GPIO_PIN_10);
        gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_9 | GPIO_PIN_10);
        nvic_irq_enable(USART0_IRQn, 1);

        rcu_periph_clock_enable(RCU_DMA);
        dma_deinit(USART0_TX_DMA_CH);

        dma_parameter_struct dma_init_struct;
        dma_struct_para_init(&dma_init_struct);
        dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
        dma_init_struct.periph_addr = (uint32_t)&USART_TDATA(USART0);
        dma_init_struct.memory_addr = 0;
        dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
        dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
        dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
        dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
        dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
        dma_init_struct.number = 0;
        dma_init(USART0_TX_DMA_CH, &dma_init_struct);
        dma_circulation_disable(USART0_TX_DMA_CH);
        dma_memory_to_memory_disable(USART0_TX_DMA_CH);
        dma_interrupt_enable(USART0_TX_DMA_CH, DMA_INT_FTF);
        nvic_irq_enable(DMA_Channel1_2_IRQn, 1);

        dma_deinit(USART0_RX_DMA_CH);
        dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
        dma_init_struct.memory_addr = (uint32_t)rx0_buf.buffer;
        dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
        dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
        dma_init_struct.number = UART0_RECV_SIZE;
        dma_init_struct.periph_addr = (uint32_t)&USART_RDATA(USART0);
        dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
        dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
        dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
        dma_init(USART0_RX_DMA_CH, &dma_init_struct);
        dma_circulation_disable(USART0_RX_DMA_CH);
        dma_channel_enable(USART0_RX_DMA_CH);

    } else if (usart_com == USART1) { // 调试串口
#ifndef RTT_ENABLE
        rcu_periph_clock_enable(RCU_USART1);
        gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2 | GPIO_PIN_3);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_2 | GPIO_PIN_3);
        gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_2 | GPIO_PIN_3);
#endif
    }
    // USART参数
    usart_deinit(usart_com);
    usart_baudrate_set(usart_com, baudrate);
    usart_word_length_set(usart_com, USART_WL_8BIT);
    usart_stop_bit_set(usart_com, USART_STB_1BIT);
    usart_parity_config(usart_com, USART_PM_NONE);
    usart_hardware_flow_rts_config(usart_com, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(usart_com, USART_CTS_DISABLE);
    usart_transmit_config(usart_com, USART_TRANSMIT_ENABLE);
    usart_receive_config(usart_com, USART_RECEIVE_ENABLE);
    usart_dma_receive_config(usart_com, USART_DENR_ENABLE);
    usart_enable(usart_com);
    usart_interrupt_enable(usart_com, USART_INT_IDLE);
}

static void USART_DMA_send_data(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0 || tx_busy)
        return;
    tx0_buf.tx_ptr = data;
    tx0_buf.tx_len = len;
    tx_busy = 1;

    RD_SET_H; // 拉高,准备发送

    dma_channel_disable(USART0_TX_DMA_CH);
    dma_memory_address_config(USART0_TX_DMA_CH, (uint32_t)tx0_buf.tx_ptr);
    dma_transfer_number_config(USART0_TX_DMA_CH, tx0_buf.tx_len);
    dma_channel_enable(USART0_TX_DMA_CH);
    usart_dma_transmit_config(USART0, USART_DENT_DISABLE);
    usart_dma_transmit_config(USART0, USART_DENT_ENABLE);

#if defined RS_485
    usart_interrupt_enable(USART0, USART_INT_TC);
#endif
}

void app_usart_tx_buf(const uint8_t *data, uint16_t length, uint32_t usart_periph)
{
    APP_PRINTF("app_usart_tx_buf\n");
    if (usart_periph == USART0) {
        USART_DMA_send_data(data, length);
    }
}

void USART0_IRQHandler(void)
{
    if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE) != RESET) {

        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_IDLE);
        rx0_buf.length = UART0_RECV_SIZE - dma_transfer_number_get(USART0_RX_DMA_CH);
        if (rx0_buf.length > 0 && rx0_callback) {
            rx0_callback(&rx0_buf);
        }
        dma_channel_disable(USART0_RX_DMA_CH);
        dma_transfer_number_config(USART0_RX_DMA_CH, UART0_RECV_SIZE);
        dma_memory_address_config(USART0_RX_DMA_CH, (uint32_t)rx0_buf.buffer);
        dma_channel_enable(USART0_RX_DMA_CH);
    }
    if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_TC) != RESET) { // 发送完成中断
        usart_flag_clear(USART0, USART_FLAG_TC);

        APP_PRINTF("1\n");
        RD_SET_L; // 发送完成,拉低准备接收
        tx_busy = 0;
        usart_receive_config(USART0, USART_RECEIVE_ENABLE); // 重新启用接收器
        usart_interrupt_enable(USART0, USART_INT_IDLE);     // 重新启用空闲中断
    }
}

void DMA_Channel1_2_IRQHandler(void)
{
    if (dma_interrupt_flag_get(USART0_TX_DMA_CH, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(USART0_TX_DMA_CH, DMA_INT_FLAG_FTF);

        usart_dma_transmit_config(USART0, USART_DENT_DISABLE);
        dma_channel_disable(USART0_TX_DMA_CH);
    }
}

int fputc(int ch, FILE *f)
{
#if defined RTT_ENABLE
    SEGGER_RTT_PutChar(0, ch);
#else
    usart_data_transmit(USART1, (uint8_t)ch);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE)) {
        ;
    }

#endif
    return ch;
}
