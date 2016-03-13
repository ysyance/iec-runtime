#include "comanager.h"




#define EVENT_INIT 0
#define EVENT_MODE EV_PRIO

RT_EVENT communicate_event_desc;
RT_TASK communicate_task_desc;

extern TaskList plc_task;

const char* Event_Flag_Name = "CommuincateEvent";

void communicate_server(void* cookie){
	int cnt = plc_task.signal_set.count;
	int err = 0;
	err = rt_event_create(&communicate_event_desc, Event_Flag_Name, EVENT_INIT, EVENT_MODE);
	for(int i = 0; i < cnt; i ++){
		uint16_t temp = plc_task.signal_set.sig[i].current;
		if(temp & (1UL<<15)){

		} else {
			if(plc_task.plcglobal[temp].v.value_u > plc_task.signal_set.sig[i].last){
				unsigned long sig_mask = 1UL << (plc_task.signal_set.sig[i].task_index);
				err = rt_event_signal(&communicate_event_desc, sig_mask);
			}
			plc_task.signal_set.sig[i].last = plc_task.plcglobal[temp].v.value_u;
		}
	}
}