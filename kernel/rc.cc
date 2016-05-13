#include "rc.h"


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
 *      首先获得同步互斥量，然后判断伺服插补队列是否为空，若为空则等待条件变量（即等待RC计算出插补值加入队列），
 *      否则，直接读取队列尾部插补值，最后更新队列尾指针，即tail值。
 * 参数：rc_shm：内存共享区指针
 *      axis_command_info：伺服电机目标位置、速度、加速度信息变量
 * 返回值：无
 */
void rc_shm_servo_read(RCMem *rc_shm, RobotInterpData *axis_command_info){
    rt_mutex_acquire(&rc_mutex_desc, TM_INFINITE);
    while(rc_shm->interp_queue.head == rc_shm->interp_queue.tail){
        rt_cond_wait(&rc_cond_desc, &rc_mutex_desc, TM_INFINITE);
    }
    int p_tail = rc_shm->interp_queue.tail;
    for(int i = 0; i < axis_command_info->size; i ++){
        axis_command_info->interp_value[i] = rc_shm->interp_queue.data[p_tail].interp_value[i];
    }
    if(rc_shm->interp_queue.tail < CIRCULAR_INTERP_QUEUE_SIZE - 1){
        rc_shm->interp_queue.tail ++;
    } else if(rc_shm->interp_queue.tail == CIRCULAR_INTERP_QUEUE_SIZE - 1){
        rc_shm->interp_queue.tail = 0;
    } else {
        LOGGER_ERR(E_INTERP_QUEUE_POINTER, "(name=tail, size=%d)", rc_shm->interp_queue.tail);
    }
    rt_mutex_release(&rc_mutex_desc);
}
