#include "vm.h"
#include "io.h"
#include "ivalue.h"
#include "opcode.h"
#include "libsys.h"
#include "logger.h"


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

/* calling stack */
#define STK     (task->stack)
#define TOP     (task->stack.top)
#define CURR_SF (STK.base[TOP-1]) /* current stack frame */
#define PREV_SF (STK.base[TOP-2]) /* previous stack frame */
#define R(i)    (CURR_SF.reg[i])
#define G(i)    (task->vglobal[i])
#define K(i)    (task->vconst[i])

/* system-level POU */
#define SPOU(i) (spou_desc[i].addr)
#define do_scall(reg_base, id) {SPOU(id)(reg_base);}

/* user-level POU */
#define UPOU_DESC(i)    (task->pou_desc[i])
#define UPOU_INPUTC(i)  (UPOU_DESC(i).input_count)
#define UPOU_INOUTC(i)  (UPOU_DESC(i).inout_count)
#define UPOU_OUTPUTC(i) (UPOU_DESC(i).output_count)
#define UPOU_LOCALC(i)  (UPOU_DESC(i).local_count)
#define UPOU_REGIC(i)   (UPOU_INPUTC(i) + UPOU_INOUTC(i))
#define UPOU_REGOC(i)   (UPOU_INOUTC(i) + UPOU_OUTPUTC(i))
#define UPOU_REGC(i)    (UPOU_REGIC(i) + UPOU_REGOC(i))
#define UPOU_ENTRY(i)   (UPOU_DESC(i).entry)

/**
 * A  = caller base_addr(index) of reg containing input args
 * Bx = called pou id
 */
#define do_ucall(caller_input_base, called_id) {                                \
    SFrame called_sf;                                                           \
    /* sframe, pou_id, ret_addr, regs_needed */                                 \
    sf_init(called_sf, called_id, PC+1, UPOU_REGC(called_id));                  \
    /* called_sf, input_base, caller_sf, input_base, inputc */                  \
    sf_regcpy(called_sf, 0, CURR_SF, caller_input_base, UPOU_REGIC(called_id)); \
    cs_push(STK, called_sf);                                                    \
    PC = UPOU_ENTRY(called_id);                                                 \
}
#define do_ret(caller_input_base, called_id) {                         \
    /* caller_sf, output_base, called_sf, output_base, outpouc */      \
    sf_regcpy(PREV_SF, caller_input_base+UPOU_INPUTC(called_id),       \
            CURR_SF, 0+UPOU_INPUTC(called_id), UPOU_REGOC(called_id)); \
    PC = CURR_SF.ret;                                                  \
    cs_pop(STK);                                                       \
}

#if LEVEL_DBG <= LOGGER_LEVEL
#define dump_opcode(i) {fprintf(stderr, "%-6s: ", #i);}
#define dump_data(s, i, v) {       \
    fprintf(stderr, #s "(%d)", i); \
    dump_value("", v);             \
}
#define dump_R(i) dump_data(R, i, R(i))
#define dump_G(i) dump_data(G, i, G(i))
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
    dump_opcode(SCALL);                                                      \
    fprintf(stderr, "Calling user-level POU(%s) [entry: instruction(%d)]\n", \
            UPOU_DESC(Bx).name, UPOU_ENTRY(Bx));                             \
}
#define dump_ret() {                                                                 \
    dump_opcode(RET);                                                                \
    fprintf(stderr, "Returning from user-level POU(%s) [return: instruction(%d)]\n", \
            UPOU_DESC(Bx).name, CURR_SF.ret);                                        \
}
#define dump_condj(n) {                                                                 \
    dump_opcode(CONDJ);                                                                \
    fprintf(stderr, "Jump over next %d instructions\n", n); \
}
#define dump_not() {                                                                 \
    dump_opcode(NOT);                                                                \
    dump_R(A);                    \
    fprintf(stderr, " <-- ~");     \
    dump_R(B);                    \
    EOL;                          \
}
#else
#define dump_opcode(i)
#define dump_data(s, i, v)
#define dump_R(i)
#define dump_G(i)
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
#endif


RT_HEAP g_ioconf_heap;
IOConfig *g_ioconf;
IOMem g_ioshm;
static void executor(void *plc_task) {
    PLCTask *task = (PLCTask *)plc_task;
    rt_task_set_periodic(NULL, TM_NOW, task->task_desc.interval);
    io_conf_bind(&g_ioconf_heap, &g_ioconf);
    io_mem_bind(&g_ioshm, g_ioconf);
    IOMem iomem;
    io_mem_create(&iomem, g_ioconf, M_LOCAL);
    while (1) {
        rt_task_wait_period(NULL);
        //TODO ADD LOCK!?
        io_mem_cpy(&iomem, &g_ioshm, g_ioconf);
        for (PC = 0; PC < EOC; ) {
            LOGGER_DBG(DFLAG_SHORT, "instruction[%d] = %0#10x, OpCode = %d", PC, instruction, opcode);
            switch (opcode) {
                case OP_GLOAD:  R(A) = G(Bx); dump_imov(GLOAD, <--, G, Bx); PC++; break; /* PC++ MUST be last */
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
                case OP_LNOT:   vlnot(R(A), R(B)); PC++; break;
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
    for (int i = 0; i < TASK_COUNT; ++i) {
        if (rt_task_create(&task_list->rt_task[i], TASK_NAME(i), 0, TASK_PRIO(i), 0) < 0) {
            LOGGER_ERR(E_PLCTASK_CREATE, "(%s)", TASK_NAME(i));
        }
    }
}
void plc_task_start(TaskList *task_list) {
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

