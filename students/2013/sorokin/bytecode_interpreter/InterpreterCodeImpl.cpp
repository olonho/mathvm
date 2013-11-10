#include "InterpreterCodeImpl.h"

namespace mathvm { 
    static const long long INSTRUCTION_LIMIT = 90000000000; //90 000 000 000   

    InterpreterCodeImpl::InterpreterCodeImpl(ostream& output):
        bytecode(0),
        myPos(0),
        myOut(output){}
    
    InterpreterCodeImpl::~InterpreterCodeImpl() { }
    
    uint16_t InterpreterCodeImpl::nextStringId() {
        return nextUInt16();
    }
    
    uint16_t InterpreterCodeImpl::nextUInt16() {
        uint16_t result = bytecode->getUInt16(myPos);
        myPos += sizeof(uint16_t);
        return result;
    }

    int64_t InterpreterCodeImpl::nextInt64() {
        int64_t result = bytecode->getInt64(myPos);
        myPos += sizeof(int64_t);
        return result;
    }

    double InterpreterCodeImpl::nextDouble() {
        double result = bytecode->getDouble(myPos);
        myPos += sizeof(double);
        return result;
    }
    
    
    Status* InterpreterCodeImpl::execute(std::vector<Var*>&) {
        try {
            enterFunction(0); // top function
                long long z = 0;

            while (true) {
                z++;
                if(z > INSTRUCTION_LIMIT) { // for a infinite cycle case.
                    myOut << "instruction limit is exceeded"<< endl;
                    return 0; 
                }
                        // По идее ip это index нашей инструкции которую мы читаем
                Instruction instruction = bytecode->getInsn(myPos);
                myPos += sizeof(int8_t); // переходим сразу за инструкцию
                switch(instruction) {
                    case BC_INVALID: 
                        throw RuntimeException("Invalid instruction") <<
                        "\nfunction id = " << context()->getId() << 
                        "\n pos = " << myPos;
                        break;
                    case BC_DLOAD: myStack.pushDouble(nextDouble()); break;
                    case BC_ILOAD: myStack.pushInt64(nextInt64()); break;
                    case BC_SLOAD: myStack.pushStringId(nextStringId()); break;
                    case BC_DLOAD0: myStack.pushDouble(0.0); break;
                    case BC_ILOAD0: myStack.pushInt64(0); break;
                    case BC_SLOAD0: break;
                    case BC_DLOAD1: myStack.pushDouble(1.0); break;
                    case BC_ILOAD1: myStack.pushInt64(1); break;
                    case BC_DLOADM1: myStack.pushDouble(-1.0); break;
                    case BC_ILOADM1: myStack.pushInt64(-1); break;
                    
                    case BC_DADD: myStack.dAdd(); break;
                    case BC_IADD: myStack.iAdd(); break;
                    case BC_DSUB: myStack.dSub(); break;
                    case BC_ISUB: myStack.iSub(); break;
                    case BC_DMUL: myStack.dMul(); break;
                    case BC_IMUL: myStack.iMul(); break;
                    case BC_DDIV: myStack.dDiv(); break;
                    case BC_IDIV: myStack.iDiv(); break;
                    case BC_IMOD: myStack.iMod(); break;
                    case BC_DNEG: myStack.dNeg(); break;
                    case BC_INEG: myStack.iNeg(); break;
                    
                    case BC_IPRINT: 
                        myOut << myStack.popInt64(); break;
                    case BC_DPRINT: 
                        myOut << myStack.popDouble(); break;
                    case BC_SPRINT:
                        myOut << this->constantById(myStack.popStringId()); break;
                    case BC_I2D: 
                        myStack.pushDouble((double)myStack.popInt64()); break;
                    case BC_D2I: 
                        myStack.pushInt64((int)myStack.popDouble()); break;
                    case BC_S2I: { 
                        int64_t val = (int64_t)constantById(myStack.popStringId()).size();
                        myStack.pushInt64(val); 
                        break; 
                    }  
                    case BC_SWAP: {
                        StackValue val1 = myStack.pop();
                        StackValue val2 = myStack.pop();
                        myStack.push(val1);
                        myStack.push(val2);
                        break;
                    }
                    case BC_POP: myStack.pop(); break;
                    case BC_LOADDVAR0:
                        myStack.pushDouble(context()->getDouble(0)); break;
                    case BC_LOADDVAR1: 
                        myStack.pushDouble(context()->getDouble(1)); break;
                    case BC_LOADDVAR2:
                        myStack.pushDouble(context()->getDouble(2)); break;
                    case BC_LOADDVAR3:
                        myStack.pushDouble(context()->getDouble(3)); break;
                    case BC_LOADIVAR0:
                        myStack.pushInt64(context()->getInt64(0)); break;
                    case BC_LOADIVAR1:
                        myStack.pushInt64(context()->getInt64(1)); break;
                    case BC_LOADIVAR2:
                        myStack.pushInt64(context()->getInt64(2)); break;
                    case BC_LOADIVAR3:
                        myStack.pushInt64(context()->getInt64(3)); break;
                    case BC_LOADSVAR0:
                        myStack.pushStringId(context()->getStringId(0)); break;
                    case BC_LOADSVAR1:
                        myStack.pushStringId(context()->getStringId(1)); break;
                    case BC_LOADSVAR2:
                        myStack.pushStringId(context()->getStringId(2)); break;
                    case BC_LOADSVAR3:
                        myStack.pushStringId(context()->getStringId(3)); break;
                    case BC_STOREDVAR0:
                        context()->storeDouble(0, myStack.popDouble()); break;
                    case BC_STOREDVAR1:
                        context()->storeDouble(1, myStack.popDouble()); break;
                    case BC_STOREDVAR2:
                        context()->storeDouble(2, myStack.popDouble()); break;
                    case BC_STOREDVAR3:
                        context()->storeDouble(3, myStack.popDouble()); break;
                        
                    case BC_STOREIVAR0:
                        context()->storeInt64(0, myStack.popInt64()); break;
                    case BC_STOREIVAR1:
                        context()->storeInt64(1, myStack.popInt64()); break;
                    case BC_STOREIVAR2:
                        context()->storeInt64(2, myStack.popInt64()); break;
                    case BC_STOREIVAR3:
                        context()->storeInt64(3, myStack.popInt64()); break;
                    case BC_STORESVAR0:
                        context()->storeStringId(0, myStack.popStringId()); break;
                    case BC_STORESVAR1:
                        context()->storeStringId(1, myStack.popStringId()); break;
                    case BC_STORESVAR2:
                        context()->storeStringId(2, myStack.popStringId()); break;
                    case BC_STORESVAR3:
                        context()->storeStringId(3, myStack.popStringId()); break;
                        //load
                    case BC_LOADDVAR:
                        myStack.pushDouble(context()->getDouble(nextUInt16())); break;
                    case BC_LOADIVAR:
                        myStack.pushInt64(context()->getInt64(nextUInt16())); break;
                    case BC_LOADSVAR:
                        myStack.pushStringId(context()->getStringId(nextUInt16())); break;
                        
                    case BC_STOREDVAR:
                        context()->storeDouble(nextUInt16(), myStack.popDouble()); break;
                    case BC_STOREIVAR:
                        context()->storeInt64(nextUInt16(), myStack.popInt64()); break;
                    case BC_STORESVAR:
                        context()->storeStringId(nextUInt16(), myStack.popStringId()); break;
                        
                    case BC_LOADCTXDVAR: loadFromContext(nextUInt16()); break;
                    case BC_LOADCTXIVAR: loadFromContext(nextUInt16()); break;
                    case BC_LOADCTXSVAR: loadFromContext(nextUInt16()); break;
                    
                    case BC_STORECTXDVAR: storeToContext(nextUInt16()); break;
                    case BC_STORECTXIVAR: storeToContext(nextUInt16()); break;
                    case BC_STORECTXSVAR: storeToContext(nextUInt16()); break;
                    case BC_DCMP: {
                        double one = myStack.popDouble();
                        double two = myStack.popDouble();
                        myStack.pushInt64(one== two ? 0 : (one < two ? -1 : 1));
                        break;
                    }
                    case BC_ICMP: {
                        int64_t one = myStack.popInt64();
                        int64_t two = myStack.popInt64();
                        myStack.pushInt64(one == two ? 0 : (one < two ? -1 : 1));
                        break;
                    }
                    case BC_JA: myPos += bytecode->getInt16(myPos); break;
                    case BC_IFICMPNE: jumpIf(not_equal_to<int64_t>()); break;
                    case BC_IFICMPE: jumpIf(equal_to<int64_t>());; break;
                    case BC_IFICMPG: jumpIf(greater<int64_t>()); break;
                    case BC_IFICMPGE: jumpIf(greater_equal<int64_t>()); break;
                    case BC_IFICMPL: jumpIf(less<int64_t>()); break;
                    case BC_IFICMPLE: jumpIf(less_equal<int64_t>()); break;
                    case BC_DUMP: {
                        throw RuntimeException("BC_DUMP - Unsupported Instruction");
                        break;
                    }
                    case BC_STOP: return 0;
                    case BC_CALL: enterFunction(nextUInt16()); break;
                    case BC_CALLNATIVE: {
                        throw RuntimeException("BC_CALLNATIVE - Unsupported Instruction");
                        break;
                    }
                    case BC_RETURN: exitFunction(); break;
                    case BC_BREAK: break;
                    
                    default: throw RuntimeException("Unknown Instruction:") <<
                            "\nfunction id = "<< context()->getId() << 
                            "\nbytecode position: " << myPos; break;

                } //switch(instruction)
            } //while (true)
            return 0;
            
        }catch(RuntimeException e) {
            myOut << e.what();
            return new Status(e.what());
        }
    } //execute
    
} // mathvm
