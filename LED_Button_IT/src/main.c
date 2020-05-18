/**
  ******************************************************************************
  * @file    main.c
  * @author  Rahul
  * @version V1.0
  * @date    16-May-2020
  * @brief   Default main function.
  ******************************************************************************
*/



#include "stm32wbxx.h"
#include "stm32wbxx_nucleo.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

#ifdef USE_SEMIHOSTING
//Used for Semi-Hosting
extern void initialise_monitor_handles(void);
#endif

//Macros
#define TRUE 			1
#define FALSE 			0
#define AVAILABLE 		TRUE
#define NOT_AVAILABLE 	FALSE
#define PRESSED			TRUE
#define NOT_PRESSED		FALSE

//Private setup functions' prototypes
static void prvSetupHardware(void);
static void prvSetupLED(void);
static void prvSetupButton(void);

//Global space for variables
char usr_msg[250];
uint8_t Button_Status_flag = NOT_PRESSED;

//Variables related Peripherals
GPIO_InitTypeDef GpioLEDpin;
GPIO_InitTypeDef GpioButtonpin;
EXTI_ConfigTypeDef EXTIConfig;
EXTI_HandleTypeDef	EXTIHandle;

//Task Handles and functions
TaskHandle_t LEDTaskHandle = NULL;
void vLEDTaskFunction(void *params);


int main(void)
{

#ifdef USE_SEMIHOSTING
	initialise_monitor_handles();
#endif

	// Enable the DWT Cycle Count Register (SEGGER Settings)
	DWT->CTRL |= (1 << 0);
	// Start recording the FreeRTOS data in SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();


	// Private function defined to setup the Hardware
	prvSetupHardware();

	// Create Button Handler Task
	xTaskCreate(vLEDTaskFunction, "LED-Task",configMINIMAL_STACK_SIZE, NULL, 1, &LEDTaskHandle);

	//Schedule tasks
	vTaskStartScheduler();

	for(;;);
}

void vLEDTaskFunction(void *params)
{
	while(1)
	{
		if(Button_Status_flag == PRESSED)
		{
			HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET);
		}
		else
		{
			HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);
		}
	}
}

static void prvSetupHardware(void)
{
	//Setup
	prvSetupLED();
	prvSetupButton();
}


static void prvSetupLED(void)
{
	//printf("LED1 (Blue), LED2 (Green), LED3 (RED) Initialization \n");
	LED1_GPIO_CLK_ENABLE();

	//Zeroing each and every member element of the structure.
	memset(&GpioLEDpin, 0, sizeof(GpioLEDpin));
	GpioLEDpin.Pin = LED1_PIN;
	GpioLEDpin.Mode = GPIO_MODE_OUTPUT_PP;
	GpioLEDpin.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GpioLEDpin.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(GPIOB, &GpioLEDpin);

	HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
	HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);

}

static void prvSetupButton(void)
{
	//Using the External Button PC2
	//Enable the clock
	__HAL_RCC_GPIOC_CLK_ENABLE();

	//Zeroing each and every member element of the structure.
	memset(&GpioButtonpin, 0, sizeof(GpioButtonpin));

	//Set the required init values of button
	GpioButtonpin.Pin = GPIO_PIN_2;
	GpioButtonpin.Mode = GPIO_MODE_INPUT;
	GpioButtonpin.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GpioButtonpin.Pull = GPIO_PULLUP;

	HAL_GPIO_Init(GPIOC, &GpioButtonpin);

	/*
	 * Interrupt configuration for the button (GPIOD - Pin 0)
	 */
	//System Configuration for the EXTI line (SYSCFG settings)

	EXTIConfig.GPIOSel = EXTI_GPIOC;
	EXTIConfig.Line = EXTI_LINE_2;
	EXTIConfig.Mode = EXTI_MODE_INTERRUPT;
	EXTIConfig.Trigger = EXTI_TRIGGER_FALLING;

	EXTIHandle.Line = EXTI_LINE_2;

	HAL_EXTI_SetConfigLine(&EXTIHandle, &EXTIConfig);

	//NVIC Settings (IRQ settings for the selected EXTI line)
	NVIC_SetPriority(EXTI2_IRQn, 5);
	NVIC_EnableIRQ(EXTI2_IRQn);
}

void EXTI2_IRQHandler()
{
	traceISR_ENTER();	//This is SEGGER function. Used to trace ISR

	//1. Clear the interrupt pending bit of EXTI line
	HAL_EXTI_ClearPending(&EXTIHandle, EXTI_TRIGGER_RISING);

	Button_Status_flag ^= 1;

	traceISR_EXIT(); 	//This is SEGGER function. Used to trace ISR

}
