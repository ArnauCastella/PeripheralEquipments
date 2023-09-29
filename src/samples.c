#include "samples.h"

void samplesConfig() {
    ADC_InitTypeDef       ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    DMA_InitTypeDef       DMA_InitStructure;
    GPIO_InitTypeDef      GPIO_InitStructure;

    /* Enable ADC3, DMA2 and GPIO clocks ****************************************/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

    /* DMA2 Stream0 channel2 configuration **************************************/
    DMA_InitStructure.DMA_Channel = DMA_Channel_2;  
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC3_DR_ADDRESS;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&samples;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = 24; // Number of 16 bit transfers, one for each sample
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);
    DMA_Cmd(DMA2_Stream0, ENABLE);

    /* Configure ADC3 Channel5 pin as analog input ******************************/
    /* PC3 - Sensor 1 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* Configure ADC3 Channel1 pin as analog input ******************************/
    /* PA1 - Sensor 2 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure ADC3 Channel2 pin as analog input ******************************/
    /* PA2 - Sensor 3 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ADC Common Init **********************************************************/
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    /* ADC3 Init ****************************************************************/
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE; // Scan multiple channels
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // Do not start next conversion after
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;	
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 3;
    ADC_Init(ADC3, &ADC_InitStructure);

    /* ADC3 regular channel13 configuration *************************************/
    ADC_RegularChannelConfig(ADC3, ADC_Channel_13, 1, ADC_SampleTime_3Cycles);

    /* ADC3 regular channel1 configuration *************************************/
    ADC_RegularChannelConfig(ADC3, ADC_Channel_1, 2, ADC_SampleTime_3Cycles);

    /* ADC3 regular channel2 configuration *************************************/
    ADC_RegularChannelConfig(ADC3, ADC_Channel_2, 3, ADC_SampleTime_3Cycles);

    /* Enable DMA request after last transfer (Single-ADC mode) */
    ADC_DMARequestAfterLastTransferCmd(ADC3, ENABLE);

    /* Enable ADC3 DMA */
    ADC_DMACmd(ADC3, ENABLE);

    /* Enable ADC3 */
    ADC_Cmd(ADC3, ENABLE);
}

void initTimerADC() {
    TIM_TimeBaseInitTypeDef timerStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

    timerStructure.TIM_Prescaler = 10 - 1; 
    timerStructure.TIM_CounterMode = TIM_CounterMode_Up;
    timerStructure.TIM_Period = (uint32_t) 1125 - 1; 
    timerStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM5, &timerStructure);

    TIM_Cmd(TIM5, ENABLE);
    TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvicStructure;

    nvicStructure.NVIC_IRQChannel = TIM5_IRQn;
    nvicStructure.NVIC_IRQChannelPreemptionPriority = 1; // TODO: Define priorities
    nvicStructure.NVIC_IRQChannelSubPriority = 1;
    nvicStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicStructure);
}

void TIM5_IRQHandler() {
    static int sample_counter = 0;
    static int send = 0;
    if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET) {
        
        ADC_SoftwareStartConv(ADC3);
        if (send) {
        	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_HTIF1|DMA_FLAG_TCIF1|DMA_FLAG_TEIF1|DMA_FLAG_FEIF1|DMA_FLAG_DMEIF1);
        	DMA_Cmd(DMA2_Stream1, ENABLE);
        	send = 0;
        }
        if (++sample_counter >= 8) {
            sample_counter = 0;
            send = 1;
            //ITM_Port32(31) = 1;
        }
        TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
    }
}

void samplesCopyConfig() {
    /* DMA2 Stream1 channel3 configuration **************************************/
    DMA_InitTypeDef DMA_MemToMem;
    DMA_MemToMem.DMA_Channel = DMA_Channel_3;
    DMA_MemToMem.DMA_PeripheralBaseAddr = (uint32_t)&samples;
    DMA_MemToMem.DMA_Memory0BaseAddr = (uint32_t)&samplesCopy;
    DMA_MemToMem.DMA_DIR = DMA_DIR_MemoryToMemory;
    DMA_MemToMem.DMA_BufferSize = 24; // Number of 16 bit transfers, one for each sample
    DMA_MemToMem.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    DMA_MemToMem.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_MemToMem.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_MemToMem.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_MemToMem.DMA_Mode = DMA_Mode_Normal;
    DMA_MemToMem.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_MemToMem.DMA_FIFOMode = DMA_FIFOMode_Enable;
    DMA_MemToMem.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
    DMA_MemToMem.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_MemToMem.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream1, &DMA_MemToMem);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_HTIF1|DMA_FLAG_TCIF1|DMA_FLAG_TEIF1|DMA_FLAG_FEIF1|DMA_FLAG_DMEIF1);
    DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);
}

Sample getMeanPos() {
    Sample result = {0,0,0};
    for (int i = 0; i < 8; i++) {
        result.sensor1 += samplesCopy[i].sensor1;
        result.sensor2 += samplesCopy[i].sensor2;
        result.sensor3 += samplesCopy[i].sensor3;
    }
    result.sensor1 = result.sensor1 *220/8/4095+10;
    result.sensor2 = result.sensor2 *220/8/4095+10;
    result.sensor3 = result.sensor3 *220/8/4095+10;

    return result;
}
