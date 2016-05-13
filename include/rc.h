#ifndef __RC_H__
#define __RC_H__

#include "plcmodel.h"
#include "logger.h"
#include <native/task.h>
#include <native/heap.h>

#define RC_TASK_NAME "rc_task"
#define RC_TASK_PRIORITY 80

#define ROBOT_AXIS_COUNT 6
#define CIRCULAR_INTERP_QUEUE_SIZE 10

 extern RT_HEAP rc_heap_desc;

/* TODO Add multiple axis ?? */
typedef struct {
    double command_pos;				
    double command_vel;
    double command_acc;
} SingleInterpData; /* Single axis interpolation command from RC */

typedef struct {
	double actual_pos;
    double actual_vel;
    double actual_acc;
} SingleAxisInfo;  /* Single axis  actual data from PLC  */

typedef struct {
	int size ;
	SingleAxisInfo axis_info[ROBOT_AXIS_COUNT]; 
} RobotAxisActualInfo;

typedef struct {
	int size;			// axis count, that is , ROBOT_AXIS_COUNT
	SingleInterpData interp_value[ROBOT_AXIS_COUNT];
} RobotInterpData;

typedef struct {
	int queue_size;
	int head;
	int tail;
	RobotInterpData data[CIRCULAR_INTERP_QUEUE_SIZE];
} CircularInterpQueue;  /* circular interpolation queue */

typedef struct {
	RobotAxisActualInfo actual_info;
	CircularInterpQueue interp_info;
} RCMem;



/*-----------------------------------------------------------------------------
 * RC Shared Memory Operation Funcions
 *---------------------------------------------------------------------------*/

 #define RC_MEM_NAME "rc_mem"
 inline void rc_mem_create(RCMem *&rcmem, RobotConfig *config) {
 	int size = sizeof(RCMem);
 	int ret = 0;
 	if ((ret = rt_heap_create(&rc_heap_desc, RC_MEM_NAME, sizeof(RCMem), H_SHARED)) < 0) {
        LOGGER_ERR(E_HEAP_CREATE, "(name=%s, size=%d, ret=%d)", RC_MEM_NAME, size, ret);
    }
    /* MUST called from realtime context (REF: Xenomai API) */
    if ((ret = rt_heap_alloc(&rc_heap_desc, sizeof(RCMem), TM_INFINITE, (void **)&rcmem)) < 0) {
        LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d, ret=%d)", RC_MEM_NAME, size, ret);
    }
 }

 inline void rc_mem_bind(RCMem *&rcmem, RobotConfig *config) {
 	int size = sizeof(RCMem);
 	int ret = 0;
 	if (rt_heap_bind(&rc_heap_desc, RC_MEM_NAME, TM_INFINITE) < 0) {
        LOGGER_ERR(E_HEAP_BIND, "(name=%s, size=%d)", RC_MEM_NAME, size);
    }
    if (rt_heap_alloc(&rc_heap_desc, 0, TM_NONBLOCK, (void **)&rcmem) < 0) {
        LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d)", RC_MEM_NAME, size);
    }
 }

inline void io_mem_unbind(RCMem *rcmem, RobotConfig *config) {
    int size = sizeof(RCMem);
   	int ret = 0;
    if (rt_heap_unbind(&rc_heap_desc) < 0) {
        LOGGER_ERR(E_HEAP_UNBIND, "(name=%s, size=%d)", RC_MEM_NAME, size);
    }
}


//static void interp_input_update();
//static void interp_output_update();
static void interp_calculate(void *config); /* TODO NOTE single/multiple axis interpolation */
static void rc_task_create();
void rc_task_init(RobotConfig *config);
void rc_task_start(RobotConfig *config);
void rc_task_delete();
#endif
