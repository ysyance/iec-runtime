#include <sys/mman.h> /* required for mlockall() */
#include <sys/socket.h>
#include <netdb.h>
#include <native/task.h>
#include <native/heap.h>
#include <unistd.h> /* required for pause() */
#include <signal.h>
#include <stdlib.h>

#include "mq.h"
#include "servo.h"
#include "logger.h"
#undef LOGGER_LEVEL
#define LOGGER_LEVEL LEVEL_ERR

RT_TASK sv_task;			/* 伺服任务描述符 */
RT_HEAP sv_heap_desc;		/* 伺服映像数据共享区描述符 */
RT_HEAP svconf_heap;		/* 伺服配置共享区描述符 */

RT_MUTEX sv_mutex_desc;		/* 伺服共享区同步对象描述符 */

ServoConfig *sv_conf;		/* 伺服配置共享区指针 */
SVMem *sv_shm;				/* 伺服映像数据共享区指针 */

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

#define HOST "223.3.37.140"
#define PORT 8888

int sockfd;
struct sockaddr_in serv_addr;

static inline void servo_mocker_init(){

	struct hostent *server;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		LOGGER_ERR(E_SOCK_OPEN, " while connecting ethercat ...");
	}
	if((server = gethostbyname(HOST)) == NULL){
		LOGGER_ERR(E_SOCK_HOST, " while connecting ethercat ...");
	}
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PORT);

	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		LOGGER_ERR(E_SOCK_CONNECT, " while connecting ethercat ...");
	} else {
		printf("EtheCAT has connected !\n");
	}
}

static inline void servo_mocker(SVMem *sv_shm, ServoConfig *sv_conf){
	double send_addr[6];
	char ch[120] = {0};
	rt_mutex_acquire(&sv_mutex_desc, TM_INFINITE);      /* 获得同步互斥量 */
	for(int i = 0; i < 6; i ++){
		send_addr[i] = sv_shm->axis_data[i].command_pos;

		// sprintf(&ch[i*20], "%f", send_addr[i]);
		// if(i != 5)
		// 	ch[(i+1)*20-1] = ':';
		// printf("%s ", ch[i*20]);
	}
	rt_mutex_release(&sv_mutex_desc);           		/* 释放同步互斥量 */
	// printf("\n");

	if(write(sockfd, (char*)send_addr, 8 * 6) < 0){
		LOGGER_DBG(DFLAG_SHORT, "lost one socket data frame while sending");
	}
	// if(write(sockfd, ch, 120) < 0){
	// 	LOGGER_DBG(DFLAG_SHORT, "lost one socket data frame while sending");
	// }
}

/* realtime context */
static void servo_update(void *cookie) {

    sv_conf_bind(&svconf_heap, &sv_conf);	/* 绑定伺服配置共享区 */
    dump_servo_conf((*sv_conf));
	sv_mem_bind(sv_shm, sv_conf);           /* 绑定伺服数据共享区 */
	sv_syncobj_bind(&sv_mutex_desc, SV_MUTEX_NAME); /* 绑定同步对象 */

	servo_mocker_init(); 					/* 初始化伺服模拟器，主要用来建立TCP连接 */

    int i = 0;
	rt_task_set_periodic(NULL, TM_NOW, sv_conf->update_interval);
	while (1) {
		rt_task_wait_period(NULL);
		servo_mocker(sv_shm, sv_conf);
        if(i ++ == 1000)
            LOGGER_INF("STUB: Update servo data...", 0);
	}
}

void sig_handler(int signo) {
    LOGGER_DBG(DFLAG_SHORT, "Servo Driver Received Signal: %d", (int)signo);
    if (signo == SIGUSR1) {
        if (rt_heap_delete(&sv_heap_desc) < 0) {
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
