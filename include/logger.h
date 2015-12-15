#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define LEVEL_OFF 0
#define LEVEL_ERR 1
#define LEVEL_WRN 2
#define LEVEL_INF 3
#define LEVEL_DBG 4
#define LEVEL_ALL 5

#define LOGGER_LEVEL LEVEL_ALL

#define EOL {fprintf(stderr, "\n");}
#define CP(x) {fprintf(stderr, "check point %d\n", x);}
/*-----------------------------------------------------------------------------
 * Assert Macro (for debug version)
 *---------------------------------------------------------------------------*/
#if LOGGER_LEVEL < LEVEL_DBG
    #define NDEBUG
#endif
#include <assert.h>
/*-----------------------------------------------------------------------------
 * Debug Macro (for debug version)
 *---------------------------------------------------------------------------*/
#define DFLAG_SHORT 1
#define DFLAG_LONG  2
#if LEVEL_DBG <= LOGGER_LEVEL
    #define LOGGER_DBG(flag, ...)                                                \
    do {                                                                              \
        if (flag == DFLAG_LONG)                                                       \
            fprintf(stderr, "TRACE: " __FILE__ ":%d %s()\n", __LINE__, __FUNCTION__); \
        fprintf(stderr, __VA_ARGS__);                                            \
        fprintf(stderr, "\n");                                                        \
    } while(0)
#else
    #define LOGGER_DBG(flag, msg, ...)
#endif
/*-----------------------------------------------------------------------------
 * Error Check Macro (for released version)
 *---------------------------------------------------------------------------*/
#if LEVEL_ERR <= LOGGER_LEVEL
    #define LOGGER_ERR(code, ...)                 \
    do {                                          \
        fprintf(stderr, "TRACE: " __FILE__ ":%d %s()\n", __LINE__, __FUNCTION__); \
        fprintf(stderr, "[ERRNO %d] ", code); \
        fprintf(stderr, "%s", err_msg[code]);     \
        fprintf(stderr, __VA_ARGS__);             \
        fprintf(stderr, "...\n");                 \
        exit(-1);                                 \
    } while(0)
#else
    #define LOGGER_ERR(code, ...)
#endif
/*-----------------------------------------------------------------------------
 * Tips Information Macro (for released version)
 *---------------------------------------------------------------------------*/
#if LEVEL_INF <= LOGGER_LEVEL
    #define LOGGER_INF(msg, ...)           \
    do {                                   \
        fprintf(stderr, msg, __VA_ARGS__); \
        fprintf(stderr, "\n");             \
    } while(0)
#else
    #define LOGGER_INF(msg, ...)
#endif

/* ORDER SENSITIVE */
enum ErrorCode {
    /* Memory */
    E_OOM,
    E_BIND_SHM,
    /* Loader & Verifier - Header */
    E_PLC_FILE,
    E_SYS_TYPE,
    E_BYTE_ORDER,
    E_SYS_VERSION,
    E_SYS_MACHINE,
    /* Loader & Verifier - I/O Segment */
    E_LDI_COUNT,
    E_LDO_COUNT,
    E_LAI_COUNT,
    E_LAO_COUNT,
    E_RDI_COUNT,
    E_RDO_COUNT,
    E_RAI_COUNT,
    E_RAO_COUNT,
    E_IO_INTERVAL,
    E_LOAD_IO_CONFIG,
    /* Loader & Verifier - Servo Segment */
    E_AXIS_COUNT,
    E_AXIS_COMBINE,
    E_AXIS_ID_RANGE,
    E_AXIS_TYPE,
    E_AXIS_OPMODE,
    E_AXIS_SW,
    E_AXIS_MAX_VEL,
    E_AXIS_MAX_ACC,
    E_AXIS_MAX_DEC,
    E_AXIS_MAX_JERK,
    E_SERVO_INTERVAL,
    E_LOAD_SERVO_CONFIG,
    /* Loader & Verifier - PLC Task Segment */
    E_TASK_COUNT,
    E_TASK_PRIORITY,
    E_TASK_TYPE,
    E_TASK_SIGNAL,
    E_TASK_INTERVAL,
    E_LOAD_SP_SIZE,
    E_TASK_POU_COUNT,
    E_TASK_CONST_COUNT,
    E_TASK_GLOBAL_COUNT,
    E_LOAD_OPCODE,
    E_LOAD_CS_CAP,
    E_POU_PARAM_COUNT,
    E_LOAD_VTYPE,
    E_LOAD_STRLEN,
    E_LOAD_STRING,
    E_LOAD_TASK_DESC,
    E_LOAD_POU_DESC,
    E_LOAD_TASK_CONST,
    E_LOAD_TASK_GLOBAL,
    E_LOAD_PLC_TASK,
    /* GUI Task */
    E_GUITASK_FORK,
    E_GUITASK_EXEC,
    /* RC RT Task */
    E_RCTASK_CREATE,
    E_RCTASK_START,
    E_RCTASK_DELETE,
    /* PLC RT Task */
    E_PLCTASK_CREATE,
    E_PLCTASK_START,
    E_PLCTASK_SUSP,
    E_PLCTASK_RESUME,
    E_PLCTASK_DELETE,
    /* IO RT Task */
    E_IOTASK_FORK,
    E_IOTASK_EXEC,
    E_IOTASK_CREATE,
    E_IOTASK_START,
    E_IOTASK_DELETE,
    /* Servo RT Task */
    E_SVTASK_FORK,
    E_SVTASK_EXEC,
    E_SVTASK_CREATE,
    E_SVTASK_START,
    E_SVTASK_DELETE,
    /* String Pool */
    E_SP_INIT,
    E_SP_FULL,
    /* Calling Stack */
    E_CS_INIT,
    E_CS_FULL,
    E_CS_EMPTY,
    /* Key */
    E_KEYDEV_OPEN,
    /* RT Heap */
    E_HEAP_CREATE,
    E_HEAP_ALLOC,
    E_HEAP_BIND,
    E_HEAP_UNBIND,
    E_HEAP_DELETE,
    /* Message Queue */
    E_MQ_CREATE,
    E_MQ_ALLOC,
    E_MQ_SEND,
    E_MQ_BIND,
    E_MQ_UNBIND,
    E_MQ_DELETE,
    /* Socket */
    E_SOCK_OPEN,
    E_SOCK_HOST,
    E_SOCK_CONNECT,
    /* Signal */
    E_SIG_IOKILL,
    E_SIG_SVKILL,
    E_SIG_PLCSTART,
    E_SIG_PLCKILL,
    /* Pipe */
    E_PIPE_OPEN,
};

static const char *err_msg[] = {
    /* Memory */
    "Out of memory when ",
    "Failed to bind to shared memory. ",
    /* Loader & Verifier - Header */
    "Incorrect format of PLC object file",
    "Mismatched system type",
    "Inverse byte order of PLC object file",
    "Incompatible system version",
    "Mismatched CPU",
    /* Loader & Verifier - I/O Segment */
    "Over maximum count of local digital input module",
    "Over maximum count of local digital output module",
    "Over maximum count of local analog input module",
    "Over maximum count of local analog output module",
    "Over maximum count of remote digital input module",
    "Over maximum count of remote digital output module",
    "Over maximum count of remote analog input module",
    "Over maximum count of remote analog output module",
    "Below minimum i/o update interval",
    "Failed to load i/o configuration",
    /* Loader & Verifier - Servo Segment */
    "Over maximum count of axis",
    "Exceptional parameter: combination of axis",
    "Out of range of axis' node id",
    "Unknown type of axis",
    "Unknown operation mode of axis",
    "Exceptional sw limit of axis",
    "Exceptional maximum velocity limit of axis",
    "Exceptional maximum accelaration limit of axis",
    "Exceptional maximum decelaration limit of axis",
    "Exceptional maximum jerk limit of axis",
    "Below minimum servo update interval",
    "Failed to load servo configuration",
    /* Loader & Verifier - PLC Task Segment */
    "Over maximum count of plc task",
    "Out of range of plc task priority",
    "Unknown type of plc task",
    "Out of range of plc task signal source",
    "Below minimum interval of plc task",
    "Over maximum size of string pool",
    "Over maximum count of POU",
    "Over maximum count of constant",
    "Over maximum count of global variables",
    "Out of range of OpCode",
    "Over maximum count of stack frame",
    "Over maximum count of pou's parameters",
    "Unknown type of value",
    "Over maximum length of string",
    "Failed to load string",
    "Failed to load plc task descriptor",
    "Failed to load plc task pou descriptors",
    "Failed to load plc task constant",
    "Failed to load plc task global variables",
    "Failed to load plc task",
    /* GUI Task */
    "Failed to fork gui task(linux process)",
    "Failed to exec gui task(linux process)",
    /* RC RT Task */
    "Failed to create rc task",
    "Failed to start rc task",
    "Failed to delete rc task",
    /* PLC RT Task */
    "Failed to create plc task",
    "Failed to start plc task",
    "Failed to suspend plc task",
    "Failed to resume plc task",
    "Failed to delete plc task",
    /* IO RT Task */
    "Failed to fork io task(linux process)",
    "Failed to exec io task(linux process)",
    "Failed to create io rt task",
    "Failed to start io rt task",
    "Failed to delete io rt task",
    /* Servo RT Task */
    "Failed to fork servo task(linux process)",
    "Failed to exec servo task(linux process)",
    "Failed to create servo rt task",
    "Failed to start servo rt task",
    "Failed to delete servo rt task",
    /* String Pool */
    "Failed to initialize stirng pool",
    "String pool is full, can't add new string",
    /* Calling Stack */
    "Failed to initialize calling stack",
    "Calling stack is full, can't push new frame",
    "Calling stack is empty, can't pop frame",
    /* Key */
    "Failed to open key device driver",
    /* RT Heap */
    "Failed to create rt heap",
    "Failed to alloc rt heap",
    "Failed to bind rt heap(created somewhere else)",
    "Failed to unbind rt heap(created somewhere else)",
    "Failed to delete rt heap",
    /* Message Queue */
    "Failed to create message queue",
    "Failed to alloc message queue",
    "Failed to send message",
    "Failed to bind message queue",
    "Failed to unbind message queue",
    "Failed to delete message queue",
    /* Socket */
    "Failed to open socket",
    "No such host",
    "Failed to connect to host",
    /* Signal */
    "Failed to set 'io kill' signal handler",
    "Failed to set 'servo kill' signal handler",
    "Failed to set 'plc start' signal handler",
    "Failed to set 'plc kill' signal handler",
    /* Pipe */
    "Failed to open pipe",
};
#endif
