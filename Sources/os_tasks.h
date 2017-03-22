/* ###################################################################
**     Filename    : os_tasks.h
**     Project     : deadline_driven_scheduler
**     Processor   : MK64FN1M0VLL12
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2017-02-27, 16:25, # CodeGen: 1
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Settings    :
**     Contents    :
**         idle_task      - void idle_task(os_task_param_t task_init_data);
**         user_task      - void user_task(os_task_param_t task_init_data);
**         generator_task - void generator_task(os_task_param_t task_init_data);
**         master_task    - void master_task(os_task_param_t task_init_data);
**         dds_task       - void dds_task(os_task_param_t task_init_data);
**
** ###################################################################*/
/*!
** @file os_tasks.h
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/
/*!
**  @addtogroup os_tasks_module os_tasks module documentation
**  @{
*/

#ifndef __os_tasks_H
#define __os_tasks_H
/* MODULE os_tasks */

#include "fsl_device_registers.h"
#include "clockMan1.h"
#include "pin_init.h"
#include "osa1.h"
#include "mqx_ksdk.h"
#include "uart1.h"
#include "fsl_mpu1.h"
#include "fsl_hwtimer1.h"
#include "MainTask.h"
#include "DDSTask.h"
#include "myUART.h"
#include "MasterTask.h"
#include "GIVENTEST_SAMPLETASK_1000.h"
#include "GIVENTEST_SAMPLETASK_2000.h"
#include "GIVENTEST_SAMPLETASK_100.h"
#include "GIVENTEST_SAMPLETASK_200.h"
#include "giventest_task_generator.h"
#include "GenTask.h"
#include "UserTask.h"
#include "IdleTask.h"
#include "message.h"
#include "dd_functions.h"

typedef struct schedule_message {
	MESSAGE_HEADER_STRUCT HEADER;
	uint32_t DEADLINE;
	uint32_t TYPE;
	_task_id T_ID;
	_queue_id Q_ID;
	_pool_id P_ID;
	struct task_list* ACTIVE_HEAD;
	bool IS_ACTIVE;
	struct overdue_tasks* OVERDUE_HEAD;
	bool IS_OVERDUE;
} SCHEDULE_MESSAGE, *SCHEDULE_MESSAGE_PTR;

#ifdef __cplusplus
extern "C" {
#endif

extern _pool_id message_pool;
extern _queue_id schedule_qid;
extern uint32_t QUEUE_ID;
#define SCHEDULE_QUEUE 8

/*
** ===================================================================
**     Callback    : idle_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void idle_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : user_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void user_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void generator_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : master_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void master_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : dds_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void dds_task(os_task_param_t task_init_data);


/*
** ===================================================================
**     Callback    : sample_task_200
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_200(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : sample_task_100
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_100(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : sample_task_2000
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_2000(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : sample_task_1000
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_1000(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : giventest_generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void giventest_generator_task(os_task_param_t task_init_data);

/* END os_tasks */

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
/* ifndef __os_tasks_H*/
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
