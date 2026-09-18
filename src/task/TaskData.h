#pragma once
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>

namespace task
{

/* * * * * * * * * * * TASKS PRIORITY SETTINGS * * * * * * * * * * */

constexpr UBaseType_t LowPriority = tskIDLE_PRIORITY + 1;

constexpr UBaseType_t  SensorCalcTaskPriority = LowPriority;            /* Raw measurements handling task priority */
constexpr UBaseType_t  SensorReadTaskPriority = LowPriority + 2;        /* Request new measurements task priority */
constexpr UBaseType_t  DisplayUpdatePriority = LowPriority;                       /* Display updating task priority */
constexpr UBaseType_t  DispTaskManagerPriority = LowPriority + 1;       /* Display updating task priority */
constexpr UBaseType_t  TerminalTaskPriority = LowPriority;              /* Terminal task priority */

/* * * * * * * TASKS HANDLERS STORAGE (FOR DIAGNISTIC) * * * * * * */

enum Tasks
{
    SensorCalcTask = 0,
    SensorReadTask,
    DisplayUpdate,
    Terminal,

    TasksEnd
};

extern TaskHandle_t tasksHandlers[];

/* * * * * * * * * * TASKS STACKS SIZES * * * * * * * * * */

static const uint32_t StackSizes[Tasks::TasksEnd]
{
    300,    /* Tasks::SensorCalcTask */
    200,    /* Tasks::SensorReadTask */
    500,    /* Tasks::DisplayUpdate */
    1000    /* Tasks::Terminal */
};

}  // namespace task
