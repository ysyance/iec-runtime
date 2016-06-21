#include "rc.h"


RobotInterpData robot_interpdata_buffer;                /* 机器人插补数据缓存，用于存放从共享内存区读到的插补数据，处理后写入伺服映像区 */
RobotAxisActualInfo robot_actual_info_buffer;           /* 机器人实际轴位置信息缓存，由伺服映像区加载得到，处理后写入RC共享内存 */

/**
 * 函数名：rc_shm_io_write
 * 功能：PLC向共享内存数据区写入IO数据；
 *      待实现
 * 参数：rc_shm：内存共享区指针
 *
 * 返回值：无
 */
void rc_shm_io_write(RCMem *rc_shm, void *cookie){
    rt_mutex_acquire(&rc_mutex_desc, TM_INFINITE);
    /* TODO */
    rt_mutex_release(&rc_mutex_desc);
}

/**
 * 函数名：rc_shm_io_read
 * 功能：PLC从共享内存数据区读出IO数据；
 *      待实现
 * 参数：rc_shm：内存共享区指针
 *
 * 返回值：无
 */
void rc_shm_io_read(RCMem *rc_shm, void *cookie){
    rt_mutex_acquire(&rc_mutex_desc, TM_INFINITE);
    /* TODO */
    rt_mutex_release(&rc_mutex_desc);
}

/**
 * 函数名：rc_shm_servo_init
 * 功能：对RC/PLC内存共享区伺服段进行初始化。
 * 参数：rc_shm：内存共享区指针
 * 返回值：无
 */
void rc_shm_servo_init(RCMem *rc_shm){
    /* 伺服实际位置信息初始化 */
    rc_shm->actual_info.size = ROBOT_AXIS_COUNT;
    for(int i = 0; i < ROBOT_AXIS_COUNT; i ++){
        rc_shm->actual_info.axis_info[i].actual_pos = 0.0;
        rc_shm->actual_info.axis_info[i].actual_vel = 0.0;
        rc_shm->actual_info.axis_info[i].actual_acc = 0.0;
    }
    /* 伺服命令位置信息初始化 */
    rc_shm->interp_queue.queue_size = CIRCULAR_INTERP_QUEUE_SIZE;
    rc_shm->interp_queue.head = 0;
    rc_shm->interp_queue.tail = 0;
    for(int i = 0; i < CIRCULAR_INTERP_QUEUE_SIZE; i ++){
        rc_shm->interp_queue.data[i].size = ROBOT_AXIS_COUNT;
        for(int j = 0; j < ROBOT_AXIS_COUNT; j ++){
            rc_shm->interp_queue.data[i].interp_value[j].command_pos = 0.0;
            rc_shm->interp_queue.data[i].interp_value[j].command_vel = 0.0;
            rc_shm->interp_queue.data[i].interp_value[j].command_acc = 0.0;
        }
    }
}

/**
 * 函数名：rc_shm_servo_write
 * 功能：PLC向共享内存数据区写入伺服电机实际位置、速度、加速度信息；
 *      首先获得同步互斥量，然后写入数据，最后释放同步互斥量。
 * 参数：rc_shm：内存共享区指针
 *      axis_actual_info：伺服电机实际位置、速度、加速度信息变量
 * 返回值：无
 */
void rc_shm_servo_write(RCMem *rc_shm, RobotAxisActualInfo *axis_actual_info){
    rt_mutex_acquire(&rc_mutex_desc, TM_INFINITE);
    for(int i =0; i < axis_actual_info->size; i ++){
        rc_shm->actual_info.axis_info[i] = axis_actual_info->axis_info[i];
    }
    rt_mutex_release(&rc_mutex_desc);
}

/**
 * 函数名：rc_shm_servo_read
 * 功能：PLC从共享内存数据区读出伺服电机目标位置、速度、加速度信息;
 *      首先获得同步互斥量，然后(1)判断伺服插补队列是否已满，若队列已满则直接读取队列尾部插补值，最后更新队列尾指针，即tail值，并
 *      向RC发送信号告知其等待的条件变量已满足；(2)判断伺服插补队列是否为空，若为空则等待条件变量（即等待RC计算出插补值加入队列），
 *      否则，直接读取队列尾部插补值，最后更新队列尾指针，即tail值。
 * 参数：rc_shm：内存共享区指针
 *      axis_command_info：伺服电机目标位置、速度、加速度信息变量
 * 返回值：无
 */
void rc_shm_servo_read(RCMem *rc_shm, RobotInterpData *axis_command_info){
    rt_mutex_acquire(&rc_mutex_desc, TM_INFINITE);      /* 获得同步互斥量 */
    /* 判断伺服插补队列是否已满 */
    if((rc_shm->interp_queue.tail == rc_shm->interp_queue.head + 1)
        || (rc_shm->interp_queue.tail == 0 && rc_shm->interp_queue.head == CIRCULAR_INTERP_QUEUE_SIZE - 1)){
            int p_tail = rc_shm->interp_queue.tail;
            printf("p_tail: %d\n", p_tail);
            for(int i = 0; i < ROBOT_AXIS_COUNT; i ++){      /* 从队列中读取插补值 */
                axis_command_info->interp_value[i].command_pos = rc_shm->interp_queue.data[p_tail].interp_value[i].command_pos;
                axis_command_info->interp_value[i].command_vel = rc_shm->interp_queue.data[p_tail].interp_value[i].command_vel;
                axis_command_info->interp_value[i].command_acc = rc_shm->interp_queue.data[p_tail].interp_value[i].command_acc;
            }
            /* 更新队列尾指针 */
            if(rc_shm->interp_queue.tail < CIRCULAR_INTERP_QUEUE_SIZE - 1){
                rc_shm->interp_queue.tail ++;
            } else if(rc_shm->interp_queue.tail == CIRCULAR_INTERP_QUEUE_SIZE - 1){
                rc_shm->interp_queue.tail = 0;
            } else {
                LOGGER_ERR(E_INTERP_QUEUE_POINTER, "(name=tail, size=%d)", rc_shm->interp_queue.tail);
            }
            rt_cond_signal(&rc_cond_desc);     /* 向RC发送信号告知其等待的条件变量已满足 */
    } else {
        while(rc_shm->interp_queue.head == rc_shm->interp_queue.tail){
            rt_cond_wait(&rc_cond_desc, &rc_mutex_desc, TM_INFINITE);   /* 等待RC计算出插补值加入队列 */
        }
        int p_tail = rc_shm->interp_queue.tail;
        printf("p_tail: %d\n", p_tail);
        for(int i = 0; i < ROBOT_AXIS_COUNT; i ++){      /* 从队列中读取插补值 */
            // axis_command_info->interp_value[i] = rc_shm->interp_queue.data[p_tail].interp_value[i];
            axis_command_info->interp_value[i].command_pos = rc_shm->interp_queue.data[p_tail].interp_value[i].command_pos;
            axis_command_info->interp_value[i].command_vel = rc_shm->interp_queue.data[p_tail].interp_value[i].command_vel;
            axis_command_info->interp_value[i].command_acc = rc_shm->interp_queue.data[p_tail].interp_value[i].command_acc;
        }
        /* 更新队列尾指针 */
        if(rc_shm->interp_queue.tail < CIRCULAR_INTERP_QUEUE_SIZE - 1){
            rc_shm->interp_queue.tail ++;
        } else if(rc_shm->interp_queue.tail == CIRCULAR_INTERP_QUEUE_SIZE - 1){
            rc_shm->interp_queue.tail = 0;
        } else {
            LOGGER_ERR(E_INTERP_QUEUE_POINTER, "(name=tail, size=%d)", rc_shm->interp_queue.tail);
        }
    }
    rt_mutex_release(&rc_mutex_desc);           /* 释放同步互斥量 */
}
