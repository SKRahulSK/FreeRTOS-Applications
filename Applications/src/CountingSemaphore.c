/*
 * CountingSemaphore.c
 *
 *  Created on: 27-May-2020
 *      Author: Rahul
 */

/*
 * This application has one Periodic task and Handler task.
 * It demonstrates the usage of counting semaphore to latch events.
 * It also shows software interrupts generation (to simulate interrupts).
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stm32wbxx.h"
#include "stm32wbxx_nucleo.h"
#include "stdio.h"
#include "string.h"
#include "queue.h"
#include "semphr.h"
#include "stdlib.h"

//Task handles and functions
xTaskHandle xHandlerTask = NULL;
xTaskHandle xPeriodicTask = NULL;
void vHandlerTaskFunction (void *params);
void vPeriodicTaskFunction (void *params);
/* Enable the software interrupt and set its priority. */
static void prvSetupSoftwareInterrupt();

//Binary semaphore - To synchronize Manager and Employee tasks
xSemaphoreHandle xCountingSemaphore;

//UART Handle and Init types
UART_HandleTypeDef Uart1;
UART_InitTypeDef Uart1Init;
GPIO_InitTypeDef GpioUARTpins;

//Private helper functions and variables
static void prvSetupUART(void);
void printmsg(char *msg);
char UsrMsg[250];

int main()
{
	// Enable the DWT Cycle Count Register (SEGGER Settings)
	DWT->CTRL |= (1 << 0);

	// Private function called to setup the Hardware
	prvSetupUART();
	prvSetupSoftwareInterrupt();

	//Start Recording for SEGGER SystemView
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	sprintf(UsrMsg,"Example of Counting semaphore for synchronization between 2 Tasks and event latching \r\n");
	printmsg(UsrMsg);

	//Create Counting Semaphore
	xCountingSemaphore = xSemaphoreCreateCounting(10, 0);

	if( xCountingSemaphore != NULL )
	{
		//Create Handler Task.
		xTaskCreate(vHandlerTaskFunction, "Handler-Task", configMINIMAL_STACK_SIZE, NULL, 2, &xHandlerTask);

		//Create Periodic Task. It will be higher priority task
		xTaskCreate(vPeriodicTaskFunction, "Periodic-Task", configMINIMAL_STACK_SIZE, NULL, 3, &xPeriodicTask);

		//Schedule the tasks
		vTaskStartScheduler();
	}
	else
	{
		sprintf(UsrMsg, "Semaphore creation failed... :( \r\n");
		printmsg(UsrMsg);
	}

	/*
	 * If scheduler can start the tasks and run them, the program will never reach here.
	 * If the program comes to the below line, that means there was a problem while creating or scheduling the tasks
	 */
	for(;;);
}


void vHandlerTaskFunction (void *params)
{
	while(1)
	{
		//Wait and take the Semaphore when it is given by the interrupt handler
		xSemaphoreTake(xCountingSemaphore, portMAX_DELAY);

		sprintf(UsrMsg, "Handler Task: Processing the event %ld \r\n", uxSemaphoreGetCount(xCountingSemaphore));
		printmsg(UsrMsg);

	}

}


void vPeriodicTaskFunction (void *params)
{
	/*
	 * This periodic task is used to simulate interrupts for every 500ms.
	 */
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(500));

		sprintf(UsrMsg, "Periodic Task: Pending the interrupt. \r\n");
		printmsg(UsrMsg);

		//Pend the interrupt
		NVIC_SetPendingIRQ(EXTI15_10_IRQn);

		sprintf(UsrMsg, "Periodic Task: Resuming. \r\n");
		printmsg(UsrMsg);

	}

}

void EXTI15_10_IRQHandler(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	uint8_t ucCount = (uint8_t) ( rand() & 0x09);	//The ucCount maximum value should be Semaphores MaxCount

	sprintf(UsrMsg, "Button Interrupt Service Routine. Giving the Semaphore to the Handler task.\r\n");
	printmsg(UsrMsg);

	//Give xcountingSemaphore ucCount number of  times
	for(uint8_t i = 0; i < ucCount; i++)
	{
		xSemaphoreGiveFromISR(xCountingSemaphore, &xHigherPriorityTaskWoken);
	}

}


static void prvSetupSoftwareInterrupt()
{
	/*
	 * Here were simulating the button interrupt by manually setting the interrupt enable bit in the NVIC enable register.
	 * The interrupt service routine uses an (interrupt safe) FreeRTOS API function so the interrupt priority
	 * must be at or below the priority defined by configSYSCALL_INTERRUPT_PRIORITY.
	 */

	NVIC_SetPriority( EXTI15_10_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );

	/* Enable the interrupt. */
	NVIC_EnableIRQ( EXTI15_10_IRQn );
}


static void prvSetupUART(void)
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
	memset(&Uart1Init, 0, sizeof(Uart1Init));
	memset(&Uart1, 0, sizeof(Uart1));

	//UART Initialization
	Uart1Init.BaudRate = 115200;
	Uart1Init.WordLength = UART_WORDLENGTH_8B;
	Uart1Init.HwFlowCtl = UART_HWCONTROL_NONE;
	Uart1Init.Mode = UART_MODE_TX_RX;
	Uart1Init.Parity = UART_PARITY_NONE;
	Uart1Init.StopBits = UART_STOPBITS_1;

	Uart1.Init = Uart1Init;
	Uart1.Instance = USART1;

	//4. Initialize the UART peripheral
	uint16_t UARTSetUpResult = HAL_UART_Init(&Uart1);

	if(UARTSetUpResult == HAL_ERROR)
	{
		//printf("USART Initialization was not successful \n");
	}



}

void printmsg(char *msg)
{
	HAL_UART_Transmit(&Uart1, (uint8_t *)msg, strlen(msg), 1);
}

//Implement the Idle Hook function
void vApplicationIdleHook()
{
	//Send the CPU to normal sleep mode
	__WFI();
}
