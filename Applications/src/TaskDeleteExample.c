/*
 * TaskDeleteExample.c
 *
 *  Created on: 19-May-2020
 *      Author: Rahul
 */

/*
 * In the example, vTaskDelete() function working is shown
 * In addition, the difference between vTaskDelay() and software delay is demonstrated.
 */
#include "stm32wbxx.h"
#include "stm32wbxx_nucleo.h"
#include "stm32wbxx_hal_usart.h"


#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "time.h"


//Task handles and function prototypes
TaskHandle_t xTask1Handle = NULL;
TaskHandle_t xTask2Handle = NULL;
void vTask1Function(void *params);
void vTask2Function(void *params);

static void prvSetupLED(void);
static void prvSetupButton(void);
void delay(uint32_t);

//Variables related Peripherals
GPIO_InitTypeDef GpioLEDpin;
GPIO_InitTypeDef GpioButtonpin;


int main(void)
{
	// Private function called to setup the Hardware
	prvSetupButton();
	prvSetupLED();

	//3. Create Tasks:
	xTaskCreate(vTask1Function, "Task-1", configMINIMAL_STACK_SIZE, NULL, 1, &xTask1Handle);
	xTaskCreate(vTask2Function, "Task-2", configMINIMAL_STACK_SIZE, NULL, 2, &xTask2Handle);

	//4. Schedule the tasks
	vTaskStartScheduler();

	for(;;);

}

void vTask1Function(void *params)
{
	while(1)
	{
		//Toggle the Green LED (LED-2) with a frequency of 500ms
		HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);

		//Difference between software delay and vTaskDelay()
		//delay(500);
		vTaskDelay(500);
	}
}

void vTask2Function(void *params)
{
	while(1)
	{
		//Toggle the Blue LED (LED-2) with a frequency of 200ms
		HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);

		//Difference between software delay and vTaskDelay()
		//delay(200);
		vTaskDelay(200);

		if(! HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_2) )
		{
			//Switch-off the BLUE LED
			HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET);
			//Toggle RED LED for 1s
			HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);
			delay(1000);
			HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);
			//Delete the Task2
			vTaskDelete(NULL);	//Since the Task2 is being deleted by its function, we need not give the taskhandle as parameter
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
