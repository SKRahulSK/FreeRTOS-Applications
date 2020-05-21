/*
 * IdleHookPowerSaving.c
 *
 *  Created on: 21-May-2020
 *      Author: Rahul
 */

/*
 * This example demonstrates the usage of Idle Hook function implementation
 * The function makes the MCU go into power saving mode.
 */

#include "stm32wbxx.h"
#include "stm32wbxx_nucleo.h"
#include "stm32wbxx_hal_usart.h"


#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "time.h"
#include "string.h"

//Macros
#define TRUE 			1
#define FALSE 			0

//Task handles and function prototypes
TaskHandle_t xTask1Handle = NULL;
TaskHandle_t xTask2Handle = NULL;
void vTask1Function(void *params);
void vTask2Function(void *params);

static void prvSetupLED(void);
static void prvSetupUSART(void);
void printmsg(char *msg);
char usr_msg[250];

//Variables related Peripherals
GPIO_InitTypeDef GpioLEDpin, GpioUARTpins;
USART_InitTypeDef Usart1Init;
USART_HandleTypeDef Usart1;

int main(void)
{

	// Enable the DWT Cycle Count Register (SEGGER Settings)
	DWT->CTRL |= (1 << 0);

	// Private function called to setup the Hardware
	prvSetupUSART();
	prvSetupLED();

	// Start recording the FreeRTOS Application data in SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Create Tasks:
	xTaskCreate(vTask1Function, "Task-1", 512, NULL, 2, &xTask1Handle);
	xTaskCreate(vTask2Function, "Task-2", 512, NULL, 3, &xTask2Handle);

	//4. Schedule the tasks
	vTaskStartScheduler();

	for(;;);

}

void vTask1Function(void *params)
{
	//This task prints the status of the Blue LED through USART1 and goes to blocking state for 1 sec.
	while(1)
	{
		if(HAL_GPIO_ReadPin(LED1_GPIO_PORT, LED1_PIN))
		{
			sprintf(usr_msg, "The Blue LED is switched ON \n\r");
		}
		else
		{
			sprintf(usr_msg, "The Blue LED is switched OFF \n\r");
		}

		printmsg(usr_msg);

		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void vTask2Function(void *params)
{
	//This task toggles the Blue LED and goes to blocked state for 1 sec.
	while(1)
	{
		HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
		vTaskDelay(pdMS_TO_TICKS(1000));
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

	//uint8_t x = HAL_USART_Init(&Usart1);
	uint16_t USARTSetUpResult = HAL_USART_Init(&Usart1);

	if(USARTSetUpResult == HAL_ERROR)
	{
		//printf("USART Initialization was not successful \n");
	}
}

void printmsg(char *msg)
{
	HAL_USART_Transmit(&Usart1, (uint8_t *)msg, strlen(msg), 1);
}

//Implement the Idel Hook function
void vApplicationIdleHook()
{
	//Send the CPU to normal sleep mode
	__WFI();
}
