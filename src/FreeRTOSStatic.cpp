#ifdef WITH_RTOS
#include <FreeRTOS.h>
#include <task.h>

extern "C" {

void vApplicationTickHook()
{}

/**
 *  @brief Callback for allocating memory for the Idle Task
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) 
{
    // Allocate memory in the static variable section (.bss)
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    // Pass the pointers and size back to FreeRTOS
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/**
 * @brief Callback for allocating memory for the Timer Service Task
 * @details Necessary if configUSE_TIMERS == 1
 */
#if (configUSE_TIMERS == 1)
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) 
{
    // Allocate memory statically.
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    // Sending the data back to FreeRTOS
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif

} // extern "C"

#endif  // WITH_RTOS
