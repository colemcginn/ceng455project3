
#ifndef __user_task_access_functions_H
#define __user_task_access_functions_H

#include <stdbool.h>
#include <stdint.h>
#include <mutex.h>
#include "Cpu.h"
#include "Events.h"
#include "rtos_main_task.h"
#include "os_tasks.h"


typedef struct open_permission{
   _task_id t_id;
   _queue_id q_id;
} open_permission;

extern open_permission read_permissions[10];
extern int num_read;
extern _task_id write_permission;
extern MUTEX_STRUCT openR_mutex;
extern MUTEX_STRUCT openW_mutex;

extern int copy_count;
extern int size_of_outline;

extern char output_copy[];
//extern char *output_copy; //="" ?
//extern int copy_count;

bool OpenR(_queue_id stream_no);
bool _getline(char *string);
_queue_id OpenW(void);
bool _putline(_queue_id qid, char *string);
bool Close(void);

#endif
