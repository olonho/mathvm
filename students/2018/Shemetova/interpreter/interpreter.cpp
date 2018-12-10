#include "interpreter.h"


namespace mathvm {

    CodeImpl::CodeImpl() {
    }

    Status* CodeImpl::execute(vector<Var*> &vars) {
        BytecodeFunction* main = dynamic_cast<BytecodeFunction*> (functionByName("<top>"));
        Interpreter interpreter = Interpreter(main->bytecode(), this);

        interpreter.interpretFunction(main);
        return Status::Ok();
    }

    ScopeInterpreter::ScopeInterpreter(ScopeInterpreter* p, uint16_t varsNumber,
            uint16_t scopeId) : parent(p), varsNumber(varsNumber), id(scopeId) {
        if (p) {
            p->addChild(this);
        }
        vars = vector<StackValue>(varsNumber);

    }

    ScopeInterpreter* ScopeInterpreter::getScopeById(uint16_t id) {
        ScopeInterpreter* cur = this;
        while (cur) {
            if (cur->getId() == id) {
                return cur;
            }
            cur = cur->parent;
        }
        throw std::runtime_error("Interpreter error: no such scope for variable " + id);
    }

    uint16_t ScopeInterpreter::getId() {
        return id;
    }

    void ScopeInterpreter::addChild(ScopeInterpreter* child) {
        children.push_back(child);
    }

    StackValue& ScopeInterpreter::varById(uint16_t id) {
        assert(id < varsNumber);
        return vars.at(id);
    }

    Interpreter::Interpreter(Bytecode* bytecode, CodeImpl* code) : bytecode(bytecode), code(code) {
        currentScope = nullptr;
    }

    void Interpreter::pushD(double local) {
        operandStack.push(StackValue(local));
    }

    void Interpreter::pushI(int64_t local) {
        operandStack.push(StackValue(local));
    }

    void Interpreter::pushS(const char* local) {
        operandStack.push(StackValue(local));
    }

    uint16_t Interpreter::jumpOffset(Bytecode* bytecode, size_t currentPos) {
        currentPos = bytecode->getInt16(currentPos + 1) + currentPos + 1;
        return currentPos;
    }

    void Interpreter::interpretFunction(BytecodeFunction* func) {
        TranslatedFunction* tf = code->functionByName(func->name());
        uint16_t localsNumber = tf->localsNumber();
        //std::cout << "localsNumber " << localsNumber << std::endl;
        Bytecode* bytecode = func->bytecode();
        //bytecode->dump(cout);
        //cout << "-----------------------------------------" << endl;
        size_t size = bytecode->length();
        //cout << "bytecode size: " << size << endl;
        ScopeInterpreter* prev = nullptr;
        if (currentScope) {
            prev = currentScope;
            currentScope = new ScopeInterpreter(currentScope, localsNumber, tf->scopeId());
        } else {
            currentScope = new ScopeInterpreter(nullptr, localsNumber, tf->scopeId());
        }
        size_t i = 0;
        while (i < size) {
            Instruction instr = bytecode->getInsn(i);
            size_t instr_length = 0;
            //cout << bytecodeName(instr, &instr_length) << endl;
            const string& instrName = bytecodeName(instr, &instr_length);
            //cout << "length " << instr_length << endl;
            uint16_t item_id;
            uint16_t context_id;
            int64_t li;
            int64_t ri;
            double ld;
            double rd;
            StackValue a;
            StackValue b;
            ScopeInterpreter* st_scope;
            BytecodeFunction* callee;
            //cout << "current pos: " << i << endl;
            switch (instr) {
                case BC_DLOAD:
                    pushD(bytecode->getDouble(i + 1));
                    break;
                case BC_ILOAD:
                    pushI(bytecode->getInt64(i + 1));
                    break;
                case BC_SLOAD:
                    item_id = bytecode->getUInt16(i + 1);
                    pushS((code->constantById(item_id)).c_str());
                    break;
                case BC_STOREIVAR:
                    item_id = bytecode->getUInt16(i + 1);
                    currentScope->varById(item_id) = operandStack.top();
                    operandStack.pop();
                    break;
                case BC_STOREDVAR:
                    item_id = bytecode->getUInt16(i + 1);
                    currentScope->varById(item_id) = operandStack.top();
                    operandStack.pop();
                    break;
                case BC_STORESVAR:
                    //strings are immutable; already in Code 
                    item_id = bytecode->getUInt16(i + 1);
                    currentScope->varById(item_id) = operandStack.top();                 
                    operandStack.pop();
                    break;
                case BC_LOADIVAR:
                    item_id = bytecode->getUInt16(i + 1);
                    pushI((currentScope->varById(item_id)).getInt());
                    break;
                case BC_LOADDVAR:
                    item_id = bytecode->getUInt16(i + 1);
                    pushD((currentScope->varById(item_id)).getDouble());
                    break;
                case BC_LOADSVAR:
                    item_id = bytecode->getUInt16(i + 1);
                    pushS((code->constantById(item_id)).c_str());
                    break;
                case BC_IPRINT:
                    cout << operandStack.top().getInt();
                    operandStack.pop();
                    break;
                case BC_DPRINT:
                    cout << operandStack.top().getDouble();
                    operandStack.pop();
                    break;
                case BC_SPRINT:
                    cout << operandStack.top().getString();
                    operandStack.pop();
                    break;
                case BC_SWAP:
                    a = operandStack.top();
                    operandStack.pop();
                    b = operandStack.top();
                    operandStack.pop();
                    operandStack.push(a);
                    operandStack.push(b);
                    break;
                case BC_IADD:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li + ri);
                    break;
                case BC_DADD:
                    ld = operandStack.top().getDouble();
                    operandStack.pop();
                    rd = operandStack.top().getDouble();
                    operandStack.pop();
                    pushD(ld + rd);
                    break;
                case BC_ISUB:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li - ri);
                    break;
                case BC_DSUB:
                    ld = operandStack.top().getDouble();
                    operandStack.pop();
                    rd = operandStack.top().getDouble();
                    operandStack.pop();
                    pushD(ld - rd);
                    break;
                case BC_IMUL:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li * ri);
                    break;
                case BC_DMUL:
                    ld = operandStack.top().getDouble();
                    operandStack.pop();
                    rd = operandStack.top().getDouble();
                    operandStack.pop();
                    pushD(ld * rd);
                    break;
                case BC_IDIV:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li / ri);
                    break;
                case BC_DDIV:
                    ld = operandStack.top().getDouble();
                    operandStack.pop();
                    rd = operandStack.top().getDouble();
                    operandStack.pop();
                    pushD(ld / rd);
                    break;
                case BC_IMOD:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li % ri);
                    break;
                case BC_INEG:
                    li = operandStack.top().getInt();
                    operandStack.top() = StackValue(-li);
                    break;
                case BC_DNEG:
                    ld = operandStack.top().getDouble();
                    operandStack.top() = StackValue(-ld);
                    break;
                case BC_IAOR:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li | ri);
                    break;
                case BC_IAAND:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li & ri);
                    break;
                case BC_IAXOR:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    pushI(li ^ ri);
                    break;
                case BC_ILOAD0:
                    pushI(0);
                    break;
                case BC_DLOAD0:
                    pushD(0.0);
                    break;
                case BC_SLOAD0:
                    pushS("");
                    break;
                case BC_ILOAD1:
                    pushI(1);
                    break;
                case BC_DLOAD1:
                    pushD(1.0);
                    break;
                case BC_ILOADM1:
                    pushI(-1);
                    break;
                case BC_DLOADM1:
                    pushD(-1.0);
                    break;
                case BC_ICMP:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    if (li < ri) {
                        pushI(-1);
                    } else if (li == ri) {
                        pushI(0);
                    } else {
                        pushI(1);
                    }
                    break;
                case BC_DCMP:
                    ld = operandStack.top().getDouble();
                    operandStack.pop();
                    rd = operandStack.top().getDouble();
                    operandStack.pop();
                    if (ld < rd) {
                        pushI(-1);
                    } else if (ld == rd) {
                        pushI(0);
                    } else {
                        pushI(1);
                    }
                    break;
                case BC_IFICMPNE:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    if (li != ri) {
                        i = jumpOffset(bytecode, i);
                        continue;
                    }
                    break;
                case BC_IFICMPE:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    if (li == ri) {
                        i = jumpOffset(bytecode, i);
                        //cout << "jump to pos: " << i << endl;
                        continue;
                    }
                    break;
                case BC_IFICMPG:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    if (ri > li) {
                        i = jumpOffset(bytecode, i);
                        continue;
                    }
                    break;
                case BC_IFICMPGE:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    if (ri >= li) {
                        i = jumpOffset(bytecode, i);
                        continue;
                    }
                    break;
                case BC_IFICMPL:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    if (ri < li) {
                        i = jumpOffset(bytecode, i);
                        continue;
                    }
                    break;
                case BC_IFICMPLE:
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    ri = operandStack.top().getInt();
                    operandStack.pop();
                    if (ri <= li) {
                        i = jumpOffset(bytecode, i);
                        continue;
                    }
                    break;
                case BC_JA:
                    i = jumpOffset(bytecode, i);
                    continue;
                    break;
                case BC_LOADCTXIVAR:
                    context_id = bytecode->getUInt16(i + 1);
                    item_id = bytecode->getUInt16(i + 3);
                    st_scope = currentScope->getScopeById(context_id);
                    pushI((st_scope->varById(item_id)).getInt());
                    break;
                case BC_LOADCTXDVAR:
                    context_id = bytecode->getUInt16(i + 1);
                    item_id = bytecode->getUInt16(i + 3);
                    st_scope = currentScope->getScopeById(context_id);
                    pushD((st_scope->varById(item_id)).getDouble());
                    break;
                case BC_LOADCTXSVAR:
                    context_id = bytecode->getUInt16(i + 1);
                    item_id = bytecode->getUInt16(i + 3);
                    st_scope = currentScope->getScopeById(context_id);
                    pushS((st_scope->varById(item_id)).getString().c_str());
                    break;
                case BC_STORECTXIVAR:
                    context_id = bytecode->getUInt16(i + 1);
                    item_id = bytecode->getUInt16(i + 3);
                    st_scope = currentScope->getScopeById(context_id);
                    st_scope->varById(item_id) = operandStack.top();
                    operandStack.pop();
                    break;
                case BC_STORECTXDVAR:
                    context_id = bytecode->getUInt16(i + 1);
                    item_id = bytecode->getUInt16(i + 3);
                    st_scope = currentScope->getScopeById(context_id);
                    st_scope->varById(item_id) = operandStack.top();
                    operandStack.pop();
                    break;
                case BC_STORECTXSVAR:
                    context_id = bytecode->getUInt16(i + 1);
                    item_id = bytecode->getUInt16(i + 3);
                    st_scope = currentScope->getScopeById(context_id);
                    st_scope->varById(item_id) = operandStack.top();
                    operandStack.pop();
                    break;
                case BC_CALL:
                    item_id = bytecode->getUInt16(i+1);
                    callee = dynamic_cast<BytecodeFunction*>(code->functionById(item_id));
                    interpretFunction(callee);
                    break;
                case BC_RETURN:
                    i = size;
                    break;
                case BC_I2D: {
                    li = operandStack.top().getInt();
                    operandStack.pop();
                    pushD((double) li);
                    break;
                }
                case BC_D2I: {
                    ld = operandStack.top().getInt();
                    operandStack.pop();
                    pushI((uint64_t) ld);
                    break;
                }
                case BC_STOP:
                   // cout << "stoped" << endl;
                    if (!operandStack.empty()) {
                        operandStack=stack<StackValue>();
                    }
                    return;
                default:throw std::runtime_error("Unknown instruction: " + instrName);
                    break;
            }
            i += instr_length;
        }
        if (prev) {
            currentScope = prev;
        }
    }


}