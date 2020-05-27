# FreeRTOS-Applications
STM32WB FreeRTOS applications

##Depending on the application from src folder, we have to modify FreeRTOSConfig.h
  1. configUSE_PREEMPTION
  2. configUSE_TIMERS

###To enable the required peripheral, we have to uncommnent them in stm32wbxx_hal_conf.h file.

##SEGGER SystemView
The applications have SEGGER SystemView setting. It makes the debugging much easier.
Applications/SEGGER_Mem_Dump folder contains some of the systemview files which are be used to debug the applications accordingly.