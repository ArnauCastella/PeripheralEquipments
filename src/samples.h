#ifndef SAMPLES_H_
#define SAMPLES_H_

#include "stm32f4xx.h"

#define ADC3_DR_ADDRESS     ((uint32_t)0x4001224C)
#define ITM_Port32(n) (*((volatile unsigned long *)(0xE0000000+4*n)))

typedef struct {
    uint16_t sensor1;
    uint16_t sensor2;
    uint16_t sensor3;
} Sample;

Sample samples[8];      // TDATA_ORIGEN
Sample samplesCopy[8];  // SDATA_ORIGEN

// Configuration of the ports, ADC, DMA to get the samples
void samplesConfig();

// Initialize timer with an interrupt every 125 us to get samples
void initTimerADC();

// Configuration of the DMA and its interrupt used to copy the value of samples
void samplesCopyConfig();

// Calculate the arithmetic mean of each sensor and returns the LCD position
Sample getMeanPos();

#endif
