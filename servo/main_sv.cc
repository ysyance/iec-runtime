#include <sys/mman.h> /* required for mlockall() */
#include <native/task.h>
#include <native/heap.h>
#include <unistd.h> /* required for pause() */
#include <signal.h>

#include "mq.h"
#include "servo.h"
#include "logger.h"
#undef LOGGER_LEVEL
#define LOGGER_LEVEL LEVEL_ERR

RT_TASK sv_task;
RT_HEAP sv_heap;
RT_HEAP svconf_heap;
ServoConfig *sv_conf;
char *sv_shm;

#define dump_servo_conf(config) {\
	LOGGER_DBG(DFLAG_LONG, "Received ServoConfig:\n .axis_count = %d\n .update_interval = %d",\
        config.axis_count, config.update_interval); \
}
#define dump_axis_conf(config) {\
    LOGGER_DBG(DFLAG_SHORT,\
            "Received AxisConfig:\n .name = %s\n .id = %d\n .type = %d\n .combined = %d\n .opmode = %d\n"\
            " .min_pos = %f\n .max_pos = %f\n .max_vel = %f\n .max_acc = %f\n .max_dec = %f\n .max_jerk = %f",\
            config.name, config.id, config.type, config.combined, config.opmode,\
            config.min_pos, config.max_pos, config.max_vel, config.max_acc, config.max_dec, config.max_jerk);\
}

/* realtime context */
static void servo_update(void *cookie) {
    sv_conf_bind(&svconf_heap, &sv_conf);
    dump_servo_conf((*sv_conf));
    int size = sv_conf->axis_count * sizeof(AXIS_DATA);
	if (rt_heap_create(&sv_heap, "sv_shm", size, H_SHARED) < 0) {
        LOGGER_ERR(E_HEAP_CREATE, "(name=sv_shm, size=%d)", size);
    }
    if (rt_heap_alloc(&sv_heap, size, TM_INFINITE, (void **)&sv_shm) < 0) {
        LOGGER_ERR(E_HEAP_ALLOC, "(name=sv_shm, size=%d)", size);
    }

	rt_task_set_periodic(NULL, TM_NOW, sv_conf->update_interval);
	while (1) {
		rt_task_wait_period(NULL);
        // LOGGER_INF("STUB: Update servo data...", 0);
	}
}

void sig_handler(int signo) {
    LOGGER_DBG(DFLAG_SHORT, "Servo Driver Received Signal: %d", (int)signo);
    if (signo == SIGUSR1) {
        if (rt_heap_delete(&sv_heap) < 0) {
            LOGGER_ERR(E_HEAP_DELETE, "");
        }
        sv_conf_unbind(&svconf_heap);
        if (rt_task_delete(&sv_task) < 0) {
            LOGGER_ERR(E_SVTASK_DELETE, "");
        }
        exit(0);
    }
}
/* non-realtime context */
int main() {
    mlockall(MCL_CURRENT|MCL_FUTURE);

    /* set signal handler */
    if (signal(SIGUSR1, &sig_handler) == SIG_ERR) {
        LOGGER_ERR(E_SIG_SVKILL, "");
    }

    /* start servo rt task */
	if (rt_task_create(&sv_task, SV_TASK_NAME, 0, SV_TASK_PRIORITY, 0) < 0) {
        LOGGER_ERR(E_SVTASK_CREATE, "");
	}
	if (rt_task_start(&sv_task, &servo_update, (void *)0) < 0) {
        LOGGER_ERR(E_SVTASK_START, "");
	}
    pause();

    return 0;
}
