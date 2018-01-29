#include "interpreter_code.h"
#include <cstring>

namespace mathvm {

InterpreterCode::InterpreterCode() {
    vars = stack<Var>();
    functions = stack<BytecodeFunction*>();
    positions = stack<uint32_t>();
    contexts = map<uint16_t, Context*>();
}

InterpreterCode::~InterpreterCode() {
    for (auto it = contexts.begin(); it != contexts.end(); ++it) {
         delete it->second;
    }
}

void InterpreterCode::addContext(Context *context) {
    contexts.insert(make_pair(context->addr(), context));
}

void InterpreterCode::interpretBytecode() {
    while (true) {
        Instruction insn = bytecode()->getInsn(position());

        if (insn == BC_STOP) {
            return;
        }
        bool jmp = false;

        switch (insn) {
        case BC_ILOAD:
            pushInt("int_constant", bytecode()->getInt64(position() + 1));
            break;
        case BC_DLOAD:
            pushDouble("double_constant", bytecode()->getDouble(position() + 1));
            break;
        case BC_SLOAD: {
            pushStr("str_constant", constantById(bytecode()->getUInt16(position() + 1)));
            break;
        }
        case BC_ILOAD0:
            pushInt("int0", 0);
            break;
        case BC_ILOAD1:
            pushInt("int1", 1);
            break;
        case BC_ILOADM1:
            pushInt("int-1", -1);
            break;
        case BC_DLOAD0:
            pushDouble("double0", 0.0);
            break;
        case BC_DLOAD1:
            pushDouble("double1", 1.0);
            break;
        case BC_DLOADM1:
            pushDouble("double-1", -1.0);
            break;
        case BC_SLOAD0:
            pushStr("str0", "");
            break;
        case BC_IPRINT:
        case BC_DPRINT:
        case BC_SPRINT:
            printVarOnTop();
            break;
        case BC_LOADCTXIVAR:
        case BC_LOADCTXDVAR:
        case BC_LOADCTXSVAR: {
            auto ctx = bytecode()->getUInt16(position() + 1);
            auto idx = bytecode()->getUInt16(position() + 3);
            auto context = getContext(ctx);
            vars.push(context->getVar(idx));
            break;
        }
        case BC_STORECTXDVAR:
        case BC_STORECTXIVAR:
        case BC_STORECTXSVAR: {
            auto ctx = bytecode()->getUInt16(position() + 1);
            auto idx = bytecode()->getUInt16(position() + 3);
            auto context = getContext(ctx);
            context->setVar(idx, vars.top());
            vars.pop();
            break;
        }
        case BC_IADD: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val1 + val2);
            break;
        }
        case BC_ISUB: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val2 - val1);
            break;
        }
        case BC_IMUL: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val1 * val2);
            break;
        }
        case BC_IDIV: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val2 / val1);
            break;
        }
        case BC_IMOD: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val1 % val2);
            break;
        }
        case BC_IAAND: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val1 & val2);
            break;
        }
        case BC_IAOR: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val1 | val2);
            break;
        }
        case BC_IAXOR: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val1 ^ val2);
            break;
        }
        case BC_DADD: {
            auto val1 = vars.top().getDoubleValue();
            vars.pop();
            auto val2 = vars.top().getDoubleValue();
            vars.pop();
            pushDouble("double_constant", val1 + val2);
            break;
        }
        case BC_DSUB: {
            auto val1 = vars.top().getDoubleValue();
            vars.pop();
            auto val2 = vars.top().getDoubleValue();
            vars.pop();
            pushDouble("double_constant", val2 - val1);
            break;
        }
        case BC_DMUL: {
            auto val1 = vars.top().getDoubleValue();
            vars.pop();
            auto val2 = vars.top().getDoubleValue();
            vars.pop();
            pushDouble("double_constant", val1 * val2);
            break;
        }
        case BC_DDIV: {
            auto val1 = vars.top().getDoubleValue();
            vars.pop();
            auto val2 = vars.top().getDoubleValue();
            vars.pop();
            pushDouble("double_constant", val2 / val1);
            break;
        }
        case BC_INEG: {
            auto val = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", -val);
            break;
        }
        case BC_DNEG: {
            auto val = vars.top().getDoubleValue();
            vars.pop();
            pushDouble("double_constant", -val);
            break;
        }
        case BC_S2I: {
            auto val = vars.top().getStringValue();
            vars.pop();
            pushInt("int_constant", strlen(val) > 0);
            break;
        }
        case BC_I2D: {
            auto val = vars.top().getIntValue();
            vars.pop();
            pushDouble("double_constant", val);
            break;
        }
        case BC_D2I: {
            auto val = vars.top().getDoubleValue();
            vars.pop();
            pushInt("int_constant", val);
            break;
        }
        case BC_SWAP: {
            auto val1 = vars.top();
            vars.pop();
            auto val2 = vars.top();
            vars.pop();
            vars.push(val1);
            vars.push(val2);
            break;
        }
        case BC_ICMP: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            pushInt("int_constant", val1 == val2 ? 0 : (val1 > val2 ? 1 : -1));
            break;
        }
        case BC_DCMP: {
            auto val1 = vars.top().getDoubleValue();
            vars.pop();
            auto val2 = vars.top().getDoubleValue();
            vars.pop();
            pushInt("int_constant", fabs(val1 - val2) < 0e-5 ? 0 : (val1 > val2 ? 1 : -1));
            break;
        }
        case BC_JA:
            position(bytecode()->getInt16(position() + 1) + 1);
            jmp = true;
            break;
        case BC_IFICMPE: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            if (val1 == val2) {
                position(bytecode()->getInt16(position() + 1) + 1);
                jmp = true;
            }
            break;
        }
        case BC_IFICMPNE: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            if (val1 == val2) {
                position(bytecode()->getInt16(position() + 1) + 1);
                jmp = true;
            }
            break;
        }
        case BC_IFICMPG: {
            auto val1 = vars.top().getIntValue();
            vars.pop();
            auto val2 = vars.top().getIntValue();
            vars.pop();
            if (val1 > val2) {
                position(bytecode()->getInt16(position() + 1) + 1);
                jmp = true;
            }
            break;
        }
        case BC_POP:
            vars.pop();
            break;
        case BC_LOADIVAR0: {
            auto val = vars.top();
            vars.push(val);
            break;
        }
        case BC_CALL: {
            auto fun = (BytecodeFunction *)functionById(bytecode()->getUInt16(position() + 1));
            functions.push(fun);
            positions.push(0);
            auto newScope = map<uint16_t, Context*>();
            for (auto it = lastContext.top().begin(); it != lastContext.top().end(); ++it) {
                if (it->second->owner() != fun) {
                    newScope.insert(make_pair(it->first, it->second));
                } else {
                    auto copyContext = new Context(*it->second);
                    newScope.insert(make_pair(it->first, copyContext));
                }
            }
            lastContext.push(newScope);
            jmp = true;
            break;
        }
        case BC_RETURN: {
            positions.pop();
            for (auto it = lastContext.top().begin(); it != lastContext.top().end(); ++it) {
                if (it->second->owner() == functions.top()) {
                    delete it->second;
                }
            }
            functions.pop();
            lastContext.pop();
            insn = BC_CALL;
            break;
        }
        default: throw InterpreterException("Invalid instruction");
        }
        incrementPosition(insn, jmp);
    }
}

Context* InterpreterCode::getContext(uint16_t ctx) {
    return lastContext.top().at(ctx);
}

void InterpreterCode::incrementPosition(Instruction insn, bool jmp) {
    static size_t sizes[] = {
#define INSN_SIZE(b, d, size) size,
            FOR_BYTECODES(INSN_SIZE)
#undef INSN_SIZE
    };

    if (!jmp) {
        position(sizes[insn]);
    }
}

void InterpreterCode::printVarOnTop() {
    Var var = vars.top();
    switch(var.type()) {
    case VT_INT:
        cout << var.getIntValue();
        break;
    case VT_DOUBLE:
        cout << var.getDoubleValue();
        break;
    case VT_STRING:
        cout << var.getStringValue();
        break;
    default:
        break;
    }
    vars.pop();
}

void InterpreterCode::pushInt(const string &name, int64_t value) {
    Var var = Var(VT_INT, name);
    var.setIntValue(value);
    vars.push(var);
}

void InterpreterCode::pushDouble(const string &name, double value) {
    Var var = Var(VT_DOUBLE, name);
    var.setDoubleValue(value);
    vars.push(var);
}

void InterpreterCode::pushStr(const string &name, const string& value) {
    Var var = Var(VT_STRING, name);
    var.setStringValue(value.c_str());
    vars.push(var);
}

}
