#include <algorithm>

#include <ast.h>
#include "context.hpp"
#include "interpreter_code_impl.hpp"

using namespace mathvm;
using std::vector;

struct VectorStack {
    void pushInt(int64_t args);
    void pushDouble(double d);
    void pushUInt16(uint16_t id);
    void pushVal(Val const& top);
    Val popVal();
    int64_t popInt();
    double popDouble();
    uint16_t popUInt16();
private:
    std::vector<Val> stack_;
};

struct StringManager {
    explicit StringManager(Code const* code): code_(code) {}

    uint16_t addNativeString(char const* str) {
        nativeStrings_.push_back(str);
        return uint16_t(nativeStrings_.size() - 1) + LIMIT;
    }

    char const * getById(uint16_t id) {
        if (id < LIMIT) {
            return code_->constantById(id).c_str();
        } else {
            return nativeStrings_[id - LIMIT];
        }
    }

private:
    static uint16_t const LIMIT = std::numeric_limits<uint16_t>::max() / 2;

    Code const* code_;
    std::vector<char const*> nativeStrings_;
};

struct ExecutionContextHolder {
    ExecutionContextHolder(uint16_t numberOfContexts) : contextsById_(numberOfContexts) {}
    ExecutionContext* topContext();
    void enterContext(BytecodeFunction* pFunction);
    void leaveContext();

    ExecutionContext* findContext(uint16_t scopeId);

private:
    std::vector<ExecutionContext> contexts_;
    std::vector<std::vector<size_t>> contextsById_;
};

ExecutionContext* ExecutionContextHolder::topContext() {
    return &contexts_.back();
}

void ExecutionContextHolder::enterContext(BytecodeFunction* pFunction) {
    contexts_.emplace_back(pFunction);
    contextsById_[topContext()->scopeId()].push_back(contexts_.size() - 1);
}

void ExecutionContextHolder::leaveContext() {
    contextsById_[topContext()->scopeId()].pop_back();
    contexts_.pop_back();
}

ExecutionContext* ExecutionContextHolder::findContext(uint16_t scopeId) {
    return &contexts_[contextsById_[scopeId].back()];
}

mathvm::Status* InterpreterCodeImpl::execute(vector<Var*>&) {
    auto main = dynamic_cast<BytecodeFunction*>(functionByName(AstFunction::top_name));

    uint16_t numberOfFunctions{0};
    {
        Code::FunctionIterator it(this);
        utils::iterate(it, [&numberOfFunctions](auto) {
            numberOfFunctions++;
        });
    }

    ExecutionContextHolder contextHolder(numberOfFunctions);
    StringManager stringManager(this);
    VectorStack stack;

    contextHolder.enterContext(main);
//    main->disassemble(std::cout);

    while (true) {
        auto instruction = contextHolder.topContext()->readInstruction();
        switch (instruction) {
            case BC_SLOAD:
                stack.pushUInt16(contextHolder.topContext()->readStringId());
                break;
            case BC_ILOAD:
                stack.pushInt(contextHolder.topContext()->readInt());
                break;
            case BC_DLOAD0:
                stack.pushDouble(0);
                break;
            case BC_ILOAD0:
                stack.pushInt(0);
                break;
            case BC_SLOAD0:
                stack.pushUInt16(0);
                break;
            case BC_DLOAD1:
                stack.pushDouble(1);
                break;
            case BC_ILOAD1:
                stack.pushInt(1);
                break;
            case BC_DLOADM1:
                stack.pushDouble(-1);
                break;
            case BC_ILOADM1:
                stack.pushInt(-1);
                break;

            case BC_DLOAD:
                stack.pushDouble(contextHolder.topContext()->readDouble());
                break;

            case BC_LOADCTXDVAR: {
                uint16_t contextIndex = contextHolder.topContext()->readUInt16();
                uint16_t varIndex = contextHolder.topContext()->readUInt16();

                auto targetContext = contextHolder.findContext(contextIndex);
                stack.pushDouble(targetContext->getVar(varIndex).doubleValue);
                break;
            }

            case BC_LOADCTXIVAR: {
                uint16_t contextIndex = contextHolder.topContext()->readUInt16();
                uint16_t varIndex = contextHolder.topContext()->readUInt16();

                auto targetContext = contextHolder.findContext(contextIndex);
                stack.pushInt(targetContext->getVar(varIndex).intValue);
                break;
            }

            case BC_LOADIVAR: {
                uint16_t index = contextHolder.topContext()->readUInt16();
                stack.pushInt(contextHolder.topContext()->getVar(index).intValue);
                break;
            }

            case BC_LOADIVAR0: {
                stack.pushInt(contextHolder.topContext()->getVar<0>().intValue);
                break;
            }

            case BC_LOADIVAR1: {
                stack.pushInt(contextHolder.topContext()->getVar<1>().intValue);
                break;
            }

            case BC_LOADIVAR2: {
                stack.pushInt(contextHolder.topContext()->getVar<2>().intValue);
                break;
            }

            case BC_LOADIVAR3: {
                stack.pushInt(contextHolder.topContext()->getVar<3>().intValue);
                break;
            }

            case BC_LOADSVAR0: {
                stack.pushUInt16(contextHolder.topContext()->getVar<0>().stringId);
                break;
            }

            case BC_LOADSVAR1: {
                stack.pushUInt16(contextHolder.topContext()->getVar<1>().stringId);
                break;
            }

            case BC_LOADSVAR2: {
                stack.pushUInt16(contextHolder.topContext()->getVar<2>().stringId);
                break;
            }

            case BC_LOADSVAR3: {
                stack.pushUInt16(contextHolder.topContext()->getVar<3>().stringId);
                break;
            }

            case BC_LOADDVAR: {
                uint16_t index = contextHolder.topContext()->readUInt16();
                stack.pushDouble(contextHolder.topContext()->getVar(index).doubleValue);
                break;
            }

            case BC_LOADDVAR0: {
                stack.pushDouble(contextHolder.topContext()->getVar<0>().doubleValue);
                break;
            }

            case BC_LOADDVAR1: {
                stack.pushDouble(contextHolder.topContext()->getVar<1>().doubleValue);
                break;
            }
            case BC_LOADDVAR2: {
                stack.pushDouble(contextHolder.topContext()->getVar<2>().doubleValue);
                break;
            }
            case BC_LOADDVAR3: {
                stack.pushDouble(contextHolder.topContext()->getVar<3>().doubleValue);
                break;
            }

            case BC_STORECTXDVAR: {
                uint16_t contextIndex = contextHolder.topContext()->readUInt16();
                uint16_t varIndex = contextHolder.topContext()->readUInt16();

                auto targetContext = contextHolder.findContext(contextIndex);
                targetContext->getVar(varIndex).doubleValue = stack.popDouble();
                break;
            }

            case BC_STORECTXIVAR: {
                uint16_t contextIndex = contextHolder.topContext()->readUInt16();
                uint16_t varIndex = contextHolder.topContext()->readUInt16();

                auto targetContext = contextHolder.findContext(contextIndex);
                targetContext->getVar(varIndex).intValue = stack.popInt();
                break;
            }

            case BC_STOREIVAR: {
                int64_t value = stack.popInt();

                uint16_t index = contextHolder.topContext()->readUInt16();
                contextHolder.topContext()->getVar(index).intValue = value;

                break;
            }

            case BC_STOREIVAR0: {
                int64_t value = stack.popInt();
                contextHolder.topContext()->getVar<0>().intValue = value;

                break;
            }

            case BC_STOREIVAR1: {
                int64_t value = stack.popInt();
                contextHolder.topContext()->getVar<1>().intValue = value;

                break;
            }

            case BC_STOREIVAR2: {
                int64_t value = stack.popInt();
                contextHolder.topContext()->getVar<2>().intValue = value;

                break;
            }

            case BC_STOREIVAR3: {
                int64_t value = stack.popInt();
                contextHolder.topContext()->getVar<3>().intValue = value;

                break;
            }

            case BC_STOREDVAR: {
                double value = stack.popDouble();

                uint16_t index = contextHolder.topContext()->readUInt16();
                contextHolder.topContext()->getVar(index).doubleValue = value;

                break;
            }

            case BC_STORESVAR: {
                uint16_t value = stack.popUInt16();

                uint16_t index = contextHolder.topContext()->readUInt16();
                contextHolder.topContext()->getVar(index).stringId = value;

                break;
            }

            case BC_STOREDVAR0: {
                double value = stack.popDouble();

                contextHolder.topContext()->getVar<0>().doubleValue = value;

                break;
            }

            case BC_STOREDVAR1: {
                double value = stack.popDouble();

                contextHolder.topContext()->getVar<1>().doubleValue = value;

                break;
            }

            case BC_STOREDVAR2: {
                double value = stack.popDouble();

                contextHolder.topContext()->getVar<2>().doubleValue = value;

                break;
            }

            case BC_STOREDVAR3: {
                double value = stack.popDouble();

                contextHolder.topContext()->getVar<3>().doubleValue = value;

                break;
            }


            case BC_STORESVAR0: {
                auto value = stack.popUInt16();
                contextHolder.topContext()->getVar<0>().stringId = value;
                break;
            }

            case BC_STORESVAR1: {
                auto value = stack.popUInt16();
                contextHolder.topContext()->getVar<1>().stringId = value;
                break;
            }

            case BC_STORESVAR2: {
                auto value = stack.popUInt16();
                contextHolder.topContext()->getVar<2>().stringId = value;
                break;
            }

            case BC_STORESVAR3: {
                auto value = stack.popUInt16();
                contextHolder.topContext()->getVar<3>().stringId = value;
                break;
            }

            case BC_I2D: {
                auto intValue = stack.popInt();
                stack.pushDouble(static_cast<double>(intValue));

                break;
            }

            case BC_D2I: {
                auto intValue = stack.popDouble();
                stack.pushInt(static_cast<int64_t>(intValue));

                break;
            }

            case BC_SWAP: {
                Val top = stack.popVal();
                Val bottom = stack.popVal();

                stack.pushVal(top);
                stack.pushVal(bottom);
                break;
            }

            case BC_POP: {
                stack.popVal();
                break;
            }

            case BC_ICMP: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                int64_t value = upper == lower ? 0 : (upper < lower ? -1 : 1);
                stack.pushInt(value);

                break;
            }

            case BC_DCMP: {
                auto upper = stack.popDouble();
                auto lower = stack.popDouble();

                int64_t value = upper > lower ? 1 : (upper < lower ? -1 : 0);
                stack.pushInt(value);

                break;
            }

            case BC_IAAND: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                stack.pushInt(upper & lower);

                break;
            }

            case BC_IAOR: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                stack.pushInt(upper | lower);

                break;
            }

            case BC_IAXOR: {
                auto lower = stack.popInt();
                auto upper = stack.popInt();

                stack.pushInt(upper ^ lower);

                break;
            }

            case BC_INEG: {
                auto val = stack.popInt();

                stack.pushInt(-val);

                break;
            }

            case BC_IADD: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                stack.pushInt(upper + lower);

                break;
            }

            case BC_ISUB: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                stack.pushInt(upper - lower);

                break;
            }

            case BC_IMUL: {
                auto lower = stack.popInt();
                auto upper = stack.popInt();

                stack.pushInt(upper * lower);

                break;
            }

            case BC_IDIV: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                stack.pushInt(upper / lower);

                break;
            }

            case BC_IMOD: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                stack.pushInt(upper % lower);

                break;
            }

            case BC_DNEG: {
                auto val = stack.popDouble();

                stack.pushDouble(-val);

                break;
            }

            case BC_DADD: {
                auto upper = stack.popDouble();
                auto lower = stack.popDouble();

                stack.pushDouble(upper + lower);

                break;
            }

            case BC_DSUB: {
                auto upper = stack.popDouble();
                auto lower = stack.popDouble();

                stack.pushDouble(upper - lower);

                break;
            }

            case BC_DMUL: {
                auto upper = stack.popDouble();
                auto lower = stack.popDouble();

                stack.pushDouble(upper * lower);

                break;
            }

            case BC_DDIV: {
                auto upper = stack.popDouble();
                auto lower = stack.popDouble();

                stack.pushDouble(upper / lower);

                break;
            }

            case BC_SPRINT: {
                uint16_t id = stack.popUInt16();
                std::cout << stringManager.getById(id);
                break;
            }

            case BC_IPRINT: {
                int64_t value = stack.popInt();
                std::cout << value;
                break;
            }

            case BC_DPRINT: {
                double value = stack.popDouble();
                std::cout << value;
                break;
            }

            case BC_CALL: {
                auto bytecodeFunction = functionById(contextHolder.topContext()->readUInt16());
                contextHolder.enterContext(dynamic_cast<BytecodeFunction*>(bytecodeFunction));
                break;
            }

            case BC_CALLNATIVE: {
                Signature const* signature;
                string const* name;
                NativeVal result;
                auto nativeFunctionHandle = NativeFunction(nativeById(contextHolder.topContext()->readUInt16(), &signature, &name));

                auto parametersNumber = signature->size() - 1;
                auto returnType = (*signature)[0].first;
                std::vector<NativeVal> params;

                params.reserve(parametersNumber);
                for (uint16_t i = 0; i < parametersNumber; i++) {
                    auto param = contextHolder.topContext()->getVar(i);
                    switch ((*signature)[i + 1].first) {
                        case VT_STRING: {
                            auto string = stringManager.getById(param.stringId);
                            params.emplace_back(string);
                            break;
                        }

                        case VT_INT: {
                            auto string = param.intValue;
                            params.emplace_back(string);
                            break;
                        }

                        case VT_DOUBLE: {
                            auto val = param.doubleValue;
                            params.emplace_back(val);
                            break;
                        }

                        default:
                            throw runtime_error("Unsupported type");
                    }
                }

                nativeFunctionHandle(params.data(), &result);

                switch (returnType) {
                    case VT_DOUBLE: {
                        stack.pushDouble(result.doubleValue);
                        break;
                    }

                    case VT_INT: {
                        stack.pushInt(result.intValue);
                        break;
                    }

                    case VT_STRING: {
                        stack.pushUInt16(stringManager.addNativeString(result.stringValue));
                        break;
                    }

                    case VT_VOID: {
                        break;
                    }

                    default:
                        throw runtime_error("Unknown return type!");
                }
            }

            case BC_RETURN: {
                contextHolder.leaveContext();
                break;
            }

            case BC_JA: {
                int16_t offset = contextHolder.topContext()->readInt16();
                contextHolder.topContext()->moveInstructionIndexByOffset(offset);
                break;
            }

            case BC_IFICMPNE: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                int16_t offset = contextHolder.topContext()->readInt16();
                if (upper != lower) {
                    contextHolder.topContext()->moveInstructionIndexByOffset(offset);
                }

                break;
            }

            case BC_IFICMPE: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                int16_t offset = contextHolder.topContext()->readInt16();
                if (upper == lower) {
                    contextHolder.topContext()->moveInstructionIndexByOffset(offset);
                }

                break;
            }

            case BC_IFICMPG: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                int16_t offset = contextHolder.topContext()->readInt16();
                if (upper > lower) {
                    contextHolder.topContext()->moveInstructionIndexByOffset(offset);
                }

                break;
            }

            case BC_IFICMPGE: {
                auto upper = stack.popInt();
                auto lower = stack.popInt();

                int16_t offset = contextHolder.topContext()->readInt16();
                if (upper >= lower) {
                    contextHolder.topContext()->moveInstructionIndexByOffset(offset);
                }

                break;
            }

            case BC_STOP:
                return Status::Ok();

            default:
                throw std::runtime_error(
                    "Unkonw instruction "s + bytecodeName(instruction, nullptr)
                );
        }
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
