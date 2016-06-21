#ifndef __RC_H__
#define __RC_H__

#include "plcmodel.h"
#include "logger.h"
#include <native/task.h>
#include <native/heap.h>
#include <native/mutex.h>
#include <native/cond.h>

#define RC_TASK_NAME "rc_task"      /* RC任务名 */
#define RC_TASK_PRIORITY 80         /* RC任务优先级 */

#define ROBOT_AXIS_COUNT 6          /* 机器人轴个数 */
#define CIRCULAR_INTERP_QUEUE_SIZE 10   /* 环形插补队列大小 */

extern RT_HEAP rc_heap_desc;
extern RT_COND rc_cond_desc;
extern RT_MUTEX rc_mutex_desc;

struct SingleInterpData{        /* 单轴插补值,来自RC */
    double command_pos;         /* 目标位置 */
    double command_vel;         /* 目标速度 */
    double command_acc;         /* 目标加速度 */
} ;

struct SingleAxisInfo{          /* 单轴实际位置值,来自PLC  */
	double actual_pos;          /* 实际位置 */
    double actual_vel;          /* 实际速度 */
    double actual_acc;          /* 实际加速度 */
} ;

struct RobotAxisActualInfo{     /* 机器人各个轴的信息 */
	int size ;                  /* 机器人轴个数 */
	SingleAxisInfo axis_info[ROBOT_AXIS_COUNT];    /* 对应各个轴的位置，速度，加速度信息 */
} ;

struct RobotInterpData{         /* 机器人各个轴的插补值 */
	int size;			        /* 机器人轴个数 */
	SingleInterpData interp_value[ROBOT_AXIS_COUNT];   /* 对应各个轴的插补值 */
} ;

struct CircularInterpQueue{/* 环形插补队列 */
	int queue_size;        /* 环形插补队列大小 */
	int head;              /* 环形队列头指针（实际是data[]数组的索引，即0～queue_size-1之间），RC由此写入插补值 */
	int tail;              /* 环形队列尾指针（实际是data[]数组的索引，即0～queue_size-1之间），PLC由此读出插补值 */
	RobotInterpData data[CIRCULAR_INTERP_QUEUE_SIZE];  /* 环形队列中存放插补值的实际数组 */
} ;

struct RCMem{                              /* RC与PLC共享内存数据结构 */
	RobotAxisActualInfo actual_info;       /* 机器人各个轴实际位置，速度，加速度值 */
	CircularInterpQueue interp_queue;      /* 机器人插补值队列 */
} ;

extern RobotInterpData robot_interpdata_buffer;  /* 机器人插补数据缓存，用于存放从共享内存区读到的插补数据，处理后写入伺服映像区 */
extern RobotAxisActualInfo robot_actual_info_buffer; /* 机器人实际轴位置信息缓存，由伺服映像区加载得到，处理后写入RC共享内存 */
extern RCMem *rc_shm;               /* RC/PLC共享区指针 */

/*-----------------------------------------------------------------------------
 * PLC/RC共享内存创建操作函数
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

 inline void rc_mem_init(RCMem *&rcmem, RobotConfig *config){
     /* TODO */
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

/*-----------------------------------------------------------------------------
 * PLC/RC共享内存同步操作函数
 *---------------------------------------------------------------------------*/
#define RC_MUTEX_NAME  "rc_mutex"
#define RC_COND_NAME   "rc_cond"

inline void rc_syncobj_create(RT_MUTEX *mutex_desc, const char* mutex_name, RT_COND *cond_desc, const char* cond_name){
    if(rt_mutex_create(mutex_desc, mutex_name) < 0){
        LOGGER_ERR(E_RCMUTEX_CREATE, "(name=%s)", mutex_name);
    }
    if(rt_cond_create(cond_desc, cond_name) < 0){
        LOGGER_ERR(E_RCCOND_CREATE, "(name=%s)",cond_name);
    }
}

inline void rc_syncobj_delete(RT_MUTEX *mutex_desc, RT_COND *cond_desc){
    if(rt_cond_delete(cond_desc) < 0){
        LOGGER_ERR(E_RCMUTEX_DEL, "(name=rc_mutex)");
    }
	if(rt_mutex_delete(mutex_desc) < 0){
        LOGGER_ERR(E_RCCOND_DEL, "(name=rc_cond)");
    }
}

void rc_shm_servo_init(RCMem *rc_shm);
void rc_shm_servo_read(RCMem *rc_shm, RobotInterpData *axis_command_info);
void rc_shm_servo_write(RCMem *rc_shm, RobotAxisActualInfo *axis_actual_info);


#endif
