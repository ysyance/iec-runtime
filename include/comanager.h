#ifndef _COMANAGER_H__
#define _COMANAGER_H__

#include <native/task.h>
#include <native/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <vector>

#include "plcmodel.h"





extern const char* Event_Flag_Name;

extern RT_TASK communicate_task_desc;


void communicate_server(void* cookie);



#endif