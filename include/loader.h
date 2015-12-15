#ifndef __LOADER_H__
#define __LOADER_H__

#include "io.h"
#include "servo.h"
#include "plcmodel.h"

void load_obj(const char *obj, IOConfig *io_config, ServoConfig *servo_config, TaskList *task_list);

static int verify_obj(FILE *fp);
static int load_io_config(FILE *fp, IOConfig *io_config);
static int load_axis_config(FILE *fp, AxisConfig *axis_config);
static int load_servo_config(FILE *fp, ServoConfig *servo_config);
static int load_task_desc(FILE *fp, TaskDesc *task_desc);
static int load_pou_desc(FILE *fp, UPOUDesc *pou_desc);
static int load_string(FILE *fp, IString *str);
static int load_value(FILE *fp, IValue *value);
static int load_plc_task(FILE *fp, PLCTask *task);
static int load_task_list(FILE *fp, TaskList *task_list);

#endif
