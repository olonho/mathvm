#include "mathvm.h"
#include "my_interpreter.h"
#include "asmjit/asmjit.h"
#include <stdexcept>
#include <iostream>

namespace mathvm{
    using namespace asmjit;
    using Interpriter::StackItem;
    uint32_t varTypeToAsmJitType(VarType type){
        switch(type){
        case VT_DOUBLE:return asmjit::kX86VarTypeXmmSd;
        case VT_INT:return asmjit::kVarTypeInt64;
        case VT_STRING:return asmjit::kVarTypeInt64;
        default: throw std::runtime_error("Can't convert type to AsmJit type");
        }
    }


    Status * ExecutableCode::execute(std::vector<Var *> &){
        BytecodeFunction * main = (BytecodeFunction *) functionById(0);
        currentScope = new Interpriter::Scope(main, NULL, NULL);
        std::vector<StackItem> stack;
        while(true){
            switch(currentScope->getInstruction()){
            case BC_INVALID: case BC_S2I: case BC_SLOAD0:
                throw std::runtime_error("Invalid instruction");
            case BC_DLOAD:{
                stack.push_back(StackItem::fromDouble(currentScope->getDouble())); break;
            }
            case BC_ILOAD:{
                stack.push_back(StackItem::fromInt(currentScope->getInt())); break;
            }
            case BC_SLOAD:{
                stack.push_back(StackItem::fromConstCharPtr(constantById(currentScope->getUint()).c_str())); break;
            }
            case BC_DLOAD0:{
                stack.push_back(StackItem::fromDouble(0)); break;
            }
            case BC_ILOAD0:{
                stack.push_back(StackItem::fromInt(0)); break;
            }
            case BC_DLOAD1:{
                stack.push_back(StackItem::fromDouble(1)); break;
            }
            case BC_ILOAD1:{
                stack.push_back(StackItem::fromInt(1)); break;
            }
            case BC_DLOADM1:{
                stack.push_back(StackItem::fromDouble(-1)); break;
            }
            case BC_ILOADM1:{
                stack.push_back(StackItem::fromInt(-1)); break;
            }
            case BC_DADD:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left+right));
                break;
            }
            case BC_IADD:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left+right));
                break;
            }
            case BC_DSUB:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left-right));
                break;
            }
            case BC_ISUB:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left-right));
                break;
            }
            case BC_DMUL:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left*right));
                break;
            }
            case BC_IMUL:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left*right));
                break;
            }
            case BC_DDIV:{
                double left = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double right = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(left/right));
                break;
            }
            case BC_IDIV:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left/right));
                break;
            }
            case BC_IMOD:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left%right));
                break;
            }
            case BC_DNEG:{
                double val = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(-val));
                break;
            }
            case BC_INEG:{
                int64_t val = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(-val));
                break;
            }
            case BC_IAOR:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left|right));
                break;
            }
            case BC_IAAND:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left&right));
                break;
            }
            case BC_IAXOR:{
                int64_t left = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t right = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(left^right));
                break;
            }
            case BC_IPRINT:{
                std::cout << stack[stack.size()-1].asInt();
                stack.pop_back();
                break;
            }
            case BC_DPRINT:{
                std::cout << stack[stack.size()-1].asDouble();
                stack.pop_back();
                break;
            }
            case BC_SPRINT:{
                std::cout << stack[stack.size()-1].asConstCharPtr();
                stack.pop_back();
                break;
            }
            case BC_I2D:{
                int64_t val = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromDouble(val));
                break;
            }
            case BC_D2I:{
                double val = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(val));
                break;
            }
            case BC_SWAP:{
                std::swap(stack[stack.size()-1], stack[stack.size()-2]); break;
            }
            case BC_POP:{
                stack.pop_back(); break;
            }
            case BC_LOADDVAR: case BC_LOADIVAR: case BC_LOADSVAR:
            {
                stack.push_back(currentScope->getVar()); break;
            }
            case BC_LOADCTXDVAR: case BC_LOADCTXIVAR: case BC_LOADCTXSVAR:
            {
                uint16_t ctx_id = currentScope->getUint();
                uint16_t var_id = currentScope->getUint();
                stack.push_back(currentScope->getCtxVar(var_id, ctx_id));
                break;
            }
            case BC_STOREDVAR: case BC_STOREIVAR: case BC_STORESVAR:{
                currentScope->setVar(stack[stack.size()-1]);
                stack.pop_back();
                break;
            }
            case BC_STORECTXDVAR: case BC_STORECTXIVAR: case BC_STORECTXSVAR:{
                uint16_t ctx_id = currentScope->getUint();
                uint16_t var_id = currentScope->getUint();
                currentScope->setCtxVar(var_id, ctx_id, stack[stack.size()-1]);
                stack.pop_back();
                break;
            }
            case BC_DCMP:{
                double upper = stack[stack.size()-1].asDouble();
                stack.pop_back();
                double lower = stack[stack.size()-1].asDouble();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(upper == lower ? 0 : upper < lower ? -1 : 1));
                break;
            }
            case BC_ICMP:{
                int64_t upper = stack[stack.size()-1].asInt();
                stack.pop_back();
                int64_t lower = stack[stack.size()-1].asInt();
                stack.pop_back();
                stack.push_back(StackItem::fromInt(upper == lower ? 0 : upper < lower ? -1 : 1));
                break;
            }
            case BC_JA:{
                int16_t offset = currentScope->getInt16();
                currentScope->jump(offset  - sizeof(int16_t));
                break;
            }
            case BC_IFICMPNE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() != stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() == stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPG:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() > stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPGE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() >= stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPL:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() < stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_IFICMPLE:{
                int16_t offset = currentScope->getInt16();
                if(stack[stack.size()-1].asInt() <= stack[stack.size()-2].asInt())
                    currentScope->jump(offset - sizeof(int16_t));
                stack.pop_back();
                stack.pop_back();
                break;
            }
            case BC_CALL:{
                uint16_t id = currentScope->getUint();
                BytecodeFunction *fn = (BytecodeFunction*)functionById(id);
                currentScope = id == currentScope->fn()->id() ?
                            new Interpriter::Scope(fn, currentScope, currentScope->clojure_parent()):
                            new Interpriter::Scope(fn, currentScope, currentScope);
                break;
            }
            case BC_RETURN:{
                Interpriter::Scope * oldScope = currentScope;
                currentScope = currentScope->parent();
                delete oldScope;
                break;
            }
            case BC_STOP:{
                return Status::Ok();
            }
            case BC_CALLNATIVE:{
                uint16_t id = currentScope->getUint();
                const Signature * signature;
                const string * name;
                const void * address = nativeById(id, &signature, &name);
                if(!address){
                    std::cerr << "Native function not found" << std::endl;
                    break;
                }
                const VarType returnType = signature->at(0).first;
                JitRuntime runtime;
                X86Compiler compiler(&runtime);
                FuncBuilderX nativeFnBuilder, mainFnBuilder;
                if (returnType != VT_VOID) {
                    nativeFnBuilder.setRet(varTypeToAsmJitType(returnType));
                    mainFnBuilder.setRet(varTypeToAsmJitType(returnType));
                }
                compiler.addFunc(kFuncConvHost, mainFnBuilder);
                X86GpVar nativeFnPtr(compiler, kVarTypeIntPtr);
                compiler.mov(nativeFnPtr, imm_ptr((void *)address));
                vector<asmjit::Var> arguments;
                for (uint16_t i = 1; i < signature->size(); ++i) {
                    uint32_t varType = varTypeToAsmJitType(signature->at(i).first);
                    nativeFnBuilder.addArg(varType);

                    switch (signature->at(i).first) {
                        case VT_DOUBLE: {
                            X86XmmVar argument(compiler, varType);
                            X86GpVar tmp = compiler.newGpVar();
                            union {
                                double dValue;
                                uint64_t iValue;
                            } val;
                            val.dValue = currentScope->getVarById(i-1).asDouble();
                            compiler.mov(tmp, val.iValue);
                            compiler.movq(argument, tmp.m());
                            compiler.unuse(tmp);
                            arguments.push_back(argument);
                            break;
                        }
                        case VT_INT: {
                            X86GpVar argument(compiler, varType);
                            compiler.mov(argument, currentScope->getVarById(i-1).asInt());
                            arguments.push_back(argument);
                            break;
                        }
                        case VT_STRING: {
                            X86GpVar argument(compiler, varType);
                            compiler.mov(argument, (int64_t)currentScope->getVarById(i-1).asConstCharPtr());
                            arguments.push_back(argument);
                            break;
                        }
                        default: throw std::runtime_error("Bad type");
                    }
                }
                X86CallNode *nativeFnCall = compiler.call(nativeFnPtr, kFuncConvHost, nativeFnBuilder);
                for (size_t i = 0; i < arguments.size(); ++i)
                    nativeFnCall->setArg(i, arguments[i]);

                switch (returnType) {
                case VT_VOID:
                      compiler.ret();
                      break;
                case VT_DOUBLE: {
                      X86XmmVar rvar(compiler, varTypeToAsmJitType(returnType));
                      nativeFnCall->setRet(0, rvar);
                      compiler.ret(rvar);
                      break;
                }
                case VT_INT:
                case VT_STRING: {
                      X86GpVar rvar(compiler, varTypeToAsmJitType(returnType));
                      nativeFnCall->setRet(0, rvar);
                      compiler.ret(rvar);
                      break;
                }
                default: throw std::runtime_error("Bad type");
                }

                compiler.endFunc();
                void *mainFnPtr = compiler.make();
                switch (returnType) {
                case VT_VOID:
                    asmjit_cast<void (*)()>(mainFnPtr)();
                    break;
                case VT_DOUBLE: {
                    double result = asmjit_cast<double (*)()>(mainFnPtr)();
                    stack.push_back(StackItem::fromDouble(result));
                    break;
                }
                case VT_INT: {
                    int64_t result = asmjit_cast<int64_t (*)()>(mainFnPtr)();
                    stack.push_back(StackItem::fromInt(result));
                    break;
                }
                case VT_STRING: {
                    const char * result = asmjit_cast<const char * (*)()>(mainFnPtr)();
                    stack.push_back(StackItem::fromConstCharPtr(result));
                    break;
                }
                default: throw std::runtime_error("Bad type");
                }

                runtime.release(mainFnPtr);
                break;
            }
            default:{
                std::cerr << "Unimplemented instruction" << std::endl;
            }
            }

        }
        return Status::Ok();
    }
}
