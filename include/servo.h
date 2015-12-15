#ifndef __SERVO_H__
#define __SERVO_H__

#include <native/heap.h>
#include <stdint.h>
#include "syslimit.h"

#define SV_TASK_NAME "sv_task"
#define SV_TASK_PRIORITY 70
/*-----------------------------------------------------------------------------
 * Servo Configuration
 *---------------------------------------------------------------------------*/
typedef struct {
	char name[MAX_AXIS_NAME_SIZE]; /* axis name, including '\0' */
	uint8_t id;                    /* axis id */
	uint8_t type;                  /* axis type: FINITE | MODULO */
	uint8_t combined;              /* independent axis | combined axis */
	uint8_t opmode;                /* operating mode: POSITION | VELOCITY | TORQUE */
	double min_pos;                /* negtive position limit (unit:) */
	double max_pos;                /* positive position limit (unit:) */
	double max_vel;                /* velocity limit (unit:) */
	double max_acc;                /* accelaration limit (unit:) */
	double max_dec;                /* decelaration limit (unit:) */
	double max_jerk;               /* jerk limit (unit:) */
} AxisConfig;

typedef struct {
	uint8_t axis_count;       /* number of axis */
	uint32_t update_interval; /* Servo data update interval */
	AxisConfig axis_config[MAX_AXIS_COUNT];   /* array of axis configuration */
} ServoConfig;

typedef struct {
    double actual_pos;
    double actual_vel;
    double actual_acc;
    double command_pos;
    double command_vel;
    double command_acc;
} AXIS_DATA; /* Axis Runtime Data */

#define SV_CONF_NAME "sv_conf"
#define SV_CONF_SIZE (sizeof(ServoConfig))
#define AXIS_CONF_SIZE (sizeof(AxisConfig))
/*-----------------------------------------------------------------------------
 * Servo Config Shared Memory Operation Funcions
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
static void sv_update(void *config);
void sv_task_init(ServoConfig *config);
void sv_task_start(ServoConfig *config);
#endif
