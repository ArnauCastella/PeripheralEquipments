#ifndef LCD_H_
#define LCD_H_

#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"

#include "samples.h"

typedef enum {
 NO_OK = 0 ,
 OK = !NO_OK
} RetSt ;

// Initialize the LCD and SDRAM configuration
void initLCD();

RetSt SetPixel(uint16_t col, uint16_t row, uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval );

RetSt DrawHorizontalLine (uint16_t col_start, uint16_t col_end, uint16_t row, uint8_t alpha, 
    uint8_t Rval, uint8_t Gval, uint8_t Bval );

RetSt DrawVerticalLine (uint16_t col, uint16_t row_start, uint16_t row_end, uint8_t alpha, 
    uint8_t Rval, uint8_t Gval, uint8_t Bval );

RetSt ClearScreen (uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval );

void displaySamples(Sample *buffer, int index);

#endif
