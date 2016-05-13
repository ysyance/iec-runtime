#include <sys/mman.h> /* required for mlockall() */
#include <linux/input.h> /* required for input event */
#include <stdio.h>
#include <unistd.h> /* required for fork/exec */
#include <fcntl.h>  /* required for open */
#include <signal.h>
#include <native/queue.h>
#include <native/heap.h>

#include "loader.h"
#include "mq.h"
#include "vm.h"
#include "io.h"
#include "servo.h"
#include "logger.h"
#include "rc.h"

#define MAX_OBJNAME 50


RT_TASK io_task;
RT_TASK sv_task;
RT_TASK rc_task;

RT_TASK_INFO io_info;
RT_TASK_INFO sv_info;
RT_QUEUE io_mq;
RT_QUEUE sv_mq;

RT_HEAP ioconf_heap;
RT_HEAP svconf_heap;
RT_HEAP rc_heap_desc;

IOConfig *io_conf;
IOMem io_shm;
RCMem *rc_shm;
ServoConfig *sv_conf;
RobotConfig *rc_conf;
TaskList plc_task;

pid_t io_pid, sv_pid, rc_pid;
char objname[MAX_OBJNAME] = "exec.obj";

void sig_handler(int signo) {
    LOGGER_DBG(DFLAG_SHORT, "Kernel Received Signal: %d", (int)signo);
    if (signo == SIGINT) {
        printf("cp 1\n");
        /* ORDER SENSITIVE */
        if (kill(io_pid, SIGUSR1) == -1)
        {
            printf("send signal to io-task error !\n");
        }

        printf("cp 2\n");
        if (kill(sv_pid, SIGUSR1) == -1)
        {
            printf("send signal to sv-task error !\n");
        }

        printf("cp 3\n");
        plc_task_delete(&plc_task);
        printf("cp 4\n");
        io_mem_delete(&io_shm, io_conf);
        printf("cp 5\n");
        rt_task_unbind(&io_task);
        printf("cp 6\n");
        rt_task_unbind(&sv_task);
        printf("cp 7\n");
        io_conf_delete(&ioconf_heap);
        printf("cp 8\n");
        sv_conf_delete(&svconf_heap);
        printf("cp 9\n");
        exit(0);
    } else {

    }
}
void insert_io_driver(pid_t io_pid) {
    if ((io_pid = fork()) < 0) {
        LOGGER_ERR(E_IOTASK_FORK, "");
    } else if (io_pid == 0) {
        if (execl("/mnt/share/iec-runtime/io-task", "") < 0) {
            LOGGER_ERR(E_IOTASK_EXEC, "");
        }
    }
}
void insert_sv_driver(pid_t sv_pid) {
    if ((sv_pid = fork()) < 0) {
        LOGGER_ERR(E_SVTASK_FORK, "");
    } else if (sv_pid == 0) {
        if (execl("/mnt/share/iec-runtime/sv-task", "") < 0) {
            LOGGER_ERR(E_SVTASK_EXEC, "");
        }
    }
}

void insert_rc_task(pid_t rc_pid){
    if((rc_pid = fork()) < 0){
        LOGGER_ERR(E_RCTASK_FORK, "");
    } else if(rc_pid == 0) {
        if(execl("/mnt/share/rc-runtime/rc-runtime.exe", "rc-runtime.exe", "/mnt/share/rc-runtime/test/lab") < 0){
            LOGGER_ERR(E_RCTASK_EXEC, "");
        } else {
            printf("OK\n");
        }
    }
}

pid_t rc_test_pid;
void insert_rc_test_task(pid_t rc_test_pid){
    if((rc_test_pid = fork()) < 0){
        LOGGER_ERR(E_RCTASK_FORK, "");
    } else if(rc_test_pid == 0) {
        if(execl("/mnt/share/rc-test-task", "") < 0){
            LOGGER_ERR(E_RCTASK_EXEC, "");
        } else {
            printf("OK\n");
        }
    }
}


RT_TASK rc_test_task;
static int initflag = 0;

static void task_routine(void *cookie) {


    printf("main task: \n");
    printf("%d\n", sizeof(*rc_shm));
    rt_task_sleep(1000000000);

}

int main(int argc, char *argv[]) {
	//Avoids memory swapping for this program
    mlockall(MCL_CURRENT|MCL_FUTURE);

    if (signal(SIGINT, &sig_handler) == SIG_ERR) {
        LOGGER_ERR(E_SIG_PLCKILL, "");
    }
    while (1) {
        if (initflag == 0) {
            initflag = 1;

            io_conf_create(&ioconf_heap, &io_conf);
            sv_conf_create(&svconf_heap, &sv_conf);

            LOGGER_DBG(DFLAG_LONG, "plc file name = %s", objname);
            load_obj(objname, io_conf, sv_conf, &plc_task);

            io_mem_create(&io_shm, io_conf, M_SHARED);
            io_mem_zero(&io_shm, io_conf);

            rc_mem_create(rc_shm, rc_conf);
            rc_shm->actual_info.size = 6;
            rc_shm->actual_info.axis_info[0].actual_pos = 12.34;
            rc_shm->actual_info.axis_info[1].actual_pos = 12.34;
            rc_shm->actual_info.axis_info[2].actual_pos = 12.34;
            rc_shm->actual_info.axis_info[3].actual_pos = 12.34;
            rc_shm->actual_info.axis_info[4].actual_pos = 12.34;
            rc_shm->actual_info.axis_info[5].actual_pos = 12.34;

            insert_io_driver(io_pid);
            insert_sv_driver(sv_pid);
            insert_rc_task(rc_pid);

            // /* block until binding successfully */
            // rt_task_bind(&io_task, IO_TASK_NAME, TM_INFINITE);
            // rt_task_bind(&sv_task, SV_TASK_NAME, TM_INFINITE);
            // rt_task_bind(&rc_task, RC_TASK_NAME, TM_INFINITE);

            // plc_task_init(&plc_task);
            // plc_task_start(&plc_task);
        }
    }
    return 0;
}
