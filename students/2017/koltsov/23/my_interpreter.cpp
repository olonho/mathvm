#include "ast.h"
#include "parser.h"

#include "my_interpreter.h"

#include <iomanip>

namespace mathvm {

using namespace std;



void Interpreter::assignVars(vector<Var*>& vars) {
    (void)vars;
}

void Interpreter::saveVars(vector<Var*>& vars) {
    (void)vars;
}


void Interpreter::mainLoop() {
    enterFunction((BytecodeFunction*)_code->functionByName(AstFunction::top_name));

    bool executing = true;
    Value v{1.0}, v2{1.0};
    int16_t offset;
    Int intVar0;

    TwoBytes scopeId, varId;

    while (executing) {
        Instruction insn = bytecode()->getInsn(ip);
        ++ip;
        DBG(bytecodeName(insn, nullptr));

        switch (insn) {
            case BC_CALL: 
                enterFunction((BytecodeFunction*)_code->functionById(getTyped<TwoBytes>()));
                break;
            case BC_D2I: 
                v = popValue();
                vals.emplace((Int)v.d);
                break;
            case BC_DADD: 
                v = popValue();
                v2 = popValue();
                vals.emplace(v.d + v2.d);
                break;
            case BC_ICMP: 
                v = popValue();
                v2 = popValue();
                vals.emplace((Int)(v.i > v2.i ? 1 : (v.i < v2.i ? -1 : 0)));
                break;
            case BC_DDIV: 
                v = popValue();
                v2 = popValue();
                vals.emplace(v.d / v2.d);
                break;
            case BC_DLOAD: 
                vals.emplace(getTyped<double>());
                break;
            case BC_DMUL: 
                v = popValue();
                v2 = popValue();
                vals.emplace(v.d * v2.d);
                break;
            case BC_DNEG: 
                v = popValue();
                vals.emplace(v.d * -1);
                break;
            case BC_DPRINT: 
                v = popValue();
                cout << setprecision(20) << v.d;
                break;
            case BC_DSUB: 
                v = popValue();
                v2 = popValue();
                vals.emplace(v.d - v2.d);
                break;
            case BC_I2D: 
                v = popValue();
                vals.emplace((double)v.i);
                break;
            case BC_IAAND: 
                v = popValue();
                v2 = popValue();
                vals.emplace((Int)(v.i & v2.i));
                break;
            case BC_IADD: 
                v = popValue();
                v2 = popValue();
                vals.emplace((Int)(v.i + v2.i));
                break;
            case BC_IAOR: 
                v = popValue();
                v2 = popValue();
                vals.emplace((Int)(v.i | v2.i));
                break;
            case BC_IAXOR: 
                v = popValue();
                v2 = popValue();
                vals.emplace((Int)(v.i ^ v2.i));
                break;
#define JUMPS(suf, op) \
case BC_IFICMP##suf:\
    offset = getTyped<int16_t>(); \
    v = popValue(); \
    v2 = popValue(); \
    if (v.i op v2.i) { \
        ip -= 2; \
        ip += offset; \
    } \
    break
            JUMPS(G, >);
            JUMPS(L, <);
            JUMPS(NE, !=);
#undef JUMPS
            case BC_ILOAD: 
                vals.emplace(getTyped<Int>());    
                break;
            case BC_ILOAD0: 
                vals.emplace((Int)0);
                break;
            case BC_ILOAD1: 
                vals.emplace((Int)1);
                break;
            case BC_IMOD: 
                v = popValue();
                v2 = popValue();
                vals.emplace((Int)(v.i % v2.i));
                break;
            case BC_INEG: 
                v = popValue();
                vals.emplace((Int)v.i * -1);
                break;
            case BC_IPRINT: 
                v = popValue();
                cout << v.i;
                break;
            case BC_ISUB: 
                v = popValue();
                v2 = popValue();
                vals.emplace((Int)(v.i - v2.i));
                break;
            case BC_JA: 
                offset = getTyped<int16_t>();
                ip += offset - 2;
                break;
            case BC_LOADCTXIVAR: 
            case BC_LOADCTXSVAR: 
            case BC_LOADCTXDVAR: 
                scopeId = getTyped<TwoBytes>();
                varId = getTyped<TwoBytes>();
                vals.push(*getVarByTwoIds(scopeId, varId));
                break;
            case BC_LOADIVAR0:
                intVar0 = 0;
                break;
            case BC_POP: 
                popValue();
                break;
            case BC_RETURN: 
                leaveFunction();
                break;
            case BC_S2I: 
                v = popValue();
                vals.emplace((Int)stoi(v.s));
                break;
            case BC_SLOAD: 
                vals.emplace(_code->constantById(getTyped<TwoBytes>()).c_str());
                break;
            case BC_SPRINT: 
                v = popValue();
                cout << v.s;
               break;
            case BC_STOP: 
               executing = false;
               break;
            case BC_STORECTXDVAR: 
            case BC_STORECTXIVAR: 
            case BC_STORECTXSVAR: 
               scopeId = getTyped<TwoBytes>();
               varId = getTyped<TwoBytes>();
               *getVarByTwoIds(scopeId, varId) = popValue();
               break;
            case BC_STOREIVAR0: 
               vals.emplace((Int)intVar0);
               break;
            case BC_SWAP: 
               v = popValue();
               v2 = popValue();
               vals.push(v);
               vals.push(v2);
               break;

            default:
              DBG(insn);
              my_error("invalid opcode");
        }
        DBG(vals.size());
    }
    cout.flush();
}

Value Interpreter::popValue() {
    auto ret = vals.top();
    vals.pop();
    return ret;
}

void Interpreter::enterFunction(BytecodeFunction* func) {
    ips.push(ip);
    funcs.push(currentFunction);
    currentFunction = func;
    scopeVarToVal[currentFunction->scopeId()].emplace();
    ip = 0;
}

void Interpreter::leaveFunction() {
    ip = ips.top();
    ips.pop();
    scopeVarToVal[currentFunction->scopeId()].pop();
    currentFunction = funcs.top();
    funcs.pop();
}

Value* Interpreter::getVarByTwoIds(uint16_t scopeId, uint16_t varId) {
    return &scopeVarToVal[scopeId].top()[varId];
}

Bytecode* Interpreter::bytecode() {
    return currentFunction->bytecode();
}

template<class T>
T Interpreter::getTyped() {
    T ret = bytecode()->getTyped<T>(ip);
    ip += sizeof(T);
    return ret;
}

} // anon namespace 
