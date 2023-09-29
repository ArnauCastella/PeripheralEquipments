/* Includes */
#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f4xx_gpio.h"

#include "samples.h"
#include "LCD.h"

#define ITM_Port32(n) (*((volatile unsigned long *)(0xE0000000+4*n)))

int engine_speed = 1000;		// RPM
int engine_speed_mode = 0; 	// 0 -> increment / 1 -> decrement
int USR_Flag = 0;
int us_counter = 0, sTDC_counter = 0;
int sPC_counter_IN = 0;
int injectionON = 0;
uint16_t InjectionAngle = 53, InjectionTime = 3200;
uint32_t timerCountNs;

void initPorts(){
	//PE14 -> sPC
	//PE15 -> sTDC
	//PD0 -> iPC
	//PD1 -> iTDC
	//PE0 -> oINJ

	//PG2 -> sPC
	//PG3 -> sTDC
	//PG13 -> iPC
	//PG14 -> iTDC
	//PG9 -> oINJ

	//PA5 -> Sensor 1
	//PA1 -> Sensor 2
	//PA2 -> Sensor 3

	//PE4 -> Debug


    GPIO_InitTypeDef gpioStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

    gpioStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_9;
	gpioStructure.GPIO_OType = GPIO_OType_PP;
	gpioStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpioStructure.GPIO_Mode = GPIO_Mode_OUT;
    gpioStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOG, &gpioStructure);

	gpioStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
    gpioStructure.GPIO_Mode = GPIO_Mode_IN;
	gpioStructure.GPIO_OType = GPIO_OType_PP;
	gpioStructure.GPIO_PuPd = GPIO_PuPd_UP;
    gpioStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOG, &gpioStructure);
}

/****** --- MODULE 1 --- ******/

void updateEngineSpeed() {
	engine_speed = engine_speed_mode ? engine_speed - 1000 : engine_speed + 1000;

	if (engine_speed == 6000) {
		engine_speed_mode = 1;
	} else if (engine_speed == 1000) {
		engine_speed_mode = 0;
	}
}

void initTimer1Ms() {
	TIM_TimeBaseInitTypeDef timerStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);

    timerStructure.TIM_Prescaler = 1000 - 1;
    timerStructure.TIM_CounterMode = TIM_CounterMode_Up;
    timerStructure.TIM_Period = 90 - 1;
    timerStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM7, &timerStructure);
    TIM_ClearITPendingBit(TIM7, TIM_IT_Update);

    TIM_Cmd(TIM7, ENABLE);
    TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

    //Enable timer interrupt
	NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = TIM7_IRQn;
	nvicStructure.NVIC_IRQChannelPreemptionPriority = 5; //TODO: Define priorities
	nvicStructure.NVIC_IRQChannelSubPriority = 0;
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicStructure);
}

int counterMs = 0;
void TIM7_IRQHandler() {
    if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
    {
    	//GPIO_ToggleBits(GPIOE, GPIO_Pin_4);
    	counterMs++;
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
		if (STM_EVAL_PBGetState(BUTTON_USER)) {
			USR_Flag = 1;
		}
    }
}

/****** --- MODULE 2 --- ******/

//Get half period in us
uint32_t getHalfPeriod() {
	return (uint32_t)60 * 1000000 / engine_speed / 36 / 2;
}

TIM_TimeBaseInitTypeDef timerSimulation;
void initTimerSim() {

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    timerSimulation.TIM_Prescaler = 90 - 1;
    timerSimulation.TIM_CounterMode = TIM_CounterMode_Up;
    timerSimulation.TIM_Period = getHalfPeriod();
    timerSimulation.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM4, &timerSimulation);

    //Enable timer interrupt
	NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = TIM4_IRQn;
	nvicStructure.NVIC_IRQChannelPreemptionPriority = 0; //TODO: Define priorities
	nvicStructure.NVIC_IRQChannelSubPriority = 0;
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicStructure);

	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_Cmd(TIM4, ENABLE);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
}

void TIM4_IRQHandler() {
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

		GPIO_ToggleBits(GPIOG, GPIO_Pin_2);
		sTDC_counter++;
		if (sTDC_counter == 72) {
			GPIO_SetBits(GPIOG, GPIO_Pin_3);
			sTDC_counter = 0;
		} else if (sTDC_counter == 2) {
			GPIO_ResetBits(GPIOG, GPIO_Pin_3);
		}
	}
}

void updatePeriodSim() {
	timerSimulation.TIM_Period = getHalfPeriod();
	TIM_TimeBaseInit(TIM4, &timerSimulation);
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_Cmd(TIM4, ENABLE);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
}

/****** --- MODULE 3 --- ******/

TIM_TimeBaseInitTypeDef injTimer;

void initSensorInts() {
	EXTI_InitTypeDef EXTI_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	/* Tell system that you will use PG2 for EXTI_Line0 */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG, EXTI_PinSource2);

	/* PD0 is connected to EXTI_Line0 */
    EXTI_InitStruct.EXTI_Line = EXTI_Line2;
    /* Enable interrupt */
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    /* Interrupt mode */
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    /* Triggers on rising */
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    /* Add to EXTI */
    EXTI_Init(&EXTI_InitStruct);

    /* Add IRQ vector to NVIC */
    /* PD0 is connected to EXTI_Line0, which has EXTI0_IRQn vector */
    NVIC_InitStruct.NVIC_IRQChannel = EXTI2_IRQn;
    /* Set priority */
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    /* Set sub priority */
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
    /* Enable interrupt */
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    /* Add to NVIC */
    NVIC_Init(&NVIC_InitStruct);


	/* Tell system that you will use PG3 for EXTI_Line1 */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG, EXTI_PinSource3);

	/* PD1 is connected to EXTI_Line1 */
    EXTI_InitStruct.EXTI_Line = EXTI_Line3;
    /* Enable interrupt */
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    /* Interrupt mode */
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    /* Triggers on rising */
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    /* Add to EXTI */
    EXTI_Init(&EXTI_InitStruct);

    /* Add IRQ vector to NVIC */
    /* PD1 is connected to EXTI_Line1, which has EXTI1_IRQn vector */
    NVIC_InitStruct.NVIC_IRQChannel = EXTI3_IRQn;
    /* Set priority */
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    /* Set sub priority */
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x01;
    /* Enable interrupt */
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    /* Add to NVIC */
    NVIC_Init(&NVIC_InitStruct);
}

// IRS for the sPC interrupt
void EXTI2_IRQHandler() {
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
		sPC_counter_IN++;
		timerCountNs = TIM_GetCounter(TIM2) * 356; // 1/90MHz * 32 (prescaler) in ns
		TIM_SetCounter(TIM2, 0);
		if (sPC_counter_IN == (360 - InjectionAngle) / 10) {
			uint32_t period = ((360 - InjectionAngle) % 10) * (timerCountNs / 1000 / 10);
			injTimer.TIM_Period = period;
			TIM_TimeBaseInit(TIM3, &injTimer);
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
			TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
			TIM_Cmd(TIM3, ENABLE);
		}
        /* Clear interrupt flag */
        EXTI_ClearITPendingBit(EXTI_Line2);
    }
}

// IRS for the iTDC signal
void EXTI3_IRQHandler() {
    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
		sPC_counter_IN = 0;
        /* Clear interrupt flag */
        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}

// Timer to calculate speed from sPC
void initSensorTimer() {
	TIM_TimeBaseInitTypeDef timerStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    timerStructure.TIM_Prescaler = 32 - 1;
    timerStructure.TIM_CounterMode = TIM_CounterMode_Up;
    timerStructure.TIM_Period = 0xFFFF;
    timerStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM2, &timerStructure);

    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    TIM_Cmd(TIM2, ENABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

/****** --- MODULE 4 --- ******/

void initInjTimer() {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    injTimer.TIM_Prescaler = 90 - 1;
    injTimer.TIM_CounterMode = TIM_CounterMode_Up;
    injTimer.TIM_Period = 0xFFFF;
    injTimer.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM3, &injTimer);
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

	//Enable timer interrupt
	NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = TIM3_IRQn;
	nvicStructure.NVIC_IRQChannelPreemptionPriority = 3; //TODO: Define priorities
	nvicStructure.NVIC_IRQChannelSubPriority = 0;
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicStructure);
}

void TIM3_IRQHandler() {
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

		if (injectionON == 0) {
			GPIO_SetBits(GPIOG, GPIO_Pin_9);

			uint32_t period = InjectionTime;
			injTimer.TIM_Period = period;
			TIM_TimeBaseInit(TIM3, &injTimer);
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
			TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
			TIM_Cmd(TIM3, ENABLE);

			TIM_SetCounter(TIM3, 0);
			injectionON = 1;
		} else {
			GPIO_ResetBits(GPIOG, GPIO_Pin_9);
			TIM_Cmd(TIM3, DISABLE);
			TIM_SetCounter(TIM3, 0);
			injectionON = 0;
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		}

    }
}

uint32_t SensorDataCounter = 0;
int newSample = 0;
void DMA2_Stream1_IRQHandler() {
	if(DMA_GetITStatus(DMA2_Stream1, DMA_IT_TCIF1)) {
		SensorDataCounter++;
        newSample = 1;
	    DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_TCIF1);
	}
}


int main(void)
{
	//Buffer storing the LCD position
	Sample samplesBuffer[300];
	for (int i = 0; i < 300; i++) {
		samplesBuffer[i].sensor1 = 0;
		samplesBuffer[i].sensor2 = 0;
		samplesBuffer[i].sensor3 = 0;
	}
	int sampleIndex = 0;

	/* Enable clock for SYSCFG */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	initPorts(); //Setup the pin configuration

	/* ------------- */
	/* FIRST PHASE */

	/***	MODULE 1	***/
	initTimer1Ms(); // Initialize 1 ms timer
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);

	/***	MODULE 2	***/
	initTimerSim();

	/***	MODULE 3	***/
	initSensorInts();
	initSensorTimer();

	/***	MODULE 4	***/
	initInjTimer();

	/* ------------- */
	/* SECOND PHASE */

	initTimerADC();
	samplesConfig();
	samplesCopyConfig();

	/* ------------- */
	/* THIRD PHASE */

	initLCD();

	while (1)
	{
		if (USR_Flag && STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET) {
			updateEngineSpeed();
			updatePeriodSim();
			USR_Flag = 0;
		}
		if (newSample) {
			newSample = 0;
			samplesBuffer[sampleIndex++] = getMeanPos();
			if (sampleIndex == 300) sampleIndex = 0;
			//GPIO_SetBits(GPIOE, GPIO_Pin_4);
			displaySamples(samplesBuffer, sampleIndex);	
			//GPIO_ResetBits(GPIOE, GPIO_Pin_4);
		}
	}
}

/*
 * Callback used by stm324xg_eval_i2c_ee.c.
 * Refer to stm324xg_eval_i2c_ee.h for more info.
 */
uint32_t sEE_TIMEOUT_UserCallback(void)
{
  /* TODO, implement your code here */
  while (1)
  {

  }
}
