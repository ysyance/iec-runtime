#include "softimer.h"



void softimer_server(void* cookie){
	int err = 0;
	int cnt = timer_manager.timer_count;
	int i = 0;
	for(;;){
		/* Wait for the next alarm to trigger. */
		err = rt_alarm_wait(&alarm_desc);
		if(!err) {
			/* Process the alarm shot. */
			for(i = 0; i < cnt; i ++){
				if(timer_manager.timer_list[i].startflag){
					if(timer_manager.timer_list[i].cur_count < timer_manager.timer_list[i].target_count){
						timer_manager.timer_list[i].cur_count += 1;
					} else {
						timer_manager.timer_list[i].cur_count = 0;
					}
				} else {
					timer_manager.timer_list[i].cur_count = 0;
				}
			}
		}
	}
}


