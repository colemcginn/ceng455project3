
#include <mqx.h>
#include <message.h>
#include "Cpu.h"
#include "Events.h"
#include "os_tasks.h"
#include "user_task_access_functions.h"

#ifdef __cplusplus
extern "C" {
#endif

_queue_id serial_qid;

bool OpenR(_queue_id stream_no){
	//check for prev permission
	for(int i=0;i<num_read;i++){
		if(read_permissions[i].q_id == stream_no){
			//already has read permission
			return false;
		}
	}
	//mutex
	if (_mutex_lock(&openR_mutex) != MQX_OK) {
	 // _task_block();
		// failed mutex
		return false;
	}
	open_permission perm = {_task_get_id(), stream_no};
	read_permissions[num_read] = perm;
	num_read++;

	_mutex_unlock(&openR_mutex);
	return true;
}
bool _getline(char *string){
	if(output_copy[0]=='\0'){
		return false;
	}
	for(int i=0;i<copy_count;i++){
		string[i] = output_copy[i];
	}
	return true;
//
////	printf("getline\n");
//	SERIAL_MESSAGE_PTR msg_ptr;
////	printf("serial task qid: %d\n",task_qid);
//	msg_ptr = _msgq_receive(task_qid, 0);
//	if (msg_ptr == NULL) {
//		// continue;
//		return false;
//	}
////	while(1) {
////		if(msg_ptr!=NULL){
////			break;
////		}
////		if(msg_ptr->DATA!=NULL){
////			break;
////		}
////		//no message
////		msg_ptr = _msgq_receive(task_qid, 0);
//////		return false;
////	}
//	printf("data: %s\n",msg_ptr->DATA);
//	for(int i=0;i<strlen(msg_ptr->DATA);i++){
//		string[i] = msg_ptr->DATA[i];
//	}
//	//Check for permission
//	return true;

}
_queue_id OpenW(void){
	if(write_permission!=0){
		//permission already given for writing
		return 0;
	}
	if (_mutex_lock(&openW_mutex) != MQX_OK) {
		// failed mutex
		printf("failed mutex write\n");
		return 0;
	}

	write_permission = _task_get_id();

	_mutex_unlock(&openW_mutex);

	return serial_qid;


}
bool _putline(_queue_id qid, char *string){
	//check permission
	if(write_permission != _task_get_id()){
		// no write permission
		return false;
	}
	printf("size %d %d\n",sizeof(string),copy_count);
	UART_DRV_SendDataBlocking(myUART_IDX, string, copy_count, 1000);
	//append \n
//	strcat(string,"\n");
//
//	//make message for entire string
//	for(int j=0;j<num_read;j++){
//
//		for(int i=0;i<strlen(string);i++){
//			SERIAL_MESSAGE_PTR msg_ptr;
//
//			// Allocate a message
//			msg_ptr = (SERIAL_MESSAGE_PTR) _msg_alloc(task_message_pool);
//
//			if (msg_ptr == NULL) {
//				_msg_free(msg_ptr);
//				return false;
//			}
//
//			msg_ptr->HEADER.TARGET_QID = read_permissions[j].q_id;//qid; // Set the target Queue ID based on queue number
//			msg_ptr->HEADER.SIZE = sizeof(MESSAGE_HEADER_STRUCT) + strlen((char *) msg_ptr->DATA) + 1; // TODO is this the right size?
//			msg_ptr->DATA[0] = string[i];
//
//			_msgq_send(msg_ptr);
//
//			_msg_free(msg_ptr);
//		}
//	}
	return true;


}
bool Close(void){
	bool had_permissions = false;
	//check write and remove
	if(write_permission == _task_get_id()){
		write_permission = 0;
		had_permissions = true;
	}

	//check read and remove
	//remake read array
	_task_id t_id = _task_get_id();
	for(int i=0;i<num_read;i++){
		if(read_permissions[i].t_id == t_id){
			num_read--;
			for(int j=i;j<num_read;j++){
				read_permissions[j] = read_permissions[j+1];
			}
			had_permissions = true;
			break;
		}
	}


	return had_permissions;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif
