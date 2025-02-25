#ifndef _MERRY_EXEC_
#define _MERRY_EXEC_

#include "../../utils/merry_logger.h"
#include "merry_request.h"
#include "imp/merry_imp.h"

struct MerryCore;

// The function structure
#define _exec_(name) void merry_execute_##name(struct MerryCore *core)
// some instructions that need full definition
#define _lexec_(name, ...) void merry_execute_##name(struct MerryCore *core, __VA_ARGS__)

#define _sign_extend8_(val) val | 0xFFFFFFFFFFFFFF00
#define _sign_extend16_(val) val | 0xFFFFFFFFFFFF0000
#define _sign_extend32_(val) val | 0xFFFFFFFFFF000000

#define _clear_(f) core->flag.f = 0
#define _fclear_(f) c->flag.f = 0

#define _LowerTopReg_(current) (current >> 48) & 15
#define _UpperTopReg_(current) (current >> 52) & 15
#define _LowerUpReg_(current) (current >> 4) & 15
#define _LowerDownReg_(current) (current & 15)
#define _Lower4byteImm_(current) (current) & 0xFFFFFFFF

#define _ArithMeticImmFrame_(sign)                                              \
    register mqword_t current = core->current_inst;                             \
    register mqword_t reg = _LowerTopReg_(current);                             \
    core->registers[reg] = core->registers[reg] sign(_Lower4byteImm_(current)); \
    _update_flags_(&core->flag);

#define _SArithMeticImmFrame_(sign)                                                                                   \
    register mqword_t current = core->current_inst;                                                                   \
    register mqword_t reg = _LowerTopReg_(current);                                                                   \
    core->registers[reg] = (msqword_t)core->registers[reg] sign(msqword_t) _sign_extend32_(_Lower4byteImm_(current)); \
    _update_flags_(&core->flag);

#define _ArithMeticRegFrame_(sign)                                                             \
    register mqword_t current = core->current_inst;                                            \
    register mqword_t reg = _LowerUpReg_(current);                                             \
    core->registers[reg] = core->registers[reg] sign core->registers[_LowerDownReg_(current)]; \
    _update_flags_(&core->flag);

#define _SArithMeticRegFrame_(sign)                                                                                  \
    register mqword_t current = core->current_inst;                                                                  \
    register mqword_t reg = _LowerUpReg_(current);                                                                   \
    core->registers[reg] = (msqword_t)core->registers[reg] sign(msqword_t) core->registers[_LowerDownReg_(current)]; \
    _update_flags_(&core->flag);

// execute the halt instruction
// _exec_(halt);

// arithmetic instructions
_exec_(add_imm);
_exec_(add_reg);
_exec_(sub_imm);
_exec_(sub_reg);
_exec_(mul_imm);
_exec_(mul_reg);
_exec_(div_imm);
_exec_(div_reg);
_exec_(mod_imm);
_exec_(mod_reg);

_exec_(iadd_imm);
_exec_(iadd_reg);
_exec_(isub_imm);
_exec_(isub_reg);
_exec_(imul_imm);
_exec_(imul_reg);
_exec_(idiv_imm);
_exec_(idiv_reg);
_exec_(imod_imm);
_exec_(imod_reg);

_exec_(fadd);
_exec_(fsub);
_exec_(fmul);
_exec_(fdiv);

_exec_(fadd32);
_exec_(fsub32);
_exec_(fmul32);
_exec_(fdiv32);

// move instructions
// _exec_(move_imm);
// _lexec_(move_imm64, mqword_t imm);
// _exec_(move_reg);
// _exec_(move_reg8);
// _exec_(move_reg16);
// _exec_(move_reg32);

_exec_(movesx_reg8);
_exec_(movesx_reg16);
_exec_(movesx_reg32);
_exec_(movesx_imm8);
_exec_(movesx_imm16);
_exec_(movesx_imm32);

// control flow instructions
_exec_(call);
_exec_(ret);
_exec_(sva);
_exec_(svc);

// stack based instructions
_exec_(push_imm);
_exec_(push_reg);
_exec_(pop);
_exec_(pusha);
_exec_(popa);

// logical instructions
// All AND, OR, XOR and CMP take 64 bits values that should follow the instruction in memory
// _lexec_(and_imm, mqword_t imm);
// _exec_(and_reg);
// _lexec_(or_imm, mqword_t imm);
// _exec_(or_reg);
// _lexec_(xor_imm, mqword_t imm);
// _exec_(xor_reg);
// _exec_(not );
// _exec_(lshift);
// _exec_(rshift);
// _exec_(cmp_imm);
// _exec_(cmp_reg);

// some extra instructions
// _exec_(inc);
// _exec_(dec);

// data movement instructions
// _exec_(lea);
_lexec_(load, mqword_t address);
_lexec_(store, mqword_t address);
_lexec_(loadb, mqword_t address);
_lexec_(storeb, mqword_t address);
_lexec_(loadw, mqword_t address);
_lexec_(storew, mqword_t address);
_lexec_(loadd, mqword_t address);
_lexec_(stored, mqword_t address);
_lexec_(load_reg, mqword_t address);
_lexec_(store_reg, mqword_t address);
_lexec_(loadb_reg, mqword_t address);
_lexec_(storeb_reg, mqword_t address);
_lexec_(loadw_reg, mqword_t address);
_lexec_(storew_reg, mqword_t address);
_lexec_(loadd_reg, mqword_t address);
_lexec_(stored_reg, mqword_t address);

_exec_(excg);
_exec_(excg8);
_exec_(excg16);
_exec_(excg32);

// _exec_(mov8);
// _exec_(mov16);
// _exec_(mov32);

// utility instructions

// _exec_(cflags);
// _exec_(reset);
// _exec_(clz);
// _exec_(cln);
// _exec_(clc);
// _exec_(clo);

// contidional jumps
// _lexec_(jnz, mqword_t address);
// _lexec_(jz, mqword_t address);

#endif