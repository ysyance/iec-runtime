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

#define MAX_OBJNAME 50              /* 目标文件名最大字符数 */

RT_TASK io_task;                    /* I/O任务描述符 */
RT_TASK sv_task;                    /* 伺服任务描述符 */
RT_TASK rc_task;                    /* RC任务描述符 */

RT_HEAP ioconf_heap;                /* I/O配置信息共享区描述符 */
RT_HEAP svconf_heap;                /* 伺服配置信息共享区描述符 */
RT_HEAP rc_heap_desc;               /* RC/PLC共享数据区描述符 */

RT_COND rc_cond_desc;               /* RC/PLC同步对象－－条件变量描述符 */
RT_MUTEX rc_mutex_desc;             /* RC/PLC同步对象－－互斥量描述符 */

IOConfig *io_conf;                  /* I/O配置信息共享区指针 */
IOMem io_shm;                       /* I/O映像区指针 */
RCMem *rc_shm;                      /* RC/PLC共享区指针 */
ServoConfig *sv_conf;               /* 伺服配置信息共享区指针 */
RobotConfig *rc_conf;               /* 机器人配置信息共享区指针 */
TaskList plc_task;                  /* PLC任务列表 */

pid_t io_pid, sv_pid, rc_pid;       /* I/O子任务，伺服子任务，RC子任务进程号 */

char objname[MAX_OBJNAME] = "exec.obj";     /* 目标文件名（PLC程序文件） */

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
        rc_syncobj_delete(&rc_mutex_desc, &rc_cond_desc);
        printf("cp 10\n");
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

RT_TASK test_desc;
RobotInterpData axis_command_info;
void test_routine(void *cookie){
    rt_task_set_periodic(NULL, TM_NOW, 8000000);
    int cnt = 1;

    axis_command_info.size = 6;

    while(1){
        rt_task_wait_period(NULL);
        rc_shm_servo_read(rc_shm, &axis_command_info);
        printf("cnt: %d\n", cnt++);
        for(int i = 0; i < 6; i ++){
            printf("plc axis %d, pos: %f, vel: %f, acc: %f\n", i,axis_command_info.interp_value[i].command_pos, axis_command_info.interp_value[i].command_vel, axis_command_info.interp_value[i].command_acc);
            // fflush(stdin);
        }
    }
}

static int initflag = 0;        /* 初始化标志 */

int main(int argc, char *argv[]) {
	//Avoids memory swapping for this program
    mlockall(MCL_CURRENT|MCL_FUTURE);

    if (signal(SIGINT, &sig_handler) == SIG_ERR) {
        LOGGER_ERR(E_SIG_PLCKILL, "");
    }
    while (1) {
        if (initflag == 0) {
            initflag = 1;       /* 初始化标志置位，初始化只进行一次 */

            io_conf_create(&ioconf_heap, &io_conf);     /* 创建I/O配置共享内存区，用于向I/O驱动子任务共享配置信息 */
            sv_conf_create(&svconf_heap, &sv_conf);     /* 创建伺服配置共享内存区，用于向伺服驱动子任务共享配置信息 */

            LOGGER_DBG(DFLAG_LONG, "plc file name = %s", objname);
            load_obj(objname, io_conf, sv_conf, &plc_task);     /* 加载目标文件 */

            io_mem_create(&io_shm, io_conf, M_SHARED);          /* 创建I/O映像区 */
            io_mem_zero(&io_shm, io_conf);                      /* I/O映像区初始化 */

            rc_mem_create(rc_shm, rc_conf);                     /* 创建RC与PLC共享内存区 */
            rc_shm_servo_init(rc_shm);                          /* 对RC/PLC内存共享区进行初始化 */
            rc_syncobj_create(&rc_mutex_desc, RC_MUTEX_NAME, &rc_cond_desc, RC_COND_NAME);    /* 创建RC/PLC同步对象 */

            insert_io_driver(io_pid);                           /* 加载I/O驱动子任务 */
            insert_sv_driver(sv_pid);                           /* 加载伺服驱动子任务 */
            insert_rc_task(rc_pid);                             /* 加载RC子任务 */

            /* block until binding successfully */
            rt_task_bind(&io_task, IO_TASK_NAME, TM_INFINITE);  /* 任务描述符io_task绑定I/O驱动子任务 */
            rt_task_bind(&sv_task, SV_TASK_NAME, TM_INFINITE);  /* 任务描述符sv_task绑定伺服驱动子任务 */
            rt_task_bind(&rc_task, RC_TASK_NAME, TM_INFINITE);  /* 任务描述符rc_task绑定RC子任务 */

            rt_task_create(&test_desc, "test_task", 0, 88, T_CPU(0));
            rt_task_start(&test_desc, &test_routine, NULL);

            // plc_task_init(&plc_task);                           /* 初始化plc任务 */
            // plc_task_start(&plc_task);                          /* 启动plc任务 */
        }
    }
    return 0;
}
