/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
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
void printmsg(char *msg);

//Global space for variables
char usr_msg[250];
uint8_t Button_Status_flag = NOT_PRESSED;

//Variables related Peripherals
GPIO_InitTypeDef GpioUARTpins;
GPIO_InitTypeDef GpioLEDpin;
GPIO_InitTypeDef GpioButtonpin;
USART_InitTypeDef Usart1Init;
USART_HandleTypeDef Usart1;



//Task Handles and functions
TaskHandle_t LEDTaskHandle = NULL;
TaskHandle_t ButtonTaskHandle = NULL;

void vLEDTaskFunction(void *params);
void vButtonTaskFunction(void *params);


int main(void)
{

#ifndef USE_SEMIHOSTING
	initialise_monitor_handles();
#endif

	//printf("RTOS application of LED and Button \n");

	// Private function defined to setup the Hardware
	prvSetupHardware();

	// Create LED Task
	xTaskCreate(vLEDTaskFunction, "LED-Task",configMINIMAL_STACK_SIZE, NULL, 1, &LEDTaskHandle);

	// Create Button Task
	xTaskCreate(vButtonTaskFunction, "Button-Task",configMINIMAL_STACK_SIZE, NULL, 1, &ButtonTaskHandle);

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
			// Turn-on the Blue LED
			HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET);
		}
		else
		{
			//Turn-off the Blue LED
			HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);
		}

	}
}

void vButtonTaskFunction(void *params)
{
	while(1)
	{
		//Check the SW2 (User button)
		if(HAL_GPIO_ReadPin(BUTTON_SW2_GPIO_PORT, BUTTON_SW2_PIN))
		{
			//Button is not pressed on the nucleo board
			Button_Status_flag = PRESSED;
		}
		else
		{
			//Button is pressed on the nucleo board
			Button_Status_flag = NOT_PRESSED;
		}
	}
}

static void prvSetupHardware(void)
{
	//Setup USART1
	prvSetupLED();
	prvSetupButton();

}


static void prvSetupLED(void)
{
	//printf("LED1 (Blue), LED2 (Green), LED3 (RED) Initialization \n");
	LED1_GPIO_CLK_ENABLE();
	//LED2_GPIO_CLK_ENABLE(); Since all the buttons are connected to same port (GPIOB),
	//LED3_GPIO_CLK_ENABLE(); We don't need to enable the GPIO port again.

	//Zeroing each and every member element of the structure.
	memset(&GpioLEDpin, 0, sizeof(GpioLEDpin));
	GpioLEDpin.Pin = LED1_PIN | LED2_PIN | LED3_PIN;
	GpioLEDpin.Mode = GPIO_MODE_OUTPUT_PP;
	GpioLEDpin.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GpioLEDpin.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(GPIOB, &GpioLEDpin);

	HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
	HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
	HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);

	HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
	//HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED2_PIN);
	//HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED3_PIN);

}

static void prvSetupButton(void)
{
	//Using the User Button 2 (SW2). It is connected to PD0
	//1. Enable the clock
	BUTTON_SW2_GPIO_CLK_ENABLE();

	//Zeroing each and every member element of the structure.
	memset(&GpioButtonpin, 0, sizeof(GpioLEDpin));

	//Set the required init values of button
	GpioButtonpin.Pin = BUTTON_SW2_PIN;
	GpioButtonpin.Mode = GPIO_MODE_INPUT;
	GpioButtonpin.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GpioButtonpin.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(BUTTON_SW2_GPIO_PORT, &GpioButtonpin);

}

void printmsg(char *msg)
{
	HAL_USART_Transmit(&Usart1, (uint8_t *)msg, strlen(msg), 1);
}
