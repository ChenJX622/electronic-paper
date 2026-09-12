/* generated vector source file - do not edit */
        #include "bsp_api.h"
        /* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
        #if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_uart_rxi_isr, /* SCI7 RXI (Receive data full) */
            [1] = sci_uart_txi_isr, /* SCI7 TXI (Transmit data empty) */
            [2] = sci_uart_tei_isr, /* SCI7 TEI (Transmit end) */
            [3] = sci_uart_eri_isr, /* SCI7 ERI (Receive error) */
            [4] = sci_uart_rxi_isr, /* SCI3 RXI (Receive data full) */
            [5] = sci_uart_txi_isr, /* SCI3 TXI (Transmit data empty) */
            [6] = sci_uart_tei_isr, /* SCI3 TEI (Transmit end) */
            [7] = sci_uart_eri_isr, /* SCI3 ERI (Receive error) */
            [8] = sci_uart_rxi_isr, /* SCI8 RXI (Receive data full) */
            [9] = sci_uart_txi_isr, /* SCI8 TXI (Transmit data empty) */
            [10] = sci_uart_tei_isr, /* SCI8 TEI (Transmit end) */
            [11] = sci_uart_eri_isr, /* SCI8 ERI (Receive error) */
            [12] = sci_uart_rxi_isr, /* SCI9 RXI (Receive data full) */
            [13] = sci_uart_txi_isr, /* SCI9 TXI (Transmit data empty) */
            [14] = sci_uart_tei_isr, /* SCI9 TEI (Transmit end) */
            [15] = sci_uart_eri_isr, /* SCI9 ERI (Receive error) */
            [16] = sci_i2c_txi_isr, /* SCI2 TXI (Transmit data empty) */
            [17] = sci_i2c_tei_isr, /* SCI2 TEI (Transmit end) */
            [18] = gpt_counter_overflow_isr, /* GPT0 COUNTER OVERFLOW (Overflow) */
            [19] = adc_scan_end_isr, /* ADC0 SCAN END (End of A/D scanning operation) */
            [20] = sci_uart_rxi_isr, /* SCI5 RXI (Receive data full) */
            [21] = sci_uart_txi_isr, /* SCI5 TXI (Transmit data empty) */
            [22] = sci_uart_tei_isr, /* SCI5 TEI (Transmit end) */
            [23] = sci_uart_eri_isr, /* SCI5 ERI (Receive error) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI7_RXI,GROUP0), /* SCI7 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI7_TXI,GROUP1), /* SCI7 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI7_TEI,GROUP2), /* SCI7 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI7_ERI,GROUP3), /* SCI7 ERI (Receive error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SCI3_RXI,GROUP4), /* SCI3 RXI (Receive data full) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TXI,GROUP5), /* SCI3 TXI (Transmit data empty) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TEI,GROUP6), /* SCI3 TEI (Transmit end) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_SCI3_ERI,GROUP7), /* SCI3 ERI (Receive error) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_SCI8_RXI,GROUP0), /* SCI8 RXI (Receive data full) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_SCI8_TXI,GROUP1), /* SCI8 TXI (Transmit data empty) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_SCI8_TEI,GROUP2), /* SCI8 TEI (Transmit end) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_SCI8_ERI,GROUP3), /* SCI8 ERI (Receive error) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_SCI9_RXI,GROUP4), /* SCI9 RXI (Receive data full) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TXI,GROUP5), /* SCI9 TXI (Transmit data empty) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TEI,GROUP6), /* SCI9 TEI (Transmit end) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_SCI9_ERI,GROUP7), /* SCI9 ERI (Receive error) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TXI,GROUP0), /* SCI2 TXI (Transmit data empty) */
            [17] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TEI,GROUP1), /* SCI2 TEI (Transmit end) */
            [18] = BSP_PRV_VECT_ENUM(EVENT_GPT0_COUNTER_OVERFLOW,GROUP2), /* GPT0 COUNTER OVERFLOW (Overflow) */
            [19] = BSP_PRV_VECT_ENUM(EVENT_ADC0_SCAN_END,GROUP3), /* ADC0 SCAN END (End of A/D scanning operation) */
            [20] = BSP_PRV_VECT_ENUM(EVENT_SCI5_RXI,GROUP4), /* SCI5 RXI (Receive data full) */
            [21] = BSP_PRV_VECT_ENUM(EVENT_SCI5_TXI,GROUP5), /* SCI5 TXI (Transmit data empty) */
            [22] = BSP_PRV_VECT_ENUM(EVENT_SCI5_TEI,GROUP6), /* SCI5 TEI (Transmit end) */
            [23] = BSP_PRV_VECT_ENUM(EVENT_SCI5_ERI,GROUP7), /* SCI5 ERI (Receive error) */
        };
        #endif
        #endif