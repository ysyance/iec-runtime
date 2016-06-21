#include <sys/mman.h> /* required for mlockall() */
#include <native/task.h>
#include <native/queue.h>
#include <native/heap.h>
#include <unistd.h> /* required for pause() */
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include "rc.h"
#include "plcmodel.h"

RCMem *rc_shm;
RT_HEAP rc_heap_desc;
RobotConfig *rc_conf;
RT_TASK rc_test_task;

/* realtime context */
static void task_routine(void *cookie) {
    printf("rc-test-task: \n");
    rc_mem_bind(rc_shm, rc_conf);
    printf("shuchu\n");
    if(rc_shm->actual_info.size == 6){
        printf("yes\n");
    } else {
        printf("no\n");
    }
    printf("%d\n", rc_shm->actual_info.size);
    printf("%f\n", rc_shm->actual_info.axis_info[0].actual_pos);
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        if (rt_heap_delete(&rc_heap_desc) < 0) {
            LOGGER_ERR(E_HEAP_DELETE, "");
        }
        io_mem_unbind(rc_shm, rc_conf);
        if (rt_task_delete(&rc_test_task) < 0) {
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
	if (rt_task_create(&rc_test_task, "rc_test", 0, 85, 0) < 0) {
        LOGGER_ERR(E_SVTASK_CREATE, "");
	}
	if (rt_task_start(&rc_test_task, &task_routine, (void *)0) < 0) {
        LOGGER_ERR(E_SVTASK_START, "");
	}
    pause();

    return 0;
}
