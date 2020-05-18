/**
  ******************************************************************************
  * @file    Task_Notify.c
  * @author  Ac6
  * @version V1.0
  * @date    18-05-2020
  ******************************************************************************
*/


#include "stm32wbxx.h"
#include "stm32wbxx_nucleo.h"
#include "stm32wbxx_hal_usart.h"


#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "string.h"
#include "time.h"

#ifndef USE_SEMIHOSTING
//Used for Semi-Hosting
extern void initialise_monitor_handles(void);
#endif


//Macros
#define TRUE 1
#define FALSE 0
#define AVAILABLE TRUE
#define NOT_AVAILABLE FALSE

TaskHandle_t xLEDTaskHandle = NULL;
TaskHandle_t xButtonTaskHandle = NULL;

//Task functions prototypes
void vLEDTaskFunction(void *params);
void vButtonTaskFunction(void *params);

static void prvSetupHardware(void);
static void prvSetupUSART(void);
static void prvSetupLED(void);
static void prvSetupButton(void);
void printmsg(char *msg);
char usr_msg[250];

//Variables related Peripherals
GPIO_InitTypeDef GpioUARTpins;
USART_InitTypeDef Usart1Init;
USART_HandleTypeDef Usart1;
GPIO_InitTypeDef GpioLEDpin;
GPIO_InitTypeDef GpioButtonpin;


int main(void)
{

#ifndef USE_SEMIHOSTING
	initialise_monitor_handles();
#endif

	//printf("Hello World RTOS \n");

	// Enable the DWT Cycle Count Register (SEGGER Settings)
	DWT->CTRL |= (1 << 0);

	// Private function defined to setup the Hardware
	prvSetupHardware();

	// Start recording the FreeRTOS data in SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Create Tasks: LED-Task and Button-Task
	xTaskCreate(vLEDTaskFunction, "LED-Task", configMINIMAL_STACK_SIZE, NULL, 2, &xLEDTaskHandle);
	xTaskCreate(vButtonTaskFunction, "Button-Task", configMINIMAL_STACK_SIZE, NULL, 2, &xButtonTaskHandle);

	//4. Schedule the tasks
	//printf("Scheduling the tasks created \n");
	vTaskStartScheduler();
	//printf("Tasks are successfully scheduled \n");

	for(;;);

}



void vLEDTaskFunction(void *params)
{
	while(1)
	{
		sprintf(usr_msg, "This is USART message from Task-1");
		printmsg(usr_msg);

	}

}


void vButtonTaskFunction(void *params)
{
	while(1)
	{
		sprintf(usr_msg, "This is USART message from Task-2");
		printmsg(usr_msg);

	}

}

static void prvSetupHardware(void)
{
	//Setup USART1
	prvSetupUSART();
	prvSetupLED();
	prvSetupButton();

}

static void prvSetupUSART(void)
{
	//1. Enable the UART1 and GPIOB Peripheral Clocks
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	//In UART connection with Virtual COM-port, PB6->TX and PB7->RX
	//2. Alternate Functionality Configuration to make Port B pins work as UART pins
	//Zeroing each and every member element of the structure.
	memset(&GpioUARTpins, 0, sizeof(GpioUARTpins));
	GpioUARTpins.Pin = GPIO_PIN_6 | GPIO_PIN_7;
	GpioUARTpins.Mode = GPIO_MODE_AF_PP;
	GpioUARTpins.Alternate = GPIO_AF7_USART1;
	GpioUARTpins.Pull = GPIO_PULLUP;

	HAL_GPIO_Init(GPIOB, &GpioUARTpins);

	//3. Configure and initialize UART parameters
	//Zeroing each and every member element of the structure.
	memset(&Usart1Init, 0, sizeof(Usart1Init));
	memset(&Usart1, 0, sizeof(Usart1));

	Usart1Init.BaudRate = 115200;
	Usart1Init.WordLength = USART_WORDLENGTH_8B;
	Usart1Init.StopBits = USART_STOPBITS_1;
	Usart1Init.Parity = USART_PARITY_NONE;
	Usart1Init.Mode = USART_MODE_TX_RX;
	Usart1Init.CLKPolarity = USART_POLARITY_LOW;
	Usart1Init.CLKPhase = USART_PHASE_1EDGE;
	Usart1Init.CLKLastBit = USART_LASTBIT_DISABLE;
	Usart1Init.ClockPrescaler = USART_PRESCALER_DIV1;

	Usart1.Instance = USART1;
	Usart1.Init = Usart1Init;

	uint16_t USARTSetUpResult = HAL_USART_Init(&Usart1);

	if(USARTSetUpResult == HAL_ERROR)
	{
		//printf("USART Initialization was not successful \n");
	}


}

static void prvSetupLED(void)
{
	//printf("LED1 (Blue LED) Initialization \n");
	LED1_GPIO_CLK_ENABLE();

	//Zeroing each and every member element of the structure.
	memset(&GpioLEDpin, 0, sizeof(GpioLEDpin));
	GpioLEDpin.Pin = LED1_PIN;
	GpioLEDpin.Mode = GPIO_MODE_OUTPUT_PP;
	GpioLEDpin.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GpioLEDpin.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(GPIOB, &GpioLEDpin);
	HAL_GPIO_TogglePin(GPIOB, LED1_PIN);
	HAL_GPIO_TogglePin(GPIOB, LED1_PIN);

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

}
void printmsg(char *msg)
{
	HAL_USART_Transmit(&Usart1, (uint8_t *)msg, strlen(msg), 1);
}
