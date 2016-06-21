#include "vm.h"
#include "io.h"
#include "ivalue.h"
#include "opcode.h"
#include "libsys.h"
#include "logger.h"
#include "softimer.h"
#include "comanager.h"


extern TaskList plc_task;
/* program counter */
#define PC  (task->pc)
#define EOC (task->task_desc.inst_count) /* end of code */

/* instruction decoder */
#define instruction (task->code[PC])
#define opcode GET_OPCODE(instruction)
#define A   GETARG_A(instruction)
#define B   GETARG_B(instruction)
#define C   GETARG_C(instruction)
#define Bx  GETARG_Bx(instruction)
#define sAx GETARG_sAx(instruction)

/* I/O */
// TODO add remote io(modify do_**() functions about io)
#define LDI_CH(pos, size) getdch(LDI(iomem), pos, size)
#define LDO_CH(pos, size) getdch(LDO(iomem), pos, size)
#define LAI_CH(iunit, ich) getach(LAI(iomem), iunit, ich)
#define LAO_CH(iunit, ich) getach(LAO(iomem), iunit, ich)
#define RDI_CH(pos, size) getdch(RDI(iomem), pos, size)
#define RDO_CH(pos, size) getdch(RDO(iomem), pos, size)
#define RAI_CH(iunit, ich) getach(RAI(iomem), iunit, ich)
#define RAO_CH(iunit, ich) getach(RAO(iomem), iunit, ich)

#define do_ldload(reg, ldi_ch) {setvuint(reg, ldi_ch);}
#define do_ldstore(ldo, pos, size, reg) {setdch(ldo, pos, size, (reg).v.value_u);}
#define do_laload(reg, lai_ch) {setvuint(reg, lai_ch);}
#define do_lastore(lao, pos, size, reg) {setach(lao, pos, size, (reg).v.value_u);}
#define do_rdload(reg, rdi_ch) {setvuint(reg, rdi_ch);}
#define do_rdstore(rdo, pos, size, reg) {setdch(rdo, pos, size, (reg).v.value_u);}
#define do_raload(reg, rai_ch) {setvuint(reg, rai_ch);}
#define do_rastore(rao, pos, size, reg) {setach(rao, pos, size, (reg).v.value_u);}

#define do_getfield(reg, index, subindex) { reg = task->vref[index.v.value_p].at(subindex.v.value_p);}
#define do_setfield(reg, index, subindex) { task->vref[index.v.value_p].at(subindex.v.value_p) = reg;}

/* calling stack */
#define STK     (task->stack)
#define TOP     (task->stack.top)
#define CURR_SF (STK.base[TOP-1]) /* current stack frame */
#define PREV_SF (STK.base[TOP-2]) /* previous stack frame */
#define R(i)    (CURR_SF.reg[i])
#define G(i)    (task->vglobal[i])
#define PG(i)   (plc_task.plcglobal[i])
#define K(i)    (task->vconst[i])
// #define REF(i)  (task->vref).at(i)

/* system-level POU */
#define SPOU(i) (spou_desc[i].addr)
#define do_scall(reg_base, id) {SPOU(id)(reg_base);}

/* user-level POU */
#define UPOU_DESC(i)    (task->pou_desc[i])
#define UPOU_INPUTC(i)  (UPOU_DESC(i).input_count)
#define UPOU_INOUTC(i)  (UPOU_DESC(i).inout_count)
#define UPOU_OUTPUTC(i) (UPOU_DESC(i).output_count)
#define UPOU_LOCALC(i)  (UPOU_DESC(i).local_count)
#define UPOU_TYPE(i)    (UPOU_DESC(i).pou_type)
#define UPOU_INSTANCE(i) (UPOU_DESC(i).pou_instance)
#define UPOU_REGIC(i)   (UPOU_INPUTC(i) + UPOU_INOUTC(i))
#define UPOU_REGOC(i)   (UPOU_INOUTC(i) + UPOU_OUTPUTC(i))
#define UPOU_REGC(i)    (UPOU_INPUTC(i) + UPOU_INOUTC(i) + UPOU_OUTPUTC(i) + UPOU_LOCALC(i))
#define UPOU_ENTRY(i)   (UPOU_DESC(i).entry)

/**
 * A  = caller base_addr(index) of reg containing input args
 * Bx = called pou id
 */
#define do_ucall(caller_input_base, called_id) {                                \
    SFrame called_sf;                                                           \
    /* sframe, pou_id, ret_addr, regs_needed */                                 \
    sf_init(called_sf, called_id, PC+1, UPOU_REGC(called_id));                  \
    if(UPOU_TYPE(called_id) == 2) {                                             \
        fb_load(called_sf, task->vref[UPOU_INSTANCE(called_id)]);               \
    }                                                                           \
    /* called_sf, input_base, caller_sf, input_base, inputc */                  \
    sf_regcpy(called_sf, 0, CURR_SF, caller_input_base, UPOU_REGIC(called_id)); \
    cs_push(STK, called_sf);                                                    \
    PC = UPOU_ENTRY(called_id);                                                 \
}
#define do_ret(caller_input_base, called_id) {                                  \
    if(UPOU_TYPE(CURR_SF.pou) == 2) {                                           \
        fb_store(task->vref[UPOU_INSTANCE(CURR_SF.pou)], CURR_SF);              \
    } else {                                                                    \
        /* caller_sf, output_base, called_sf, output_base, outpouc */           \
        sf_regcpy(PREV_SF, CURR_SF.retreg,                                      \
                CURR_SF, 0+UPOU_INPUTC(CURR_SF.pou), UPOU_REGOC(CURR_SF.pou));  \
    }                                                                           \
    PC = CURR_SF.ret;                                                           \
    cs_pop(STK);                                                                \
}


#define do_tp(timer_index, timer_instance_index) {                              \
    if((task->vref[timer_instance_index][4].v.value_u ==0) && (task->vref[timer_instance_index][0].v.value_u != 0)){ \
        timer_manager.timer_list[timer_index].startflag = true;                 \
    }                                                                           \
    if((task->vref[timer_instance_index][0].v.value_u == 0) && timer_manager.timer_list[timer_index].arriveflag == true){  \
        timer_manager.timer_list[timer_index].startflag = false;                \
    }                                                                           \
    timer_manager.timer_list[timer_index].target_count = task->vref[timer_instance_index][2].v.value_u; \
    task->vref[timer_instance_index][3].v.value_u = timer_manager.timer_list[timer_index].cur_count;    \
    task->vref[timer_instance_index][1].v.value_u = !timer_manager.timer_list[timer_index].arriveflag;  \
    task->vref[timer_instance_index][4].v.value_u = task->vref[timer_instance_index][0].v.value_u;      \
}
#define do_ton(timer_index, timer_instance_index) {                             \
    if(task->vref[timer_instance_index][0].v.value_u != 0){                     \
        timer_manager.timer_list[timer_index].startflag = true;                 \
    } else {                                                                    \
        timer_manager.timer_list[timer_index].startflag = false;                \
    }                                                                           \
    timer_manager.timer_list[timer_index].target_count = task->vref[timer_instance_index][2].v.value_u; \
    task->vref[timer_instance_index][3].v.value_u = timer_manager.timer_list[timer_index].cur_count;    \
    task->vref[timer_instance_index][1].v.value_u = timer_manager.timer_list[timer_index].arriveflag;   \
}
#define do_tof(timer_index, timer_instance_index) {                             \
    if(task->vref[timer_instance_index][0].v.value_u == 0){                     \
        timer_manager.timer_list[timer_index].startflag = true;                 \
    } else {                                                                    \
        timer_manager.timer_list[timer_index].startflag = false;                \
    }                                                                           \
    timer_manager.timer_list[timer_index].target_count = task->vref[timer_instance_index][2].v.value_u; \
    task->vref[timer_instance_index][3].v.value_u = timer_manager.timer_list[timer_index].cur_count;    \
    task->vref[timer_instance_index][1].v.value_u = !timer_manager.timer_list[timer_index].arriveflag;  \
}

#if LEVEL_DBG <= LOGGER_LEVEL
#define dump_opcode(i) {fprintf(stderr, "%-6s: ", #i);}
#define dump_data(s, i, v) {       \
    fprintf(stderr, #s "(%d)", i); \
    dump_value("", v);             \
}
#define dump_R(i) dump_data(R, i, R(i))
#define dump_G(i) dump_data(G, i, G(i))
#define dump_PG(i) dump_data(PG, i, PG(i))
#define dump_K(i) dump_data(K, i, K(i))
/* data mov instruction */
#define dump_imov(i, arrow, src, index) { \
    dump_opcode(i);                       \
    dump_R(A);                            \
    fprintf(stderr, " " #arrow " ");      \
    dump_##src(index);                    \
    EOL;                                  \
}
/* arithmetic instructions */
#define dump_iarith(i, op) {      \
    dump_opcode(i);               \
    dump_R(A);                    \
    fprintf(stderr, " <-- ");     \
    dump_R(B);                    \
    fprintf(stderr, " " #op " "); \
    dump_R(C);                    \
    EOL;                          \
}
/* i/o instructions */
#define dump_iio(i, arrow, io) {                 \
    dump_opcode(i);                              \
    dump_R(A);                                   \
    fprintf(stderr, " " #arrow " ");             \
    fprintf(stderr, #io);                        \
    fprintf(stderr, " [%0#x]\n", (uint32_t)io##_CH(B,C)); \
}
#define dump_icmp(i, sym, cmp) {                                 \
    dump_opcode(i);                                              \
    fprintf(stderr, "{ ");                                       \
    dump_R(B);                                                   \
    fprintf(stderr, " " #sym " ");                               \
    dump_R(C);                                                   \
    fprintf(stderr, " } [%d == %d]\n", is_##cmp(R(B), R(C)), A); \
}
#define dump_jmp() {                                          \
    dump_opcode(JMP);                                         \
    fprintf(stderr, "Jump over next %d instructions\n", sAx-1); \
}
#define dump_halt() {                 \
    dump_opcode(HALT);                \
    fprintf(stderr, "End of code\n"); \
}
#define dump_scall() {                                                     \
    dump_opcode(SCALL);                                                    \
    fprintf(stderr, "Calling system-level POU(%s)\n", spou_desc[Bx].name); \
}
#define dump_ucall() {                                                       \
    dump_opcode(UCALL);                                                      \
    fprintf(stderr, "Calling user-level POU(%s) [entry: instruction(%d)]\n", \
            UPOU_DESC(Bx).name, UPOU_ENTRY(Bx));                             \
}
#define dump_ret() {                                                                 \
    dump_opcode(RET);                                                                \
    fprintf(stderr, "Returning from user-level POU(%s) [return: instruction(%d)]\n", \
            UPOU_DESC(Bx).name, CURR_SF.ret);                                        \
}
#define dump_condj(n) {                                                              \
    dump_opcode(CONDJ);                                                              \
    fprintf(stderr, "Jump over next %d instructions\n", n);                          \
}
#define dump_not() {              \
    dump_opcode(NOT);             \
    dump_R(A);                    \
    fprintf(stderr, " <-- ~");    \
    dump_R(B);                    \
    EOL;                          \
}
#define dump_lnot() {              \
    dump_opcode(LNOT);             \
    dump_R(A);                    \
    fprintf(stderr, " <-- !");    \
    dump_R(B);                    \
    EOL;                          \
}
#define dump_getfield() {         \
    dump_opcode(GETFIELD);        \
    dump_R(A);                    \
    fprintf(stderr, " <-- ");     \
    dump_R(B);                    \
    fprintf(stderr, ".");         \
    dump_R(C);                    \
    EOL;                          \
}
#define dump_setfield() {         \
    dump_opcode(SETFIELD);        \
    dump_R(B);                    \
    fprintf(stderr, ".");         \
    dump_R(C);                    \
    fprintf(stderr, " <-- ");     \
    dump_R(A);                    \
    EOL;                          \
}

#define dump_tp(timer_index, timer_instance_index) {                                    \
    dump_opcode(OP_TP);                                                                 \
    fprintf(stderr, "Timer:%d -> Instance:%d", timer_index, timer_instance_index);    \
    EOL;                                                                                \
}

#define dump_ton(timer_index, timer_instance_index) {                                   \
    dump_opcode(OP_TON);                                                                \
    fprintf(stderr, "Timer:%d -> Instance:%d", timer_index, timer_instance_index);    \
    EOL;                                                                                \
}
#define dump_tof(timer_index, timer_instance_index) {                                   \
    dump_opcode(OP_TOF);                                                                \
    fprintf(stderr, "Timer:%d -> Instance:%d", timer_index, timer_instance_index);    \
    EOL;                                                                                \
}
#else
#define dump_opcode(i)
#define dump_data(s, i, v)
#define dump_R(i)
#define dump_G(i)
#define dump_PG(i)
#define dump_K(i)
#define dump_imov(i, arrow, src, index)
#define dump_iarith(i, op)
#define dump_iio(i, arrow, io)
#define dump_icmp(i, sym, cmp)
#define dump_jmp()
#define dump_halt()
#define dump_scall()
#define dump_ucall()
#define dump_ret()
#define dump_condj(n)
#define dump_not()
#define dump_lnot()
#define dump_getfield()
#define dump_setfield()
#define dump_tp()
#define dump_ton()
#define dump_tof()
#endif

RT_EVENT comm_event_desc;
RT_HEAP g_ioconf_heap;
IOConfig *g_ioconf;
IOMem g_ioshm;
static void executor(void *plc_task_cookie) {
    PLCTask *task = (PLCTask *)plc_task_cookie;
    uint8_t temp_task_type = task->task_desc.type;
    unsigned long signal_wait_mask = 0;
    unsigned long mask_ret;
    if(temp_task_type == 1){
        signal_wait_mask = 1 << task->task_index;
        rt_event_bind(&comm_event_desc, Event_Flag_Name, TM_INFINITE);
    } else {
        rt_task_set_periodic(NULL, TM_NOW, task->task_desc.interval);
    }
    io_conf_bind(&g_ioconf_heap, &g_ioconf);
    io_mem_bind(&g_ioshm, g_ioconf);
    IOMem iomem;
    io_mem_create(&iomem, g_ioconf, M_LOCAL);
    while (1) {
        if(temp_task_type == 1){
            rt_event_clear(&comm_event_desc, signal_wait_mask, &mask_ret);
            rt_event_wait(&comm_event_desc, signal_wait_mask, &mask_ret, EV_ALL, TM_INFINITE);
        } else {
            rt_task_wait_period(NULL);
        }
        //TODO ADD LOCK!?
        io_mem_cpy(&iomem, &g_ioshm, g_ioconf);
        for (PC = 0; PC < EOC; ) {
            LOGGER_DBG(DFLAG_SHORT, "instruction[%d] = %0#10x, OpCode = %d", PC, instruction, opcode);
            switch (opcode) {
                case OP_GLOAD:  R(A) = G(Bx);  dump_imov(GLOAD, <--, G, Bx); PC++; break; /* PC++ MUST be last */
                case OP_GSTORE: G(Bx) = R(A); dump_imov(GSTORE, -->, G, Bx); PC++; break;
                case OP_KLOAD:  R(A) = K(Bx); dump_imov(KLOAD, <--, K, Bx); PC++; break;
                case OP_LDLOAD:  do_ldload(R(A), LDI_CH(B, C)); dump_iio(LDLOAD, <--, LDI); PC++; break;
                case OP_LDSTORE: do_ldstore(LDO(iomem), B, C, R(A));   dump_iio(LDSTORE, -->, LDO); PC++; break;
                case OP_LALOAD:  do_laload(R(A), LAI_CH(B, C)); dump_iio(LALOAD, <--, LAI); PC++; break;
                case OP_LASTORE: do_lastore(LAO(iomem), B, C, R(A));   dump_iio(LASTORE, -->, LAO); PC++; break;
                case OP_RDLOAD:  do_rdload(R(A), RDI_CH(B, C)); dump_iio(RDLOAD, <--, RDI); PC++; break;
                case OP_RDSTORE: do_rdstore(RDO(iomem), B, C, R(A));   dump_iio(RDSTORE, -->, RDO); PC++; break;
                case OP_RALOAD:  do_raload(R(A), RAI_CH(B, C)); dump_iio(RALOAD, <--, RAI); PC++; break;
                case OP_RASTORE: do_rastore(RAO(iomem), B, C, R(A));   dump_iio(RASTORE, -->, RAO); PC++; break;
                case OP_MOV:    R(A) = R(B); dump_imov(MOV, <--, R, B); PC++; break;
                case OP_ADD:    vadd(R(A), R(B), R(C)); dump_iarith(ADD, +); PC++; break;
                case OP_SUB:    vsub(R(A), R(B), R(C)); dump_iarith(SUB, -); PC++; break;
                case OP_MUL:    vmul(R(A), R(B), R(C)); dump_iarith(MUL, *); PC++; break;
                case OP_DIV:    vdiv(R(A), R(B), R(C)); dump_iarith(DIV, /); PC++; break;
                case OP_SHL:    vshl(R(A), R(B), R(C)); dump_iarith(SHL, <<); PC++; break;
                case OP_SHR:    vshr(R(A), R(B), R(C)); dump_iarith(SHR, >>); PC++; break;
                case OP_AND:    vand(R(A), R(B), R(C)); dump_iarith(AND, &); PC++; break;
                case OP_OR:     vor(R(A), R(B), R(C));  dump_iarith(OR, |); PC++; break;
                case OP_XOR:    vxor(R(A), R(B), R(C)); dump_iarith(XOR, ^); PC++; break;
                case OP_NOT:    vnot(R(A), R(B)); dump_not(); PC++; break;
                case OP_LAND:   vland(R(A), R(B), R(C)); dump_iarith(LAND, &&); PC++; break;
                case OP_LOR:    vlor(R(A), R(B), R(C));  dump_iarith(LOR, ||); PC++; break;
                case OP_LXOR:   vlxor(R(A), R(B), R(C)); dump_iarith(LXOR, ^^); PC++; break;
                case OP_LNOT:   vlnot(R(A), R(B)); dump_lnot(); PC++; break;
                case OP_LT:     vlt(R(A), R(B), R(C)); dump_iarith(LT, <); PC++; break;
                case OP_LE:     vle(R(A), R(B), R(C)); dump_iarith(LE, <=); PC++; break;
                case OP_GT:     vgt(R(A), R(B), R(C)); dump_iarith(GT, >); PC++; break;
                case OP_GE:     vge(R(A), R(B), R(C)); dump_iarith(GE, >=); PC++; break;
                case OP_EQ:     veq(R(A), R(B), R(C)); dump_iarith(EQ, ==); PC++; break;
                case OP_NE:     vne(R(A), R(B), R(C)); dump_iarith(NE, !=); PC++; break;
                case OP_CONDJ:  if (vuint(R(A)) == 0) {dump_condj(Bx-1); PC += Bx;} else {dump_condj(0); PC++;} break; /* MUST follow LT/LE/... */
                case OP_JMP:    dump_jmp(); PC += sAx; break;
                case OP_HALT:   dump_halt(); PC = EOC; break;
                case OP_SCALL:  dump_scall(); do_scall(&R(A), Bx); PC++; break;
                case OP_UCALL:  dump_ucall(); do_ucall(A, Bx); break;
                case OP_RET:    dump_ret(); do_ret(A, Bx); break;
                case OP_GETFIELD: do_getfield(R(A), R(B), R(C)); dump_getfield(); PC++; break;
                case OP_SETFIELD: do_setfield(R(A), R(B), R(C)); dump_setfield(); PC++; break;
                case OP_TP: dump_tp(A, Bx); do_tp(A, Bx); PC++; break;
                case OP_TON: dump_ton(A, Bx); do_ton(A, Bx); PC++; break;
                case OP_TOF: dump_tof(A, Bx); do_tof(A, Bx); PC++; break;
                case OP_PGLOAD: R(A) = PG(Bx);  dump_imov(PGLOAD, <--, PG, Bx); PC++; break;
                case OP_PGSTORE: PG(Bx) = R(A); dump_imov(PGSTORE, -->, PG, Bx); PC++; break;
                default: LOGGER_DBG(DFLAG_SHORT, "Unknown OpCode(%d)", opcode); break;
            }
        }
        EOL;
        //TODO ADD LOCK!?
        io_mem_cpy(&g_ioshm, &iomem, g_ioconf);
    }
}

#define TASK_LIST    (task_list)
#define TASK_COUNT   (TASK_LIST->task_count)
#define TASK_DESC(i) (TASK_LIST->plc_task[i].task_desc)
#define TASK_NAME(i) (TASK_DESC(i).name)
#define TASK_PRIO(i) (TASK_DESC(i).priority)


void plc_task_init(TaskList *task_list) {
    plc_timer_task_init(task_list);
    plc_comanager_task_init(task_list);
    for (int i = 0; i < TASK_COUNT; ++i) {
        if (rt_task_create(&task_list->rt_task[i], TASK_NAME(i), 0, TASK_PRIO(i), 0) < 0) {
            LOGGER_ERR(E_PLCTASK_CREATE, "(%s)", TASK_NAME(i));
        }
    }
}
void plc_task_start(TaskList *task_list) {
    plc_timer_task_start(task_list);
    plc_comanager_task_start(task_list);
    for (int i = 0; i < TASK_COUNT; ++i) {
        printf("task_list\n");
        if (rt_task_start(&task_list->rt_task[i], &executor, (void *)&task_list->plc_task[i]) < 0) {
            LOGGER_ERR(E_PLCTASK_START, "(%s)", TASK_NAME(i));
        }
    }
}
void plc_task_suspend(TaskList *task_list) {
    for (int i = 0; i < TASK_COUNT; ++i) {
        if (rt_task_suspend(&task_list->rt_task[i]) < 0) {
            LOGGER_ERR(E_PLCTASK_SUSP, "(%s)", TASK_NAME(i));
        }
    }
}
void plc_task_resume(TaskList *task_list) {
    for (int i = 0; i < TASK_COUNT; ++i) {
        if (rt_task_resume(&task_list->rt_task[i]) < 0) {
            LOGGER_ERR(E_PLCTASK_RESUME, "(%s)", TASK_NAME(i));
        }
    }
}
void plc_task_delete(TaskList *task_list) {
    io_mem_unbind(&g_ioshm, g_ioconf);
    io_conf_unbind(&g_ioconf_heap);
    for (int i = 0; i < TASK_COUNT; ++i) {
        if (rt_task_delete(&task_list->rt_task[i]) < 0) {
            LOGGER_ERR(E_PLCTASK_DELETE, "(%s)", TASK_NAME(i));
        }
    }
}

void plc_timer_task_init(TaskList *task_list){
    timer_manager.timer_count = task_list->timer_count;

    if(task_list->timer_count > 0){
        int err = 0;
        err = rt_alarm_create(&alarm_desc,"TickTimer");
        if(!err){
            err = rt_task_create(&timer_task_desc, "TimerServer", 4, 96, 0);
        }
        if(!err){
            for(int i = 0; i < task_list->timer_count; i ++){
                timer_manager.timer_list.push_back({0, 0, false, false});
            }
        }
    }
}

void plc_timer_task_start(TaskList *task_list){
    if(task_list->timer_count > 0){
        int err = rt_alarm_start(&alarm_desc, 50000, 1000000);
        if (!err)
            rt_task_start(&timer_task_desc, &softimer_server, NULL);
    }
}

void plc_comanager_task_init(TaskList *task_list){
    if(task_list->signal_set.count > 0){
        int err = rt_task_create(&communicate_task_desc, "CommuincateServer", 4, 95, 0);
    }
}

void plc_comanager_task_start(TaskList *task_list){
    if(task_list->signal_set.count > 0){
        int err = rt_task_start(&communicate_task_desc, &communicate_server, NULL);
    }
}
