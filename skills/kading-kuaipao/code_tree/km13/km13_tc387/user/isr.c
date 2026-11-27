/*
 * KM13 TC387 interrupt handlers.
 * Based on the SeekFree TC387 open-source library (GPL-3.0-or-later).
 * Copyright (c) 2022 SEEKFREE Technology.
 */
#include "isr_config.h"
#include "isr.h"
#include "car_nav.h"
#include "car_voice.h"

IFX_INTERRUPT(cc60_pit_ch0_isr, CCU6_0_CH0_INT_VECTAB_NUM, CCU6_0_CH0_ISR_PRIORITY)
{
    interrupt_global_enable(0);
    pit_clear_flag(CCU60_CH0);
    if(car_nav_10ms_pending < 20)
    {
        car_nav_10ms_pending++;
    }
}

IFX_INTERRUPT(cc60_pit_ch1_isr, CCU6_0_CH1_INT_VECTAB_NUM, CCU6_0_CH1_ISR_PRIORITY)
{
    interrupt_global_enable(0);
    pit_clear_flag(CCU60_CH1);
}

IFX_INTERRUPT(cc61_pit_ch0_isr, CCU6_1_CH0_INT_VECTAB_NUM, CCU6_1_CH0_ISR_PRIORITY)
{
    interrupt_global_enable(0);
    pit_clear_flag(CCU61_CH0);
}

IFX_INTERRUPT(cc61_pit_ch1_isr, CCU6_1_CH1_INT_VECTAB_NUM, CCU6_1_CH1_ISR_PRIORITY)
{
    interrupt_global_enable(0);
    pit_clear_flag(CCU61_CH1);
}

IFX_INTERRUPT(exti_ch0_ch4_isr, EXTI_CH0_CH4_INT_VECTAB_NUM, EXTI_CH0_CH4_INT_PRIO)
{
    interrupt_global_enable(0);
    if(exti_flag_get(ERU_CH0_REQ0_P15_4)) exti_flag_clear(ERU_CH0_REQ0_P15_4);
    if(exti_flag_get(ERU_CH4_REQ13_P15_5)) exti_flag_clear(ERU_CH4_REQ13_P15_5);
}

IFX_INTERRUPT(exti_ch1_ch5_isr, EXTI_CH1_CH5_INT_VECTAB_NUM, EXTI_CH1_CH5_INT_PRIO)
{
    interrupt_global_enable(0);
    if(exti_flag_get(ERU_CH1_REQ10_P14_3)) exti_flag_clear(ERU_CH1_REQ10_P14_3);
    if(exti_flag_get(ERU_CH5_REQ1_P15_8)) exti_flag_clear(ERU_CH5_REQ1_P15_8);
}

IFX_INTERRUPT(exti_ch3_ch7_isr, EXTI_CH3_CH7_INT_VECTAB_NUM, EXTI_CH3_CH7_INT_PRIO)
{
    interrupt_global_enable(0);
    if(exti_flag_get(ERU_CH3_REQ6_P02_0))
    {
        exti_flag_clear(ERU_CH3_REQ6_P02_0);
        camera_vsync_handler();
    }
    if(exti_flag_get(ERU_CH7_REQ16_P15_1)) exti_flag_clear(ERU_CH7_REQ16_P15_1);
}

IFX_INTERRUPT(dma_ch5_isr, DMA_INT_VECTAB_NUM, DMA_INT_PRIO)
{
    interrupt_global_enable(0);
    camera_dma_handler();
}

IFX_INTERRUPT(uart0_tx_isr, UART0_INT_VECTAB_NUM, UART0_TX_INT_PRIO)
{
    interrupt_global_enable(0);
}

IFX_INTERRUPT(uart0_rx_isr, UART0_INT_VECTAB_NUM, UART0_RX_INT_PRIO)
{
    interrupt_global_enable(0);
#if DEBUG_UART_USE_INTERRUPT
    debug_interrupr_handler();
#endif
}

IFX_INTERRUPT(uart1_tx_isr, UART1_INT_VECTAB_NUM, UART1_TX_INT_PRIO)
{
    interrupt_global_enable(0);
}

IFX_INTERRUPT(uart1_rx_isr, UART1_INT_VECTAB_NUM, UART1_RX_INT_PRIO)
{
    interrupt_global_enable(0);
    camera_uart_handler();
}

IFX_INTERRUPT(uart3_tx_isr, UART3_INT_VECTAB_NUM, UART3_TX_INT_PRIO)
{
    interrupt_global_enable(0);
}

IFX_INTERRUPT(uart3_rx_isr, UART3_INT_VECTAB_NUM, UART3_RX_INT_PRIO)
{
    interrupt_global_enable(0);
    car_voice_uart_callback();
}

#define KM13_EMPTY_UART_ISR(index) \
    IFX_INTERRUPT(uart##index##_tx_isr, UART##index##_INT_VECTAB_NUM, UART##index##_TX_INT_PRIO) \
    { interrupt_global_enable(0); } \
    IFX_INTERRUPT(uart##index##_rx_isr, UART##index##_INT_VECTAB_NUM, UART##index##_RX_INT_PRIO) \
    { interrupt_global_enable(0); }

KM13_EMPTY_UART_ISR(2)
KM13_EMPTY_UART_ISR(4)
KM13_EMPTY_UART_ISR(5)
KM13_EMPTY_UART_ISR(6)
KM13_EMPTY_UART_ISR(8)
KM13_EMPTY_UART_ISR(9)
KM13_EMPTY_UART_ISR(10)
KM13_EMPTY_UART_ISR(11)

#define KM13_UART_ERROR_ISR(index) \
    IFX_INTERRUPT(uart##index##_er_isr, UART##index##_INT_VECTAB_NUM, UART##index##_ER_INT_PRIO) \
    { \
        interrupt_global_enable(0); \
        IfxAsclin_Asc_isrError(&uart##index##_handle); \
    }

KM13_UART_ERROR_ISR(0)
KM13_UART_ERROR_ISR(1)
KM13_UART_ERROR_ISR(2)
KM13_UART_ERROR_ISR(3)
KM13_UART_ERROR_ISR(4)
KM13_UART_ERROR_ISR(5)
KM13_UART_ERROR_ISR(6)
KM13_UART_ERROR_ISR(8)
KM13_UART_ERROR_ISR(9)
KM13_UART_ERROR_ISR(10)
KM13_UART_ERROR_ISR(11)
