#ifndef __MQ_H__
#define __MQ_H__

#include <native/queue.h>
#include <stdlib.h>
#include "logger.h"

#define MQ_SIZE 256
#define MQ_IO "io_mq"
#define MQ_SV "sv_mq"

inline void mq_create(RT_QUEUE *mq, const char *name, int size) {
    if (rt_queue_create(mq, name, size, Q_UNLIMITED, Q_FIFO) < 0) {
        LOGGER_ERR(E_MQ_CREATE, "(%s)", name);
    }
}
inline void mq_send(RT_QUEUE *mq, void *data, int size) {
    void *bufp = NULL;
    if ((bufp = rt_queue_alloc(mq, size)) == NULL) {
        LOGGER_ERR(E_MQ_ALLOC, "");
    }
    memcpy(bufp, data, size);
    if (rt_queue_send(mq, bufp, size, Q_NORMAL) < 0) {
        LOGGER_ERR(E_MQ_SEND, "");
    }
}
inline void mq_receive(RT_QUEUE *mq, void *data, int size) {
    void *bufp;
    while (rt_queue_receive(mq, &bufp, TM_INFINITE) != size); /* block */
    memcpy(data, bufp, size);
    rt_queue_free(mq, bufp);
}
inline void mq_bind(RT_QUEUE *mq, const char *name) {
    if (rt_queue_bind(mq, name, TM_INFINITE) < 0) { /* block */
        LOGGER_ERR(E_MQ_BIND, "");
    }
}
inline void mq_unbind(RT_QUEUE *mq) {
    if (rt_queue_unbind(mq) < 0) {
        LOGGER_ERR(E_MQ_UNBIND, "");
    }
}
inline void mq_delete(RT_QUEUE *mq) {
    if (rt_queue_delete(mq) < 0) {
        LOGGER_ERR(E_MQ_DELETE, "");
    }
}
#endif
