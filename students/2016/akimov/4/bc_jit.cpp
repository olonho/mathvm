#include "bc_jit.h"
#include <ast.h>

#include <set>

using namespace mathvm;
using namespace asmjit;


static void loadArgsFromMem(const Signature* signature, X86Compiler& c, X86GpVar& mem, vector<X86Var>& vars) {
    for (size_t j = 0; j < signature->size() - 1; ++j) {
        switch (signature->at(j + 1).first) {
            case VT_STRING:
            case VT_INT: {
                X86GpVar v = c.newInt64();
                c.mov(v, x86::qword_ptr(mem, j * 8));
                vars.push_back(v);
                break;
            }
            case VT_DOUBLE: {
                X86XmmVar v = c.newXmm();
                c.movsd(v, x86::qword_ptr(mem, j * 8));
                vars.push_back(v);
                break;
            }
        }
    }
}

static FuncBuilderX builderFromSignature(const Signature* signature) {
    FuncBuilderX builder;
    switch (signature->at(0).first) {
        case VT_INT:
            builder.setRetT<int64_t>();
            break;
        case VT_DOUBLE:
            builder.setRetT<double>();
            break;
        case VT_STRING:
            builder.setRetT<const char*>();
            break;
        case VT_VOID:
            builder.setRetT<void>();
            break;
    }
    for (size_t j = 1; j < signature->size(); ++j) {
        switch (signature->at(j).first) {
            case VT_INT:
                builder.addArgT<int64_t>();
                break;
            case VT_DOUBLE:
                builder.addArgT<double>();
                break;
            case VT_STRING:
                builder.addArgT<const char*>();
                break;
        }
    }
    builder.setCallConv(kCallConvHost);
    return builder;
}

void* InterpreterCodeImpl::processNativeFunction(const Signature* signature, const void* ptr) {
    X86Assembler a(getRuntime());
    X86Compiler c(&a);
    //FileLogger logger(stderr);
    //a.setLogger(&logger);

    FuncBuilderX builder = builderFromSignature(signature);

    c.addFunc(FuncBuilder1<void, void*>(kCallConvHost));
    X86GpVar stack = c.newIntPtr();
    c.setArg(0, stack);

    vector<X86Var> vars;
    loadArgsFromMem(signature, c, stack, vars);

    X86CallNode* call = c.call(imm_ptr(ptr), builder);
    for (size_t i = 0; i < vars.size(); ++i) {
        call->setArg(i, vars[i]);
    }

    switch (signature->at(0).first) {
        case VT_STRING:
        case VT_INT: {
            X86GpVar retVar = c.newInt64();
            call->setRet(0, retVar);
            c.mov(x86::qword_ptr(stack), retVar);
            break;
        }
        case VT_DOUBLE: {
            X86XmmVar v = c.newXmm();
            call->setRet(0, v);
            c.movsd(x86::qword_ptr(stack), v);
            break;
        }
        case VT_VOID:
            break;
    }
    c.ret();
    c.endFunc();

    c.finalize();

    return a.make();
}



set<uint32_t> getLabelOffsets(Bytecode* bc) {
    set<uint32_t> labelOffsets;
    for (uint32_t bci = 0; bci < bc->length();) {
        Instruction insn = bc->getInsn(bci);
        size_t length;
        bytecodeName(insn, &length);
        switch (insn) {
            case BC_IFICMPNE:
            case BC_IFICMPE:
            case BC_IFICMPG:
            case BC_IFICMPGE:
            case BC_IFICMPL:
            case BC_IFICMPLE:
            case BC_JA: {
                labelOffsets.insert(bc->getInt16(bci + 1) + bci + 1);
                break;
            }
            default:
                break;
        }
        bci += length;
    }
    return labelOffsets;
}

struct stack_t {
    stack_t(X86Compiler& c, X86GpVar& stack)
            : c(c)
            , stack(stack)
    {}

    void pushVal(void* data) {
        int64_t value = *(int64_t*)data;

        X86GpVar tmp = c.newInt64();
        c.mov(tmp, imm(value));

        c.mov(x86::qword_ptr(stack), tmp);
        c.unuse(tmp);

        c.add(stack, 8);
    }

    void pushGp(X86GpVar& var) {
        c.mov(x86::qword_ptr(stack), var);
        c.add(stack, 8);
    }

    void pushXmm(X86XmmVar& var) {
        c.movsd(x86::qword_ptr(stack), var);
        c.add(stack, 8);
    }

    void popGp(X86GpVar& var) {
        c.sub(stack, 8);
        c.mov(var, x86::qword_ptr(stack));
    }

    void popXmm(X86XmmVar& var) {
        c.sub(stack, 8);
        c.movsd(var, x86::qword_ptr(stack));
    }

    void pop() {
        c.sub(stack, 8);
    }

private:
    X86Compiler& c;
    X86GpVar& stack;
};

void print_i(int64_t value) { cout << value; cout.flush(); }
void print_d(double value) { cout << value; cout.flush(); }
void print_s(const char* value) { cout << value; cout.flush(); }
int64_t icmp(int64_t a, int64_t b) { return a == b ? 0 : a < b ? -1 : 1; }
int64_t dcmp(double a, double b) { return a == b ? 0 : a < b ? -1 : 1; }
double i2d(int64_t value) { return static_cast<double>(value); }
int64_t d2i(double value) { return static_cast<int64_t>(value); }

void InterpreterCodeImpl::processFunction(BytecodeFunction* func) {
    Bytecode* bc = func->bytecode();
    uint16_t scopeId = func->scopeId();
    uint16_t funcId = func->id();

    X86Assembler aa(getRuntime());
    X86Compiler c(&aa);
    //FileLogger logger(stderr);
    //aa.setLogger(&logger);

    c.addFunc(FuncBuilder1<void, void*>(kCallConvHost));
    X86GpVar stack = c.newIntPtr();
    c.setArg(0, stack);

    X86GpVar mem_map = c.newIntPtr();
    c.mov(mem_map, imm_ptr(_scopes));
    X86GpVar functions = c.newIntPtr();
    c.mov(functions, imm_ptr(_functions));

    stack_t st(c, stack);

    map<uint32_t, asmjit::Label> labels;
    for (uint32_t offset : getLabelOffsets(bc)) {
        labels[offset] = c.newLabel();
    }

    uint32_t bci = 0;
    while (bci < bc->length()) {
        if (labels.count(bci)) {
            c.bind(labels[bci]);
        }

        Instruction insn = bc->getInsn(bci);
        size_t length;
        bytecodeName(insn, &length);

        switch (insn) {

#define OPL(T, v) { T value = v; st.pushVal(&value); }
            case BC_DLOAD: OPL(double, bc->getDouble(bci + 1)); break;
            case BC_ILOAD: OPL(int64_t, bc->getInt64(bci + 1)); break;
            case BC_SLOAD: OPL(const char*, constantById(bc->getUInt16(bci + 1)).c_str()); break;

            case BC_DLOAD0: OPL(double, 0.0); break;
            case BC_ILOAD0: OPL(int64_t, 0); break;
            case BC_DLOAD1: OPL(double, 1.0); break;
            case BC_ILOAD1: OPL(int64_t, 1); break;
            case BC_DLOADM1: OPL(double, -1.0); break;
            case BC_ILOADM1: OPL(int64_t, -1); break;
            case BC_SLOAD0: OPL(const char*, constantById(0).c_str()); break;

#define OP2I(op) { X86GpVar a = c.newInt64();  \
                   st.popGp(a);                \
                   X86GpVar b = c.newInt64();  \
                   st.popGp(b);                \
                   c.op(a, b);                 \
                   st.pushGp(a);               \
                   c.unuse(a);                 \
                   c.unuse(b);                 \
                 }
            case BC_IADD: OP2I(add); break;
            case BC_ISUB: OP2I(sub); break;
            case BC_IMUL: OP2I(imul); break;
            case BC_IAOR: OP2I(or_); break;
            case BC_IAAND: OP2I(and_); break;
            case BC_IAXOR: OP2I(xor_); break;
            case BC_IDIV: {
                X86GpVar a = c.newInt64();
                st.popGp(a);
                X86GpVar b = c.newInt64();
                st.popGp(b);
                X86GpVar r = c.newInt64();
                c.mov(r, imm(0));

                c.idiv(r, a, b);
                st.pushGp(a);

                c.unuse(a);
                c.unuse(b);
                c.unuse(r);
                break;
            }
            case BC_IMOD: {
                X86GpVar a = c.newInt64();
                st.popGp(a);
                X86GpVar b = c.newInt64();
                st.popGp(b);
                X86GpVar r = c.newInt64();
                c.mov(r, imm(0));

                c.idiv(r, a, b);
                st.pushGp(r);

                c.unuse(a);
                c.unuse(b);
                c.unuse(r);
                break;
            }
            case BC_INEG: {
                X86GpVar a = c.newInt64();
                st.popGp(a);
                c.neg(a);
                st.pushGp(a);
                c.unuse(a);
                break;
            }
#define OP2D(op) { X86XmmVar a = c.newXmm(); \
                   st.popXmm(a);             \
                   X86XmmVar b = c.newXmm(); \
                   st.popXmm(b);             \
                   c.op(a, b);               \
                   st.pushXmm(a);            \
                   c.unuse(a);               \
                   c.unuse(b);               \
                 }
            case BC_DADD: OP2D(addsd); break;
            case BC_DSUB: OP2D(subsd); break;
            case BC_DMUL: OP2D(mulsd); break;
            case BC_DDIV: OP2D(divsd); break;
            case BC_DNEG: OPL(double, 0.0); OP2D(subsd); break;

            case BC_IPRINT: {
                X86GpVar a = c.newInt64();
                st.popGp(a);
                X86CallNode* call = c.call(imm_ptr(&print_i), FuncBuilder1<void, int64_t>(kCallConvHost));
                call->setArg(0, a);
                c.unuse(a);
                break;
            }
            case BC_DPRINT: {
                X86XmmVar a = c.newXmm();
                st.popXmm(a);
                X86CallNode* call = c.call(imm_ptr(&print_d), FuncBuilder1<void, double>(kCallConvHost));
                call->setArg(0, a);
                c.unuse(a);
                break;
            }
            case BC_SPRINT: {
                X86GpVar a = c.newIntPtr();
                st.popGp(a);
                X86CallNode* call = c.call(imm_ptr(&print_s), FuncBuilder1<void, const char*>(kCallConvHost));
                call->setArg(0, a);
                c.unuse(a);
                break;
            }

            case BC_I2D: {
                X86GpVar a = c.newInt64();
                st.popGp(a);
                X86XmmVar r = c.newXmm();
                X86CallNode* call = c.call(imm_ptr(&i2d), FuncBuilder1<double, int64_t>(kCallConvHost));
                call->setArg(0, a);
                call->setRet(0, r);
                st.pushXmm(r);
                c.unuse(a);
                c.unuse(r);
                break;
            }
            case BC_D2I: {
                X86XmmVar a = c.newXmm();
                st.popXmm(a);
                X86GpVar r = c.newInt64();
                X86CallNode* call = c.call(imm_ptr(&d2i), FuncBuilder1<int64_t, double>(kCallConvHost));
                call->setArg(0, a);
                call->setRet(0, r);
                st.pushGp(r);
                c.unuse(a);
                c.unuse(r);
                break;
            }
            case BC_S2I: break;

            case BC_SWAP: {
                X86GpVar a = c.newInt64();
                st.popGp(a);
                X86GpVar b = c.newInt64();
                st.popGp(b);

                st.pushGp(a);
                st.pushGp(b);

                c.unuse(a);
                c.unuse(b);
                break;
            }
            case BC_POP: {
                st.pop();
                break;
            }

            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR: {
                uint16_t scope = bc->getUInt16(bci + 1);
                uint16_t var = bc->getUInt16(bci + 3);
                X86GpVar a = c.newIntPtr();
                c.mov(a, x86::qword_ptr(mem_map, scope * 8));
                c.mov(a, x86::qword_ptr(a, var * 8));
                st.pushGp(a);
                c.unuse(a);
                break;
            }
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR: {
                uint16_t scope = bc->getUInt16(bci + 1);
                uint16_t var = bc->getUInt16(bci + 3);
                X86GpVar p = c.newIntPtr();
                X86GpVar a = c.newInt64();
                st.popGp(a);
                c.mov(p, x86::qword_ptr(mem_map, scope * 8));
                c.mov(x86::qword_ptr(p, var * 8), a);
                c.unuse(a);
                c.unuse(p);
                break;
            }

            case BC_DCMP: {
                X86XmmVar a = c.newXmm();
                st.popXmm(a);
                X86XmmVar b = c.newXmm();
                st.popXmm(b);
                st.pushXmm(b);
                st.pushXmm(a);
                X86GpVar r = c.newInt64();
                X86CallNode* call = c.call(imm_ptr(&dcmp),
                                           FuncBuilder2<int64_t, double, double>(kCallConvHost));
                call->setArg(0, a);
                call->setArg(1, b);
                call->setRet(0, r);
                st.pushGp(r);
                c.unuse(a);
                c.unuse(b);
                c.unuse(r);
                break;
            }
            case BC_ICMP: {
                X86GpVar a = c.newInt64();
                st.popGp(a);
                X86GpVar b = c.newInt64();
                st.popGp(b);
                st.pushGp(b);
                st.pushGp(a);
                X86GpVar r = c.newInt64();
                X86CallNode* call = c.call(imm_ptr(&icmp),
                                           FuncBuilder2<int64_t, int64_t, int64_t>(kCallConvHost));
                call->setArg(0, a);
                call->setArg(1, b);
                call->setRet(0, r);
                st.pushGp(r);
                c.unuse(a);
                c.unuse(b);
                c.unuse(r);
                break;
            }

            case BC_JA: {
                uint32_t offset = bci + 1 + bc->getInt16(bci + 1);
                c.jmp(labels[offset]);
                break;
            }

#define JMP(op) { uint32_t offset = bci + 1 + bc->getInt16(bci + 1); \
                  X86GpVar a = c.newInt64();  \
                  st.popGp(a);                \
                  X86GpVar b = c.newInt64();  \
                  st.popGp(b);                \
                  st.pushGp(b);               \
                  st.pushGp(a);               \
                  c.cmp(a, b);                \
                  c.op(labels[offset]);       \
                  c.unuse(a);                 \
                  c.unuse(b);                 \
                }
            case BC_IFICMPNE: JMP(jne); break;
            case BC_IFICMPE:  JMP(je); break;
            case BC_IFICMPG:  JMP(jg); break;
            case BC_IFICMPGE: JMP(jge); break;
            case BC_IFICMPL:  JMP(jl); break;
            case BC_IFICMPLE: JMP(jle); break;

            case BC_DUMP: break;

            case BC_STOP: {
                X86GpVar ret = c.newInt64();
                c.mov(ret, imm(0));
                X86CallNode* call = c.call(imm_ptr(&exit), FuncBuilder1<void, int>(kCallConvHost));
                call->setArg(0, ret);
                c.unuse(ret);
                break;
            }

            case BC_CALL: {
                uint16_t functionId = bc->getUInt16(bci + 1);
                BytecodeFunction* callFunc = functionById(functionId);
                uint16_t callArgs = callFunc->parametersNumber();
                uint16_t callScopeId = callFunc->scopeId();

                X86GpVar old_scope = c.newIntPtr();
                c.mov(old_scope, x86::qword_ptr(mem_map, callScopeId * 8));

                // now stack refers to arguments
                c.sub(stack, callArgs * 8);

                // first variables of scope = arguments
                X86GpVar new_scope = c.newIntPtr();
                c.mov(new_scope, stack);

                // all scope variables just below stack
                X86GpVar new_stack = c.newIntPtr();
                c.mov(new_stack, new_scope);
                c.add(new_stack, _scopeSize[callScopeId] * 8);

                // save new scope
                c.mov(x86::qword_ptr(mem_map, callScopeId * 8), new_scope);
                c.unuse(new_scope);

                X86GpVar p = c.newIntPtr();
                c.mov(p, x86::qword_ptr(functions, functionId * 8));

                X86CallNode* call = c.call(p, FuncBuilder1<void, void*>(kCallConvHost));
                call->setArg(0, new_stack);

                c.unuse(p);

                // restore old scope back
                c.mov(x86::qword_ptr(mem_map, callScopeId * 8), old_scope);
                c.unuse(old_scope);

                if (callFunc->returnType() != VT_VOID) {
                    X86GpVar ret = c.newInt64();
                    c.mov(ret, x86::qword_ptr(new_stack));
                    c.mov(x86::qword_ptr(stack), ret);
                    c.add(stack, 8);
                    c.unuse(ret);
                }

                c.unuse(new_stack);
                break;
            }

            case BC_CALLNATIVE: {
                uint16_t functionId = bc->getUInt16(bci + 1);
                const Signature* signature;
                const string* name;
                const void* nativePtr = nativeById(functionId, &signature, &name);

                X86GpVar scope = c.newIntPtr();
                c.mov(scope, x86::qword_ptr(mem_map, scopeId * 8));

                X86CallNode* call = c.call(imm_ptr(nativePtr), FuncBuilder1<void, void*>(kCallConvHost));
                call->setArg(0, scope);

                if (signature->at(0).first != VT_VOID) {
                    X86GpVar ret = c.newInt64();
                    c.mov(ret, x86::qword_ptr(scope));
                    c.mov(x86::qword_ptr(stack), ret);
                    c.add(stack, 8);
                    c.unuse(ret);
                }

                break;
            }

            case BC_RETURN: {
                c.ret();
                break;
            }

            case BC_BREAK:
            case BC_INVALID:
            default:
                break;
        }

        bci += length;
    }
    c.endFunc();
    c.finalize();
    _functions[funcId] = aa.make();
}

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
    size_t functionsCount = 0;
    for (Code::FunctionIterator it(this); it.hasNext();) {
        TranslatedFunction* function = it.next();
        uint16_t scope = function->scopeId();

        if (scope >= _scopeSize.size()) {
            _scopeSize.resize(scope + 1);
        }
        _scopeSize[scope] = function->localsNumber();

        ++functionsCount;
    }
    unique_ptr<void*> functions(new void*[functionsCount]);
    _functions = functions.get();
    unique_ptr<void*> scopes(new void*[_scopeSize.size()]);
    _scopes = scopes.get();

    for (Code::FunctionIterator it(this); it.hasNext();) {
        TranslatedFunction* function = it.next();
        BytecodeFunction* bc_function = functionById(function->id());
        processFunction(bc_function);
    }

    BytecodeFunction* topFunction = functionByName(AstFunction::top_name);
    uint16_t functionId = topFunction->id();
    uint16_t scopeId = topFunction->scopeId();

    unique_ptr<void*> stack(new void*[64 * 1024 * 1024]);
    void** scope_ptr = stack.get();
    void** stack_ptr = scope_ptr + _scopeSize[functionId];

    for (size_t i = 0; i< vars.size(); ++i) {
        switch (vars[i]->type()) {
            case VT_INT:
                *(int64_t*)(scope_ptr + i) = vars[i]->getIntValue();
                break;
            case VT_DOUBLE:
                *(double*)(scope_ptr + i) = vars[i]->getDoubleValue();
                break;
            case VT_STRING:
                *(const char**)(scope_ptr + i) = vars[i]->getStringValue();
                break;
        }
    }

    _scopes[scopeId] = scope_ptr;
    asmjit_cast<void (*)(void*)>(_functions[functionId])(stack_ptr);

    return Status::Ok();
}


