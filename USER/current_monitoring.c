#include "current_monitoring.h"
#include "ct_ratio.h" // Include RATIO calculation layer
#include <math.h>     // For abs()
#include <stdlib.h>

/* ================================================================== */
/* 1. Global Variables Definitions                                    */
/* ================================================================== */

// DMA Ping-Pong buffers
uint16_t ADC_CycleBuffer[2][TOTAL_DMA_SIZE];

// State machine flag: 0xFF=Not ready, 0=Buf0 ready, 1=Buf1 ready
volatile uint8_t latest_ready_idx = 0xFF; 

/* DSP work pool memory */
// Compile-time locked memory pool for DSP
static uint8_t g_dsp_work_pool[DSP_CALC_WORK_BUF_SIZE(CH_NUM, POINTS_PER_PERIOD)] = {0};

// Allocate array for point_rms output
static float g_w_res_point_buf[POINTS_PER_PERIOD];

/* ================================================================== */
/* 2. Hardware Interrupt Services                                     */
/* ================================================================== */
/**
 * @brief DMA2 Stream0 IRQ Handler.
 * Note: Hooked into stm32f4xx_it.c
 */
void DMA2_Stream0_IRQHandler(void) {
    // Check Half-Transfer Complete
    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_HTIF0)) {
        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_HTIF0); // Clear IT flag
        latest_ready_idx = 0; // Flag Buf0 ready
    }
    
    // Check Transfer Complete
    if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0)) {
        DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0); // Clear IT flag
        latest_ready_idx = 1; // Flag Buf1 ready
    }
}


/* ================================================================== */
/* 3. Hardware Initialization: ADC+DMA config                         */
/* ================================================================== */
void ADC_DMA_Config(void) {
    ADC_InitTypeDef       ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    DMA_InitTypeDef       DMA_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    GPIO_InitTypeDef      GPIO_InitStructure;

    /* --- 1. Enable Clocks --- */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* --- 2. GPIO Configuration --- */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | //GPIO_Pin_0 
                                  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; // Analog Input
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* --- 3. TIM2 Configuration --- */
    /* Hardware clock config constraint warning: Requires 8MHz HSE to get 168MHz/84MHz APB1. */
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;      // Prescaler to 1MHz
    // Auto-calculate reload value
    TIM_TimeBaseStructure.TIM_Period = (1000000 / SAMPLE_FREQ_HZ) - 1; 
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update); // Generate TRGO on Update
    TIM_Cmd(TIM2, ENABLE);

    /* --- 4. DMA Configuration (Circular Ping-Pong) --- */
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;  
    // Base memory address
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADC_CycleBuffer[0][0]; 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = TOTAL_DMA_SIZE * 2; // Total buffer size
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // HalfWord (16-bit)
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; // Circular mode
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);

    // Enable HT and TC interrupts for Ping-Pong
    DMA_ITConfig(DMA2_Stream0, DMA_IT_HT | DMA_IT_TC, ENABLE);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn); // Enable NVIC
    DMA_Cmd(DMA2_Stream0, ENABLE);

    /* --- 5. ADC Configuration (Triggered by TIM2) --- */
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4; 
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;          // Scan mode
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;   // Disable continuous mode
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO; // TIM2 Trigger
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = CH_NUM;       // 8 Channels
    ADC_Init(ADC1, &ADC_InitStructure);

    // Scan sequence configuration
    // Pin mapping strategy
    for(uint8_t i = 0; i < CH_NUM; i++) {
        // Set channel ranks
        ADC_RegularChannelConfig(ADC1, 7 - i, i + 1, ADC_SampleTime_15Cycles); 
    }

    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    
    /* --- 6. Initialize DSP Algorithm Library --- */
    DSP_Algo_Init(CH_NUM, POINTS_PER_PERIOD, SETTLE_PERIODS, SAMPLE_FREQ_HZ, 
                  g_dsp_work_pool, sizeof(g_dsp_work_pool), SYS_Get_Ch_Enable);
}

/* ================================================================== */
/* 4. Business Logic Scheduling Task                                  */
/* ================================================================== */
void Process_Cycle_Task(void) {
    // 1. Check readiness
    if (latest_ready_idx == 0xFF) return;

    // 2. Fetch buffer index and clear flag
    uint8_t buf_idx = latest_ready_idx;
    latest_ready_idx = 0xFF;

    // Static array to avoid stack overflow
    static uint16_t ch_data_temp[POINTS_PER_PERIOD]; 
    static WaveformResult w_res; // Static allocation for DSP result
    w_res.point_rms = g_w_res_point_buf; // Link memory to output pointer
    
    uint8_t settled_flag = 0; // 1s settlement flag

    // Cross-channel vector sum calc
    Calc_Accumulate_VectorSum(&ADC_CycleBuffer[buf_idx][0]);

    // 3. Process each channel
    for (uint8_t ch = 0; ch < CH_NUM; ch++) {
        
        // --- A. Permission Check ---
        // Skip if channel is disabled
        if (SYS_Get_Ch_Enable(ch) == 0) {
            continue; 
        }

        // --- B. Data De-interleaving ---
        // DMA interleaving format
        // De-interleave into 256-point array
        for (uint16_t i = 0; i < POINTS_PER_PERIOD; i++) {
            ch_data_temp[i] = ADC_CycleBuffer[buf_idx][i * CH_NUM + ch];
        }

        // --- C. DSP Calculation ---
        int16_t dummy_offset = 0;
        
        // Push data into DSP engine
        // 1 indicates a 1s cycle completed
        if (Calc_Accumulate_WithAlign(ch, ch_data_temp, &dummy_offset) == 1) {
            // 1. Calculate final RMS and features
            Calc_GetFullWaveform(ch, HW_ADC_COEFF, dummy_offset, SETTLE_PERIODS, &w_res);
            
            // 2. Distribute results to Modbus
            Final_Deep_Processing(ch, &w_res, dummy_offset);
            
            settled_flag = 1;
        }
    }

    // Vector sum Modbus update
    if (settled_flag) {
        // Group 1
        float vec_rms_1 = Calc_GetVectorSumRMS(0, HW_ADC_COEFF, SETTLE_PERIODS);
        vec_rms_1 *= SYS_Get_CT_Ratio(0); // Apply ratio
        SYS_Push_Float_To_Modbus(MB_REG_VECTOR_SUM_START, 0, vec_rms_1);

        // Group 2
        float vec_rms_2 = Calc_GetVectorSumRMS(1, HW_ADC_COEFF, SETTLE_PERIODS);
        vec_rms_2 *= SYS_Get_CT_Ratio(4); 
        SYS_Push_Float_To_Modbus(MB_REG_VECTOR_SUM_START, 1, vec_rms_2);
    }
}
