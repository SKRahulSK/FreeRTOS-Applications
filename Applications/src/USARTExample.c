/**
  ******************************************************************************
  * @file    main.c
  * @author  Rahul
  * @version V1.0
  * @date    06-May-2020
  * @brief   Default main function.
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

TaskHandle_t xTask1Handle = NULL;
TaskHandle_t xTask2Handle = NULL;

//Task functions prototypes
void vTask1Function(void *params);
void vTask2Function(void *params);

static void prvSetupHardware(void);
static void prvSetupUSART(void);
static void prvSetupLED(void);
void delay();
void printmsg(char *msg);
char usr_msg[250];



//Mutex variable
uint8_t USART_ACCESS_T1 = AVAILABLE;
uint8_t USART_ACCESS_T2 = AVAILABLE;

//Variables related Peripherals
GPIO_InitTypeDef GpioUARTpins;
USART_InitTypeDef Usart1Init;
USART_HandleTypeDef Usart1;
GPIO_InitTypeDef GpioLEDpin;


void delay()
{
	for(uint32_t i=0 ; i<50000; i++);
}


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

	//printf("This is USART testing \n");

	sprintf(usr_msg, "This is the start of the hello world application \r\n");
	printmsg(usr_msg);

	//printf("Msg using USART is sent \n");

	// Start recording the FreeRTOS data in SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Create Tasks: Task1 and Task2
	//printf("Creating Task-1 \n");
	xTaskCreate(vTask1Function,
			"Task-1",
			108,
			NULL,
			2,
			&xTask1Handle);

	//printf("Creating Task-2 \n");
	xTaskCreate(vTask2Function,
			"Task-2",
			108,
			NULL,
			2,
			&xTask2Handle);


	//4. Schedule the tasks
	//printf("Scheduling the tasks created \n");
	vTaskStartScheduler();
	//printf("Tasks are successfully scheduled \n");

	for(;;);

}



void vTask1Function(void *params)
{
	while(1)
	{
		if( USART_ACCESS_T1 == AVAILABLE )
		{
			USART_ACCESS_T1 = NOT_AVAILABLE;

			sprintf(usr_msg, "This is the USART message from Task-1 \r\n");
			printmsg(usr_msg);

			HAL_GPIO_TogglePin(GPIOB, LED1_PIN); // Toggle Blue LED pin

			USART_ACCESS_T2 = AVAILABLE;

			SEGGER_SYSVIEW_Print("Task-1 is yielding");
			traceISR_EXIT_TO_SCHEDULER(); //This is used to show the Scheduler PendSV Handler in the SEGGER systemview
			taskYIELD();
		}
	}

}


void vTask2Function(void *params)
{
	while(1)
	{
		if( USART_ACCESS_T2 == AVAILABLE )
		{
			USART_ACCESS_T2 = NOT_AVAILABLE;

			sprintf(usr_msg, "This is the USART message from Task-2 \r\n");
			printmsg(usr_msg);


			HAL_GPIO_TogglePin(GPIOB, LED2_PIN); // Toggle Green LED pin

			USART_ACCESS_T1 = AVAILABLE;

			SEGGER_SYSVIEW_Print("Task-2 is yielding");
			traceISR_EXIT_TO_SCHEDULER(); //This is used to show the Scheduler PendSV Handler in the SEGGER systemview
			taskYIELD();
		}
	}

}


static void prvSetupUSART(void)
{
	//printf("USART1 Initialization: \n");

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

static void prvSetupLED(void)
{
	//printf("LED1 (Blue LED) and LED2 (Green LED) Initialization \n");
	LED1_GPIO_CLK_ENABLE();
	LED2_GPIO_CLK_ENABLE();

	//Zeroing each and every member element of the structure.
	memset(&GpioLEDpin, 0, sizeof(GpioLEDpin));
	GpioLEDpin.Pin = LED1_PIN | LED2_PIN;
	GpioLEDpin.Mode = GPIO_MODE_OUTPUT_PP;
	GpioLEDpin.Speed = GPIO_SPEED_FREQ_MEDIUM;
	GpioLEDpin.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(GPIOB, &GpioLEDpin);
	//HAL_GPIO_TogglePin(GPIOB, LED1_PIN);
	//HAL_GPIO_TogglePin(GPIOB, LED2_PIN);
}

static void prvSetupHardware(void)
{
	//Setup USART1
	prvSetupUSART();
	prvSetupLED();

}

void printmsg(char *msg)
{
	HAL_USART_Transmit(&Usart1, (uint8_t *)msg, strlen(msg), 1);
}
