#include <algorithm>

#include <ast.h>
#include "context.hpp"
#include "interpreter_code_impl.hpp"

using namespace mathvm;
using std::vector;

mathvm::Status* InterpreterCodeImpl::execute(vector<Var*>&) {
    auto main = dynamic_cast<BytecodeFunction*>(functionByName(AstFunction::top_name));
    contextHolder_.enterContext(main);
//    main->disassemble(std::cout);

    while (contextHolder_.topContext()->instructionIndex() < contextHolder_.topContext()->bytecode()->length()) {
        execute(contextHolder_.topContext()->readInstruction());
    }

    return Status::Ok();
}

void InterpreterCodeImpl::execute(Instruction instruction) {
    switch (instruction) {
        case BC_SLOAD:
            stack_.pushUInt16(contextHolder_.topContext()->readStringId());
            break;
        case BC_ILOAD:
            stack_.pushInt(contextHolder_.topContext()->readInt());
            break;
        case BC_DLOAD0:
            stack_.pushDouble(0);
            break;
        case BC_ILOAD0:
            stack_.pushInt(0);
            break;
        case BC_SLOAD0:
            stack_.pushUInt16(0);
            break;
        case BC_DLOAD1:
            stack_.pushDouble(1);
            break;
        case BC_ILOAD1:
            stack_.pushInt(1);
            break;
        case BC_DLOADM1:
            stack_.pushDouble(-1);
            break;
        case BC_ILOADM1:
            stack_.pushInt(-1);
            break;

        case BC_DLOAD:
            stack_.pushDouble(contextHolder_.topContext()->readDouble());
            break;

        case BC_LOADCTXIVAR: {
            uint16_t contextIndex = contextHolder_.topContext()->readUInt16();
            uint16_t varIndex = contextHolder_.topContext()->readUInt16();

            auto targetContext = contextHolder_.findContext(contextIndex);
            stack_.pushInt(targetContext->getVar(varIndex).intValue);
            break;
        }

        case BC_LOADIVAR: {
            uint16_t index = contextHolder_.topContext()->readUInt16();
            stack_.pushInt(contextHolder_.topContext()->getVar(index).intValue);
            break;
        }

        case BC_LOADIVAR0: {
            stack_.pushInt(contextHolder_.topContext()->getVar<0>().intValue);
            break;
        }

        case BC_LOADIVAR1: {
            stack_.pushInt(contextHolder_.topContext()->getVar<1>().intValue);
            break;
        }

        case BC_LOADIVAR2: {
            stack_.pushInt(contextHolder_.topContext()->getVar<2>().intValue);
            break;
        }

        case BC_LOADIVAR3: {
            stack_.pushInt(contextHolder_.topContext()->getVar<3>().intValue);
            break;
        }

        case BC_LOADDVAR: {
            uint16_t index = contextHolder_.topContext()->readUInt16();
            stack_.pushDouble(contextHolder_.topContext()->getVar(index).doubleValue);
            break;
        }

        case BC_LOADDVAR0: {
            stack_.pushDouble(contextHolder_.topContext()->getVar<0>().doubleValue);
            break;
        }

        case BC_LOADDVAR1: {
            stack_.pushDouble(contextHolder_.topContext()->getVar<1>().doubleValue);
            break;
        }
        case BC_LOADDVAR2: {
            stack_.pushDouble(contextHolder_.topContext()->getVar<2>().doubleValue);
            break;
        }
        case BC_LOADDVAR3: {
            stack_.pushDouble(contextHolder_.topContext()->getVar<3>().doubleValue);
            break;
        }

        case BC_STORECTXIVAR: {
            uint16_t contextIndex = contextHolder_.topContext()->readUInt16();
            uint16_t varIndex = contextHolder_.topContext()->readUInt16();

            auto targetContext = contextHolder_.findContext(contextIndex);
            targetContext->getVar(varIndex).intValue = stack_.popInt();
            break;
        }

        case BC_STOREIVAR: {
            int64_t value = stack_.popInt();

            uint16_t index = contextHolder_.topContext()->readUInt16();
            contextHolder_.topContext()->getVar(index).intValue = value;

            break;
        }

        case BC_STOREIVAR0: {
            int64_t value = stack_.popInt();
            contextHolder_.topContext()->getVar<0>().intValue = value;

            break;
        }

        case BC_STOREIVAR1: {
            int64_t value = stack_.popInt();
            contextHolder_.topContext()->getVar<1>().intValue = value;

            break;
        }

        case BC_STOREIVAR2: {
            int64_t value = stack_.popInt();
            contextHolder_.topContext()->getVar<2>().intValue = value;

            break;
        }

        case BC_STOREIVAR3: {
            int64_t value = stack_.popInt();
            contextHolder_.topContext()->getVar<3>().intValue = value;

            break;
        }

        case BC_STOREDVAR: {
            double value = stack_.popDouble();

            uint16_t index = contextHolder_.topContext()->readUInt16();
            contextHolder_.topContext()->getVar(index).doubleValue = value;

            break;
        }

        case BC_STOREDVAR0: {
            double value = stack_.popDouble();

            contextHolder_.topContext()->getVar<0>().doubleValue = value;

            break;
        }

        case BC_STOREDVAR1: {
            double value = stack_.popDouble();

            contextHolder_.topContext()->getVar<1>().doubleValue = value;

            break;
        }

        case BC_STOREDVAR2: {
            double value = stack_.popDouble();

            contextHolder_.topContext()->getVar<2>().doubleValue = value;

            break;
        }

        case BC_STOREDVAR3: {
            double value = stack_.popDouble();

            contextHolder_.topContext()->getVar<3>().doubleValue = value;

            break;
        }

        case BC_I2D: {
            auto intValue = stack_.popInt();
            stack_.pushDouble(static_cast<double>(intValue));

            break;
        }

        case BC_D2I: {
            auto intValue = stack_.popDouble();
            stack_.pushInt(static_cast<int64_t>(intValue));

            break;
        }

        case BC_SWAP: {
            Val top = stack_.popVal();
            Val bottom = stack_.popVal();

            stack_.pushVal(top);
            stack_.pushVal(bottom);
            break;
        }

        case BC_ICMP: {
            auto left = stack_.popInt();
            auto right = stack_.popInt();

            int64_t value = left == right ? 0 : (left < right ? -1 : 1);
            stack_.pushInt(value);

            break;
        }

        case BC_DCMP: {
            auto left = stack_.popDouble();
            auto right = stack_.popDouble();

            int64_t value = left > right ? 1 : (left < right ? -1 : 0);
            stack_.pushInt(value);

            break;
        }

        case BC_IAAND: {
            auto left = stack_.popInt();
            auto right = stack_.popInt();

            stack_.pushInt(left & right);

            break;
        }

        case BC_IAOR: {
            auto left = stack_.popInt();
            auto right = stack_.popInt();

            stack_.pushInt(left | right);

            break;
        }

        case BC_IAXOR: {
            auto right = stack_.popInt();
            auto left = stack_.popInt();

            stack_.pushInt(left ^ right);

            break;
        }

        case BC_INEG: {
            auto val = stack_.popInt();

            stack_.pushInt(-val);

            break;
        }

        case BC_IADD: {
            auto right = stack_.popInt();
            auto left = stack_.popInt();

            stack_.pushInt(left + right);

            break;
        }

        case BC_ISUB: {
            auto left = stack_.popInt();
            auto right = stack_.popInt();

            stack_.pushInt(left - right);

            break;
        }

        case BC_IMUL: {
            auto right = stack_.popInt();
            auto left = stack_.popInt();

            stack_.pushInt(left * right);

            break;
        }

        case BC_IDIV: {
            auto left = stack_.popInt();
            auto right = stack_.popInt();

            stack_.pushInt(left / right);

            break;
        }

        case BC_IMOD: {
            auto left = stack_.popInt();
            auto right = stack_.popInt();

            stack_.pushInt(left % right);

            break;
        }

        case BC_DNEG: {
            auto val = stack_.popDouble();

            stack_.pushDouble(-val);

            break;
        }

        case BC_DADD: {
            auto left = stack_.popDouble();
            auto right = stack_.popDouble();

            stack_.pushDouble(left + right);

            break;
        }

        case BC_DSUB: {
            auto left = stack_.popDouble();
            auto right = stack_.popDouble();

            stack_.pushDouble(left - right);

            break;
        }

        case BC_DMUL: {
            auto left = stack_.popDouble();
            auto right = stack_.popDouble();

            stack_.pushDouble(left * right);

            break;
        }

        case BC_DDIV: {
            auto left = stack_.popDouble();
            auto right = stack_.popDouble();

            stack_.pushDouble(left / right);

            break;
        }

        case BC_SPRINT: {
            uint16_t id = stack_.popUInt16();
            std::cout << constantById(id);
            break;
        }

        case BC_IPRINT: {
            int64_t value = stack_.popInt();
            std::cout << value;
            break;
        }

        case BC_DPRINT: {
            double value = stack_.popDouble();
            std::cout << value;
            break;
        }

        case BC_CALL: {
            auto bytecodeFunction = functionById(contextHolder_.topContext()->readUInt16());
            contextHolder_.enterContext(dynamic_cast<BytecodeFunction*>(bytecodeFunction));
            break;
        }

        case BC_RETURN: {
            contextHolder_.leaveContext();
            break;
        }

        case BC_JA: {
            int16_t offset = contextHolder_.topContext()->readInt16();
            contextHolder_.topContext()->moveInstructionIndexByOffset(offset);
            break;
        }

        case BC_IFICMPE: {
            auto right = stack_.popInt();
            auto left = stack_.popInt();

            int16_t offset = contextHolder_.topContext()->readInt16();
            if (left == right) {
                contextHolder_.topContext()->moveInstructionIndexByOffset(offset);
            }

            break;
        }

        case BC_STOP:
            break;
        default:
            throw std::runtime_error(
                "Unkonw instruction "s + bytecodeName(instruction, nullptr)
            );
    }
}

void VectorStack::pushVal(Val const& top) {
    stack_.emplace_back(top);
}

void VectorStack::pushInt(int64_t args) {
    stack_.emplace_back(args);
}

void VectorStack::pushDouble(double d) {
    stack_.emplace_back(d);
}

void VectorStack::pushUInt16(uint16_t id) {
    stack_.emplace_back(id);
}

double VectorStack::popDouble() {
    return popVal().doubleValue;
}

uint16_t VectorStack::popUInt16() {
    return popVal().stringId;
}

int64_t VectorStack::popInt() {
    return popVal().intValue;
}

Val VectorStack::popVal() {
    auto value = stack_.back();
    stack_.pop_back();
    return value;
}

ExecutionContext* ExecutionContextHolder::topContext() {
    return &contexts_.back();
}

void ExecutionContextHolder::enterContext(BytecodeFunction* pFunction) {
    contexts_.emplace_back(pFunction);
    auto executionContext = &contexts_.back();
    auto& contexts = scopeIdToContextMap_[executionContext->scopeId()];
    contexts.push_back(contexts_.size() - 1);
}

void ExecutionContextHolder::leaveContext() {
    scopeIdToContextMap_[contexts_.back().scopeId()].pop_back();
    contexts_.pop_back();
}

ExecutionContext* ExecutionContextHolder::findContext(uint16_t scopeId) {
    return &contexts_[scopeIdToContextMap_[scopeId].back()];
}
