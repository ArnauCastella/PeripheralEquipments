#include "LCD.h"

static uint32_t frameBuffer = LCD_FRAME_BUFFER;

// Check that column and row do not exceed the limits
int checkLimits (uint16_t col, uint16_t row) {
    return (col >= 0 && col < LCD_PIXEL_WIDTH && row >= 0 && row < LCD_PIXEL_HEIGHT);
}

// Compose the 32-bit ARGB8888 color value
uint32_t ARGBtoInt(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) {    
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

// Change to background or foreground layers
void changeLayer(uint32_t layer) {
    switch (layer) {
        case LCD_BACKGROUND_LAYER:
            frameBuffer = LCD_FRAME_BUFFER;
            break;
        case LCD_FOREGROUND_LAYER:
            frameBuffer = LCD_FRAME_BUFFER + BUFFER_OFFSET;
            break;
    }
}

RetSt SetPixel(uint16_t col, uint16_t row, uint8_t alpha, uint8_t Rval, uint8_t Gval, 
uint8_t Bval ) {

    if (!checkLimits(col, row)) return NO_OK;

    *(__IO uint32_t*) (frameBuffer + 4 * (LCD_PIXEL_WIDTH * row + col)) = ARGBtoInt(alpha, Rval, Gval, Bval);

	return OK;
}

RetSt DrawHorizontalLine (uint16_t col_start, uint16_t col_end, uint16_t row, uint8_t alpha, 
uint8_t Rval, uint8_t Gval, uint8_t Bval ) {
    if (!checkLimits(col_start, row) || !checkLimits(col_end, row)) return NO_OK;

	if (col_start < col_end) {
        for (uint16_t i = col_start; i <= col_end; i++) {
		    SetPixel(i, row, alpha, Rval, Gval, Bval);
	    }
    } else {
        for (uint16_t i = col_start; i >= col_end; i--) {
		    SetPixel(i, row, alpha, Rval, Gval, Bval);
	    }
    }

	return OK;
}

RetSt DrawVerticalLine (uint16_t col, uint16_t row_start, uint16_t row_end, uint8_t alpha, 
uint8_t Rval, uint8_t Gval, uint8_t Bval ) {
    if (!checkLimits(col, row_start) || !checkLimits(col, row_end)) return NO_OK;

    if (row_start < row_end) {
        for (uint16_t i = row_start; i <= row_end; i++) {
		    SetPixel(col, i, alpha, Rval, Gval, Bval);
	    }
    } else {
        for (uint16_t i = row_start; i >= row_end; i--) {
		    SetPixel(col, i, alpha, Rval, Gval, Bval);
	    }
    }
	
	return OK;
}

// Clear current layer
RetSt ClearScreen (uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval ) {

	for (uint32_t i = 0; i < BUFFER_OFFSET; i++) {
	    *(__IO uint32_t*)(frameBuffer + (4*i)) = ARGBtoInt(alpha, Rval, Gval, Bval);
	}

	return OK;
}

RetSt DrawRectangle (uint16_t col_start, uint16_t row_start, uint16_t col_end, uint16_t row_end, 
uint8_t alpha, uint8_t Rval, uint8_t Gval, uint8_t Bval) {
    for (uint16_t i = col_start; i <= col_end; i++) {
        DrawVerticalLine(i, row_start, row_end, alpha, Rval, Gval, Bval);
    }
    return OK;
}

// Draw the limits, axis and sensor color rectangles 
void drawBackground() {
    // Samples zone
    DrawVerticalLine(9, 9, 311, 0xFF, 0, 0, 0);
    DrawVerticalLine(231, 9, 311, 0xFF, 0, 0, 0);
    DrawHorizontalLine(9, 231, 9, 0xFF, 0, 0, 0);
    DrawHorizontalLine(9, 231, 311, 0xFF, 0, 0, 0);

    // X and Y axis
    DrawHorizontalLine(9, 231, 160, 0xFF, 0, 0, 0);
    DrawVerticalLine(120, 9, 311, 0xFF, 0, 0, 0);

    // Color rectangles
    DrawRectangle(2, 88, 7, 103, 0xFF, 0xFF, 0, 0); // Red - Sensor 1
    DrawRectangle(2, 153, 7, 167, 0xFF, 0, 0xFF, 0); // Green - Sensor 2
    DrawRectangle(2, 217, 7, 232, 0xFF, 0, 0, 0xFF); // Blue - Sensor 3
}

void initLCD () {

    // Initialize LCD 
    LCD_Init();
    LCD_LayerInit();

    // Initialize SDRAM
    SDRAM_Init();
    FMC_SDRAMWriteProtectionConfig(FMC_Bank2_SDRAM, DISABLE);

    // Set pixel format for both layers
    LTDC_Cmd(ENABLE);
    LTDC_LayerPixelFormat(LTDC_Layer1, LTDC_Pixelformat_ARGB8888);
	LTDC_LayerPixelFormat(LTDC_Layer2, LTDC_Pixelformat_ARGB8888);
	LTDC_ReloadConfig(LTDC_VBReload);

    // Draw first layer (background)
    ClearScreen(0xFF, 0xFF, 0xFF, 0xFF); // White
    drawBackground();

    // Change layer to start drawing sensor data
    changeLayer(LCD_FOREGROUND_LAYER);
    ClearScreen(0, 0, 0, 0);

}

typedef struct {
    uint16_t col1;
    uint16_t col2;
    uint16_t col3;
} LCDPos;

LCDPos prevPos[300];

void displaySamples (Sample *buffer, int index) {
    for (int i = 0; i < 300; i++) {
        //Erase previous pixels
        SetPixel(prevPos[i].col1, i+10, 0, 0, 0, 0);
        SetPixel(prevPos[i].col2, i+10, 0, 0, 0, 0);
        SetPixel(prevPos[i].col3, i+10, 0, 0, 0, 0);

        //Draw new ones
        prevPos[i].col1 = buffer[index].sensor1;
        SetPixel(buffer[index].sensor1, i+10, 0xFF, 0xFF, 0, 0);
        
        prevPos[i].col2 = buffer[index].sensor2;
        SetPixel(buffer[index].sensor2, i+10, 0xFF, 0, 0xFF, 0);
        
        prevPos[i].col3 = buffer[index].sensor3;
        SetPixel(buffer[index].sensor3, i+10, 0xFF, 0, 0, 0xFF);

        index++;
        if (index == 300) index = 0;
    }
}
