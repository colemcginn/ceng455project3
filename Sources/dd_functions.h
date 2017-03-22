#ifndef __dd_functions_H
#define __dd_functions_H

#include <mqx.h>
#include <message.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <mutex.h>
#include "Cpu.h"
#include "Events.h"
#include "os_tasks.h"


//task_type:
//0 = periodic
//1 = aperiodic
//2 = sporadic
struct task_list {
	_task_id tid;
	uint32_t deadline;
	uint32_t task_type;
	uint32_t creation_time;
	struct task_list *next_cell;
	struct task_list *previous_cell;
} task_list;

struct overdue_tasks {
	_task_id tid;
	uint32_t deadline;
	uint32_t task_type;
	uint32_t creation_time;
	struct overdue_tasks *next_cell;
	struct overdue_tasks *previous_cell;
} overdue_tasks;

struct periodic_task {
    uint32_t deadline;
    uint32_t runtime;
} periodic_task;

_task_id dd_tcreate(uint32_t template_index, uint32_t deadline);
//_task_id dd_tcreate_runtime(uint32_t template_index, uint32_t deadline, uint32_t runtime);
uint32_t dd_delete(_task_id task_id);
uint32_t dd_return_active_list(struct task_list **list);
uint32_t dd_return_overdue_list(struct overdue_tasks **list);

#endif
