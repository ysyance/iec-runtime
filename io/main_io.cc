#include <sys/mman.h> /* required for mlockall() */
#include <sys/socket.h>
#include <netdb.h>
#include <native/task.h>
#include <native/queue.h>
#include <native/heap.h>
#include <unistd.h> /* required for pause() */
#include <signal.h>
#include <stdlib.h>

#include "io.h"
#include "mq.h"
#include "logger.h"
#undef LOGGER_LEVEL
#define LOGGER_LEVEL LEVEL_ERR

RT_TASK io_task;
RT_HEAP ioconf_heap;
IOConfig *io_conf;
IOMem io_shm;

#define HOST "223.3.48.53"
#define PORT 8888
static inline void rio_mocker(char *send_addr, int send_size, char *recv_addr, int recv_size) {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOGGER_ERR(E_SOCK_OPEN, " while connecting remote io");
    }
    if ((server = gethostbyname(HOST)) == NULL) {
        LOGGER_ERR(E_SOCK_HOST, " while connecting remote io");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(PORT);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        LOGGER_ERR(E_SOCK_CONNECT, "while connecting remote io");
    }
    if (write(sockfd, send_addr, send_size) < 0) {
        LOGGER_DBG(DFLAG_SHORT, "lost one socket data frame while sending");
    }
    if (read(sockfd, recv_addr, recv_size) < 0) {
        LOGGER_DBG(DFLAG_SHORT, "lost one socket data frame while sending");
    }
    //LOGGER_DBG(DFLAG_SHORT, "received data=%x", recv_addr[0]);
    close(sockfd);
}
static inline void ldi_mocker(IOConfig *config) {
    for (int i = 0; i < LDI_SIZE(config); i++) {
        LDI(io_shm)[i] = i % 2;
    }
}
static inline void ldio_update(IOConfig *config) {
    ldi_mocker(config);
    //rio_mocker(LDO(io_shm), LDO_SIZE(config), LDI(io_shm), LDI_SIZE(config));
#if LEVEL_DBG <= LOGGER_LEVEL
    dump_mem("LDI", LDI(io_shm), LDI_SIZE(config));
    dump_mem("LDO", LDO(io_shm), LDO_SIZE(config));
#endif
}
static inline void laio_update(IOConfig *config) {
    /*dump_mem("LAI", LAI(io_shm), LAI_SIZE(config));*/
    /*dump_mem("LAO", LAO(io_shm), LAO_SIZE(config));*/
}
static inline void rdio_update(IOConfig *config) {
    rio_mocker(RDO(io_shm), RDO_SIZE(config), RDI(io_shm), RDI_SIZE(config));
#if LEVEL_DBG <= LOGGER_LEVEL
    dump_mem("RDI", RDI(io_shm), RDI_SIZE(config));
    dump_mem("RDO", RDO(io_shm), RDO_SIZE(config));
#endif
}
static inline void raio_update(IOConfig *config) {
    /*dump_mem("RAI", RAI(io_shm), RAI_SIZE(config));*/
    /*dump_mem("RAO", RAO(io_shm), RAO_SIZE(config));*/
}
/* realtime context */
static void io_update(void *cookie) {
    /* MUST bind in realtime context */
    io_conf_bind(&ioconf_heap, &io_conf);
    io_mem_bind(&io_shm, io_conf);
    dump_io_conf(io_conf);
    LOGGER_DBG(DFLAG_LONG, "I/O Shared Memory:\n .total_size = %d\n .diu_size = %d\n .dou_size = %d\n .aiu_size = %d\n .aou_size = %d",
        IO_SHM_SIZE(io_conf), DIU_SIZE(io_conf), DOU_SIZE(io_conf), AIU_SIZE(io_conf), AOU_SIZE(io_conf));

	rt_task_set_periodic(NULL, TM_NOW, io_conf->update_interval);
	while (1) {
		rt_task_wait_period(NULL);
        if (LDI_COUNT(io_conf) != 0 || LDO_COUNT(io_conf) != 0) ldio_update(io_conf);
        if (LAI_COUNT(io_conf) != 0 || LAO_COUNT(io_conf) != 0) laio_update(io_conf);
        if (RDI_COUNT(io_conf) != 0 || RDO_COUNT(io_conf) != 0) rdio_update(io_conf);
        if (RAI_COUNT(io_conf) != 0 || RAO_COUNT(io_conf) != 0) raio_update(io_conf);
        // LOGGER_INF("STUB: Update IO data...", 0);
	}
}

void sig_handler(int signo) {
    LOGGER_DBG(DFLAG_SHORT, "IO Driver Received Signal: %d", (int)signo);
    if (signo == SIGUSR1) {
        /* ORDER SENSITIVE */
        io_mem_unbind(&io_shm, io_conf);
        io_conf_unbind(&ioconf_heap);
        if (rt_task_delete(&io_task)) {
            LOGGER_ERR(E_IOTASK_DELETE, "");
        }
        exit(0);
    }
}
/* non-realtime context */
int main() {
    mlockall(MCL_CURRENT|MCL_FUTURE);

    /* set signal handler */
    if (signal(SIGUSR1, &sig_handler) == SIG_ERR) {
        LOGGER_ERR(E_SIG_IOKILL, "");
    }

    /* start io rt task */
    if (rt_task_create(&io_task, IO_TASK_NAME, 0, IO_TASK_PRIORITY, 0) < 0) {
        LOGGER_ERR(E_IOTASK_CREATE, "");
    }
	if (rt_task_start(&io_task, &io_update, (void *)0) < 0) {
        LOGGER_ERR(E_IOTASK_START, "");
	}
    pause();

    return 0;
}
