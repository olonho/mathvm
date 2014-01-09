#include <string>
#include <stack>
#include <cstdlib>

#include "parser.h"

#include "codeimpl.hpp"

namespace mathvm {

size_t bc_len(Instruction insn) {
    size_t result;
    bytecodeName(insn, &result);
    return result;
}

void CodeImpl::disassemble(std::ostream& out, FunctionFilter* f) {
    Code::disassemble(out, f);
    m_bc->dump(out);
}

Status* CodeImpl::execute(std::vector<Var *> &vars) {
    try {
        stack_t stack;
        vars_t vs(1000);
        run(stack, vs);
    } catch (InterpretationError& e) {
        return new Status(e.what());
    }

    return NULL;
}

void CodeImpl::run(stack_t &stack, vars_t &vars) {
    std::vector<Bytecode*> bcs;
    std::vector<uint32_t> ips;
    bcs.push_back(m_bc);
    ips.push_back(0);
    while (!bcs.empty()) {
        uint32_t& ip = ips.back();
        Bytecode& bc = *bcs.back();
//        if (bc.length() <= ip) {
//            ips.pop_back();
//            bcs.pop_back();
//            continue;
//        }
        Instruction instruction = bc.getInsn(ip);
        LOGGER << "ip: " << ip << ", instruction: " << instruction << std::endl;
        switch (instruction) {
        case BC_ILOAD: {
            Var var(VT_INT, "");
            var.setIntValue(bc.getInt64(ip+1));
            stack.push_back(var);
            break; }
        case BC_ILOAD0: {
            Var var(VT_INT, "");
            var.setIntValue(0);
            stack.push_back(var);
            break; }
        case BC_ILOAD1: {
            Var var(VT_INT, "");
            var.setIntValue(1);
            stack.push_back(var);
            break; }
        case BC_ILOADM1: {
            Var var(VT_INT, "");
            var.setIntValue(-1);
            stack.push_back(var);
            break; }
        case BC_DLOAD: {
            Var var(VT_DOUBLE, "");
            var.setDoubleValue(bc.getDouble(ip+1));
            stack.push_back(var);
            break; }
        case BC_SLOAD: {
            Var var(VT_STRING, "");
            var.setStringValue(constantById(bc.getInt16(ip+1)).c_str());
            stack.push_back(var);
            break; }
        case BC_IADD:
            binary_op_i<PlusI>(stack);
            break;
        case BC_DADD:
            binary_op_d<PlusD>(stack);
            break;
        case BC_ISUB:
            binary_op_i<SubI>(stack);
            break;
        case BC_DSUB:
            binary_op_d<SubD>(stack);
            break;
        case BC_DMUL:
            binary_op_d<MulD>(stack);
            break;
        case BC_IMUL:
            binary_op_i<MulI>(stack);
            break;
        case BC_IDIV:
            binary_op_i<DivI>(stack);
            break;
        case BC_DDIV:
            binary_op_d<DivD>(stack);
            break;
        case BC_IAAND:
            binary_op_i<AndI>(stack);
            break;
        case BC_IAOR:
            binary_op_i<OrI>(stack);
            break;
        case BC_IAXOR:
            binary_op_i<XorI>(stack);
            break;
        case BC_INEG:
            unary_op_i<NegI>(stack);
            break;
        case BC_DNEG:
            unary_op_d<NegD>(stack);
            break;
        case BC_STOREDVAR:
        case BC_STOREIVAR:
            storevar(stack, vars, bc.getInt16(ip+1));
            break;
        case BC_STOREIVAR0:
        case BC_STOREDVAR0:
            storevar(stack, vars, 0);
            break;
        case BC_LOADIVAR:
        case BC_LOADDVAR:
            stack.push_back(vars[bc.getInt16(ip+1)].back());
            break;
        case BC_SPRINT: {
            Var v = stack.back();
            stack.pop_back();
            std::cout << v.getStringValue();
            break; }
        case BC_IPRINT: {
            Var v = stack.back();
            stack.pop_back();
            std::cout << v.getIntValue();
            break; }
        case BC_DPRINT: {
            Var v = stack.back();
            stack.pop_back();
            std::cout << v.getDoubleValue();
            break; }
        case BC_CALL: {
            TranslatedFunction* f = functionById(bc.getUInt16(ip+1));
            bcs.push_back(static_cast<BytecodeFunction*>(f)->bytecode());
            ips.push_back(0);
            break; }
        case BC_JA: {
            ip += bc.getInt16(ip + 1) + 1;
            continue;
        }
        case BC_IFICMPE: {
            if (!run_if<EqB>(stack, ip))
                break;
            ip += bc.getInt16(ip+1) + 1;
            continue; }
        case BC_IFICMPNE: {
            if (!run_if<NeqB>(stack, ip))
                break;
            ip += bc.getInt16(ip+1) + 1;
            continue;
        }
        case BC_IFICMPG: {
            if (!run_if<GtB>(stack, ip))
                break;
            ip += bc.getInt16(ip+1) + 1;
            continue;
        }
        case BC_IFICMPGE: {
            if (!run_if<GeB>(stack, ip))
                break;
            ip += bc.getInt16(ip+1) + 1;
            continue;
        }
        case BC_IFICMPL: {
            if (!run_if<LtB>(stack, ip))
                break;
            ip += bc.getInt16(ip+1) + 1;
            continue;
        }
        case BC_IFICMPLE: {
            if (!run_if<LeB>(stack, ip))
                break;
            ip += bc.getInt16(ip+1) + 1;
            continue;
        }
        case BC_RETURN: {
            ips.pop_back();
            bcs.pop_back();
            if (!ips.empty())
                ips.back() += bc_len(BC_CALL);
            continue; }
        case BC_STOP: {
            ips.clear();
            bcs.clear();
            continue;
        }
        case BC_SWAP: {
            Var v1 = stack.back();
            stack.pop_back();
            Var v2 = stack.back();
            stack.pop_back();
            stack.push_back(v1);
            stack.push_back(v2);
            break;
        }
        case BC_INVALID:
            throw InterpretationError("Invalid instruction");
        default:
            throw InterpretationError("Unknown instruction");
        }
        ip += bc_len(instruction);
    }
}

} //namespace
