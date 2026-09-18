#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Library includes. */
#include "stm32h7xx.h"

#ifdef __cplusplus
    extern "C" {
#endif

#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler

void xPortSysTickHandler(void);

#define INCLUDE_xTaskGetSchedulerState       1
/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE. 
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION		1
//#define configUSE_TIME_SLICING      0
#define configUSE_QUEUE_SETS        1
#define configUSE_IDLE_HOOK			0
#define configUSE_TICK_HOOK			1
#define configCPU_CLOCK_HZ			( ( unsigned long ) 400000000 )	
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES		( 5 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 17 * 1024 ) )
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_TRACE_FACILITY	1   /* For using vTaskGetInfo API */
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1
#define configUSE_MUTEXES			1

#define configUSE_TASK_NOTIFICATIONS 1
#define configSUPPORT_STATIC_ALLOCATION 1


/* Software timers settings */
#define configUSE_TIMERS                1 
#define configTIMER_TASK_PRIORITY       5
#define configTIMER_TASK_STACK_DEPTH    configMINIMAL_STACK_SIZE * 2
#define configTIMER_QUEUE_LENGTH        10

#define configUSE_COUNTING_SEMAPHORES   1 


/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTimerPendFunctionCall  1
#define INCLUDE_uxTaskGetStackHighWaterMark   1     // For debug stack control

/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY 		255
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 /* equivalent to 0xb0, or priority 11. */


/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#ifdef __cplusplus
    }
#endif

#endif /* FREERTOS_CONFIG_H */

