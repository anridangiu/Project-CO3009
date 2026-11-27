#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#ifndef SCH_MAX_TASKS
#define SCH_MAX_TASKS 16U
#endif

typedef void (*SCH_TaskFunction)(void);

void SCH_Init(void);

/* Delay and period are measured in scheduler ticks. ID 0 means failure. */
uint32_t SCH_Add_Task(SCH_TaskFunction pFunc, uint32_t delay, uint32_t period);
uint8_t SCH_Delete_Task(uint32_t taskID);

/* Call SCH_Update from the 10 ms timer interrupt; it only touches the head task. */
void SCH_Update(void);

/* Run ready tasks in foreground context. */
void SCH_Dispatch_Tasks(void);

#endif