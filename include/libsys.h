#ifndef __LIBSYS_H__
#define __LIBSYS_H__

#include <stdint.h>
#include "ivalue.h"
#include "rc.h"
#include "servo.h"

#define MAX_SPOU_NAME_SIZE 50
typedef struct {
    char name[MAX_SPOU_NAME_SIZE];
    uint8_t output_count;
    uint8_t input_count;
    void (*addr)(IValue*);
} SPOUDesc; /* System-level POU(Library) descriptor */

/* ORDER SENSITIVE(顺序敏感) */
enum SPOU_ID {
    /******************************** 标准库函数 *******************************/
    /* 数值函数 */
    SFUN_ABS = 0,
    SFUN_SQRT,
    SFUN_LOG,
    SFUN_LN,
    SFUN_EXP,
    SFUN_SIN,
    SFUN_COS,
    SFUN_TAN,
    SFUN_ASIN,
    SFUN_ACOS,
    SFUN_ATAN,
    /* 算术函数 */
    SFUN_ADD,
    SFUN_MUL,
    SFUN_MOD,
    SFUN_EXPT,
    /* 位操作函数 */
    SFUN_AND,
    SFUN_OR,
    SFUN_XOR,
    SFUN_ROL,
    SFUN_ROR,
    /* 比较函数 */
    SFUN_LT,
    SFUN_LE,
    SFUN_GT,
    SFUN_GE,
    SFUN_EQ,
    /* 选择函数 */
    SFUN_SEL,
    SFUN_MAX,
    SFUN_MIN,
    SFUN_LIMIT,
    SFUN_MUX,
    /* 字符串函数 */
    SFUN_LEN,
    SFUN_LEFT,
    SFUN_RIGHT,
    SFUN_MID,
    SFUN_CONCAT,
    SFUN_INSERT,
    SFUN_DELETE,
    SFUN_REPLACE,
    SFUN_FIND,

    /**************************** 生产商定义库函数 ******************************/
    /* RC/PLC交互函数 */
    SFUN_INTERP_UPDATE,
    /* PLC/Servo交互函数 */
    SFUN_SERVO_INPUT_UPDATE,
    SFUN_SERVO_OUTPUT_UPDATE,
};

/* 数值函数 */
void sfun_abs(IValue *reg_base);
void sfun_sqrt(IValue *reg_base);
void sfun_log(IValue *reg_base);
void sfun_ln(IValue *reg_base);
void sfun_exp(IValue *reg_base);
void sfun_sin(IValue *reg_base);
void sfun_cos(IValue *reg_base);
void sfun_tan(IValue *reg_base);
void sfun_asin(IValue *reg_base);
void sfun_acos(IValue *reg_base);
void sfun_atan(IValue *reg_base);
/* 算术函数 */
void sfun_add(IValue *reg_base);
void sfun_mul(IValue *reg_base);
void sfun_mod(IValue *reg_base);
void sfun_expt(IValue *reg_base);
/* 位操作函数 */
void sfun_and(IValue *reg_base);
void sfun_or(IValue *reg_base);
void sfun_xor(IValue *reg_base);
void sfun_rol(IValue *reg_base);
void sfun_ror(IValue *reg_base);
/* 比较函数 */
void sfun_lt(IValue *reg_base);
void sfun_le(IValue *reg_base);
void sfun_gt(IValue *reg_base);
void sfun_ge(IValue *reg_base);
void sfun_eq(IValue *reg_base);
/* 选择函数 */
void sfun_sel(IValue *reg_base);
void sfun_max(IValue *reg_base);
void sfun_min(IValue *reg_base);
void sfun_limit(IValue *reg_base);
void sfun_mux(IValue *reg_base);
/* 字符串函数 */
void sfun_len(IValue *reg_base);
void sfun_left(IValue *reg_base);
void sfun_right(IValue *reg_base);
void sfun_mid(IValue *reg_base);
void sfun_concat(IValue *reg_base);
void sfun_insert(IValue *reg_base);
void sfun_delete(IValue *reg_base);
void sfun_replace(IValue *reg_base);
void sfun_find(IValue *reg_base);

/* RC/PLC交互函数 */
inline void sfun_interp_update(IValue *reg_base){
    rc_shm_servo_read(rc_shm, &robot_interpdata_buffer);
#if 0
    // just for debug
    for(int i = 0; i < 6; i ++){
        printf("plc axis %d, pos: %f, vel: %f, acc: %f\n", i,robot_interpdata_buffer.interp_value[i].command_pos, robot_interpdata_buffer.interp_value[i].command_vel, robot_interpdata_buffer.interp_value[i].command_acc);
    }
#endif
}

inline void sfun_servo_input_update(IValue *reg_base) {
    sv_shm_servo2plc(sv_shm, &robot_actual_info_buffer);
}

inline void sfun_servo_output_update(IValue *reg_base) {
    sv_shm_plc2servo(sv_shm, &robot_interpdata_buffer);
}


#define VARIABLE_LEN 0
/* ORDER SENSITIVE（顺序敏感） */
static const SPOUDesc spou_desc[] = {
    /* 数值函数 */
    {"SFUN_ABS", 1, 1, sfun_abs},
    {"SFUN_SQRT", 1, 1, sfun_sqrt},
    {"SFUN_LOG", 1, 1, sfun_log},
    {"SFUN_LN", 1, 1, sfun_ln},
    {"SFUN_EXP", 1, 1, sfun_exp},
    {"SFUN_SIN", 1, 1, sfun_sin},
    {"SFUN_COS", 1, 1, sfun_cos},
    {"SFUN_TAN", 1, 1, sfun_tan},
    {"SFUN_ASIN", 1, 1, sfun_asin},
    {"SFUN_ACOS", 1, 1, sfun_acos},
    {"SFUN_ATAN", 1, 1, sfun_atan},
    /* 算术函数 */
    {"SFUN_ADD", 1, VARIABLE_LEN, sfun_add},
    {"SFUN_MUL", 1, VARIABLE_LEN, sfun_mul},
    {"SFUN_MOD", 1, 2, sfun_mod},
    {"SFUN_EXPT", 1, 2, sfun_expt},
    /* 位操作函数 */
    {"SFUN_AND", 1, VARIABLE_LEN, sfun_and},
    {"SFUN_OR", 1, VARIABLE_LEN, sfun_or},
    {"SFUN_XOR", 1, VARIABLE_LEN, sfun_xor},
    {"SFUN_ROL", 1, 2, sfun_rol},
    {"SFUN_ROR", 1, 2, sfun_ror},
    /* 比较函数 */
    {"SFUN_LT", 1, VARIABLE_LEN, sfun_lt},
    {"SFUN_LE", 1, VARIABLE_LEN, sfun_le},
    {"SFUN_GT", 1, VARIABLE_LEN, sfun_gt},
    {"SFUN_GE", 1, VARIABLE_LEN, sfun_ge},
    {"SFUN_EQ", 1, VARIABLE_LEN, sfun_eq},
    /* 选择函数 */
    {"SFUN_SEL", 1, 3, sfun_sel},
    {"SFUN_MAX", 1, VARIABLE_LEN, sfun_max},
    {"SFUN_MIN", 1, VARIABLE_LEN, sfun_min},
    {"SFUN_LIMIT", 1, 3, sfun_limit},
    {"SFUN_MUX", 1, VARIABLE_LEN, sfun_mux},
    /* 字符串函数 */
    {"SFUN_LEN", 1, 1, sfun_len},
    {"SFUN_LEFT", 1, 2, sfun_left},
    {"SFUN_RIGHT", 1, 2, sfun_right},
    {"SFUN_MID", 1, 3, sfun_mid},
    {"SFUN_CONCAT", 1, VARIABLE_LEN, sfun_concat},
    {"SFUN_INSERT", 1, 3, sfun_insert},
    {"SFUN_DELETE", 1, 3, sfun_delete},
    {"SFUN_REPLACE", 1, 4, sfun_replace},
    {"SFUN_FIND", 1, 2, sfun_find},
    /* RC/PLC交互函数 */
    {"SFUN_INTERP_UPDATE", 0, 0, sfun_interp_update},
    {"SFUN_SERVO_INPUT_UPDATE", 0, 0, sfun_servo_input_update},
    {"SFUN_SERVO_OUTPUT_UPDATE", 0, 0, sfun_servo_output_update},
};

#endif
