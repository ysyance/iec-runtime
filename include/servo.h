#ifndef __SERVO_H__
#define __SERVO_H__

#include <native/heap.h>
#include <native/task.h>
#include <native/heap.h>
#include <native/mutex.h>
#include <native/cond.h>
#include <stdint.h>
#include "syslimit.h"
#include "rc.h"

#define SV_TASK_NAME "sv_task"		/* 伺服任务名 */
#define SV_TASK_PRIORITY 90			/* 伺服任务优先级 */

extern RT_HEAP sv_heap_desc;		/* 伺服映像区heap描述符 */
extern RT_MUTEX sv_mutex_desc;		/* 伺服映像同步互斥量描述符 */
/*-----------------------------------------------------------------------------
 * Servo Configuration
 *---------------------------------------------------------------------------*/
typedef struct {
	char name[MAX_AXIS_NAME_SIZE]; /* 轴名称：axis name, including '\0' */
	uint8_t id;                    /* 轴ID：axis id */
	uint8_t type;                  /* 轴类型：axis type: FINITE | MODULO */
	uint8_t combined;              /* （单轴|联合轴）independent axis | combined axis */
	uint8_t opmode;                /* 运行模式：operating mode: POSITION | VELOCITY | TORQUE */
	double min_pos;                /* 最小位置（反向极限位置）：negtive position limit (unit:) */
	double max_pos;                /* 最大位置（正向极限位置）：positive position limit (unit:) */
	double max_vel;                /* 最大速度：velocity limit (unit:) */
	double max_acc;                /* 最大加速度：accelaration limit (unit:) */
	double max_dec;                /* 最大减速度：decelaration limit (unit:) */
	double max_jerk;               /* 最大加加速度：jerk limit (unit:) */
} AxisConfig;

typedef struct {
	uint8_t axis_count;       /* 轴个数 */
	uint32_t update_interval; /* 伺服数据更新周期 */
	AxisConfig axis_config[MAX_AXIS_COUNT];   /* 轴配置信息数组 */
} ServoConfig;

typedef struct {
    double actual_pos;			/* 实际位置 */
    double actual_vel;			/* 实际速度 */
    double actual_acc;			/* 实际加速度 */
    double command_pos;			/* 目标位置 */
    double command_vel;			/* 目标速度 */
    double command_acc;			/* 目标加速度 */
} AXIS_DATA; /* Axis Runtime Data */

typedef struct {
	uint32_t device_type ;   	/* 设备类型（此处为主站设备） */
	uint32_t error_register; 	/* 错误寄存器 */
	uint32_t device_id;      	/* 设备ID */

    uint32_t control_word;  	/* 控制字 */
    uint32_t status_word;		/* 状态字 */
    uint32_t opreate_mode;		/* 运行模式 */
    uint32_t mode_display;  	/* 运行模式显示 */

    uint32_t velocity_factor;	/* 速度控制参数 */
    uint32_t position_factor; 	/* 位置控制参数 */

} AXIS_CTRL_AREA; /* 伺服控制域，基于CiA DS402选取 */

typedef struct {
	AXIS_CTRL_AREA ctrl_area;				/* 伺服控制域 */
	uint8_t axis_count; 					/* 轴个数 */
	AXIS_DATA axis_data[MAX_AXIS_COUNT];	/* 伺服数据域 */
} SVMem;   /* 伺服映像区数据结构，包括伺服控制域和伺服数据域 */

#define SV_CONF_NAME "sv_conf"
#define SV_CONF_SIZE (sizeof(ServoConfig))
#define AXIS_CONF_SIZE (sizeof(AxisConfig))
/*-----------------------------------------------------------------------------
 * 伺服配置域共享内存操作函数
 *---------------------------------------------------------------------------*/
inline void sv_conf_create(RT_HEAP *heap, ServoConfig **conf) {
    if (rt_heap_create(heap, SV_CONF_NAME, SV_CONF_SIZE, H_SHARED) < 0) {
        LOGGER_ERR(E_HEAP_CREATE, "(name=%s, size=%d)", SV_CONF_NAME, SV_CONF_SIZE);
    }
    if (rt_heap_alloc(heap, SV_CONF_SIZE, TM_INFINITE, (void **)conf) < 0) {
        LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d)", SV_CONF_NAME, SV_CONF_SIZE);
    }
}
inline void sv_conf_bind(RT_HEAP *heap, ServoConfig **conf) {
    if (rt_heap_bind(heap, SV_CONF_NAME, TM_INFINITE) < 0) {
        LOGGER_ERR(E_HEAP_BIND, "(name=%s, size=%d)", SV_CONF_NAME, SV_CONF_SIZE);
    }
    if (rt_heap_alloc(heap, 0, TM_NONBLOCK, (void **)conf) < 0) {
        LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d)", SV_CONF_NAME, SV_CONF_SIZE);
    }
}
inline void sv_conf_unbind(RT_HEAP *heap) {
    rt_heap_unbind(heap);
}
inline void sv_conf_delete(RT_HEAP *heap) {
    if (rt_heap_delete(heap) < 0) {
        LOGGER_ERR(E_HEAP_DELETE, "(name=%s, size=%d)", SV_CONF_NAME, SV_CONF_SIZE);
    }
}

extern SVMem *sv_shm;               /* 伺服共享区指针 */

/*-----------------------------------------------------------------------------
 * 伺服数据域共享内存操作函数
 *---------------------------------------------------------------------------*/
#define SV_MEM_NAME "sv_mem"

inline void sv_mem_create(SVMem *&svmem, ServoConfig *config) {
	int size = sizeof(SVMem);
	int ret = 0;
	if ((ret = rt_heap_create(&sv_heap_desc, SV_MEM_NAME, sizeof(SVMem), H_SHARED)) < 0) {
	   LOGGER_ERR(E_HEAP_CREATE, "(name=%s, size=%d, ret=%d)", SV_MEM_NAME, size, ret);
	}
	/* MUST called from realtime context (REF: Xenomai API) */
	if ((ret = rt_heap_alloc(&sv_heap_desc, sizeof(SVMem), TM_INFINITE, (void **)&svmem)) < 0) {
	   LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d, ret=%d)", SV_MEM_NAME, size, ret);
	}
}

inline void sv_mem_init(SVMem *&svmem, ServoConfig *config){
	/* TODO */
	// svmem->axis_count = config->axis_count;   /* 目前测试阶段以6轴为例，不采用此句 */
	svmem->axis_count = 6;
}

inline void sv_mem_bind(SVMem *&svmem, ServoConfig *config) {
   int size = sizeof(SVMem);
   int ret = 0;
   if (rt_heap_bind(&sv_heap_desc, SV_MEM_NAME, TM_INFINITE) < 0) {
	   LOGGER_ERR(E_HEAP_BIND, "(name=%s, size=%d)", SV_MEM_NAME, size);
   }
   if (rt_heap_alloc(&sv_heap_desc, 0, TM_NONBLOCK, (void **)&svmem) < 0) {
	   LOGGER_ERR(E_HEAP_ALLOC, "(name=%s, size=%d)", SV_MEM_NAME, size);
   }
}

inline void io_mem_unbind(SVMem *svmem, ServoConfig *config) {
   int size = sizeof(SVMem);
   int ret = 0;
   if (rt_heap_unbind(&sv_heap_desc) < 0) {
	   LOGGER_ERR(E_HEAP_UNBIND, "(name=%s, size=%d)", SV_MEM_NAME, size);
   }
}

/*-----------------------------------------------------------------------------
 * 伺服数据域共享内存同步对象操作函数
 *---------------------------------------------------------------------------*/
#define SV_MUTEX_NAME  "sv_mutex"

inline void sv_syncobj_create(RT_MUTEX *mutex_desc, const char* mutex_name){
    if(rt_mutex_create(mutex_desc, mutex_name) < 0){
        LOGGER_ERR(E_SVMUTEX_CREATE, "(name=%s)", mutex_name);
    }
}

inline void sv_syncobj_delete(RT_MUTEX *mutex_desc){
	if(rt_mutex_delete(mutex_desc) < 0){
        LOGGER_ERR(E_SVMUTEX_DEL, "(name=sv_mutex)");
    }
}

inline void sv_syncobj_bind(RT_MUTEX *mutex_desc, const char* mutex_name){
	if(rt_mutex_bind(mutex_desc, mutex_name, TM_INFINITE) < 0){
        LOGGER_ERR(E_SVMUTEX_BIND, "(name=%s)", mutex_name);
    }
}

/*-----------------------------------------------------------------------------
 * 伺服数据域共享内存数据交换操作函数:由PLC任务调用
 *---------------------------------------------------------------------------*/
inline void sv_shm_plc2servo(SVMem *sv_shm, RobotInterpData *axis_command_info){
	rt_mutex_acquire(&sv_mutex_desc, TM_INFINITE);      /* 获得同步互斥量 */
    for(int i = 0; i < sv_shm->axis_count; i ++){
		sv_shm->axis_data[i].command_pos = axis_command_info->interp_value[i].command_pos;
		sv_shm->axis_data[i].command_vel = axis_command_info->interp_value[i].command_vel;
		sv_shm->axis_data[i].command_acc = axis_command_info->interp_value[i].command_acc;
	}
	rt_mutex_release(&sv_mutex_desc);           		/* 释放同步互斥量 */
}

inline void sv_shm_servo2plc(SVMem *sv_shm, RobotAxisActualInfo *robot_actual_info_buffer){
	rt_mutex_acquire(&sv_mutex_desc, TM_INFINITE);      /* 获得同步互斥量 */
	for(int i = 0; i < sv_shm->axis_count; i ++){
		robot_actual_info_buffer->axis_info[i].actual_pos = sv_shm->axis_data[i].actual_pos;
		robot_actual_info_buffer->axis_info[i].actual_vel = sv_shm->axis_data[i].actual_vel;
		robot_actual_info_buffer->axis_info[i].actual_acc = sv_shm->axis_data[i].actual_acc;
	}
	rt_mutex_release(&sv_mutex_desc);           		/* 释放同步互斥量 */
}









static void sv_update(void *config);
void sv_task_init(ServoConfig *config);
void sv_task_start(ServoConfig *config);
#endif
