#ifndef __SOFTIMER_H__
#define __SOFTIMER_H__

#include <sys/mman.h>
#include <native/task.h>
#include <native/alarm.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <vector>

typedef struct{
	unsigned long target_count;    	/* unit: ms */
	unsigned long cur_count;	  	/* current count value */
	bool startflag;					/* indicate if start up the timer */
	bool arriveflag;				/* indicate that target time has arrived */
}softimer_t;



typedef struct{
	unsigned int timer_count;		/* how many softimers in the rumtime system */
	std::vector<softimer_t> timer_list; /* softimer list */
}softimer_manager_t;



softimer_manager_t timer_manager;

RT_ALARM alarm_desc;


void softimer_server(void* cookie);



#endif