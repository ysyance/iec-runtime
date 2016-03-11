#ifndef __CALLSTK_H__
#define __CALLSTK_H__

#include <stdint.h>
#include "ivalue.h"

typedef struct {
    uint16_t pou;   /* index of current pou descriptors */
    uint32_t ret; /* return address(AKA index of instructions) */
    uint8_t retreg;
    IValue *reg;  /* register group base address */
} SFrame; /* Stack Frame */

typedef struct {
    uint16_t capacity; /* stack capacity */
    uint16_t top;      /* top of stack(always point next stack frame) */
    SFrame *base;      /* stack base address */
} CStack; /* Calling Stack */

#define sf_init(sf, pou_id, ret_addr, regc) { \
    (sf).pou = pou_id;                          \
    (sf).ret = ret_addr;                        \
    (sf).retreg = 0;                        \
    (sf).reg = new IValue[regc];              \
}
/* sf1.reg[base1 ... base1+count] <-- sf2.reg[base2 ... base1+count] */
#define sf_regcpy(sf1, base1, sf2, base2, count) { \
    for (int i = 0; i < count; i++) {              \
        (sf1).reg[base1+i] = (sf2).reg[base2+i];   \
    }                                              \
    (sf1).retreg = base2;                          \
}
/* premise: stack capacity is enough */
#define cs_push(stk, sf) {      \
    (stk).base[(stk).top] = sf;  \
    (stk).top++;                 \
}
#define cs_pop(stk) {                     \
    delete[] (stk).base[(stk).top - 1].reg; \
    (stk).top--;                       \
}

#define fb_load(sf, fb_instance) { \
    for(int i = 0; i < fb_instance.size(); i ++){            \
        (sf).reg[i] = fb_instance[i];      \
    }                                           \
}       

#define fb_store(fb_instance, sf) {      \
    for(int i = 0; i < fb_instance.size(); i ++) {           \
        fb_instance[i] = (sf).reg[i];           \
    }                                           \
}                                        

int cs_init(CStack *stk, uint16_t cap);
//int cs_push(CStack *stk, SFrame *frame);
//int cs_pop(CStack *stk);

#endif

