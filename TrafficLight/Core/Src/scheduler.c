#include "scheduler.h"
#include "stm32f1xx.h"

#include <stddef.h>

typedef struct {
    SCH_TaskFunction pTask;
    uint32_t Delay;       /* Relative to the task before this one. */
    uint32_t Period;      /* Zero means one-shot. */
    uint8_t RunMe;
    uint32_t TaskID;
} sTask;

static volatile sTask task_list[SCH_MAX_TASKS];
static volatile uint8_t task_count;
static uint32_t next_task_id = 1U;

/* A running periodic task reserves a slot while its callback executes. */
static volatile uint32_t running_task_id;
static volatile uint8_t running_task_cancelled;
static volatile uint8_t running_task_reserved;

static uint32_t irq_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void irq_unlock(uint32_t primask)
{
    if ((primask & 1U) == 0U) {
        __enable_irq();
    }
}

static sTask task_load(uint8_t index)
{
    sTask task;
    task.pTask = task_list[index].pTask;
    task.Delay = task_list[index].Delay;
    task.Period = task_list[index].Period;
    task.RunMe = task_list[index].RunMe;
    task.TaskID = task_list[index].TaskID;
    return task;
}

static void task_store(uint8_t index, const sTask *task)
{
    task_list[index].pTask = task->pTask;
    task_list[index].Delay = task->Delay;
    task_list[index].Period = task->Period;
    task_list[index].RunMe = task->RunMe;
    task_list[index].TaskID = task->TaskID;
}

static void task_copy(uint8_t destination, uint8_t source)
{
    sTask task = task_load(source);
    task_store(destination, &task);
}

static uint8_t task_id_exists(uint32_t taskID)
{
    for (uint8_t i = 0U; i < task_count; ++i) {
        if (task_list[i].TaskID == taskID) {
            return 1U;
        }
    }
    return (running_task_id == taskID) ? 1U : 0U;
}

static uint32_t allocate_task_id(void)
{
    for (uint8_t attempt = 0U; attempt < (SCH_MAX_TASKS + 1U); ++attempt) {
        uint32_t candidate = next_task_id;
        if (candidate == 0U) {
            candidate = 1U;
        }

        next_task_id = candidate + 1U;
        if (next_task_id == 0U) {
            next_task_id = 1U;
        }

        if (!task_id_exists(candidate)) {
            return candidate;
        }
    }
    return 0U;
}

/* Caller holds the scheduler lock and has already checked capacity. */
static void task_insert(const sTask *new_task, uint32_t delay)
{
    uint32_t remaining = delay;
    uint8_t index = 0U;

    while (index < task_count && remaining >= task_list[index].Delay) {
        remaining -= task_list[index].Delay;
        ++index;
    }

    if (index < task_count) {
        sTask next = task_load(index);
        next.Delay -= remaining;
        task_store(index, &next);
    }

    for (uint8_t i = task_count; i > index; --i) {
        task_copy(i, (uint8_t)(i - 1U));
    }

    sTask inserted = *new_task;
    inserted.Delay = remaining;
    inserted.RunMe = (remaining == 0U) ? 1U : 0U;
    task_store(index, &inserted);
    ++task_count;
}

static void task_remove(uint8_t index)
{
    sTask removed = task_load(index);

    if ((uint8_t)(index + 1U) < task_count) {
        sTask next = task_load((uint8_t)(index + 1U));
        next.Delay += removed.Delay;
        task_store((uint8_t)(index + 1U), &next);
    }

    for (uint8_t i = index; (uint8_t)(i + 1U) < task_count; ++i) {
        task_copy(i, (uint8_t)(i + 1U));
    }
    --task_count;

    if (task_count > 0U && task_list[0].Delay == 0U) {
        task_list[0].RunMe = 1U;
    }
}

void SCH_Init(void)
{
    uint32_t primask = irq_lock();
    task_count = 0U;
    next_task_id = 1U;
    running_task_id = 0U;
    running_task_cancelled = 0U;
    running_task_reserved = 0U;
    irq_unlock(primask);
}

uint32_t SCH_Add_Task(SCH_TaskFunction pFunc, uint32_t delay, uint32_t period)
{
    if (pFunc == NULL) {
        return 0U;
    }

    uint32_t primask = irq_lock();
    uint8_t reserved = (running_task_reserved != 0U) ? 1U : 0U;
    if ((uint16_t)task_count + reserved >= SCH_MAX_TASKS) {
        irq_unlock(primask);
        return 0U;
    }

    uint32_t taskID = allocate_task_id();
    if (taskID == 0U) {
        irq_unlock(primask);
        return 0U;
    }

    sTask task = {pFunc, delay, period, 0U, taskID};
    task_insert(&task, delay);
    irq_unlock(primask);
    return taskID;
}

uint8_t SCH_Delete_Task(uint32_t taskID)
{
    if (taskID == 0U) {
        return 0U;
    }

    uint32_t primask = irq_lock();
    if (running_task_id == taskID) {
        running_task_cancelled = 1U;
        irq_unlock(primask);
        return 1U;
    }

    for (uint8_t i = 0U; i < task_count; ++i) {
        if (task_list[i].TaskID == taskID) {
            task_remove(i);
            irq_unlock(primask);
            return 1U;
        }
    }

    irq_unlock(primask);
    return 0U;
}

void SCH_Update(void)
{
    if (task_count == 0U) {
        return;
    }

    if (task_list[0].Delay > 0U) {
        --task_list[0].Delay;
    }
    if (task_list[0].Delay == 0U) {
        task_list[0].RunMe = 1U;
    }
}

void SCH_Dispatch_Tasks(void)
{
    for (;;) {
        uint32_t primask = irq_lock();
        if (task_count == 0U) {
            irq_unlock(primask);
            return;
        }

        if (task_list[0].Delay == 0U) {
            task_list[0].RunMe = 1U;
        }
        if (task_list[0].RunMe == 0U) {
            irq_unlock(primask);
            return;
        }

        sTask current = task_load(0U);
        task_remove(0U);
        running_task_id = current.TaskID;
        running_task_cancelled = 0U;
        running_task_reserved = (current.Period > 0U) ? 1U : 0U;
        irq_unlock(primask);

        if (current.pTask != NULL) {
            current.pTask();
        }

        primask = irq_lock();
        uint8_t cancelled = running_task_cancelled;
        running_task_id = 0U;
        running_task_cancelled = 0U;
        running_task_reserved = 0U;

        if (current.Period > 0U && cancelled == 0U) {
            current.Delay = current.Period;
            current.RunMe = 0U;
            task_insert(&current, current.Period);
        }
        if (task_count > 0U && task_list[0].Delay == 0U) {
            task_list[0].RunMe = 1U;
        }
        irq_unlock(primask);
    }
}