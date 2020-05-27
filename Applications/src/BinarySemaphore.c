/*
 * BinarySemaphore.c
 *
 *  Created on: 27-May-2020
 *      Author: Rahul
 */

/*
 * This is a very basic binary semaphore example.
 * It should be improved to make it more dependent on semaphore rather that being dependent of Queue
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
xTaskHandle xManagerTask = NULL;
xTaskHandle xEmployeeTask = NULL;
void vManagerTaskFunction (void *params);
void vEmployeeTaskFunction (void *params);

//Binary semaphore - To synchronize Manager and Employee tasks
xSemaphoreHandle xWorkSemaphore;

//Queue to store the task assigned to the Employee
xQueueHandle xWorkQueue;

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

	//Start Recording for SEGGER SystemView
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	sprintf(UsrMsg,"Example of Binary semaphore synchronization between 2 Tasks \r\n");
	printmsg(UsrMsg);

	//Create Semaphore
	vSemaphoreCreateBinary(xWorkSemaphore);

	//Create Queue
	xWorkQueue = xQueueCreate(1, sizeof(unsigned int));

	if( (xWorkSemaphore != NULL) && (xWorkQueue != NULL) )
	{
		//Create Manager Task. It will be higher priority task
		xTaskCreate(vManagerTaskFunction, "Manager-Task", configMINIMAL_STACK_SIZE, NULL, 2, &xManagerTask);

		//Create Employee Task.
		xTaskCreate(vEmployeeTaskFunction, "Employee-Task", configMINIMAL_STACK_SIZE, NULL, 2, &xEmployeeTask);

		//Schedule the tasks
		vTaskStartScheduler();
	}
	else
	{
		sprintf(UsrMsg, "Semaphore or Queue creation failed... :( \r\n");
		printmsg(UsrMsg);
	}

	/*
	 * If scheduler can start the tasks and run them, the program will never reach here.
	 * If the program comes to the below line, that means there was a problem while creating or scheduling the tasks
	 */
	for(;;);
}


void vManagerTaskFunction (void *params)
{
	unsigned int ulWorkTaskID;
	BaseType_t xSendStatus = pdFAIL;
	/*
	 * Semaphore is created in an 'empty' state. At first, we should give it using
	 * xSemaphoreGive() API function before taking it.
	 */
	xSemaphoreGive(xWorkSemaphore);

	while(1)
	{
		//Get a working Task Id.
		ulWorkTaskID = (unsigned int) (rand() & 0x1FF);

		//Send the created WorkTaskID to Employee task using the queue
		xSendStatus = xQueueSend(xWorkQueue, &ulWorkTaskID, portMAX_DELAY);

		if( xSendStatus != pdPASS )
		{
			sprintf(UsrMsg, "Manager Task: Could not send the WorkTaskID to Queue... :( \r\n");
			printmsg(UsrMsg);
		}
		else
		{
			//QueueSend() successful
			//Notify the Employee task by giving the semaphore
			xSemaphoreGive(xWorkSemaphore);

			//yield the process so that blocked Employee task can be run
			taskYIELD();
		}

	}
}


void vEmployeeTaskFunction (void *params)
{
	unsigned int ulWorkTaskID;
	BaseType_t xReceiveStatus = pdFAIL;

	while(1)
	{
		//Take the Semaphore
		if(xSemaphoreTake(xWorkSemaphore, 0))
		{

			//Receive the TaskID from the Queue
			xReceiveStatus = xQueueReceive(xWorkQueue, &ulWorkTaskID, 0);

			if(xReceiveStatus == pdPASS)
			{
				//Received the TaskID
				sprintf(UsrMsg, "Employee Task: Working on Task ID - %d \r\n", ulWorkTaskID);
				printmsg(UsrMsg);
				//Block for some amount of time to work on the given task
				vTaskDelay(ulWorkTaskID);
			}
			else
			{
				//Nothing is received
				sprintf(UsrMsg, "No Task is received to work on. \r\n");
				printmsg(UsrMsg);
			}
		}
	}
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
