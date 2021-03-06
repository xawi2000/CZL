#ifndef CZL_OPT_H
#define CZL_OPT_H

#include "czl_vm.h"

///////////////////////////////////////////////////////////////
//检查释放数组连接缓存、函数返回值: tmp buf check free
#define CZL_TB_CF(gp, res) \
if (res->quality) czl_tmp_buf_free(gp, res); \

//临时缓存扩容大小
#define CZL_TB_SZIE(pc, left, al, bl) \
(CZL_ARRLINK_VAR == left->quality && \
 (!pc || !pc->next || (pc+1)->flag != CZL_BINARY2_OPT || (pc+1)->kind != CZL_ADD_A) ? \
 al+bl+1 : (al)*2+bl+1)
///////////////////////////////////////////////////////////////
#ifdef CZL_TIMER
//检查定时器状态: check timer state
#define CZL_CTiS(gp) \
if (gp->timerEventCnt && !czl_timer_cb_fun_run(gp)) goto CZL_EXCEPTION_CATCH;
#endif //#ifdef CZL_TIMER
///////////////////////////////////////////////////////////////
#ifdef CZL_MULT_THREAD
//检查线程状态: check thread state
#define CZL_CThS(gp) \
if (!gp->thread_pipe) \
    break; \
else if (gp->thread_pipe->kill) { \
    gp->exit_code = CZL_EXIT_KILL; \
    goto CZL_EXCEPTION_CATCH; \
} \
else if (gp->thread_pipe->suspend) { \
    gp->thread_pipe->suspend = 0; \
    czl_event_wait(&gp->thread_pipe->notify_event); \
}
#endif //#ifdef CZL_MULT_THREAD
///////////////////////////////////////////////////////////////
//与运算检查跳跃: and and operator check jump
#define CZL_AA_CJ(gp, ret, pc) \
if (!CZL_EIT(ret)) { \
    pc += pc->rt; \
    CZL_TB_CF(gp, pc->res); \
    pc->res->type = CZL_INT; \
    pc->res->val.inum = 0; \
} \
++pc;

//或运算检查跳跃: or or operator check jump
#define CZL_OO_CJ(gp, ret, pc) \
if (CZL_EIT(ret)) { \
    pc += pc->rt; \
    CZL_TB_CF(gp, pc->res); \
    pc->res->type = CZL_INT; \
    pc->res->val.inum = 1; \
} \
++pc;

//执行三目运算符指令: run three opt
#define CZL_RTO(ret, pc) \
pc += (CZL_EIT(ret) ? 1 : pc->rt);

//三目运算符结果搬移: three opt res move
#define CZL_TORM(gp, lo, ret, pc) \
if (lo) { \
    CZL_TB_CF(gp, lo); \
    *lo = *ret; \
    if (CZL_ARRBUF_VAR == ret->quality) \
        ret->quality = CZL_DYNAMIC_VAR; \
    else if (CZL_FUNRET_VAR == ret->quality) \
        ret->type = CZL_NIL; \
} \
(pc+pc->rt-1)->res = ret; \
pc += pc->rt;

//检查操作数尾: check opr tail
#define CZL_COT(gp, pc) \
switch (pc->lt) \
{ \
case CZL_OPERAND: ++pc; break; \
case CZL_AND_AND: CZL_AA_CJ(gp, pc->res, pc); break; \
case CZL_OR_OR: CZL_OO_CJ(gp, pc->res, pc); break; \
case CZL_THREE_OPT: CZL_RTO(pc->res, pc); break; \
default: CZL_TORM(gp, pc->lo, pc->res, pc); break; \
}

//获取操作数: get opr res
#define CZL_GOR(gp, pc) \
if (pc->kind != CZL_REG_VAR || CZL_OBJ_REF == pc->ro->type) { \
    if (!(pc->res=czl_get_opr(gp, pc->kind, pc->ro))) {\
        if (CZL_USR_FUN == pc->kind) goto CZL_RUN_FUN; \
        else goto CZL_EXCEPTION_CATCH; \
    } \
} \
else if (pc->res != pc->ro) \
    pc->res = pc->ro;

//执行单目运算符指令: run unary opt
#define CZL_RUO(gp, pc) \
if (pc->lt != CZL_REG_VAR || CZL_OBJ_REF == pc->lo->type) { \
    if (!(pc->res=czl_get_opr(gp, pc->lt, pc->lo))) { \
        if (CZL_USR_FUN == pc->lt) goto CZL_RUN_FUN; \
        else goto CZL_EXCEPTION_CATCH; \
    } \
} \
else if (pc->res != pc->lo) \
    pc->res = pc->lo; \
if (CZL_ADD_SELF == pc->kind) \
    switch (pc->res->type) { \
    case CZL_INT: ++pc->res->val.inum; break; \
    case CZL_FLOAT: ++pc->res->val.fnum; break; \
    default: goto CZL_EXCEPTION_CATCH; \
    } \
else \
    switch (pc->res->type) { \
    case CZL_INT: --pc->res->val.inum; break; \
    case CZL_FLOAT: --pc->res->val.fnum; break; \
    default: goto CZL_EXCEPTION_CATCH; \
    } \
++pc;

//执行有临时结果的单目运算符指令: run unary2 opt
#define CZL_RU2O(gp, pc) \
if (pc->lt != CZL_REG_VAR || CZL_OBJ_REF == pc->lo->type) { \
    if (!(pc->ro=czl_get_opr(gp, pc->lt, pc->lo))) { \
        if (CZL_USR_FUN == pc->lt) goto CZL_RUN_FUN; \
        else goto CZL_EXCEPTION_CATCH; \
    } \
} \
else if (pc->ro != pc->lo) \
    pc->ro = pc->lo; \
CZL_TB_CF(gp, pc->res); \
switch (pc->kind) { \
case CZL_CONDITION: \
    pc->res->type = CZL_INT; \
    pc->res->val.inum = CZL_EIT(pc->ro); \
    break; \
case CZL_OBJ_TYPE: \
    pc->res->type = CZL_INT; \
    pc->res->val.inum = pc->ro->type; \
    break; \
default: \
    if (!czl_opt_cac_funs[pc->kind](gp, pc->res, pc->ro)) \
        goto CZL_EXCEPTION_CATCH; \
} \
++pc;

//执行有临时结果的双目运算符指令: run binary2 opt
#define CZL_RB2O(gp, lo, ro, pc) \
if (pc->lo != pc->res) \
    CZL_TB_CF(gp, pc->res); \
if (CZL_DYNAMIC_VAR == pc->res->quality && (CZL_ADD_A == pc->kind || CZL_DEC_A == pc->kind)) { \
    pc->res->quality = CZL_ARRLINK_VAR; \
    gp->add_pc = pc; \
} \
if (CZL_REG_VAR == pc->lt && pc->lo->type != CZL_OBJ_REF) { \
    lo = pc->lo; \
    if (pc->lo != pc->res) { \
        pc->res->type = lo->type; \
        pc->res->val = lo->val; \
    } \
} \
else { \
    if (!(lo=czl_get_opr(gp, pc->lt, pc->lo))) { \
        if (CZL_USR_FUN == pc->lt) goto CZL_RUN_FUN; \
        else goto CZL_EXCEPTION_CATCH; \
    } \
    pc->res->type = lo->type; \
    pc->res->val = lo->val; \
    if (CZL_FUNRET_VAR == lo->quality) { \
        pc->res->quality = CZL_FUNRET_VAR; \
        lo->type = CZL_NIL; \
    } \
} \
if (CZL_REG_VAR == pc->rt && pc->ro->type != CZL_OBJ_REF) \
    ro = pc->ro; \
else { \
    if (lo->quality != CZL_OBJ_ELE) \
        ro = czl_get_opr(gp, pc->rt, pc->ro); \
    else { \
        CZL_LOCK_OBJ(lo); \
        ro = czl_get_opr(gp, pc->rt, pc->ro); \
        CZL_UNLOCK_OBJ(lo); \
    } \
    if (!ro) { \
        if (CZL_USR_FUN == pc->rt) goto CZL_RUN_FUN; \
        else goto CZL_EXCEPTION_CATCH; \
    } \
} \
if (!czl_opt_cac_funs[pc->kind](gp, pc->res, ro)) \
    goto CZL_EXCEPTION_CATCH; \
++pc;

//执行双目运算符指令: run binary opt
#define CZL_RBO(gp, ro, pc) \
if (CZL_REG_VAR == pc->rt && pc->ro->type != CZL_OBJ_REF) \
    ro = pc->ro; \
else if (!(ro=czl_get_opr(gp, pc->rt, pc->ro))) { \
    if (CZL_USR_FUN == pc->rt) goto CZL_RUN_FUN; \
    else goto CZL_EXCEPTION_CATCH; \
} \
switch (pc->lt) { \
case CZL_REG_VAR: \
    if (CZL_OBJ_REF == pc->lo->type) pc->res = CZL_GRV(pc->lo); \
    else if (pc->res != pc->lo) pc->res = pc->lo; \
    break; \
case CZL_MEMBER: \
    if (ro->quality != CZL_OBJ_ELE) \
        pc->res = czl_get_member_res(gp, (czl_obj_member*)pc->lo); \
    else { \
        CZL_LOCK_OBJ(ro); \
        pc->res = czl_get_member_res(gp, (czl_obj_member*)pc->lo); \
        CZL_UNLOCK_OBJ(ro); \
    } \
    if (!pc->res) goto CZL_EXCEPTION_CATCH; \
    break; \
default: \
    pc->res = (CZL_OBJ_REF == ((czl_ins_var*)pc->lo)->var->type ? \
               CZL_GRV(((czl_ins_var*)pc->lo)->var) : ((czl_ins_var*)pc->lo)->var); \
    break; \
} \
if (!czl_opt_cac_funs[pc->kind](gp, pc->res, ro)) \
    goto CZL_EXCEPTION_CATCH; \
if (CZL_STR_ELE == pc->res->quality) \
    czl_set_char(gp); \
if (CZL_STR_ELE == ro->quality) \
    czl_set_char(gp); \
++pc;

//执行赋值运算符指令: run ass opt
#define CZL_RAO(gp, ro, pc) \
if (CZL_REG_VAR == pc->rt && \
    (pc->ro->type != CZL_OBJ_REF || CZL_REF_ELE == pc->ro->quality)) \
    ro = pc->ro; \
else if (!(ro=czl_get_opr(gp, pc->rt, pc->ro))) { \
    if (CZL_USR_FUN == pc->rt) goto CZL_RUN_FUN; \
    else goto CZL_EXCEPTION_CATCH; \
} \
if (CZL_MEMBER == pc->lt) { \
    gp->cur_var = ro; \
    if (ro->quality != CZL_OBJ_ELE) \
        pc->res = czl_get_member_res(gp, (czl_obj_member*)pc->lo); \
    else { \
        CZL_LOCK_OBJ(ro); \
        pc->res = czl_get_member_res(gp, (czl_obj_member*)pc->lo); \
        CZL_UNLOCK_OBJ(ro); \
    } \
    if (!pc->res) \
        goto CZL_EXCEPTION_CATCH; \
    if (CZL_CIRCLE_REF_VAR == gp->tmp_var.quality) \
        ro = &gp->tmp_var; \
} \
else if (CZL_OBJ_REF == ro->type) { \
    if (CZL_INS_VAR == pc->lt) \
        pc->res = ((czl_ins_var*)pc->lo)->var; \
    else if (pc->res != pc->lo) \
        pc->res = pc->lo; \
} \
else if (CZL_REG_VAR == pc->lt) { \
    if (CZL_OBJ_REF == pc->lo->type) pc->res = CZL_GRV(pc->lo); \
    else if (pc->res != pc->lo) pc->res = pc->lo; \
} \
else { \
    pc->res = (CZL_OBJ_REF == ((czl_ins_var*)pc->lo)->var->type ? \
               CZL_GRV(((czl_ins_var*)pc->lo)->var) : ((czl_ins_var*)pc->lo)->var); \
} \
if (!czl_ass_cac(gp, pc->res, ro)) \
    goto CZL_EXCEPTION_CATCH; \
if (CZL_STR_ELE == pc->res->quality) \
    czl_set_char(gp); \
++pc;

//执行foreach语句: run foreach sentence
#define CZL_RFS(gp, pc) \
switch (pc->kind ? \
        czl_foreach_range(gp, (czl_foreach*)pc->res, pc->lt) : \
        czl_foreach_object(gp, (czl_foreach*)pc->res, pc->lt)) \
{ \
case 1: pc = pc->pl.pc; break; \
case 0: pc = pc->next; break; \
default: goto CZL_EXCEPTION_CATCH; \
}

//执行switch语句: run switch sentence
#define CZL_RSS(gp, pc) \
CZL_TB_CF(gp, pc->res); \
*pc->res = *(pc-1)->res; \
if (CZL_FUNRET_VAR == pc->res->quality) \
    (pc-1)->res->type = CZL_NIL; \
else if (CZL_ARRBUF_VAR == pc->res->quality) \
    (pc-1)->res->quality = CZL_DYNAMIC_VAR; \
else if (CZL_STRING == pc->res->type) { \
    CZL_SRCA1((pc-1)->res->val.str); \
    pc->res->quality = CZL_ARRBUF_VAR; \
} \
++pc;

//执行case/default语句: run case sentence
#define CZL_RCS(gp, pc) pc = czl_switch_case_cmp(gp, pc);

//执行return/yeild语句: run return/yeild sentence
#define CZL_RRYS(gp, pc) \
if (pc->res && !czl_val_copy(gp, pc->res, (pc-1)->res)) goto CZL_EXCEPTION_CATCH; \
goto CZL_FUN_RETURN;

//执行try语句: run try sentence
#define CZL_RTS(gp, pc, stack, size) \
if (CZL_TRY_EXIT == pc->kind) { \
    gp->exit_flag = 1; \
    gp->exit_code = CZL_EXIT_TRY; \
    goto CZL_EXCEPTION_CATCH; \
} \
pc = pc->pl.pc; \
gp->exceptionCode = CZL_EXCEPTION_NO;
///////////////////////////////////////////////////////////////
//检查释放函数返回值: check free fun ret
#define CZL_CF_FR(gp, var) \
if (CZL_FUNRET_VAR == var->quality) { \
    czl_val_del(gp, var); \
    var->type = CZL_NIL; \
}

//检查释放函数返回值: check free fun ret
#define CZL_CF_FR2(gp, ret) \
if (ret->type != CZL_NIL) { \
    czl_val_del(gp, ret); \
    ret->type = CZL_NIL; \
}

//执行函数单目运算符指令: run fun unary opt
#define CZL_RFUO(gp, lo, pc) \
CZL_TB_CF(gp, pc->res); \
switch (pc->kind) { \
case CZL_CONDITION: \
    pc->res->type = CZL_INT; \
    pc->res->val.inum = CZL_EIT(lo); \
    break; \
case CZL_OBJ_TYPE: \
    pc->res->type = CZL_INT; \
    pc->res->val.inum = lo->type; \
    break; \
default: \
    if (!czl_opt_cac_funs[pc->kind](gp, pc->res, lo)) \
        goto CZL_EXCEPTION_CATCH; \
} \
CZL_CF_FR2(gp, lo); \
++pc;

//执行函数双目运算符指令: run fun binary opt
#define CZL_RFBO(gp, ro, pc) \
if (!czl_opt_cac_funs[pc->kind](gp, pc->res, ro)) { \
    if (CZL_STR_ELE == pc->res->quality) \
        czl_set_char(gp); \
    goto CZL_EXCEPTION_CATCH; \
} \
if (CZL_STR_ELE == pc->res->quality) \
    czl_set_char(gp); \
CZL_CF_FR2(gp, ro); \
++pc;

//执行函数带临时结果双目运算符指令: run fun binary2 opt
#define CZL_RFB2O(gp, ro, pc) \
if (!czl_opt_cac_funs[pc->kind](gp, pc->res, ro)) \
    goto CZL_EXCEPTION_CATCH; \
CZL_CF_FR(gp, ro); \
++pc;

//设置函数双目运算符操作数： set fun binary opt opr
#define CZL_SFBOO(gp, ro, pc, cur) \
ro = &cur->fun->ret; \
if (CZL_ASS_OPT == pc->flag && CZL_MEMBER == pc->lt) \
    gp->cur_var = ro; \
if (pc->lt != CZL_REG_VAR || CZL_OBJ_REF == pc->lo->type) { \
    if (!(pc->res=czl_get_opr(gp, pc->lt, pc->lo))) \
        goto CZL_EXCEPTION_CATCH; \
} \
else if (pc->res != pc->lo) \
    pc->res = pc->lo;

//设置函数带临时结果双目运算符操作数： set fun binary2 opt opr
#define CZL_SFB2OO(gp, ro, pc, cur, index) \
if (pc->lt != CZL_USR_FUN) { \
    ro = &cur->fun->ret; \
} \
else if (pc->rt != CZL_USR_FUN) { \
    if (CZL_REG_VAR == pc->rt && pc->ro->type != CZL_OBJ_REF) \
        ro = pc->ro; \
    else if (!(ro=czl_get_opr(gp, pc->rt, pc->ro))) \
        goto CZL_EXCEPTION_CATCH; \
    *pc->res = cur->fun->ret; \
    cur->fun->ret.type = CZL_NIL; \
} \
else if (CZL_FUNCTION == pc->res->type) { \
    *pc->res = cur->fun->ret; \
    cur->fun->ret.type = CZL_NIL; \
    if (!(cur->fun=czl_fun_run_prepare(gp, (czl_exp_fun*)pc->ro))) \
        goto CZL_EXCEPTION_CATCH; \
    ++index; \
    pc = (cur->fun->pc ? cur->fun->pc : cur->fun->opcode); \
    goto CZL_BEGIN; \
} \
else { \
    ro = &cur->fun->ret; \
}

//检查调整用户函数栈大小: check resize usrfun stack size
//函数调用在16层以内不会导致stack内存变动
#define CZL_CRUFSS(gp, index, size, stack) \
if (index >= 16 && index <= size>>2) { \
    czl_usrfun_stack *tmp = (czl_usrfun_stack*)CZL_STACK_REALLOC(gp, stack, \
                            (size>>1)*sizeof(czl_usrfun_stack), \
                            size*sizeof(czl_usrfun_stack)); \
    if (tmp) { \
        stack = tmp; \
        size >>= 1; \
    } \
}

//用户函数返回: usr fun return
#define CZL_UFR(gp, index, size, stack, cur, pc, ro) \
if (0 == index) { \
    CZL_STACK_FREE(gp, stack, size*sizeof(czl_usrfun_stack)); \
    if (CZL_YEILD_SENTENCE == pc->flag) gp->yeild_pc = pc + 1; \
    return 1; \
} \
CZL_CRUFSS(gp, index, size, stack); \
cur = stack + --index; \
if (CZL_RETURN_SENTENCE == pc->flag) { \
    czl_fun_local_vars_clean(gp, cur->fun); \
} \
else { \
    cur->fun->pc = pc + 1; \
    cur->fun->state = CZL_IN_IDLE; \
    czl_coroutine_paras_backup(cur->fun, (czl_var*)cur->fun->backup_vars); \
    if (cur->fun->cur_ins) \
        cur->fun->cur_ins = NULL; \
} \
pc = cur->pc; \
switch (pc->flag) \
{ \
case CZL_OPERAND: \
    ro = pc->res = &cur->fun->ret; \
    CZL_COT(gp, pc); \
    CZL_CF_FR2(gp, ro); \
    break; \
case CZL_UNARY2_OPT: \
    ro = &cur->fun->ret; \
    CZL_RFUO(gp, ro, pc); \
    break; \
case CZL_ASS_OPT: case CZL_BINARY_OPT: \
    CZL_SFBOO(gp, ro, pc, cur); \
    CZL_RFBO(gp, ro, pc); \
    break; \
default: \
    CZL_SFB2OO(gp, ro, pc, cur, index); \
    CZL_RFB2O(gp, ro, pc); \
    break; \
} \
goto CZL_BEGIN;

//执行用户函数: run usr fun
#define CZL_RUF(gp, index, size, stack, cur, pc, lo) \
if (index == size) { \
    unsigned long new_size = (size ? size<<1 : 1); \
    czl_usrfun_stack *tmp = (czl_usrfun_stack*)CZL_STACK_REALLOC(gp, stack, \
                            new_size*sizeof(czl_usrfun_stack), \
                            size*sizeof(czl_usrfun_stack)); \
    if (!tmp) \
        goto CZL_EXCEPTION_CATCH; \
    stack = tmp; \
    size = new_size; \
} \
cur = stack + index; \
cur->pc = pc; \
switch (pc->flag) \
{ \
case CZL_OPERAND: lo = pc->ro; break; \
case CZL_UNARY2_OPT: lo = pc->lo; break; \
case CZL_ASS_OPT: case CZL_BINARY_OPT: lo = pc->ro; break; \
default: \
    if (CZL_USR_FUN == pc->lt) { lo = pc->lo; pc->res->type = CZL_FUNCTION; } \
    else lo = pc->ro; \
    break; \
} \
if ((cur->fun=czl_fun_run_prepare(gp, (czl_exp_fun*)lo))) { \
    ++index; \
    pc = (cur->fun->pc ? cur->fun->pc : cur->fun->opcode); \
    goto CZL_BEGIN; \
}
///////////////////////////////////////////////////////////////
#define CZL_ADD_SELF_CAC(pc) \
if (CZL_INT == pc->lt) ++pc->res->val.inum; \
else ++pc->res->val.fnum; \
++pc;

#define CZL_DEC_SELF_CAC(pc) \
if (CZL_INT == pc->lt) --pc->res->val.inum; \
else --pc->res->val.fnum; \
++pc;

#define CZL_NUMBER_NOT_CAC(pc) \
if (CZL_INT == pc->lt) pc->res->val.inum = -pc->lo->val.inum; \
else pc->res->val.fnum = -pc->lo->val.fnum; \
++pc;

#define CZL_LOGIC_NOT_CAC(pc) \
pc->res->val.inum = !pc->lo->val.inum; \
++pc;

#define CZL_LOGIC_FLIP_CAC(pc) \
pc->res->val.inum = ~pc->lo->val.inum; \
++pc;

#define CZL_SELF_ADD_CAC(pc) \
if (CZL_INT == pc->lt) pc->res->val.inum = pc->lo->val.inum++; \
else pc->res->val.fnum = pc->lo->val.fnum++; \
++pc;

#define CZL_SELF_DEC_CAC(pc) \
if (CZL_INT == pc->lt) pc->res->val.inum = pc->lo->val.inum--; \
else pc->res->val.fnum = pc->lo->val.fnum--; \
++pc;

#define CZL_ASS_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->ro->val.inum : pc->ro->val.fnum); \
else \
    pc->res->val.fnum = (CZL_FLOAT == pc->rt ? pc->ro->val.fnum : pc->ro->val.inum); \
++pc;

#define CZL_ADD_A_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum += (CZL_INT == pc->rt ? pc->ro->val.inum : pc->ro->val.fnum); \
else \
    pc->res->val.fnum += (CZL_FLOAT == pc->rt ? pc->ro->val.fnum : pc->ro->val.inum); \
++pc;

#define CZL_DEC_A_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum -= (CZL_INT == pc->rt ? pc->ro->val.inum : pc->ro->val.fnum); \
else \
    pc->res->val.fnum -= (CZL_FLOAT == pc->rt ? pc->ro->val.fnum : pc->ro->val.inum); \
++pc;

#define CZL_MUL_A_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum *= (CZL_INT == pc->rt ? pc->ro->val.inum : pc->ro->val.fnum); \
else \
    pc->res->val.fnum *= (CZL_FLOAT == pc->rt ? pc->ro->val.fnum : pc->ro->val.inum); \
++pc;

#define CZL_DIV_A_CAC(pc) \
if (0 == pc->ro->val.inum) goto CZL_EXCEPTION_CATCH; \
if (CZL_INT == pc->lt) \
    pc->res->val.inum /= (CZL_INT == pc->rt ? pc->ro->val.inum : pc->ro->val.fnum); \
else \
    pc->res->val.fnum /= (CZL_FLOAT == pc->rt ? pc->ro->val.fnum : pc->ro->val.inum); \
++pc;

#define CZL_MOD_A_CAC(pc) \
if (0 == pc->ro->val.inum) goto CZL_EXCEPTION_CATCH; \
pc->res->val.inum %= pc->ro->val.inum; \
++pc;

#define CZL_OR_A_CAC(pc) \
pc->res->val.inum |= pc->ro->val.inum; \
lo = (pc++)->lo;

#define CZL_XOR_A_CAC(pc) \
pc->res->val.inum ^= pc->ro->val.inum; \
++pc;

#define CZL_AND_A_CAC(pc) \
pc->res->val.inum &= pc->ro->val.inum; \
++pc;

#define CZL_L_SHIFT_A_CAC(pc) \
pc->res->val.inum <<= pc->ro->val.inum; \
++pc;

#define CZL_R_SHIFT_A_CAC(pc) \
pc->res->val.inum >>= pc->ro->val.inum; \
++pc;

#define CZL_MORE_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum > pc->ro->val.inum : \
                                             pc->ro->val.inum > pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum > pc->ro->val.fnum : \
                                               pc->ro->val.fnum > pc->ro->val.inum); \
++pc;

#define CZL_MORE_EQU_CAC(pc) \
    if (CZL_INT == pc->lt) \
        pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum >= pc->ro->val.inum : \
                                                 pc->ro->val.inum >= pc->ro->val.fnum); \
    else \
        pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum >= pc->ro->val.fnum : \
                                                   pc->ro->val.fnum >= pc->ro->val.inum); \
++pc;

#define CZL_LESS_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum < pc->ro->val.inum : \
                                             pc->ro->val.inum < pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum < pc->ro->val.fnum : \
                                               pc->ro->val.fnum < pc->ro->val.inum); \
++pc;

#define CZL_LESS_EQU_CAC(pc) \
    if (CZL_INT == pc->lt) \
        pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum <= pc->ro->val.inum : \
                                                 pc->ro->val.inum <= pc->ro->val.fnum); \
    else \
        pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum <= pc->ro->val.fnum : \
                                                   pc->ro->val.fnum <= pc->ro->val.inum); \
++pc;

#define CZL_EQU_EQU_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum == pc->ro->val.inum : \
                                             pc->ro->val.inum == pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum == pc->ro->val.fnum : \
                                               pc->ro->val.fnum == pc->ro->val.inum); \
++pc;

#define CZL_NOT_EQU_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum != pc->ro->val.inum : \
                                             pc->ro->val.inum != pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum != pc->ro->val.fnum : \
                                               pc->ro->val.fnum != pc->ro->val.inum); \
++pc;

#define CZL_EQU_3_CAC(pc) \
if (pc->lt != pc->rt) pc->res->val.inum = 0; \
else if (CZL_INT == pc->lt) \
    pc->res->val.inum = pc->lo->val.inum == pc->ro->val.inum; \
else \
    pc->res->val.inum = pc->lo->val.fnum == pc->ro->val.fnum; \
++pc;

#define CZL_XOR_XOR_CAC(pc) \
pc->res->val.inum = (pc->lo->val.inum ? (pc->ro->val.inum ? 0 : 1) : \
                                        (pc->ro->val.inum ? 1 : 0)); \
++pc;

#define CZL_ADD_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum + pc->ro->val.inum : \
                                             pc->ro->val.inum + pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum + pc->ro->val.fnum : \
                                               pc->ro->val.fnum + pc->ro->val.inum); \
++pc;

#define CZL_DEC_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum - pc->ro->val.inum : \
                                             pc->ro->val.inum - pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum - pc->ro->val.fnum : \
                                               pc->ro->val.fnum - pc->ro->val.inum); \
++pc;

#define CZL_MUL_CAC(pc) \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum * pc->ro->val.inum : \
                                             pc->ro->val.inum * pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum * pc->ro->val.fnum : \
                                               pc->ro->val.fnum * pc->ro->val.inum); \
++pc;

#define CZL_DIV_CAC(pc) \
if (0 == pc->ro->val.inum) goto CZL_EXCEPTION_CATCH; \
if (CZL_INT == pc->lt) \
    pc->res->val.inum = (CZL_INT == pc->rt ? pc->lo->val.inum / pc->ro->val.inum : \
                                             pc->ro->val.inum / pc->ro->val.fnum); \
else \
    pc->res->val.inum = (CZL_FLOAT == pc->rt ? pc->lo->val.fnum / pc->ro->val.fnum : \
                                               pc->ro->val.fnum / pc->ro->val.inum); \
++pc;

#define CZL_MOD_CAC(pc) \
if (0 == pc->ro->val.inum) goto CZL_EXCEPTION_CATCH; \
pc->res->val.inum = pc->lo->val.inum % pc->ro->val.inum; \
++pc;

#define CZL_OR_CAC(pc) \
pc->res->val.inum = pc->lo->val.inum | pc->ro->val.inum; \
++pc;

#define CZL_XOR_CAC(pc) \
pc->res->val.inum = pc->lo->val.inum ^ pc->ro->val.inum; \
++pc;

#define CZL_AND_CAC(pc) \
pc->res->val.inum = pc->lo->val.inum & pc->ro->val.inum; \
++pc;

#define CZL_L_SHIFT_CAC(pc) \
pc->res->val.inum = pc->lo->val.inum << pc->ro->val.inum; \
++pc;

#define CZL_R_SHIFT_CAC(pc) \
pc->res->val.inum = pc->lo->val.inum >> pc->ro->val.inum; \
++pc;
///////////////////////////////////////////////////////////////
extern char (*const czl_opt_cac_funs[])(czl_gp*, czl_var*, czl_var*);
///////////////////////////////////////////////////////////////
char czl_add_buf_resize(czl_gp*, unsigned long);
char czl_strbuf_copy(czl_gp*, czl_var*);
///////////////////////////////////////////////////////////////
char czl_ass_cac(czl_gp*, czl_var*, czl_var*);
char czl_equ_equ_cac(czl_gp*, czl_var*, czl_var*);
///////////////////////////////////////////////////////////////
#endif //CZL_OPT_H
