#include "bytecode_interpreter.h"
#include "translation_exception.h"

#include <cstdint>
#include <vector>
#include <iostream>
#include <algorithm>

namespace mathvm {

using std :: vector;
using std :: swap;
using std :: cout;
using namespace mathvm::details;

Status* ExecutableCode :: execute(vector<Var*>& vars) {
    BytecodeFunction* main = (BytecodeFunction*) functionById(0);
    _env = new Context(main);
    vector<StackValue> stack;

    while(true) {
        switch (_env->getCurrentInstruction()) {
            case BC_CALL: {
                uint16_t functionId = _env->getShort();
                BytecodeFunction* fun = (BytecodeFunction*) functionById(functionId);
                _env = new Context(fun, _env);
                break;
            }

            case BC_RETURN: {
                Context* prev = _env;
                _env = _env->parent();
                delete prev;
                break;
            }

            case BC_STOP: {
                return Status::Ok();
                break;
            }

            case BC_DLOAD: {
                stack.push_back(StackValue::saveDouble(_env->getDouble()));
                break;
            }

            case BC_ILOAD: {
                stack.push_back(StackValue::saveInt(_env->getInt()));
                break;
            }

            case BC_SLOAD: {
                stack.push_back(StackValue::saveStringRef(_env->getStringRef()));
                break;
            }

            case BC_DLOAD0: {
                stack.push_back(StackValue::saveDouble(0.0));
                break;
            }

            case BC_ILOAD0: {
                stack.push_back(StackValue::saveInt(0));
                break;
            }

            case BC_DLOAD1: {
                stack.push_back(StackValue::saveDouble(1.0));
                break;
            }

            case BC_ILOAD1: {
                stack.push_back(StackValue::saveInt(1));
                break;
            }

            case BC_DLOADM1: {
                stack.push_back(StackValue::saveDouble(-1.0));
                break;
            }

            case BC_ILOADM1: {
                stack.push_back(StackValue::saveInt(-1));
                break;
            }

            case BC_DADD: {
                double upper = stack.back().doubleVal();
                stack.pop_back();
                double lower = stack.back().doubleVal();
                stack.pop_back();
                stack.push_back(StackValue::saveDouble(lower + upper));
                break;
            }

            case BC_IADD: {
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(lower + upper));
                break;
            }

            case BC_DSUB: {
                double lower = stack.back().doubleVal();
                stack.pop_back();
                double upper = stack.back().doubleVal();
                stack.pop_back();
                stack.push_back(StackValue::saveDouble(lower - upper));
                break;
            }

            case BC_ISUB: {
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(lower - upper));
                break;
            }

            case BC_DMUL: {
                double upper = stack.back().doubleVal();
                stack.pop_back();
                double lower = stack.back().doubleVal();
                stack.pop_back();
                stack.push_back(StackValue::saveDouble(lower * upper));
                break;
            }

            case BC_IMUL: {
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(lower * upper));
                break;
            }

            case BC_DDIV: {
                double upper = stack.back().doubleVal();
                stack.pop_back();
                double lower = stack.back().doubleVal();
                stack.pop_back();
                stack.push_back(StackValue::saveDouble(upper / lower));
                break;
            }

            case BC_IDIV: {
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(upper / lower));
                break;
            }

            case BC_IMOD: {
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(upper % lower));
                break;
            }

            case BC_INEG: {
                int64_t operand = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(-operand));
                break;
            }

            case BC_DNEG: {
                double operand = stack.back().doubleVal();
                stack.pop_back();
                stack.push_back(StackValue::saveDouble(-operand));
                break;
            }

            case BC_IAOR: {
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(upper | lower));
                break;
            }

            case BC_IAAND: {
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(upper & lower));
                break;
            }

            case BC_IAXOR: {
                int64_t upper = stack.back().intVal();
                stack.pop_back();
                int64_t lower = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveInt(upper ^ lower));
                break;
            }

            case BC_IPRINT: {
                int64_t op = stack.back().intVal();
                stack.pop_back();
                cout << op;
                break;
            }

            case BC_DPRINT: {
                double op = stack.back().doubleVal();
                stack.pop_back();
                cout << op;
                break;
            }

            case BC_SPRINT: {
                uint16_t ref = stack.back().stringRef();
                cout << constantById(ref);
                break;
            }

            case BC_I2D: {
                int64_t val = stack.back().intVal();
                stack.pop_back();
                stack.push_back(StackValue::saveDouble((double)val));
                break;
            }

            case BC_D2I: {
                double val = stack.back().doubleVal();
                stack.pop_back();
                stack.push_back(StackValue::saveDouble((int64_t)val));
                break;
            }

            case BC_SWAP: {
                StackValue first = stack.back();
                stack.pop_back();
                StackValue second = stack.back();
                stack.pop_back();
                stack.push_back(first);
                stack.push_back(second);
                break;
            }

            case BC_POP: {
                stack.pop_back();
                break;
            }

            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR: {
                stack.push_back(_env->getVal());
                break;
            }

            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR: {
                stack.push_back(_env->getContextVal());
                break;
            }

            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR: {
                StackValue val = stack.back();
                _env->setVal(val);
                stack.pop_back();
                break;
            }

            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR: {
                StackValue val = stack.back();
                _env->setContextVal(val);
                stack.pop_back();
                break;
            }

            case BC_DCMP: {
                double upper = stack.back().doubleVal();
                stack.pop_back();
                double lower = stack.back().doubleVal();
                stack.pop_back();
                int64_t res = (upper == lower) ? 0 : (upper < lower ? -1 : 1);
                stack.push_back(StackValue::saveInt(res));
                break;
            }

            case BC_ICMP: {
                int64_t upper = stack.back().doubleVal();
                stack.pop_back();
                int64_t lower = stack.back().doubleVal();
                stack.pop_back();
                cout << "INT: " << upper << " " << lower << std :: endl;
                int64_t res = (upper == lower) ? 0 : (upper < lower ? -1 : 1);
                cout << "INT: " << res << std :: endl;
                stack.push_back(StackValue::saveInt(res));
                break;
            }

            case BC_JA: {
                int16_t offset = _env->getShort();
                _env->jump(offset - sizeof(int16_t));
                break;
            }

            case BC_IFICMPNE: {
                int16_t offset = _env->getShort();
                if (stack.back().intVal() != stack.at(stack.size()-2).intVal()) {
                    _env->jump(offset - sizeof(int16_t));
                }
                stack.pop_back();
                stack.pop_back();
                break;
            }

            case BC_IFICMPE: {
                int16_t offset = _env->getShort();
                if (stack.back().intVal() == stack.at(stack.size()-2).intVal()) {
                    _env->jump(offset - sizeof(int16_t));
                }
                stack.pop_back();
                stack.pop_back();
                break;
            }

            case BC_IFICMPL: {
                int16_t offset = _env->getShort();
                if (stack.back().intVal() < stack.at(stack.size()-2).intVal()) {
                    _env->jump(offset - sizeof(int16_t));
                }
                stack.pop_back();
                stack.pop_back();
                break;
            }

            case BC_IFICMPLE: {
                int16_t offset = _env->getShort();
                if (stack.back().intVal() <= stack.at(stack.size()-2).intVal()) {
                    _env->jump(offset - sizeof(int16_t));
                }
                stack.pop_back();
                stack.pop_back();
                break;
            }

            case BC_IFICMPG: {
                int16_t offset = _env->getShort();
                if (stack.back().intVal() > stack.at(stack.size()-2).intVal()) {
                    _env->jump(offset - sizeof(int16_t));
                }
                stack.pop_back();
                stack.pop_back();
                break;
            }

            case BC_IFICMPGE: {
                int16_t offset = _env->getShort();
                if (stack.back().intVal() >= stack.at(stack.size()-2).intVal()) {
                    _env->jump(offset - sizeof(int16_t));
                }
                stack.pop_back();
                stack.pop_back();
                break;
            }

            default: {
                throw TranslationException("Invalid or unknown instruction.");
            }
        }
    }

}

Bytecode* ExecutableCode :: getCurrentBytecode() {
    return _env->getCurrentBytecode();
}

}
