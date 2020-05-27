/*
 * UARTInterrupt.c
 *
 *  Created on: 23-May-2020
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

//Task handles and function prototypes
TaskHandle_t xUSARTWriteTaskHandle = NULL;
TaskHandle_t xMenuPrintTaskHandle = NULL;

void vUSARTWriteTaskFunction(void *params);
void vMenuPrintTaskFunction(void *params);


//Variable related to peripherals
GPIO_InitTypeDef GpioLEDpin, GpioUARTpins;
USART_InitTypeDef Usart1Init;
USART_HandleTypeDef Usart1;


static void prvSetupLED(void);
static void prvSetupUART(void);

void printmsg(char *msg);
char usr_msg[250];

uint8_t CmdBuffer[20] = {0};
uint8_t CmdLength=0;

char menu[] = {"\
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

	sprintf(usr_msg, "This is the Queue Processing Example application: \n\r");
	printmsg(usr_msg);

	// Start recording the FreeRTOS Application data in SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();


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

void vMenuPrintTaskFunction(void *params)
{

	char *pData = menu;
	while(1)
	{
		xQueueSend(UsartWriteQueueHandle, &pData, portMAX_DELAY);
		//Wait until the user notifies with his/her command
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
	}
}

void vCmdHandleTaskFunction(void *params)
{
	while(1)
	{

	}
}

void vCmdProcessTaskFunction(void *params)
{
	while(1)
	{

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

	//4. Initialize the USART1 peripheral
	uint16_t USARTSetUpResult = HAL_USART_Init(&Usart1);
	if(USARTSetUpResult == HAL_ERROR)
	{
		//printf("USART Initialization was not successful \n");
	}

	//5. Enable the USART byte reception interrupt in the MCU
	//__HAL_USART_ENABLE_IT(&Usart1, USART_IT_RXNE);

	//6. Set the USART1 interrupt priority in NVIC
	//NVIC_SetPriority(USART1_IRQn, 5); //Priority should be less than or equal to configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY

	//7. Enable the USART1 IRQ in NVIC
	//NVIC_EnableIRQ(USART1_IRQn);


}

void printmsg(char *msg)
{
	HAL_USART_Transmit(&Usart1, (uint8_t *)msg, strlen(msg), 10);
}


void USART1_IRQHandler(void)
{
	uint8_t RxData;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	/*
	 * This handler is the common interrupt handler for all the interrupts related to USART1.
	 * Therefore we should check for the particular RXNE interrupt to read the data.
	 */
	if(__HAL_USART_GET_FLAG(&Usart1,USART_FLAG_RXNE))
	{
		//Read the USART message
		HAL_USART_Receive(&Usart1,&RxData, sizeof(uint8_t), 10);

		CmdBuffer[CmdLength++] = RxData;

		if(RxData == '\r')
		{
			//Then user has pressed the Enter button. It is the end of the command.

			//Notify the Command Handling Task
			xTaskNotifyFromISR(xCmdHandleTaskHandle,0,eNoAction, &xHigherPriorityTaskWoken);
		}

		//Clear the RXNE Flag
		__HAL_USART_CLEAR_FLAG(&Usart1,USART_FLAG_RXNE);
	}

}


