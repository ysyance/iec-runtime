#ifndef __PLC_MODEL_H__
#define __PLC_MODEL_H__

#include <native/task.h>
#include "ivalue.h"
#include "istring.h"
#include "callstk.h"
#include "opcode.h"
#include "syslimit.h"

/*-----------------------------------------------------------------------------
 * Robot Configuration
 *---------------------------------------------------------------------------*/
typedef struct {
    int axis_count;
    int stub_param2;
} RobotConfig;
/*-----------------------------------------------------------------------------
 * PLC Task Model
 *---------------------------------------------------------------------------*/
typedef struct {
	char name[MAX_TASK_NAME_SIZE]; /* plc task name */
	uint8_t priority;              /* priority: 80-95 */
    uint8_t type;                  /* task type: SIGNAL | INTERVAL */
    uint8_t signal;                /* signal source: TIMER | I/O */
	uint32_t interval;             /* time interval (uint: ns) */
    uint32_t sp_size;              /* capacity of string pool(unit: Byte) */
	uint16_t cs_size;              /* capacity of calling stack(number of stack frame) */
    uint16_t pou_count;            /* Program Organization Unit: FUN | FB | PROG */
    uint16_t const_count;          /* number of constant */
    uint16_t global_count;         /* number of global variables */
	uint32_t inst_count;           /* number of instructions(code) */
} TaskDesc; /* PLC Task Descriptor */

typedef struct {
    char name[MAX_POU_NAME_SIZE]; /* POU name */
    uint8_t input_count;          /* number of input parameters */
    uint8_t inout_count;          /* number of in-out parameters */
    uint8_t output_count;         /* number of output parameters */
    uint8_t local_count;          /* number of local parameters */
    uint32_t entry;               /* POU entry address(AKA index of instruction) */
} UPOUDesc; /* User-level POU Descriptor */

typedef struct {
    TaskDesc task_desc; /* PLC task descriptor */
    StrPool strpool;    /* string pool */
    UPOUDesc *pou_desc; /* POU descriptor */
    IValue *vconst;     /* constant pool */
    IValue *vglobal;    /* global variables */
    Instruction *code;  /* task code(instruction) */
    CStack stack;       /* calling stack(constant capacity) */
    uint32_t pc;        /* program counter(AKA instruction pointer) */
} PLCTask;

typedef struct {
    uint8_t task_count; /* number of plc task */
    RT_TASK *rt_task;
    RT_TASK_INFO *rt_info;
    PLCTask *plc_task;
} TaskList;

#endif
