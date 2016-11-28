#include <ast.h>
#include "BytecodeInterpreter.h"
#include "mathvm.h"

namespace mathvm {
    BytecodeInterpreter::BytecodeInterpreter(ostream &out) : out(out),
                                                             currentFunctionId(0), index(0) {}

    Status *BytecodeInterpreter::execute(vector<Var *> &vars) {
        BytecodeFunction *function = (BytecodeFunction *) functionByName(AstFunction::top_name);
        bytecode = function->bytecode();
        currentFunctionId = function->id();
//        Code::FunctionIterator functionIterator{this};
//        while (functionIterator.hasNext()) {
//            auto *f = functionIterator.next();
//            cout << f->name() << endl;
//            f->disassemble(cout);
//        }

        localsMap.insert(make_pair(currentFunctionId,
                                   vector<generalValue>{(uint16_t) function->localsNumber(), generalValue{}}));


        for (index = 0; index < bytecode->length();) {
            executeInstruction();
        }

        return Status::Ok();
    }

    void BytecodeInterpreter::executeInstruction() {
        Instruction instruction = bytecode->getInsn(index++);
//        size_t length;
//        const char *name = bytecodeName(instruction, &length);
//        cout << name;
        generalValue a;
        generalValue b;
        switch (instruction) {
            case BC_INVALID:
                throw InterpreterException("Invalid command!");

            case BC_DLOAD:
                pushTypedToStack<double>(bytecode->getDouble(index));
                index += 8;
                break;
            case BC_ILOAD:
                pushTypedToStack<int64_t>(bytecode->getInt64(index));
                index += 8;
                break;
            case BC_SLOAD:
                pushTypedToStack<uint16_t>(bytecode->getUInt16(index));
                index += 2;
                break;

            case BC_DLOAD0:
                pushTypedToStack<double>(0.);
                break;
            case BC_ILOAD0:
                pushTypedToStack<int64_t>(0);
                break;
            case BC_SLOAD0:
                pushTypedToStack<uint16_t>(makeStringConstant(""));
                break;
            case BC_DLOAD1:
                pushTypedToStack<double>(1.);
                break;
            case BC_ILOAD1:
                pushTypedToStack<int64_t>(1);
                break;

            case BC_DLOADM1:
                pushTypedToStack<double>(-1.);
                break;
            case BC_ILOADM1:
                pushTypedToStack<int64_t>(-1);
                break;

            case BC_DADD:
                pushTypedToStack<double>(popTypedFromStack<double>() + popTypedFromStack<double>());
                break;
            case BC_IADD:
                pushTypedToStack<int64_t>(popTypedFromStack<int64_t>() + popTypedFromStack<int64_t>());
                break;

            case BC_DSUB:
                a.doubleValue = popTypedFromStack<double>();
                b.doubleValue = popTypedFromStack<double>();
                pushTypedToStack<double>(b.doubleValue - a.doubleValue);
                break;
            case BC_ISUB:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                pushTypedToStack<int64_t>(b.intValue - a.intValue);
                break;

            case BC_IMUL:
                pushTypedToStack<int64_t>(popTypedFromStack<int64_t>() * popTypedFromStack<int64_t>());
                break;
            case BC_DMUL:
                pushTypedToStack<double>(popTypedFromStack<double>() * popTypedFromStack<double>());
                break;

            case BC_DDIV:
                a.doubleValue = popTypedFromStack<double>();
                b.doubleValue = popTypedFromStack<double>();
                pushTypedToStack<double>(b.doubleValue / a.doubleValue);
                break;
            case BC_IDIV:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                pushTypedToStack<int64_t>(b.intValue / a.intValue);
                break;
            case BC_IMOD:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                pushTypedToStack<int64_t>(b.intValue % a.intValue);
                break;

            case BC_DNEG:
                pushTypedToStack<double>(-popTypedFromStack<double>());
                break;
            case BC_INEG:
                pushTypedToStack<int64_t>(-popTypedFromStack<int64_t>());
                break;

            case BC_IAOR:
                pushTypedToStack<int64_t>(popTypedFromStack<int64_t>() | popTypedFromStack<int64_t>());
                break;
            case BC_IAAND:
                pushTypedToStack<int64_t>(popTypedFromStack<int64_t>() & popTypedFromStack<int64_t>());
                break;
            case BC_IAXOR:
                pushTypedToStack<int64_t>(popTypedFromStack<int64_t>() ^ popTypedFromStack<int64_t>());
                break;

            case BC_IPRINT:
                out << popTypedFromStack<int64_t>();
                break;
            case BC_DPRINT:
                out << popTypedFromStack<double>();
                break;
            case BC_SPRINT:
                out << constantById(popTypedFromStack<uint16_t>());
                break;

            case BC_I2D:
                pushTypedToStack<double>(popTypedFromStack<int64_t>());
                break;
            case BC_D2I:
                pushTypedToStack<int64_t>(static_cast<int64_t>(popTypedFromStack<double>()));
                break;
            case BC_S2I:
                pushTypedToStack<int64_t>(stoi(constantById(popTypedFromStack<uint16_t>())));
                break;

            case BC_SWAP:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                pushTypedToStack(a.intValue);
                pushTypedToStack(b.intValue);
                break;

            case BC_POP:
                popTypedFromStack<uint8_t>();
                break;

            case BC_LOADDVAR0:
                pushTypedToStack<double>(var1.doubleValue);
                break;
            case BC_LOADDVAR1:
                pushTypedToStack<double>(var2.doubleValue);
                break;
            case BC_LOADDVAR2:
                pushTypedToStack<double>(var3.doubleValue);
                break;
            case BC_LOADDVAR3:
                pushTypedToStack<double>(var4.doubleValue);
                break;
            case BC_LOADIVAR0:
                pushTypedToStack<int64_t>(var1.intValue);
                break;
            case BC_LOADIVAR1:
                pushTypedToStack<int64_t>(var2.intValue);
                break;
            case BC_LOADIVAR2:
                pushTypedToStack<int64_t>(var3.intValue);
                break;
            case BC_LOADIVAR3:
                pushTypedToStack<int64_t>(var4.intValue);
                break;
            case BC_LOADSVAR0:
                pushTypedToStack<uint16_t>(var1.stringIdValue);
                break;
            case BC_LOADSVAR1:
                pushTypedToStack<uint16_t>(var2.stringIdValue);
                break;
            case BC_LOADSVAR2:
                pushTypedToStack<uint16_t>(var3.stringIdValue);
                break;
            case BC_LOADSVAR3:
                pushTypedToStack<uint16_t>(var4.stringIdValue);
                break;
            case BC_STOREDVAR0:
                var1.doubleValue = popTypedFromStack<double>();
                break;
            case BC_STOREDVAR1:
                var2.doubleValue = popTypedFromStack<double>();
                break;
            case BC_STOREDVAR2:
                var3.doubleValue = popTypedFromStack<double>();
                break;
            case BC_STOREDVAR3:
                var4.doubleValue = popTypedFromStack<double>();
                break;
            case BC_STOREIVAR0:
                var1.intValue = popTypedFromStack<int64_t>();
                break;
            case BC_STOREIVAR1:
                var2.intValue = popTypedFromStack<int64_t>();
                break;
            case BC_STOREIVAR2:
                var3.intValue = popTypedFromStack<int64_t>();
                break;
            case BC_STOREIVAR3:
                var4.intValue = popTypedFromStack<int64_t>();
                break;
            case BC_STORESVAR0:
                var1.stringIdValue = popTypedFromStack<uint16_t>();
                break;
            case BC_STORESVAR1:
                var2.stringIdValue = popTypedFromStack<uint16_t>();
                break;
            case BC_STORESVAR2:
                var3.stringIdValue = popTypedFromStack<uint16_t>();
                break;
            case BC_STORESVAR3:
                var4.stringIdValue = popTypedFromStack<uint16_t>();
                break;

            case BC_LOADDVAR:
                pushTypedToStack<double>(getLocalDouble(bytecode->getUInt16(index)));
                index += 2;
                break;
            case BC_LOADIVAR:
                pushTypedToStack<int64_t>(getLocalInt(bytecode->getUInt16(index)));
                index += 2;
                break;
            case BC_LOADSVAR:
                pushTypedToStack<uint16_t>(getLocalStringId(bytecode->getUInt16(index)));
                index += 2;
                break;

            case BC_STOREDVAR:
                putLocalDouble(bytecode->getUInt16(index), popTypedFromStack<double>());
                index += 2;
                break;
            case BC_STOREIVAR:
                putLocalInt(bytecode->getUInt16(index), popTypedFromStack<int64_t>());
                index += 2;
                break;
            case BC_STORESVAR:
                putLocalStringId(bytecode->getUInt16(index), popTypedFromStack<uint16_t>());
                index += 2;
                break;

            case BC_LOADCTXDVAR:
                pushTypedToStack<double>(getContextDouble(bytecode->getUInt16(index), bytecode->getUInt16(index + 2)));
                index += 4;
                break;
            case BC_LOADCTXIVAR:
                pushTypedToStack<int64_t>(getContextInt(bytecode->getUInt16(index), bytecode->getUInt16(index + 2)));
                index += 4;
                break;
            case BC_LOADCTXSVAR:
                pushTypedToStack<uint16_t>(
                        getContextStringId(bytecode->getUInt16(index), bytecode->getUInt16(index + 2)));
                index += 4;
                break;
            case BC_STORECTXDVAR:
                putContextDouble(bytecode->getUInt16(index), bytecode->getUInt16(index + 2),
                                 popTypedFromStack<double>());
                index += 4;
                break;
            case BC_STORECTXIVAR:
                putContextInt(bytecode->getUInt16(index), bytecode->getUInt16(index + 2), popTypedFromStack<int64_t>());
                index += 4;
                break;
            case BC_STORECTXSVAR:
                putContextStringId(bytecode->getUInt16(index), bytecode->getUInt16(index + 2),
                                   popTypedFromStack<uint16_t>());
                index += 4;
                break;

            case BC_CALL:
                callFunction();
                break;
            case BC_RETURN:
                returnFromFunction();
                break;


            case BC_DCMP:
                a.doubleValue = popTypedFromStack<double>();
                b.doubleValue = popTypedFromStack<double>();
                pushTypedToStack<int64_t>((b.doubleValue > a.doubleValue) - (b.doubleValue < a.doubleValue));
                break;
            case BC_ICMP:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                pushTypedToStack<int64_t>((b.intValue > a.intValue) - (b.intValue < a.intValue));
                break;

            case BC_JA:
                index += bytecode->getInt16(index);
                break;
            case BC_IFICMPE:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                if (b.intValue == a.intValue) index += bytecode->getInt16(index);
                else index += 2;
                break;
            case BC_IFICMPNE:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                if (b.intValue != a.intValue) index += bytecode->getInt16(index);
                else index += 2;
                break;
            case BC_IFICMPG:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                if (b.intValue > a.intValue) index += bytecode->getInt16(index);
                else index += 2;
                break;
            case BC_IFICMPGE:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                if (b.intValue >= a.intValue) index += bytecode->getInt16(index);
                else index += 2;
                break;
            case BC_IFICMPL:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                if (b.intValue < a.intValue) index += bytecode->getInt16(index);
                else index += 2;
                break;
            case BC_IFICMPLE:
                a.intValue = popTypedFromStack<int64_t>();
                b.intValue = popTypedFromStack<int64_t>();
                if (b.intValue <= a.intValue) index += bytecode->getInt16(index);
                else index += 2;
                break;


            default:
                throw InterpreterException{"No such instruction"};

            case BC_DUMP:
                break;
            case BC_STOP:
                break;
            case BC_CALLNATIVE:
                break;
            case BC_BREAK:
                break;
            case BC_LAST:
                break;
        }
    }

    void BytecodeInterpreter::callFunction() {
        uint16_t id = bytecode->getUInt16(index);

        BytecodeFunction *function = (BytecodeFunction *) functionById(id);
        bytecode = function->bytecode();

        callStack.emplace(make_pair(currentFunctionId, index + 2));

        index = 0;
        currentFunctionId = id;

        if (localsMap.find(id) == localsMap.end()) {
            localsMap.insert(make_pair(id, vector<generalValue>{}));
        }

        for (uint16_t i = 0; i < (uint16_t) function->localsNumber(); ++i) {
            localsMap.at(id).push_back(generalValue{});
        }
    }

    void BytecodeInterpreter::returnFromFunction() {
        localsMap.at(currentFunctionId).resize(localOffset());

        pair<uint16_t, uint16_t> p = callStack.top();
        callStack.pop();
        index = p.second;
        currentFunctionId = p.first;

        BytecodeFunction *function = (BytecodeFunction *) functionById(currentFunctionId);

        bytecode = function->bytecode();
    }

    template<class T>
    T BytecodeInterpreter::popTypedFromStack() {
        union {
            T val;
            uint8_t bytes[sizeof(T)];
        } u;
        for (uint8_t i = sizeof(T) - 1; i != 255; --i) {
            u.bytes[i] = _stack.back();
            _stack.pop_back();
        }
        return u.val;
    }

    template<class T>
    void BytecodeInterpreter::pushTypedToStack(T value) {
        union {
            T val;
            uint8_t bytes[sizeof(T)];
        } u;
        u.val = value;
        for (uint8_t i = 0; i < sizeof(T); ++i) {
            _stack.push_back(u.bytes[i]);
        }
    }

    int64_t BytecodeInterpreter::getLocalInt(uint16_t index) {
        return localsMap.at(currentFunctionId).at(localOffset() + index).intValue;
    }

    void BytecodeInterpreter::putLocalInt(uint16_t index, int64_t value) {
        localsMap.at(currentFunctionId).at(localOffset() + index).intValue = value;
    }

    double BytecodeInterpreter::getLocalDouble(uint16_t index) {
        return localsMap.at(currentFunctionId).at(localOffset() + index).doubleValue;
    }

    void BytecodeInterpreter::putLocalDouble(uint16_t index, double value) {
        localsMap.at(currentFunctionId).at(localOffset() + index).doubleValue = value;
    }

    uint16_t BytecodeInterpreter::getLocalStringId(uint16_t index) {
        return localsMap.at(currentFunctionId).at(localOffset() + index).stringIdValue;
    }

    void BytecodeInterpreter::putLocalStringId(uint16_t index, uint16_t value) {
        localsMap.at(currentFunctionId).at(localOffset() + index).stringIdValue = value;
    }

    int64_t BytecodeInterpreter::getContextInt(uint16_t contextId, uint16_t index) {
        return localsMap.at(contextId).at(offsetOf(contextId) + index).intValue;
    }

    double BytecodeInterpreter::getContextDouble(uint16_t contextId, uint16_t index) {
        return localsMap.at(contextId).at(offsetOf(contextId) + index).doubleValue;
    }

    uint16_t BytecodeInterpreter::getContextStringId(uint16_t contextId, uint16_t index) {
        return localsMap.at(contextId).at(offsetOf(contextId) + index).stringIdValue;
    }

    void BytecodeInterpreter::putContextInt(uint16_t contextId, uint16_t index, int64_t value) {
        localsMap.at(contextId).at(offsetOf(contextId) + index).intValue = value;
    }

    void BytecodeInterpreter::putContextDouble(uint16_t contextId, uint16_t index, double value) {
        localsMap.at(contextId).at(offsetOf(contextId) + index).doubleValue = value;
    }

    void BytecodeInterpreter::putContextStringId(uint16_t contextId, uint16_t index, uint16_t value) {
        localsMap.at(contextId).at(offsetOf(contextId) + index).stringIdValue = value;
    }

    uint16_t BytecodeInterpreter::localOffset() {
        return offsetOf(currentFunctionId);
    }

    uint16_t BytecodeInterpreter::offsetOf(const uint16_t funcId) {
        int32_t res = (int32_t) localsMap.at(funcId).size() - functionById(funcId)->localsNumber();
        if (res < 0) {
            return 0;
        }
        return (uint16_t) res;
    }

    InterpreterException::InterpreterException(const string &__arg) : runtime_error(__arg) {}
}
