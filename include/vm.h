#ifndef __VM_H__
#define __VM_H__

#include "plcmodel.h"


static void executor(void *task);
void plc_task_init(TaskList *task_list);
void plc_task_start(TaskList *task_list);
void plc_task_suspend(TaskList *task_list);
void plc_task_resume(TaskList *task_list);
void plc_task_delete(TaskList *task_list);
void plc_timer_task_init(TaskList *task_list);
void plc_timer_task_start(TaskList *task_list);
void plc_comanager_task_init(TaskList *task_list);
void plc_comanager_task_start(TaskList *task_list);




#endif
