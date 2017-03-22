/* ###################################################################
**     Filename    : os_tasks.c
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
** @file os_tasks.c
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/
/*!
**  @addtogroup os_tasks_module os_tasks module documentation
**  @{
*/
/* MODULE os_tasks */

#include "Cpu.h"
#include "Events.h"
#include "rtos_main_task.h"
#include "os_tasks.h"
//#include <sys/time.h>


#ifdef __cplusplus
extern "C" {
#endif


/* User includes (#include below this line is not maintained by Processor Expert) */

#include "dd_functions.h"

#include <timer.h>
#include <unistd.h>
#include <stdio.h>

// TIME MEASUREMENT CONFIGURATION
#define TM_WALLCLOCK 0
#define TM_TICKS 1
#define TIME_MEASUREMENT TM_WALLCLOCK

MUTEX_STRUCT timer_mutex;
struct task_list* task_head;
struct overdue_tasks* overdue_head;

_pool_id message_pool;
_queue_id schedule_qid;
uint32_t QUEUE_ID;
int n_total_tasks = 0;
int stats_period = 5000;
int idle_time = 0;
//int time_to_check_stats = 0;

// IS CALLED WHEN TIMER EXPIRES
void timer_callback(_timer_id t, void* dataptr, unsigned int seconds, unsigned int miliseconds){
    (*(bool*)dataptr) = false;
}

// CREATE BUSY-WAIT DELAY FOR A GIVEN DURATION
void synthetic_compute(unsigned int mseconds){
    bool flag = true;
    if(TIME_MEASUREMENT==TM_WALLCLOCK){
        unsigned int miliseconds = mseconds;
        _timer_start_oneshot_after(timer_callback, &flag, TIMER_KERNEL_TIME_MODE, miliseconds);
    }else if(TIME_MEASUREMENT==TM_TICKS){
        MQX_TICK_STRUCT Ticks;
        _time_init_ticks(&Ticks, 0);
        _time_add_sec_to_ticks(&Ticks, (int)ceil(mseconds / 50.0));
        _timer_start_oneshot_after(timer_callback, &flag, TIMER_KERNEL_TIME_MODE, &Ticks);
    }
    // busy wait loop
    while (flag){}
}

// IS CALLED WHEN TIMER EXPIRES
void deadline_callback(_timer_id t, _task_id tid, unsigned int seconds, unsigned int miliseconds){
    //send message with tid
	// Send create_task_msg to ddscheduler
//	while(_mutex_lock(&timer_mutex) != MQX_OK){}
	SCHEDULE_MESSAGE_PTR msg_ptr;

	// Allocate a message
	msg_ptr = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);

	if (msg_ptr == NULL) {
		_msg_free(msg_ptr);
		_task_block();
	}

	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULE_QUEUE); // Set the target Queue ID based on queue number
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
	msg_ptr->T_ID = tid;
	msg_ptr->TYPE = 4;
	_msgq_send(msg_ptr);
	_msg_free(msg_ptr);
//	 _mutex_unlock(&timer_mutex);
}
void timer_deadline(unsigned int mseconds, _task_id tid){


    if(TIME_MEASUREMENT==TM_WALLCLOCK){
        unsigned int miliseconds = mseconds;
        _timer_start_oneshot_after(deadline_callback, tid, TIMER_KERNEL_TIME_MODE, miliseconds);
    }else if(TIME_MEASUREMENT==TM_TICKS){
        MQX_TICK_STRUCT Ticks;
        _time_init_ticks(&Ticks, 0);
        _time_add_sec_to_ticks(&Ticks, (int)ceil(mseconds / 50.0));
        _timer_start_oneshot_after(deadline_callback, tid, TIMER_KERNEL_TIME_MODE, &Ticks);
    }

}

void create_periodic(_timer_id t, void* dataptr, unsigned int seconds, unsigned int miliseconds){
	struct periodic_task* data =  dataptr;
	dd_tcreate_runtime(USERTASK_TASK, data->deadline, data->runtime);
    n_total_tasks++;
};

void get_stats_periodic(_timer_id t, void* dataptr, unsigned int seconds, unsigned int miliseconds){
	struct task_list * active_tasks_head_ptr = NULL;
	struct overdue_tasks * overdue_tasks_head_ptr = NULL;

	// OBTAIN STATUS FROM SCHEDULER
	printf("TASK GENERATOR: collecting statistics\n\r");
	if(!dd_return_active_list(&active_tasks_head_ptr) || !dd_return_overdue_list(&overdue_tasks_head_ptr)){
		printf("error: failed to obtain the tasks list!\n\r");
		return 1;
	}

	int n_completed_tasks = 0;
	int n_failed_tasks = 0;
	int n_running_tasks = 0;

	struct task_list *temp_at_ptr = active_tasks_head_ptr;
	while(temp_at_ptr){
		n_running_tasks++;
		temp_at_ptr = temp_at_ptr->next_cell;
	}

	struct overdue_tasks *temp_ot_ptr = overdue_tasks_head_ptr;
	while(temp_ot_ptr){
		n_failed_tasks++;
		temp_ot_ptr = temp_ot_ptr->next_cell;
	}

	n_completed_tasks = n_total_tasks-(n_failed_tasks+n_running_tasks);
	printf("TASK GENERATOR: %d failed, %d completed, %d still running.\n\r", n_failed_tasks, n_completed_tasks, n_running_tasks);

	int execution_time = stats_period - idle_time;
	float total_time = stats_period;
	printf("Idle time: %dms Execution time: %dms Total time: %dms Utilization: %d%%\n",idle_time, execution_time, (int)total_time, (int)((float)execution_time / total_time * 100));

	// idle_time = 0;
	// uncomment if never call get_stats()
}

void get_stats(int time_to_check_stats){
	struct task_list * active_tasks_head_ptr = NULL;
	struct overdue_tasks * overdue_tasks_head_ptr = NULL;

	// OBTAIN STATUS FROM SCHEDULER
	printf("TASK GENERATOR: collecting statistics\n\r");
	if(!dd_return_active_list(&active_tasks_head_ptr) || !dd_return_overdue_list(&overdue_tasks_head_ptr)){
		printf("error: failed to obtain the tasks list!\n\r");
		return 1;
	}

	int n_completed_tasks = 0;
	int n_failed_tasks = 0;
	int n_running_tasks = 0;

	struct task_list *temp_at_ptr = active_tasks_head_ptr;
	while(temp_at_ptr){
		n_running_tasks++;
		temp_at_ptr = temp_at_ptr->next_cell;
	}

	struct overdue_tasks *temp_ot_ptr = overdue_tasks_head_ptr;
	while(temp_ot_ptr){
		n_failed_tasks++;
		temp_ot_ptr = temp_ot_ptr->next_cell;
	}

	n_completed_tasks = n_total_tasks-(n_failed_tasks+n_running_tasks);
	printf("TASK GENERATOR: %d failed, %d completed, %d still running.\n\r", n_failed_tasks, n_completed_tasks, n_running_tasks);

	int execution_time = time_to_check_stats - idle_time;
	float total_time = time_to_check_stats;
	printf("Idle time: %dms Execution time: %dms Total time: %dms Utilization: %d%%\n",idle_time, execution_time, (int)total_time, (int)((float)execution_time / total_time * 100));

	idle_time = 0;
}

// FOR THE DEMO PURPOSE, PLEASE COMMENT ALL PROMPTS (EXCEPT MONITOR TASK) FROM YOUR SCHEDULER
// SO WE WILL BE ABLE TO CLEARLY SEE MESSAGES FROM FOLLOWING TESTS

void report_statistics(int test_id, int n_total_tasks){
    struct task_list * active_tasks_head_ptr = NULL;
    struct overdue_tasks * overdue_tasks_head_ptr = NULL;

    if(!dd_return_active_list(&active_tasks_head_ptr) || !dd_return_overdue_list(&overdue_tasks_head_ptr)){
        printf("error: failed to obtain the tasks list!\n\r");
        return 1;
    }

    int n_completed_tasks = 0;
    int n_failed_tasks = 0;
    int n_running_tasks = 0;

    struct task_list *temp_at_ptr = active_tasks_head_ptr;
    while(temp_at_ptr){
        n_running_tasks++;
        temp_at_ptr = temp_at_ptr->next_cell;
    }

    struct overdue_tasks *temp_ot_ptr = overdue_tasks_head_ptr;
    while(temp_ot_ptr){
        n_failed_tasks++;
        temp_ot_ptr = temp_ot_ptr->next_cell;
    }

    n_completed_tasks = n_total_tasks-(n_failed_tasks+n_running_tasks);
    printf("TASK GENERATOR TEST %d: %d failed, %d completed, %d still running.\n\r", test_id, n_failed_tasks, n_completed_tasks, n_running_tasks);
}


void insert_active(struct task_list* node){
	//prints list
//	struct task_list* temp = task_head;
//	while(temp!=NULL){
//		printf("list: %d\n",temp->tid);
//		temp=temp->next_cell;
//	}
	if(task_head==NULL){
		task_head = node;
		_mqx_uint old_prior;
		_task_set_priority(node->tid,20,&old_prior);
		_task_get_priority(node->tid,&old_prior);
		TD_STRUCT_PTR td_ptr = _task_get_td(task_head->tid);
		_task_ready(td_ptr);
		return;
	}
	struct task_list* tmp = task_head;
	if(node->deadline < tmp->deadline){
//		printf("new head %d %d\n",node->tid, node->deadline);
		_mqx_uint old_prior;
		_task_set_priority(tmp->tid,22,&old_prior);
		_task_set_priority(node->tid,20,&old_prior);
		node->next_cell = tmp;
		tmp->previous_cell = node;
		task_head = node;
		TD_STRUCT_PTR td_ptr = _task_get_td(task_head->tid);
		_task_ready(td_ptr);
		return;
	}
	while(node->deadline >= tmp->deadline){
		if(tmp->next_cell==NULL){
//			tmp->next_cell->previous_cell = node;
			node->next_cell = NULL;
			tmp->next_cell = node;
			node->previous_cell = tmp;
			return;
		}
		tmp = tmp->next_cell;
	}
	if(tmp->previous_cell!=NULL){
		tmp->previous_cell->next_cell = node;
		node->previous_cell = tmp->previous_cell;
	}
	tmp->previous_cell = node;
	node->next_cell = tmp;
//	if(tmp->next_cell!=NULL) tmp->next_cell->previous_cell = node;
//	node->next_cell = tmp->next_cell;
//	tmp->next_cell = node;
//	node->previous_cell = tmp;
//	printf("%d next %d\n",tmp->tid,tmp->next_cell->tid);
	return;
}

void insert_overdue(struct overdue_tasks* node){
	_mqx_uint old_prior;
	_task_set_priority(node->tid,22,&old_prior);
	if(overdue_head==NULL){
		overdue_head = node;
		return;
	}

	struct task_list* tmp = overdue_head;
	node->next_cell = tmp;
	tmp->previous_cell = node;
	overdue_head = node;
}

struct task_list* delete_active(_task_id t_id){

	if(task_head==NULL) return NULL;
//	prints list
	bool exists = false;
	struct task_list* temp = task_head;
//	printf("deleting %d\n",t_id);
	while(temp!=NULL){
//		printf("list: %d\n",temp->tid);
		if(temp->tid==t_id) exists=true;
		temp=temp->next_cell;
	}
	if(!exists) return NULL;
	struct task_list* node = task_head;
	struct task_list* tmp = node;
	if(node->tid==t_id){
		tmp = node;
		if(node->next_cell!=NULL) {
			node->next_cell->previous_cell = NULL;
			task_head = node->next_cell;
		}
		else{
			task_head = NULL;
		}
		node = NULL;

		if(task_head!=NULL){
			_mqx_uint old_prior;
			_task_set_priority(task_head->tid,20,&old_prior);
			TD_STRUCT_PTR td_ptr = _task_get_td(task_head->tid);
			_task_ready(td_ptr);
		}
		return tmp;
	}

	while(node!=NULL){
		if(node->tid==t_id){
			break;
		}
		node = node->next_cell;
	}

	tmp = node;
	if(node->previous_cell!=NULL && node->next_cell!=NULL){
		node->previous_cell->next_cell = node->next_cell;
		node->next_cell->previous_cell = node->previous_cell;
	}
	node = NULL;
	return tmp;
}
//do we even need this?
void delete_overdue(struct overdue_tasks* node){
	node->previous_cell->next_cell = node->next_cell;
	node->next_cell->previous_cell = node->previous_cell;
}

void reset(){
	overdue_head = NULL;
	task_head = NULL;
}


/*
** ===================================================================
**     Callback    : dds_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void dds_task(os_task_param_t task_init_data)
{
	QUEUE_ID = 9;
	task_head= NULL;
	overdue_head = NULL;
	_mutex_init(&timer_mutex,NULL);



	// Open a message queue.   _msgq_open(queue_num, max_size);
	schedule_qid = _msgq_open(SCHEDULE_QUEUE, 0);
//	printf("schedule task qid: %d\n",schedule_qid);
	if (schedule_qid == 0) {
		printf("\nCould not open the message queue dds\n");
		_task_block();
	}

	// Create a message pool.   _msgpool_create(msg_size, num_msg, grow_size, grow_lim);
	message_pool = _msgpool_create(sizeof(SCHEDULE_MESSAGE), 32, 0, 0);
	if (message_pool == MSGPOOL_NULL_POOL_ID) {
		printf("\nCould not create a message pool dds\n");
		_task_block();
	}
	SCHEDULE_MESSAGE_PTR msg_ptr;
	_mqx_uint old_prior;
	_task_get_priority(_task_get_id(),&old_prior);
	_task_set_priority(_task_get_id(),9,&old_prior);
	_task_get_priority(_task_get_id(),&old_prior);
#ifdef PEX_USE_RTOS
  while (1) {
#endif
//    OSA_TimeDelay(10);                 /* Example code (for task release) */

//    if(head_task->creation_time + head_task->deadline > current_time)

    /* If no head - run idle
     * Check head deadline
     * Change priority of that task to 20
     * Start timer for deadline amount
     * if message from task saying complete:
     * 	repeat from start
     * else if timer end:
     * 	move head to overdue list
     * 	repeat
     */
    // Check schedule queue
	msg_ptr = _msgq_receive(schedule_qid, 0);
	if (msg_ptr == NULL) {
		continue;
//		_task_block();
	}

	int command_num = msg_ptr->TYPE; // First char of each message is a command int.
//	printf("received messge %d\n", command_num);
	SCHEDULE_MESSAGE_PTR msg_ptr_active;
	struct task_list* node = malloc(sizeof(struct task_list));
	struct overdue_tasks* overdue_node = malloc(sizeof(struct overdue_tasks));
	struct task_list* result = malloc(sizeof(struct task_list));
	TIME_STRUCT current_time;
	switch(command_num) {
		case 0: // create task
			node->tid = msg_ptr->T_ID;
//			printf("task id %d\n",node->tid);
//			node->deadline = msg_ptr->DEADLINE;

			node->task_type = 1; // change in future
			_time_get(&current_time);
			node->creation_time = (current_time.SECONDS*1000 + current_time.MILLISECONDS);
			node->deadline = (msg_ptr->DEADLINE + node->creation_time);
//			printf("%d %ld %ld\n",node->tid,node->deadline,msg_ptr->DEADLINE);
			node->next_cell = NULL;
			node->previous_cell = NULL;
			timer_deadline(msg_ptr->DEADLINE,node->tid);

			insert_active(node);

			break;

		case 1: // delete task
			delete_active(msg_ptr->T_ID);
			break;

		case 2: // return active
			// Allocate a message
			msg_ptr_active = (SCHEDULE_MESSAGE_PTR) _msg_alloc(msg_ptr->P_ID);

			if (msg_ptr_active == NULL) {
				_msg_free(msg_ptr_active);
				_task_block();
			}
			msg_ptr_active->HEADER.TARGET_QID = _msgq_get_id(0, msg_ptr->Q_ID); // Set the target Queue ID based on queue number
			msg_ptr_active->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
			if(task_head==NULL){
				msg_ptr_active->IS_ACTIVE = false;
				msg_ptr_active->ACTIVE_HEAD = NULL;
			}
			else{
				msg_ptr_active->IS_ACTIVE = true;
				msg_ptr_active->ACTIVE_HEAD = task_head; // TODO
			}
			_msgq_send(msg_ptr_active);
			_msg_free(msg_ptr_active);
			break;

		case 3: // return overdue
			// Send create_task_msg to ddscheduler

			// Allocate a message
			msg_ptr_active = (SCHEDULE_MESSAGE_PTR) _msg_alloc(msg_ptr->P_ID);

			if (msg_ptr_active == NULL) {
				_msg_free(msg_ptr_active);
				_task_block();
			}

			msg_ptr_active->HEADER.TARGET_QID = _msgq_get_id(0, msg_ptr->Q_ID); // Set the target Queue ID based on queue number
			msg_ptr_active->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
			if(overdue_head==NULL){
				msg_ptr_active->IS_OVERDUE = false;
				msg_ptr_active->OVERDUE_HEAD = NULL;
			}
			else{
				msg_ptr_active->IS_OVERDUE = true;
				msg_ptr_active->OVERDUE_HEAD = overdue_head; // TODO
			}
			_msgq_send(msg_ptr_active);
			_msg_free(msg_ptr_active);
			break;

		case 4: //check for overdue task

			result = delete_active(msg_ptr->T_ID);
			if(result!=NULL){
//				printf("overdue: %d\n",result->tid);
				overdue_node->tid = result->tid;
				overdue_node->deadline = result->deadline;
				overdue_node->task_type = result->task_type;
				overdue_node->creation_time = result->creation_time;
				overdue_node->next_cell = NULL;
				overdue_node->previous_cell = NULL;
				insert_overdue(overdue_node);
			}
			break;
		default:
			break;
	}


#ifdef PEX_USE_RTOS
  }
#endif
}


/*
** ===================================================================
**     Callback    : idle_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void idle_task(os_task_param_t task_init_data)
{
  /* Write your local variable definition here */
	_mqx_uint old_prior;
	_task_set_priority(_task_get_id(),21,&old_prior);
	_task_get_priority(_task_get_id(),&old_prior);
//	printf("idle task start\n");


  
#ifdef PEX_USE_RTOS
  while (1) {
#endif
   	bool flag = true;
    int miliseconds = 5;
    _timer_start_oneshot_after(timer_callback, &flag, TIMER_KERNEL_TIME_MODE, miliseconds);
	while (flag){}
	idle_time +=5;
    
#ifdef PEX_USE_RTOS   
  }
#endif    
}

/*
** ===================================================================
**     Callback    : user_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void user_task(os_task_param_t task_init_data)
{
//	uint32_t runtime = task_init_data;
//	if(runtime==0) runtime = 1000;
//	runtime=1000;
//	printf("user task start %d\n",_task_get_id());
    synthetic_compute(1000); // task's actual computation simulated by a busy loop
    dd_delete(_task_get_id());

}

/*
** ===================================================================
**     Callback    : generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void generator_task(os_task_param_t task_init_data)
{
	_mqx_uint old_prior;
	_task_set_priority(_task_get_id(),10,&old_prior);
	_task_get_priority(_task_get_id(),&old_prior);



	//issue if same deadline and is overdue
	// CREATE SAMPLE TASKS

	_task_id t1 = dd_tcreate(USERTASK_TASK, 4000);
	_task_id t2 = dd_tcreate(USERTASK_TASK, 8000);
	_task_id t3 = dd_tcreate(USERTASK_TASK, 8100);
	_task_id t4 = dd_tcreate(USERTASK_TASK, 2000);
	n_total_tasks = 4;

//	struct periodic_task* task_data = malloc(sizeof(struct periodic_task));
//	task_data->deadline = 4000;
//	task_data->runtime = 2000;
//	int period_ms = 5000;
//	_timer_start_periodic_every(create_periodic, task_data, TIMER_KERNEL_TIME_MODE, period_ms);

	printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);

//	_timer_start_periodic_every(get_stats_periodic, NULL, TIMER_KERNEL_TIME_MODE, stats_period);

	// WAIT FOR CERTAIN TIME

	int time_to_check_stats = 15000;
	if(TIME_MEASUREMENT==TM_WALLCLOCK){
		_time_delay(time_to_check_stats);
	}else if(TIME_MEASUREMENT==TM_TICKS){
		_time_delay_for(time_to_check_stats/50);
	}

	get_stats(time_to_check_stats);


	return 0;
}

/*
** ===================================================================
**     Callback    : master_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void master_task(os_task_param_t task_init_data)
{

	printf("starting master\n");
  /* Write your local variable definition here */
	_task_create(0,DDSTASK_TASK,0);
//	_task_create(0,GENTASK_TASK,0);
	_task_create(0,GIVENTEST_TASK_GENERATOR_TASK,0);
	_task_create(0,IDLETASK_TASK,0);
	_task_block();

}


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
/*
** ===================================================================
**     Callback    : sample_task_200
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_200(os_task_param_t task_init_data)
{
//	printf("user task 200 start %d\n",_task_get_id());
//	int counter=0;
//	while(counter<2){
//		synthetic_compute(100); // task's actual computation simulated by a busy loop
//		counter++;
//	}
	synthetic_compute(200);
    dd_delete(_task_get_id());
}

/*
** ===================================================================
**     Callback    : sample_task_100
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_100(os_task_param_t task_init_data)
{
//	printf("user task 100 start %d\n",_task_get_id());
    synthetic_compute(100); // task's actual computation simulated by a busy loop
    dd_delete(_task_get_id());
}

/*
** ===================================================================
**     Callback    : sample_task_2000
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_2000(os_task_param_t task_init_data)
{
//	printf("user task 2000 start %d\n",_task_get_id());
	int counter = 0;
	while(counter<20){
		synthetic_compute(100); // task's actual computation simulated by a busy loop
		counter++;
	}

    dd_delete(_task_get_id());
}

/*
** ===================================================================
**     Callback    : sample_task_1000
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void sample_task_1000(os_task_param_t task_init_data)
{
//	printf("user task 1000 start %d\n",_task_get_id());
	int counter=0;
	while(counter<10){
		synthetic_compute(100); // task's actual computation simulated by a busy loop
		counter++;
	}
//	synthetic_compute(1000);

    dd_delete(_task_get_id());
}

/*
** ===================================================================
**     Callback    : giventest_generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void giventest_generator_task(os_task_param_t task_init_data)
{

//    {
//        // TEST BEGINS: feasible
//        int test_id = 1;
//        printf("RUNNING TEST %d\n", test_id);
//        // CREATE SAMPLE TASKS
//        int n_total_tasks = 4;
//        _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        _task_id t2 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        _task_id t3 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        _task_id t4 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);
//        // WAIT FOR CERTAIN TIME
//        _time_delay(5000);
//        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 0, 4, 0);
//        // REPORT STATISTICS
//        report_statistics(test_id, n_total_tasks);
//        // TEST ENDS
//    }
//    // FLUSH SCHEDULER: feasible, collect in middle and end
//    _time_delay(5000);
//    reset();
//    {
//        // TEST BEGINS:
//        int test_id = 2;
//        printf("RUNNING TEST %d\n", test_id);
//        // CREATE SAMPLE TASKS
//        int n_total_tasks = 4;
//        _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        _task_id t2 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        _task_id t3 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        _task_id t4 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5000);
//        printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);
//        // WAIT FOR CERTAIN TIME
//        _time_delay(2500);
//        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 0, 2, 2);
//        // REPORT STATISTICS
//        report_statistics(test_id, n_total_tasks);
//        // WAIT FOR CERTAIN TIME
//        _time_delay(2500);
//        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 0, 4, 0);
//        // REPORT STATISTICS
//        report_statistics(test_id, n_total_tasks);
//        // TEST ENDS
//    }
//    // FLUSH SCHEDULER: not feasible
//    _time_delay(5000);
//    reset();
//    {
//        // TEST BEGINS:
//        int test_id = 3;
//        printf("RUNNING TEST %d\n", test_id);
//        // CREATE SAMPLE TASKS
//        int n_total_tasks = 4;
//        _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 1500);
//        _task_id t2 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 2500);
//        _task_id t3 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 3500);
//        _task_id t4 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 3510);
//        printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);
//        // WAIT FOR CERTAIN TIME
//        _time_delay(5000);
//        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 1, 3, 0);
//        // REPORT STATISTICS
////        get_stats();
//        report_statistics(test_id, n_total_tasks);
//        // TEST ENDS
//    }
//    // FLUSH SCHEDULER: higher priority comes in, all complete
//    _time_delay(5000);
//    reset();
//    {
//        // TEST BEGINS:
//        int test_id = 4;
//        printf("RUNNING TEST %d\n", test_id);
//        // CREATE SAMPLE TASKS
//        int n_total_tasks = 4;
//        _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_2000_TASK, 4500);
//        _task_id t2 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 4510);
//        _time_delay(1000);
//        _task_id t3 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 1300);
//        _task_id t4 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 4500);
//        printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);
//        // WAIT FOR CERTAIN TIME
//        _time_delay(5000);
//        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 0, 4, 0);
//        // REPORT STATISTICS
//        report_statistics(test_id, n_total_tasks);
//        // TEST ENDS
//    }
////     FLUSH SCHEDULER: higher priority comes in, one fails
//    _time_delay(5000);
//    reset();
//    {
//        // TEST BEGINS:
//        int test_id = 5;
//        printf("RUNNING TEST %d\n", test_id);
//        // CREATE SAMPLE TASKS
//        int n_total_tasks = 4;
//        _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_2000_TASK, 2500);
//        _task_id t2 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 3600); // this should fail
//        _time_delay(2200); //2000
//        _task_id t3 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 1200);
//        _task_id t4 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 4100);
//        printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);
//        // WAIT FOR CERTAIN TIME
//        _time_delay(5000);
//        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 1, 3, 0);
//        // REPORT STATISTICS
//        report_statistics(test_id, n_total_tasks);
//        // TEST ENDS
//    }
//    // FLUSH SCHEDULER: preempt on overdue
//    _time_delay(5000);
//    reset();
//    {
//        // TEST BEGINS:
//        int test_id = 6;
//        printf("RUNNING TEST %d\n", test_id);
//        // CREATE SAMPLE TASKS
//        int n_total_tasks = 3;
//        _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_2000_TASK, 3000);
//        _task_id t2 = dd_tcreate(GIVENTEST_SAMPLETASK_2000_TASK, 3010);
//        _task_id t3 = dd_tcreate(GIVENTEST_SAMPLETASK_1000_TASK, 5100);
//        printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);
//        // WAIT FOR CERTAIN TIME
//        _time_delay(5200);
//        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 1, 2, 0);
//        // REPORT STATISTICS
//        report_statistics(test_id, n_total_tasks);
//        // TEST ENDS
//    }
//    // FLUSH SCHEDULER: Overall Performance
//    _time_delay(5000);
//    reset();
    {
        // TEST BEGINS:
        int test_id = 7;
        printf("RUNNING TEST %d\n", test_id);
        // CREATE SAMPLE TASKS
        int n_total_tasks = 80;
        int i;
        for(i=0; i<50; ++i){
            _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_100_TASK, 3000+i);
            _time_delay(1);
        }
        _time_delay(100);
        printf("2nd 20\n");
        for(i=0; i<30; ++i){
            _task_id t1 = dd_tcreate(GIVENTEST_SAMPLETASK_200_TASK, 2000+i);
            _time_delay(1);
        }
        printf("TASK GENERATOR: %d tasks created.\n\r", n_total_tasks);
        // WAIT FOR CERTAIN TIME
        _time_delay(2000);
        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 0+0, 10+10, 40+20);
        // REPORT STATISTICS
        report_statistics(test_id, n_total_tasks);
        // WAIT FOR CERTAIN TIME
        _time_delay(5000);
        printf("TASK GENERATOR TEST %d EXPECTED: %d failed, %d completed, %d running\n", test_id, 40+20, 10+10, 0);
        // REPORT STATISTICS
        report_statistics(test_id, n_total_tasks);
        // TEST ENDS
    }
    return 0;
}


