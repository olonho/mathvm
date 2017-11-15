#include "ast.h"
#include "mathvm.h"
#include "parser.h"
#include "bytecode_translator.h"

using namespace my;

using mathvm::BytecodeFunction;

void Code::disassemble(std::ostream& out, mathvm::FunctionFilter* filter)
{
    FunctionIterator ifun(this);
    while (ifun.hasNext()) {
        BytecodeFunctionE * bfun = dynamic_cast<BytecodeFunctionE*>(ifun.next());
        if (filter == 0 || filter->matches(bfun)) {
            bfun->disassemble(out);
        }
    }
}

using I = mathvm::Instruction;

mathvm::Status* Code::execute(std::vector<mathvm::Var*>& vars)
{
    BytecodeFunctionE * main = dynamic_cast<BytecodeFunctionE*>(functionById(0));
    memory.push();

    for (auto var : vars) {
        uint16_t id = globalVars[var->name()];
        if (var->type() == mathvm::VT_INT) {
            memory.back()[1][id].I = var->getIntValue();
        } else if (var->type() == mathvm::VT_DOUBLE) {
            memory.back()[1][id].D = var->getDoubleValue();
        } else {
            memory.back()[1][id].S = var->getStringValue();
        }
    }

    instructions.push(std::make_pair(main->bytecode(), 0));
    minID.push_back(main->minID());
    bytecode = main->bytecode();
    IP = &instructions.top().second;

    while (hasNextInstruction()) {
        I ins = fetch();
        switch (ins) {
            #define CASE_INS(b, s, l) case I::BC_##b: b(); break;
                FOR_BYTECODES(CASE_INS)
            #undef CASE_INS
            default: break;
        }
    }

    for (auto var : vars) {
        uint16_t id = globalVars[var->name()];
        if (var->type() == mathvm::VT_INT) {
            var->setIntValue(memory.back()[1][id].I);
        } else if (var->type() == mathvm::VT_DOUBLE) {
            var->setDoubleValue(memory.back()[1][id].D);
        } else {
            var->setStringValue(memory.back()[1][id].S);
        }
    }

    return mathvm::Status::Ok();
}

I Code::fetch()
{
    return bytecode->getInsn((*IP)++);
}

bool Code::hasNextInstruction()
{
    if (!instructions.empty()) {
        return (*IP < bytecode->length());
    }

    return false;
}

void Code::INVALID()
{
    std::cerr << "invalid\n";
}

void Code::DLOAD()
{
    Val val;
    val.D = bytecode->getDouble(*IP);
    *IP += 8;
    stack.push(val);
}

void Code::ILOAD()
{
    Val val;
    val.I = bytecode->getInt64(*IP);
    *IP += 8;
    stack.push(val);
}

void Code::SLOAD()
{
    Val val;
    val.S = constantById(bytecode->getUInt16(*IP)).c_str();
    *IP += 2;
    stack.push(val);
}

void Code::DLOAD0()
{
    Val val;
    val.D = 0.0;
    stack.push(val);
}

void Code::ILOAD0()
{
    Val val;
    val.I = 0;
    stack.push(val);
}

void Code::SLOAD0()
{
    Val val;
    val.S = constantById(0).c_str();;
    stack.push(val);
}

void Code::DLOAD1()
{
    Val val;
    val.D = 1;
    stack.push(val);
}

void Code::ILOAD1()
{
    Val val;
    val.I = 1;
    stack.push(val);
}

void Code::DLOADM1()
{
    Val val;
    val.D = -1.0;
    stack.push(val);
}

void Code::ILOADM1()
{
    Val val;
    val.I = -1;
    stack.push(val);
}

void Code::DADD()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.D = l.D + r.D;

    stack.push(v);
}

void Code::IADD()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I + r.I;

    stack.push(v);
}

void Code::DSUB()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.D = l.D - r.D;

    stack.push(v);
}

void Code::ISUB()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I - r.I;

    stack.push(v);
}

void Code::DMUL()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.D = l.D * r.D;

    stack.push(v);
}

void Code::IMUL()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I * r.I;

    stack.push(v);
}

void Code::DDIV()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.D = l.D / r.D;

    stack.push(v);
}

void Code::IDIV()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I / r.I;

    stack.push(v);
}

void Code::IMOD()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I % r.I;

    stack.push(v);
}

void Code::DNEG()
{
    Val v;

    v = stack.top();
    stack.pop();

    v.D = -v.D;

    stack.push(v);
}

void Code::INEG()
{
    Val v;

    v = stack.top();
    stack.pop();

    v.I = -v.I;

    stack.push(v);
}

void Code::IAOR()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I | r.I;

    stack.push(v);
}

void Code::IAAND()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I & r.I;

    stack.push(v);
}

void Code::IAXOR()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I ^ r.I;

    stack.push(v);
}

void Code::IPRINT()
{
    Val v = stack.top();
    stack.pop();

    std::cout << v.I;
}

void Code::DPRINT()
{
    Val v = stack.top();
    stack.pop();

    std::cout << v.D;
}

void Code::SPRINT()
{
    Val v = stack.top();
    stack.pop();

    std::cout << v.S;
}

void Code::I2D()
{
    Val &v = stack.top();
    v.D = (double) v.I;
}

void Code::D2I()
{
    Val &v = stack.top();
    v.I = (double) v.D;
}

void Code::S2I()
{
    // PASS
}

void Code::SWAP()
{
    Val u = stack.top();
    stack.pop();

    Val l = stack.top();
    stack.pop();

    stack.push(u);
    stack.push(l);
}

void Code::POP()
{
    stack.pop();
}

void Code::LOADDVAR0()
{
    stack.push(vars[0]);
}

void Code::LOADDVAR1()
{
    stack.push(vars[1]);
}

void Code::LOADDVAR2()
{
    stack.push(vars[2]);
}

void Code::LOADDVAR3()
{
    stack.push(vars[3]);
}

void Code::LOADIVAR0()
{
    stack.push(vars[0]);
}

void Code::LOADIVAR1()
{
    stack.push(vars[1]);
}

void Code::LOADIVAR2()
{
    stack.push(vars[2]);
}

void Code::LOADIVAR3()
{
    stack.push(vars[3]);
}

void Code::LOADSVAR0()
{
    stack.push(vars[0]);
}

void Code::LOADSVAR1()
{
    stack.push(vars[1]);
}

void Code::LOADSVAR2()
{
    stack.push(vars[2]);
}

void Code::LOADSVAR3()
{
    stack.push(vars[3]);
}

void Code::STOREDVAR0()
{
    vars[0] = stack.top();
    stack.pop();
}

void Code::STOREDVAR1()
{
    vars[1] = stack.top();
    stack.pop();
}

void Code::STOREDVAR2()
{
    vars[2] = stack.top();
    stack.pop();
}

void Code::STOREDVAR3()
{
    vars[3] = stack.top();
    stack.pop();
}

void Code::STOREIVAR0()
{
    vars[0] = stack.top();
    stack.pop();
}

void Code::STOREIVAR1()
{
    vars[1] = stack.top();
    stack.pop();
}

void Code::STOREIVAR2()
{
    vars[2] = stack.top();
    stack.pop();
}

void Code::STOREIVAR3()
{
    vars[3] = stack.top();
    stack.pop();
}

void Code::STORESVAR0()
{
    vars[0] = stack.top();
    stack.pop();
}

void Code::STORESVAR1()
{
    vars[1] = stack.top();
    stack.pop();
}

void Code::STORESVAR2()
{
    vars[2] = stack.top();
    stack.pop();
}

void Code::STORESVAR3()
{
    vars[3] = stack.top();
    stack.pop();
}

void Code::LOADDVAR()
{
    uint16_t id = bytecode->getUInt16(*IP);
    *IP += 2;
    stack.push(vars[id]);
}

void Code::LOADIVAR()
{
    uint16_t id = bytecode->getUInt16(*IP);
    *IP += 2;
    stack.push(vars[id]);
}

void Code::LOADSVAR()
{
    uint16_t id = bytecode->getUInt16(*IP);
    *IP += 2;
    stack.push(vars[id]);
}

void Code::STOREDVAR()
{
    Val val = stack.top();
    stack.pop();
    uint16_t id = bytecode->getUInt16(*IP);
    *IP += 2;
    vars[id] = val;
}

void Code::STOREIVAR()
{
    Val val = stack.top();
    stack.pop();
    uint16_t id = bytecode->getUInt16(*IP);
    *IP += 2;
    vars[id] = val;   
}

void Code::STORESVAR()
{
    Val val = stack.top();
    stack.pop();
    uint16_t id = bytecode->getUInt16(*IP);
    *IP += 2;
    vars[id] = val;
}

void Code::LOADCTXDVAR()
{
    uint16_t scopeID = bytecode->getUInt16(*IP);
    *IP += 2;
    uint16_t varID = bytecode->getUInt16(*IP);
    *IP += 2;
    stack.push(getVal(scopeID, varID));
}

void Code::LOADCTXIVAR()
{
    uint16_t scopeID = bytecode->getUInt16(*IP);
    *IP += 2;
    uint16_t varID = bytecode->getUInt16(*IP);
    *IP += 2;
    stack.push(getVal(scopeID, varID));
}

void Code::LOADCTXSVAR()
{
    uint16_t scopeID = bytecode->getUInt16(*IP);
    *IP += 2;
    uint16_t varID = bytecode->getUInt16(*IP);
    *IP += 2;
    stack.push(getVal(scopeID, varID));
}

void Code::STORECTXDVAR()
{
    Val val = stack.top();
    stack.pop();

    uint16_t scopeID = bytecode->getUInt16(*IP);
    *IP += 2;
    uint16_t varID = bytecode->getUInt16(*IP);
    *IP += 2;

    getVal(scopeID, varID) = val;
}

void Code::STORECTXIVAR()
{
    Val val = stack.top();
    stack.pop();

    uint16_t scopeID = bytecode->getUInt16(*IP);
    *IP += 2;
    uint16_t varID = bytecode->getUInt16(*IP);
    *IP += 2;

    getVal(scopeID, varID) = val;
}

void Code::STORECTXSVAR()
{
    Val val = stack.top();
    stack.pop();

    uint16_t scopeID = bytecode->getUInt16(*IP);
    *IP += 2;
    uint16_t varID = bytecode->getUInt16(*IP);
    *IP += 2;

    getVal(scopeID, varID) = val;
}

void Code::DCMP()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    double d = l.I - r.I;
    d = d ? d / abs(d) : 0;

    v.I = (int) d;

    stack.push(v);
}

void Code::ICMP()
{
    Val r, l, v;

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    v.I = l.I - r.I;
    v.I = v.I ? v.I / abs(v.I) : 0;

    stack.push(v);
}

void Code::JA()
{
    int16_t offset = bytecode->getInt16(*IP);
    *IP += offset;
}

void Code::IFICMPNE()
{
    Val r, l;
    int16_t offset = bytecode->getInt16(*IP);

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    *IP += l.I != r.I ? offset : 2;
}

void Code::IFICMPE()
{
    Val r, l;
    int16_t offset = bytecode->getInt16(*IP);

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    *IP += l.I == r.I ? offset : 2;
}

void Code::IFICMPG()
{
    Val r, l;
    int16_t offset = bytecode->getInt16(*IP);

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    *IP += l.I > r.I ? offset : 2;
}

void Code::IFICMPGE()
{
    Val r, l;
    int16_t offset = bytecode->getInt16(*IP);

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    *IP += l.I >= r.I ? offset : 2;
}

void Code::IFICMPL()
{
    Val r, l;
    int16_t offset = bytecode->getInt16(*IP);

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    *IP += l.I < r.I ? offset : 2;
}

void Code::IFICMPLE()
{
    Val r, l;
    int16_t offset = bytecode->getInt16(*IP);

    l = stack.top();
    stack.pop();

    r = stack.top();
    stack.pop();

    *IP += l.I <= r.I ? offset : 2;
}

void Code::DUMP()
{
    std::cerr << "dump" << " " << stack.top().I << " " << stack.top().D << "\n";
}

void Code::STOP()
{
    // PASS
}

Code::Val& Code::getVal(uint16_t scopeID, uint16_t varID)
{
    if (scopeID < minID.back()) {
        for (int i = memory.size() - 1; i >= 0; --i) {
            if (scopeID < minID[i]) {
                continue;
            }

            return memory[i][scopeID][varID];
        }

        assert(false);
    } else {
        return memory.back()[scopeID][varID];
    }
}

void Code::CALL()
{
    uint16_t funID = bytecode->getUInt16(*IP);
    *IP += 2;
    BytecodeFunctionE * bfun = dynamic_cast<BytecodeFunctionE*>(functionById(funID));
    bytecode = bfun->bytecode();
    instructions.push(std::make_pair(bytecode, 0));
    IP = &instructions.top().second;
    memory.push();
    minID.push_back(bfun->minID());
}

void Code::CALLNATIVE()
{
    // PASS
}

void Code::RETURN()
{
    memory.pop();
    minID.pop_back();
    instructions.pop();
    bytecode = instructions.top().first;
    IP = &instructions.top().second;
}

void Code::BREAK()
{
    // PASS
}