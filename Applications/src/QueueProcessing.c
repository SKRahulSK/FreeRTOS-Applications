/*
 * QueueProcessing.c
 *
 *  Created on: 22-May-2020
 *      Author: Rahul
 */


#include "FreeRTOS.h"
#include "task.h"
#include "stm32wbxx.h"
#include "stm32wbxx_nucleo.h"
#include "stdio.h"
#include "string.h"
#include "queue.h"
#include "timers.h"	//For software timers

//Macros
#define TRUE 			1
#define FALSE 			0

//Command macros
#define LED_ON_CMD				1
#define LED_OFF_CMD				2
#define LED_TOGGLE_CMD			3
#define LED_TOGGLE_OFF_CMD		4
#define LED_READ_STATUS_CMD		5
#define RTC_PRINT_DATETIME_CMD	6
#define EXIT_CMD				0

//Task handles and function prototypes
TaskHandle_t xUSARTWriteTaskHandle = NULL;
TaskHandle_t xMenuHandleTaskHandle = NULL;
TaskHandle_t xCmdHandleTaskHandle = NULL;
TaskHandle_t xCmdProcessTaskHandle = NULL;
TimerHandle_t LEDTimerHandle = NULL;
void vUSARTWriteTaskFunction(void *params);
void vMenuHandleTaskFunction(void *params);
void vCmdHandleTaskFunction(void *params);
void vCmdProcessTaskFunction(void *params);

/*
 * Queue Handles and related variable
 */
QueueHandle_t AppCmdQueueHandle = NULL;
QueueHandle_t UsartWriteQueueHandle = NULL;
//Command structure
typedef struct AppCmd
{
	uint8_t CmdNumber;
	uint8_t CmdArgs[10];
}AppCmd_t;

//Variable related to peripherals
GPIO_InitTypeDef GpioLEDpin, GpioUARTpins;
UART_HandleTypeDef Uart1;
UART_InitTypeDef Uart1Init;
RTC_HandleTypeDef RTCHandle;
RTC_InitTypeDef RTCInit;

//Helper functions
static void prvSetupRTC(void);
static void prvSetupLED(void);
static void prvSetupUART(void);
uint8_t getCommandCode(uint8_t *buffer);
void getArguments(uint8_t *buffer);
void LEDToggleStart(void);
void LEDToggleStop(void);
void PrintLEDStatus(char *LEDStatus);
void PrintRTCInfo(char *RTCInfo);

//Helper variables
void printmsg(char *msg);
char usr_msg[250];
uint8_t CmdBuffer[20] = {0};
uint8_t CmdLength=0;
char Menu[] = {"\
\r\nLED_ON			---> 1 \
\r\nLED_OFF			---> 2 \
\r\nLED_TOGGLE		---> 3 \
\r\nLED_TOGGLE_OFF		---> 4 \
\r\nLED_READ_STATUS		---> 5 \
\r\nRTC_PRINT_DATETIME	---> 6 \
\r\nEXIT_APP		---> 0 \
\r\nType your option here: " };

int main()
{
	// Enable the DWT Cycle Count Register (SEGGER Settings)
	DWT->CTRL |= (1 << 0);

	// Private function called to setup the Hardware
	prvSetupLED();
	prvSetupUART();
	prvSetupRTC();

	sprintf(usr_msg, "\r\nThis is the Queue Processing Example application: \r\n");
	printmsg(usr_msg);

	// Start recording the FreeRTOS Application data in SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//Create queues ( Command queue and Usart queue)
	/*
	 * The below queue create statement creates a queue with size 10 words (40 bytes),
	 * whereas xQueueCreate(10,sizeof(AppCmd_t)) creates queue with size of 110 bytes
	 */
	AppCmdQueueHandle = xQueueCreate(10,sizeof(AppCmd_t *));
	if(AppCmdQueueHandle == NULL)
	{
		sprintf(usr_msg, " App Command Queue creation failed !");
		printmsg(usr_msg);
		return 0;
	}

	UsartWriteQueueHandle = xQueueCreate(10, sizeof(char *));
	if(UsartWriteQueueHandle == NULL)
	{
		sprintf(usr_msg, "Write message Queue creation failed !");
		printmsg(usr_msg);
		return 0;
	}

	//Create tasks
	xTaskCreate(vUSARTWriteTaskFunction, "USART-Write", 500, NULL, 5, &xUSARTWriteTaskHandle);
	xTaskCreate(vMenuHandleTaskFunction, "USARTRead-MenuPrint", 500, NULL, 4, &xMenuHandleTaskHandle);
	xTaskCreate(vCmdHandleTaskFunction, "Command-Handling", 500, NULL, 5, &xCmdHandleTaskHandle);
	xTaskCreate(vCmdProcessTaskFunction, "Command-Processing", 500, NULL, 5, &xCmdProcessTaskHandle);

	//Schedule the tasks
	vTaskStartScheduler();

	for(;;);
}

void vUSARTWriteTaskFunction(void *params)
{
	char *UsartWriteBuffer = NULL;
	while(1)
	{
		//Read from the Queue
		xQueueReceive(UsartWriteQueueHandle, &UsartWriteBuffer, portMAX_DELAY);
		//Write the message to the USART1
		printmsg(UsartWriteBuffer);

	}
}

void vMenuHandleTaskFunction(void *params)
{
	char *pData = Menu;

	while(1)
	{
		xQueueSend(UsartWriteQueueHandle, &pData, portMAX_DELAY);
		//Wait until the user notifies with the command
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
	}
}

void vCmdHandleTaskFunction(void *params)
{
	uint8_t CmdCode = 0;
	AppCmd_t *NewCmd;

	while(1)
	{
		//Wait to be notified from USART IRQ Handler
		xTaskNotifyWait(0,0,NULL,portMAX_DELAY);

		//Set the command number and arguments
		NewCmd = (AppCmd_t *) pvPortMalloc(sizeof(AppCmd_t));

		//Here we are accessing shared resource which is also being used by ISR.
		//Therefore, make this as a critical section
		taskENTER_CRITICAL();	//It disables interrupts
		CmdCode = getCommandCode(CmdBuffer);
		NewCmd->CmdNumber = CmdCode;
		getArguments(NewCmd->CmdArgs);
		taskEXIT_CRITICAL();	//It enables interrupts

		//Send the command to queue
		xQueueSend(AppCmdQueueHandle, &NewCmd, portMAX_DELAY);

	}
}

void vCmdProcessTaskFunction(void *params)
{
	AppCmd_t *CmdToProcess;
	char *LEDStatus = NULL;
	char RTCInfo[50];
	char *ErrorMsg = NULL;

	while(1)
	{
		xQueueReceive(AppCmdQueueHandle, (void*)&CmdToProcess, portMAX_DELAY);

		switch (CmdToProcess->CmdNumber)
		{
			case LED_ON_CMD:
				HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, SET);
				break;

			case LED_OFF_CMD:
				HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, RESET);
				break;

			case LED_TOGGLE_CMD:
				//Toggle LED using Software Timers
				LEDToggleStart();
				break;

			case LED_TOGGLE_OFF_CMD:
				//Stop the LED Toggle using Software Timers
				LEDToggleStop();
				break;

			case LED_READ_STATUS_CMD:
				//Print the LED status
				PrintLEDStatus(LEDStatus);
				break;

			case RTC_PRINT_DATETIME_CMD:
				//Print the RTC info
				PrintRTCInfo(RTCInfo);
				break;

			case EXIT_CMD:
				//Delete the tasks
				vTaskDelete(xUSARTWriteTaskHandle);
				vTaskDelete(xCmdHandleTaskHandle);
				vTaskDelete(xMenuHandleTaskHandle);

				//Disable all interrupts
				__HAL_UART_DISABLE_IT(&Uart1, UART_IT_RXNE);

				//Delete the running task (i.e., Command Processing Task)
				vTaskDelete(NULL);

				//After this the MCU will go into low power mode

				break;

			default:
				//Print the error message
				ErrorMsg = "\r\n Invalid command.!";
				xQueueSend(UsartWriteQueueHandle, &ErrorMsg, portMAX_DELAY);
				break;
		}

		//Free the allocated memory for the CmdToProcess variable
		vPortFree(CmdToProcess);
	}
}

static void prvSetupRTC(void)
{
	/*
	 * Rahul - I should improve this to get the real-time date and time.
	 */
	//Enable the RTC peripheral clock
	//__HAL_RCC_RTC_ENABLE();

	/*
	//Initialize the RTC
	memset(&RTCInit, 0, sizeof(RTCInit));
	memset(&RTCHandle, 0, sizeof(RTCHandle));

	RTCInit.HourFormat = RTC_HOURFORMAT_24;
	RTCInit.OutPut = RTC_OUTPUT_WAKEUP;
	RTCInit.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	RTCInit.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	RTCInit.OutPutType = RTC_OUTPUT_TYPE_PUSHPULL;

	RTCHandle.Init = RTCInit;
	RTCHandle.Instance = RTC;

 	HAL_RTC_Init(&RTCHandle);
	*/
}

static void prvSetupLED(void)
{
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

	//5. Enable the USART byte reception interrupt in the MCU
	__HAL_UART_ENABLE_IT(&Uart1, UART_IT_RXNE);

	//6. Set the USART1 interrupt priority in NVIC
	NVIC_SetPriority(USART1_IRQn, 5); //Priority should be less than or equal to configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

	//7. Enable the USART1 IRQ in NVIC
	NVIC_EnableIRQ(USART1_IRQn);


}

void printmsg(char *msg)
{
	HAL_UART_Transmit(&Uart1, (uint8_t *)msg, strlen(msg), 1);
}


void USART1_IRQHandler(void)
{
	uint8_t RxData;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;


	/*
	 * This handler is the common interrupt handler for all the interrupts related to USART1.
	 * Therefore we should check for the particular RXNE interrupt to read the data.
	 */

	if( __HAL_UART_GET_FLAG(&Uart1, UART_FLAG_RXNE) )
	{
		//Read the UART message
		HAL_UART_Receive(&Uart1,&RxData, sizeof(uint8_t), 10);

		CmdBuffer[CmdLength++] = RxData;

		if(RxData == '\r')
		{
			//The user has pressed the Enter button. It is the end of the command.

			//Reset the command length
			CmdLength = 0;

			//Notify the Command Handling Task
			xTaskNotifyFromISR(xCmdHandleTaskHandle,0,eNoAction, &xHigherPriorityTaskWoken);
			//Notify the Menu Handling Task
			xTaskNotifyFromISR(xMenuHandleTaskHandle,0,eNoAction, &xHigherPriorityTaskWoken);
		}
	}

	/*
	 * If the above FreeRTOS APIs unblocks any other higher priority tasks,
	 * then yield the processor to the higher priority task which has been unblocked.
	 */
	if(xHigherPriorityTaskWoken)
	{
		taskYIELD();
	}

}

uint8_t getCommandCode(uint8_t *buffer)
{
	return (buffer[0] - 48);
}

void getArguments(uint8_t *buffer)
{

}

void ToggleLED(TimerHandle_t xTimer)
{
	HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
}

void LEDToggleStart(void)
{
	uint32_t ToggleDuration = pdMS_TO_TICKS(500);

	if(LEDTimerHandle == NULL)
	{
		//Create Software Timer
		LEDTimerHandle = xTimerCreate("LED-Timer", ToggleDuration, pdTRUE, NULL, ToggleLED);
		//Start the Software Timer
		xTimerStart(LEDTimerHandle, portMAX_DELAY);
	}
	else
	{
		//Start the Software Timer
		xTimerStart(LEDTimerHandle, portMAX_DELAY);
	}
}

void LEDToggleStop(void)
{
	//Stop the Software Timer
	xTimerStop(LEDTimerHandle, portMAX_DELAY);
}

void PrintLEDStatus(char *LEDStatus)
{
	if(HAL_GPIO_ReadPin(LED1_GPIO_PORT, LED1_PIN))
	{
		//Print "LED is ON!!" msg via USART Write Queue
		LEDStatus = "\r\n LED is ON!! \r\n";
		xQueueSend(UsartWriteQueueHandle, &LEDStatus, portMAX_DELAY);
	}
	else {
		//Print "LED is OFF!!" msg via USART Write Queue
		LEDStatus = "\r\n LED is OFF!! \r\n";
		xQueueSend(UsartWriteQueueHandle, &LEDStatus, portMAX_DELAY);
	}
}

void PrintRTCInfo(char *RCTInfo)
{
	RTC_TimeTypeDef TimeStructure;
	RTC_DateTypeDef DateStructure;

	//We must call HAL_RTC_GetDate() after HAL_RTC_GetTime() to unlock the values/.
	//(Check the RTC peripheral for more details)
	HAL_RTC_GetTime(&RTCHandle, &TimeStructure, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&RTCHandle, &DateStructure, RTC_FORMAT_BIN);

	sprintf(RCTInfo, "\r\n Time: %02d:%02d:%02d \r\n Date: %02d/%02d/%04d \r\n", TimeStructure.Hours, TimeStructure.Minutes, TimeStructure.Seconds, DateStructure.Date, DateStructure.Month, DateStructure.Year);
	xQueueSend(UsartWriteQueueHandle, &RCTInfo, portMAX_DELAY);
}

//Implement the Idle Hook function
void vApplicationIdleHook()
{
	//Send the CPU to normal sleep mode
	__WFI();
}
