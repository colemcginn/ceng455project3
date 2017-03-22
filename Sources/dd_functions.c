
#include <mqx.h>
#include <message.h>
#include "Cpu.h"
#include "Events.h"
#include "os_tasks.h"
#include "os_tasks.c"
#include "dd_functions.h"

#ifdef __cplusplus
extern "C" {
#endif

_pool_id message_pool;


_task_id dd_tcreate(uint32_t template_index, uint32_t deadline) {
	// Create a task and assign min priorty
	_task_id t_id = _task_create_blocked(0,template_index,0);

	// Send create_task_msg to ddscheduler
	SCHEDULE_MESSAGE_PTR msg_ptr;

	// Allocate a message
	msg_ptr = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);

	if (msg_ptr == NULL) {
		_msg_free(msg_ptr);
		_task_block();
	}

	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULE_QUEUE); // Set the target Queue ID based on queue number
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
	msg_ptr->T_ID = t_id;
	msg_ptr->DEADLINE = deadline;
	msg_ptr->TYPE = 0;
	_msgq_send(msg_ptr);
	_msg_free(msg_ptr);
}

//_task_id dd_tcreate_runtime(uint32_t template_index, uint32_t deadline, uint32_t runtime) {
//	// Create a task and assign min priorty
//	_task_id t_id = _task_create_blocked(0,template_index,runtime);
//
//	// Send create_task_msg to ddscheduler
//	SCHEDULE_MESSAGE_PTR msg_ptr;
//
//	// Allocate a message
//	msg_ptr = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);
//
//	if (msg_ptr == NULL) {
//		_msg_free(msg_ptr);
//		_task_block();
//	}
//
//	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULE_QUEUE); // Set the target Queue ID based on queue number
//	msg_ptr->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
//	msg_ptr->T_ID = t_id;
//	msg_ptr->DEADLINE = deadline;
//	msg_ptr->TYPE = 0;
//	_msgq_send(msg_ptr);
//	_msg_free(msg_ptr);
//}

uint32_t dd_delete(_task_id task_id) {
	// Send create_task_msg to ddscheduler
	SCHEDULE_MESSAGE_PTR msg_ptr;

	// Allocate a message
	msg_ptr = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);

	if (msg_ptr == NULL) {
		_msg_free(msg_ptr);
		_task_block();
	}
	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULE_QUEUE); // Set the target Queue ID based on queue number
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
	msg_ptr->T_ID = task_id;
	msg_ptr->TYPE = 1;
	_msgq_send(msg_ptr);
	_msg_free(msg_ptr);

	if(_task_abort(task_id) != MQX_OK) {
		// failed abort
		printf("failed abort\n");
		return 0;
	}

}

uint32_t dd_return_active_list(struct task_list **list){
	_queue_id qid = _msgq_open(QUEUE_ID, 0);
	if (qid == 0) {
		printf("\nCould not open the  message queue return active\n");
		_task_block();
	}

	// Send create_task_msg to ddscheduler
	SCHEDULE_MESSAGE_PTR msg_ptr;

	// Allocate a message
	msg_ptr = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);
	if (msg_ptr == NULL) {
		_msg_free(msg_ptr);
		_task_block();
	}

	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULE_QUEUE); // Set the target Queue ID based on queue number
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
	msg_ptr->TYPE = 2;
	msg_ptr->Q_ID = qid;
	msg_ptr->P_ID = message_pool;

	_msgq_send(msg_ptr);
	_msg_free(msg_ptr);

	SCHEDULE_MESSAGE_PTR msg_ptr_active;
	msg_ptr_active = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);
	msg_ptr_active = _msgq_receive(qid, 2000);
	if (msg_ptr_active == NULL) {
		if(!_msgq_close(qid)){
				return 0;
			}
		return 0;

	}
	if (!msg_ptr_active->IS_ACTIVE) {
		list = NULL;
		if(!_msgq_close(qid)){
				return 0;
			}
		return 1;
	}

	*list = msg_ptr_active->ACTIVE_HEAD;

	_msg_free(msg_ptr_active);

	//delete queue and pool
	if(!_msgq_close(qid)){
		return 0;
	}
	return 1;

}

uint32_t dd_return_overdue_list(struct overdue_tasks **list){
	_queue_id qid = _msgq_open(QUEUE_ID+1, 0);
	if (qid == 0) {
		printf("\nCould not open the system message queue return overdue\n");
		_task_block();
	}


	// Send create_task_msg to ddscheduler
	SCHEDULE_MESSAGE_PTR msg_ptr;

	// Allocate a message
	msg_ptr = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);

	if (msg_ptr == NULL) {
		_msg_free(msg_ptr);
		_task_block();
	}

	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULE_QUEUE); // Set the target Queue ID based on queue number
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULE_MESSAGE);
	msg_ptr->TYPE = 3;
	msg_ptr->Q_ID = qid;
	msg_ptr->P_ID = message_pool;

	_msgq_send(msg_ptr);
	_msg_free(msg_ptr);

	SCHEDULE_MESSAGE_PTR msg_ptr_active;
	msg_ptr_active = (SCHEDULE_MESSAGE_PTR) _msg_alloc(message_pool);
	msg_ptr_active = _msgq_receive(qid, 2000);
	if (msg_ptr_active == NULL) {
		if(!_msgq_close(qid)){
				return 0;
			}
		return 0;
	}
	if (!msg_ptr_active->IS_OVERDUE) {
		list = NULL;
		if(!_msgq_close(qid)){
				return 0;
			}
		return 1;
	}

	*list = msg_ptr_active->OVERDUE_HEAD;
	_msg_free(msg_ptr_active);

	//delete queue and pool
	if(!_msgq_close(qid)){
		return 0;
	}
	return 1;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif
