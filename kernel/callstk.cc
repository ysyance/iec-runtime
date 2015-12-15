#include "callstk.h"
#include "syslimit.h"
#include "logger.h"

int cs_init(CStack *stk, uint16_t cap) {
    assert(stk != NULL);
    assert(cap <= MAX_CS_CAP); /* guaranteed by verifier */

    stk->capacity = cap+1;
    stk->top = 1; /* MUST be 1(reduce bound case for main()) */
    stk->base = new SFrame[stk->capacity];
    if (stk->base == NULL) {
        LOGGER_ERR(E_OOM, "initializing calling stack");
    }
    return 0;
}
//int cs_push(CStack *stk, SFrame *frame) {
    //assert(stk != NULL);
    //assert(frame != NULL);

    //if (stk->top == stk->capacity) {
        //LOGGER_ERR(E_CS_FULL, ""); //TODO expand capacity automatically
    //}
    //stk->base[stk->top] = *frame;
    //stk->top++;
    //return 0;
//}
//int cs_pop(CStack *stk) {
    //assert(stk != NULL);

    //if (stk->top == 0) {
        //LOGGER_ERR(E_CS_EMPTY, "");
    //}
    //if (stk->base[stk->top].reg_base != NULL) {
        //delete[] stk->base[stk->top-1].reg_base;
    //}
    //stk->top--;
    //return 0;
//}
