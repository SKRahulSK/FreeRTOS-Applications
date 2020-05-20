/*
 * TaskPriority.c
 *
 *  Created on: 20-May-2020
 *      Author: Rahul
 */

/*
 * In the example, vTaskPrioritySet() function working is demonstrated.
 * It also demonstrates the GPIO interrupt handling
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
static void prvSetupButton(void);
static void prvSetupUSART(void);
void printmsg(char *msg);
void delay(uint32_t);
UBaseType_t Task1Priority, Task2Priority;
uint8_t SwitchPriority = FALSE;
char usr_msg[250];

//Variables related Peripherals
GPIO_InitTypeDef GpioLEDpin, GpioUARTpins;
GPIO_InitTypeDef GpioButtonpin;
EXTI_HandleTypeDef EXTIHandle;
EXTI_ConfigTypeDef EXTIConfig;
USART_InitTypeDef Usart1Init;
USART_HandleTypeDef Usart1;

int main(void)
{

	// Enable the DWT Cycle Count Register (SEGGER Settings)
	DWT->CTRL |= (1 << 0);

	// Private function called to setup the Hardware
	prvSetupUSART();
	prvSetupButton();
	prvSetupLED();

	// Start recording the FreeRTOS Application data in SEGGER
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Create Tasks:
	xTaskCreate(vTask1Function, "Task-1", 512, NULL, 4, &xTask1Handle);
	xTaskCreate(vTask2Function, "Task-2", 512, NULL, 3, &xTask2Handle);

	//4. Schedule the tasks
	vTaskStartScheduler();

	for(;;);

}

void vTask1Function(void *params)
{
	sprintf(usr_msg,"Task-1 is running\r\n");
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-1 Priority %ld\r\n",uxTaskPriorityGet(xTask1Handle));
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-2 Priority %ld\r\n",uxTaskPriorityGet(xTask2Handle));
	printmsg(usr_msg);

	while(1)
	{
		//Toggle the Green LED (LED-2) with a frequency of 500ms
		HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
		delay(500);

		if( SwitchPriority )
		{
			SwitchPriority = FALSE;
			//Switch-off the Green LED
			HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET);
			//Toggle RED LED for 1.5s
			HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);
			delay(1500);
			HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);

			//Switch the priorities of Task-1 and Task-2
			Task1Priority = uxTaskPriorityGet(xTask1Handle);
			Task2Priority = uxTaskPriorityGet(xTask2Handle);
			vTaskPrioritySet(xTask1Handle,Task2Priority);
			vTaskPrioritySet(xTask2Handle,Task1Priority);
		}
	}
}

void vTask2Function(void *params)
{
	sprintf(usr_msg,"Task-2 is running\r\n");
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-1 Priority %ld\r\n",uxTaskPriorityGet(xTask1Handle));
	printmsg(usr_msg);

	sprintf(usr_msg,"Task-2 Priority %ld\r\n",uxTaskPriorityGet(xTask2Handle));
	printmsg(usr_msg);

	while(1)
	{
		//Toggle the Blue LED (LED-2) with a frequency of 250ms
		HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
		delay(250);

		if( SwitchPriority )
		{
			SwitchPriority = FALSE;
			//Switch-off the BLUE LED
			HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);
			//Toggle RED LED for 1.5s
			HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);
			delay(1500);
			HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);

			//Switch the priorities of Task-1 and Task-2
			Task1Priority = uxTaskPriorityGet(xTask1Handle);
			Task2Priority = uxTaskPriorityGet(xTask2Handle);
			vTaskPrioritySet(xTask1Handle,Task2Priority);
			vTaskPrioritySet(xTask2Handle,Task1Priority);
		}
	}
}


static void prvSetupLED(void)
{
	//printf("LED1 (Blue LED), LED2(Green), and LED3(Red) Initialization \n");
	LED1_GPIO_CLK_ENABLE();

	//Zeroing each and every member element of the structure.
	memset(&GpioLEDpin, 0, sizeof(GpioLEDpin));
	GpioLEDpin.Pin = LED1_PIN | LED2_PIN | LED3_PIN;
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

	SwitchPriority = TRUE;

	traceISR_EXIT(); 	//This is SEGGER function. Used to trace ISR

}


void delay(uint32_t delay_ms)
{
	/*
	 *It depends on the value configTICK_RATE_HZ.
	 *If it is 1000 then each count is 1ms.
	 *If it is 500 then each count is 2ms
	 */
	uint32_t current_tick_count = xTaskGetTickCount();

	//This will convert the required amount of ticks to count depending on configTICK_RATE_HZ.
	uint32_t delay_in_ticks = ( delay_ms * configTICK_RATE_HZ ) / 1000;

	while( xTaskGetTickCount() < current_tick_count + delay_in_ticks);

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
