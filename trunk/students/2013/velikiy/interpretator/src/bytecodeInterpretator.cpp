
#include "bytecodeInterpretator.h"

#include <iostream>
#include <stdlib.h>

#include <inttypes.h>
#include <stdlib.h>
#include "mathvm.h"
#include <iomanip>
#include <stdio.h>
#include <stack>

using namespace std;

namespace mathvm {

    struct ExecContext {
        size_t beforeBci;
        size_t bci;
        FunctionContex* context;
        const BytecodeFunction* fun;
        OuterContexts contexts;
    };

    int64_t S64(const char *s) {
        return (int64_t) strtoll(s, NULL, 0);
    }

    Status* BytecodeInterpretator::interpretate(const BytecodeCode& code,
            vector<Var*>& vars) {
        Code::FunctionIterator fi(&code);
        while (fi.hasNext()) {
            functions.push_back((BytecodeFunction*) fi.next());
        }

        Code::ConstantIterator ci(&code);
        while (ci.hasNext()) {
            constants.push_back(&ci.next());
        }

        if (functions.empty()) {
            return new Status("Nothing to execute", 0);
        }

        execStatus = NULL;

        setRootVars(code, vars);
        execFunction(functions[0], OuterContexts());

        callDepth = 0;

        return execStatus;
    }

    void BytecodeInterpretator::setRootVars(const BytecodeCode& code, vector<Var*>& vars) {
        BytecodeFunction* fun = new BytecodeFunction(*functions[0]);
        functions[0] = fun;

        for (size_t i = 0; i < vars.size(); i++) {
            string name(vars[i]->name());
            assert(code.globalVars()->find(name) != code.globalVars()->end());
            uint16_t pos = code.globalVars()->find(name)->second;
            if (vars[i]->type() == VT_INT) {
                fun->bytecode()->setTyped<int64_t>(pos, vars[i]->getIntValue());
            }
            if (vars[i]->type() == VT_DOUBLE) {
                fun->bytecode()->setTyped<double>(pos, vars[i]->getDoubleValue());
            }
            if (vars[i]->type() == VT_STRING) {
                constants.push_back(new string(vars[i]->getStringValue()));
                uint16_t id = constants.size();
                fun->bytecode()->setTyped<uint16_t>(pos, id);
            }
        }
    }

    void BytecodeInterpretator::execFunction(const BytecodeFunction* fun, OuterContexts contexts) {

        double dv;
        double dv2;
        int64_t iv;
        int64_t iv2;
        uint16_t idv;
        uint16_t idv2;
        DataBytecode* d = &dstack;
        FunctionContex* context;
        size_t beforeBci;
        size_t bci;

        stack<ExecContext> execStack;


EXECFUNCTION:


        context = new FunctionContex(fun);
        OuterContexts deeperContexts(contexts);
        deeperContexts[fun->id()] = context;

        int id = fun->id();
        if (id == 1) {
            int kkk = 10;
        }

        const Bytecode* b = fun->bytecode();

        {
            // read params 
            size_t dc = 0, ic = 0, sc = 0;
            for (uint16_t i = 0; i < fun->parametersNumber(); i++) {
                VarType type = fun->parameterType(i);
                if (type == VT_DOUBLE) {
                    dv = d->popd();
                    context->setd(dc++, dv);
                }
                if (type == VT_INT) {
                    iv = d->popi();
                    context->seti(ic++, iv);
                }
                if (type == VT_STRING) {
                    idv = d->popid();
                    context->sets(sc++, idv);
                }
            }
        }

        beforeBci = dstack.length();

        size_t length;
        Instruction insn;
        const char* name;
        bool jumpCase;

        for (bci = 0; bci < b->length();) {



            if (bci == 30 && fun->id() == 1) {
                int kkk = 10;
            }

            insn = b->getInsn(bci);
            name = bytecodeName(insn, &length);
            jumpCase = false;

            switch (insn) {
                case BC_INVALID:
                    break;

                    // CASTS
                case BC_I2D:
                    iv = d->popi();
                    d->pushd((double) iv);
                    break;
                case BC_D2I:
                    dv = d->popd();
                    d->pushi((int64_t) iv);
                    break;
                case BC_S2I:
                    idv = d->popid();
                    idv = b->getInt16(bci + 1) + bci + 1;
                    d->pushd(S64(constants[iv]->c_str()));
                    break;

                    // STACK LOAD
                case BC_DLOAD:
                    dv = b->getDouble(bci + 1);
                    d->pushd(dv);
                    break;
                case BC_ILOAD:
                    iv = b->getInt64(bci + 1);
                    d->pushi(iv);
                    break;
                case BC_SLOAD:
                    idv = b->getUInt16(bci + 1);
                    d->pushid(idv);
                    break;

                    // STACK LOAD
                case BC_ILOAD0:
                    d->pushi(0);
                    break;
                case BC_ILOAD1:
                    d->pushi(1);
                    break;
                case BC_DLOAD0:
                    d->pushd(0);
                    break;
                case BC_DLOAD1:
                    d->pushd(1);
                    break;

                    // VAR LOADS
                case BC_LOADIVAR:
                    idv = b->getUInt16(bci + 1);
                    dstack.pushi(context->geti(idv));
                    break;
                case BC_LOADDVAR:
                    idv = b->getUInt16(bci + 1);
                    dstack.pushd(context->getd(idv));
                    break;
                case BC_LOADSVAR:
                    idv = b->getUInt16(bci + 1);
                    dstack.pushid(context->gets(idv));
                    break;

                    // VAR STORES
                case BC_STOREDVAR:
                    idv = b->getUInt16(bci + 1);
                    dv = dstack.popd();
                    context->setd(idv, dv);
                    break;
                case BC_STOREIVAR:
                    idv = b->getUInt16(bci + 1);
                    iv = dstack.popi();
                    context->seti(idv, iv);
                    break;
                case BC_STORESVAR:
                    idv = b->getUInt16(bci + 1);
                    idv2 = dstack.popid();
                    context->sets(idv, idv2);
                    break;

                    // VAR LOAD (outer context)
                case BC_LOADCTXDVAR:
                {
                    uint16_t idContext = b->getUInt16(bci + 1);
                    uint16_t idVar = b->getUInt16(bci + 3);
                    dv = contexts[idContext]->getd(idVar);
                }
                    d->pushd(dv);
                    break;
                case BC_LOADCTXIVAR:
                {
                    uint16_t idContext = b->getUInt16(bci + 1);
                    uint16_t idVar = b->getUInt16(bci + 3);
                    iv = contexts[idContext]->geti(idVar);
                }
                    d->pushi(iv);
                    break;
                case BC_LOADCTXSVAR:
                {
                    uint16_t idContext = b->getUInt16(bci + 1);
                    uint16_t idVar = b->getUInt16(bci + 3);
                    idv = contexts[idContext]->gets(idVar);
                }
                    d->pushid(idv);
                    break;

                    // VAR STORE (outer context)
                case BC_STORECTXDVAR:
                    dv = d->popd();
                {
                    uint16_t idContext = b->getUInt16(bci + 1);
                    uint16_t idVar = b->getUInt16(bci + 3);
                    contexts[idContext]->setd(idVar, dv);
                }
                    break;
                case BC_STORECTXIVAR:
                    iv = d->popi();
                {
                    uint16_t idContext = b->getUInt16(bci + 1);
                    uint16_t idVar = b->getUInt16(bci + 3);
                    contexts[idContext]->seti(idVar, iv);
                }
                    break;
                case BC_STORECTXSVAR:
                    idv = d->popid();
                {
                    uint16_t idContext = b->getUInt16(bci + 1);
                    uint16_t idVar = b->getUInt16(bci + 3);
                    contexts[idContext]->sets(idVar, idv);
                }
                    break;

                    // JUMPS
                case BC_JA:
                    idv = b->getInt16(bci + 1) + bci + 1;
                    bci = idv;
                    jumpCase = true;
                    break;

                case BC_IFICMPNE:
                    idv = b->getInt16(bci + 1) + bci + 1;
                    iv2 = d->popi();
                    iv = d->popi();
                    if (iv != iv2) {
                        bci = idv;
                        jumpCase = true;
                    }
                    break;
                case BC_IFICMPE:
                    idv = b->getInt16(bci + 1) + bci + 1;
                    iv2 = d->popi();
                    iv = d->popi();
                    if (iv == iv2) {
                        bci = idv;
                        jumpCase = true;
                    }
                    break;
                case BC_IFICMPG:
                    idv = b->getInt16(bci + 1) + bci + 1;
                    iv2 = d->popi();
                    iv = d->popi();
                    if (iv > iv2) {
                        bci = idv;
                        jumpCase = true;
                    }
                    break;
                case BC_IFICMPGE:
                    idv = b->getInt16(bci + 1) + bci + 1;
                    iv2 = d->popi();
                    iv = d->popi();
                    if (iv >= iv2) {
                        bci = idv;
                        jumpCase = true;
                    }
                    break;
                case BC_IFICMPL:
                    idv = b->getInt16(bci + 1) + bci + 1;
                    iv2 = d->popi();
                    iv = d->popi();
                    if (iv < iv2) {
                        bci = idv;
                        jumpCase = true;
                    }
                    break;
                case BC_IFICMPLE:
                    idv = b->getInt16(bci + 1) + bci + 1;
                    iv2 = d->popi();
                    iv = d->popi();
                    if (iv <= iv2) {
                        bci = idv;
                        jumpCase = true;
                    }
                    break;

                    // ARITHMETIC
                case BC_DADD:
                    dv2 = d->popd();
                    dv = d->popd();
                    d->pushd(dv + dv2);
                    break;
                case BC_DSUB:
                    dv2 = d->popd();
                    dv = d->popd();
                    d->pushd(dv - dv2);
                    break;
                case BC_DMUL:
                    dv2 = d->popd();
                    dv = d->popd();
                    d->pushd(dv * dv2);
                    break;
                case BC_DDIV:
                    dv2 = d->popd();
                    dv = d->popd();
                    d->pushd(dv / dv2);
                    break;
                case BC_DCMP:
                    dv2 = d->popd();
                    dv = d->popd();
                    if (dv < dv2)
                        d->pushi(-1);
                    if (dv == dv2)
                        d->pushi(0);
                    if (dv > dv2)
                        d->pushi(1);
                    break;


                case BC_IADD:
                    iv2 = d->popi();
                    iv = d->popi();
                    d->pushi(iv + iv2);
                    break;
                case BC_ISUB:
                    iv2 = d->popi();
                    iv = d->popi();
                    d->pushi(iv - iv2);
                    break;
                case BC_IMUL:
                    iv2 = d->popi();
                    iv = d->popi();
                    d->pushi(iv * iv2);
                    break;
                case BC_IDIV:
                    iv2 = d->popi();
                    iv = d->popi();
                    d->pushi(iv / iv2);
                    break;
                case BC_IAAND:
                    iv2 = d->popi();
                    iv = d->popi();
                    d->pushi(iv & iv2);
                    break;
                case BC_IAOR:
                    iv2 = d->popi();
                    iv = d->popi();
                    d->pushi(iv | iv2);
                    break;
                case BC_IAXOR:
                    iv2 = d->popi();
                    iv = d->popi();
                    d->pushi(iv ^ iv2);
                    break;
                case BC_ICMP:
                    iv2 = d->popi();
                    iv = d->popi();
                    if (iv < iv2)
                        d->pushi(-1);
                    if (iv == iv2)
                        d->pushi(0);
                    if (iv > iv2)
                        d->pushi(1);
                    break;

                    // SWAP
                case BC_ISWAP:
                    iv = d->popi();
                    iv2 = d->popi();
                    d->pushi(iv);
                    d->pushi(iv2);
                    break;
                case BC_DSWAP:
                    dv = d->popd();
                    dv2 = d->popd();
                    d->pushi(dv);
                    d->pushi(dv2);
                    break;
                case BC_SSWAP:
                    idv = d->popid();
                    idv2 = d->popid();
                    d->pushid(idv);
                    d->pushid(idv2);
                    break;

                    // PRINT
                case BC_DPRINT:
                    dv = d->popd();
                    cout << dv;
                    break;
                case BC_IPRINT:
                    iv = d->popi();
                    cout << iv;
                    break;
                case BC_SPRINT:
                    idv = d->popid() - 1;
                    cout << *constants[idv];
                    break;

                case BC_CALL:
                    idv = b->getUInt16(bci + 1);



                {
                    ExecContext ec;
                    ec.beforeBci = beforeBci;
                    ec.bci = bci;
                    ec.context = context;
                    ec.fun = fun;
                    ec.contexts = contexts;
                    execStack.push(ec);
                }

                    fun = functions[idv];
                    contexts = deeperContexts;


                    if (id == 1 && idv == 3) {
                        int ikkkasdf = 12;
                    }
                    if (id == 1 && idv == 2) {
                        int ikkkasdf = 12;
                    }

                    goto EXECFUNCTION;

RETURNFROMEXE:



                    deeperContexts = contexts;

                {
                    ExecContext ec = execStack.top();
                    execStack.pop();
                    beforeBci = ec.beforeBci;
                    bci = ec.bci;
                    context = ec.context;
                    fun = ec.fun;
                    contexts = ec.contexts;
                }

                    b = fun->bytecode();
                    insn = b->getInsn(bci);
                    name = bytecodeName(insn, &length);
                    jumpCase = false;

                    break;
                case BC_CALLNATIVE:
                    execStatus = new Status("Can't call a native function", 0);
                    return;
                    break;

                case BC_RETURN:
                    goto RETURN;
                    break;
                default:
                    cout << name << endl;
                    // assert(false);
            }
            if (!jumpCase)
                bci += length;
        }

RETURN:

        // dropping everything except return value

        if (fun->returnType() == VT_DOUBLE)
            dv = d->popd();
        if (fun->returnType() == VT_INT)
            iv = d->popi();
        if (fun->returnType() == VT_STRING)
            idv = d->popid();

        dstack.dropToSize(beforeBci);

        if (fun->returnType() == VT_DOUBLE)
            d->pushd(dv);
        if (fun->returnType() == VT_INT)
            d->pushi(iv);
        if (fun->returnType() == VT_STRING)
            d->pushid(idv);

        delete context;


        if (!execStack.empty())
            goto RETURNFROMEXE;

    }

    BytecodeInterpretator::~BytecodeInterpretator() {
        delete functions[0];
    }

}