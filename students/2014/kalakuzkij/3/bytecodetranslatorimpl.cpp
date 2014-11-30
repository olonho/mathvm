#include "mathvm.h"
#include "parser.h"
#include "interpretercodeimpl.h"
#include "visitors.h"
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <iostream>
using std::cerr;
using std::endl;
#include <map>
using std::map;
using std::make_pair;
using std::pair;
#include "asmjit/asmjit.h"
using namespace asmjit;
using namespace asmjit::x86;
#include <stdexcept>
using std::runtime_error;

namespace mathvm{
	class TranslatorVisitor: public AstVisitor{
		private:
		typedef unsigned int uint;
		InterpreterCodeImpl** code;
        BytecodeFunction * curfun;
        VarType top_type;


        VarType max_type(const VarType l, const VarType r){
            if ( l == r) return l;
            if ( l == VT_INT && r == VT_DOUBLE) return VT_DOUBLE;
            if ( l == VT_DOUBLE && r == VT_INT) return VT_DOUBLE;
            if ( (l == VT_STRING && r == VT_INT) ||
                 (r == VT_STRING && l == VT_INT) ) return VT_INT;

           return VT_INVALID; 
        }

        VarType convert_type(VarType from, VarType to)
        {
            if (from != to){
                switch(from){
                case VT_INT:
                    if (to == VT_DOUBLE){
                        curfun->bytecode()->addInsn(BC_I2D);
                    } else {
                        throw runtime_error("Can't convert from int");
                    }
                    break;
                case VT_DOUBLE:
                    if (to == VT_INT){
                        curfun->bytecode()->addInsn(BC_D2I);
                    } else {
                        throw runtime_error("Can't convert from double");
                    }
                    break;
                case VT_STRING:
                    if (to == VT_INT){
                        curfun->bytecode()->addInsn(BC_S2I);
                    } else {
                        throw runtime_error("Cant convert from string");
                    }
                    break;
                default:
                    throw runtime_error("Unknown type convertion");
                    break;
                }
            }
            return from;
        }

        public:
        TranslatorVisitor(BytecodeFunction* fun, InterpreterCodeImpl** c):
            AstVisitor(),
            code(c),
            curfun(fun)
        {
            (*code)->addFunction(curfun);
        }

        virtual void visitFunctionNode(FunctionNode * n){
            curfun->setScopeId((*code)->fresh_abs_var_context());
            for (uint i = 0 ; i < n->parametersNumber(); ++i){
                (*code)->present_var(n->parameterName(i), n->parameterType(i));
            }
			n->visitChildren(this);
            if (n->name() == "<top>"){
                curfun->bytecode()->addInsn(BC_STOP);
			}
		}
        
        virtual void visitBinaryOpNode(BinaryOpNode *n){
            n->left()->visit(this);
            VarType l_t = top_type;

            n->right()->visit(this);
            VarType r_t = top_type;

            VarType m_t = max_type(l_t, r_t);

            top_type = m_t;
            if ( l_t != m_t && l_t == VT_INT){
                curfun->bytecode()->addInsn(BC_SWAP);
                curfun->bytecode()->addInsn(BC_I2D);
                curfun->bytecode()->addInsn(BC_SWAP);
            }
            if (r_t != m_t && r_t == VT_INT){
                curfun->bytecode()->addInsn(BC_I2D);
            }
            if (n->kind() >= tEQ && n->kind() <= tLE){
                Label end(curfun->bytecode());
                Label _false(curfun->bytecode());
                if (top_type == VT_INT){
                    curfun->bytecode()->addInsn(BC_ICMP);
                } else if(top_type == VT_DOUBLE) {
                    curfun->bytecode()->addInsn(BC_DCMP);
                } else {
                    throw runtime_error("Bad type in comparision operation");
                }
                if (n->kind() == tEQ || n->kind() == tNEQ){
                    curfun->bytecode()->addInsn(BC_ILOAD0);
                } else if (n->kind() == tLT || n->kind() == tGE){
                    curfun->bytecode()->addInsn(BC_ILOAD1);
                } else if (n->kind() == tGT || n->kind() == tLE){
                    curfun->bytecode()->addInsn(BC_ILOADM1);
                } else {
                    throw runtime_error("Bad type in comparision operation");
                }
                if (n->kind() == tEQ || n->kind() == tLT || n->kind() == tGT){
                    curfun->bytecode()->addBranch(BC_IFICMPE, _false);
                }else {
                    curfun->bytecode()->addBranch(BC_IFICMPNE, _false);
                }

                curfun->bytecode()->addInsn(BC_POP);
                curfun->bytecode()->addInsn(BC_POP);
                curfun->bytecode()->addInsn(BC_ILOAD0);
                curfun->bytecode()->addBranch(BC_JA, end);
                curfun->bytecode()->bind(_false);
                curfun->bytecode()->addInsn(BC_POP);
                curfun->bytecode()->addInsn(BC_POP);
                curfun->bytecode()->addInsn(BC_ILOAD1);
                curfun->bytecode()->bind(end);
                top_type = VT_INT;
            } else
            if (m_t == VT_DOUBLE){
                switch (n->kind()){
                case tADD:
                    curfun->bytecode()->addInsn(BC_DADD);
                    break;
                case tSUB:
                    curfun->bytecode()->addInsn(BC_DSUB);
                    break;
                case tDIV:
                    curfun->bytecode()->addInsn(BC_DDIV);
                    break;
                case tMUL:
                    curfun->bytecode()->addInsn(BC_DMUL);
                    break;
                default:
                    throw runtime_error("Bad token in binary operation");
                    break;
                }
            } else if(m_t == VT_INT){
                switch (n->kind()){
                case tADD:
                    curfun->bytecode()->addInsn(BC_IADD);
                    break;
                case tSUB:
                    curfun->bytecode()->addInsn(BC_ISUB);
                    break;
                case tDIV:
                    curfun->bytecode()->addInsn(BC_IDIV);
                    break;
                case tMUL:
                    curfun->bytecode()->addInsn(BC_IMUL);
                    break;
                case tMOD:
                    curfun->bytecode()->addInsn(BC_IMOD);
                    break;
                case tAOR:
                    curfun->bytecode()->addInsn(BC_IAOR);
                    break;
                case tAAND:
                    curfun->bytecode()->addInsn(BC_IAAND);
                    break;
                case tAXOR:
                    curfun->bytecode()->addInsn(BC_IAXOR);
                    break;
                case tOR:{
                    Label end(curfun->bytecode());
                    Label _true1(curfun->bytecode());
                    Label _true2(curfun->bytecode());
                    curfun->bytecode()->addInsn(BC_ILOAD0);
                    curfun->bytecode()->addBranch(BC_IFICMPNE, _true1);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_ILOAD0);
                    curfun->bytecode()->addBranch(BC_IFICMPNE, _true2);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_ILOAD0);
                    curfun->bytecode()->addBranch(BC_JA, end);
                    curfun->bytecode()->bind(_true1);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->bind(_true2);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_ILOAD1);
                    curfun->bytecode()->bind(end);
                    top_type = VT_INT;
                    break;
                    }
                case tAND:{
                    Label end(curfun->bytecode());
                    Label _false1(curfun->bytecode());
                    Label _false2(curfun->bytecode());
                    curfun->bytecode()->addInsn(BC_ILOAD0);
                    curfun->bytecode()->addBranch(BC_IFICMPE, _false1);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_ILOAD0);
                    curfun->bytecode()->addBranch(BC_IFICMPE, _false2);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_ILOAD1);
                    curfun->bytecode()->addBranch(BC_JA, end);
                    curfun->bytecode()->bind(_false1);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->bind(_false2);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_POP);
                    curfun->bytecode()->addInsn(BC_ILOAD0);
                    curfun->bytecode()->bind(end);
                    top_type = VT_INT;
                    break;
                }
                default:
                    throw runtime_error("Bad token in binary operation");
                    break;
                }
            } else {
                throw runtime_error("Bad type in binary operation");
            }
		}
        virtual void visitUnaryOpNode(UnaryOpNode *n){
			n->visitChildren(this);	
            switch (n->kind()){
                case tSUB:
                    switch(top_type){
                        case VT_INT:
                            curfun->bytecode()->addInsn(BC_INEG);
                            break;
                        case VT_DOUBLE:
                            curfun->bytecode()->addInsn(BC_DNEG);
                            break;
                        default:
                            throw runtime_error("Bad type in unary operation");
                            break;
                    }
                    break;
                case tNOT:
                    if (top_type ==  VT_INT || top_type == VT_STRING) {
                        Label end(curfun->bytecode());
                        Label zero(curfun->bytecode());
                        if (top_type == VT_STRING){
                            curfun->bytecode()->addInsn(BC_S2I);
                        }
                        curfun->bytecode()->addInsn(BC_ILOAD0);
                        curfun->bytecode()->addBranch(BC_IFICMPNE, zero);
                        curfun->bytecode()->addInsn(BC_POP);
                        curfun->bytecode()->addInsn(BC_POP);
                        curfun->bytecode()->addInsn(BC_ILOAD1);
                        curfun->bytecode()->addBranch(BC_JA, end);
                        curfun->bytecode()->bind(zero);
                        curfun->bytecode()->addInsn(BC_POP);
                        curfun->bytecode()->addInsn(BC_POP);
                        curfun->bytecode()->addInsn(BC_ILOAD0);
                        curfun->bytecode()->bind(end);
                    } else {
                        throw runtime_error("Bad type in unary operation");
                    }
                    break;
                default:
                    throw runtime_error("Bad token in unary operation");
                    break;
            }
		}

        virtual void visitStringLiteralNode(StringLiteralNode *n){
            curfun->bytecode()->addInsn(BC_SLOAD);
            char * tmp = new char[n->literal().size()+1];
            strcpy(tmp, n->literal().c_str());
            curfun->bytecode()->addInt16((*code)->ptrmap.add(tmp));
            top_type = VT_STRING;
		}

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *n){
            curfun->bytecode()->addInsn(BC_DLOAD);
            curfun->bytecode()->addDouble(n->literal());
            top_type = VT_DOUBLE;
		}
        virtual void visitIntLiteralNode(IntLiteralNode *n){
            curfun->bytecode()->addInsn(BC_ILOAD);
            curfun->bytecode()->addInt64(n->literal());
            top_type = VT_INT;
		}

        virtual void visitLoadNode(LoadNode *n){
            int16_t vid = (*code)->get_var_by_name(n->var()->name());
            top_type = n->var()->type();
            switch (n->var()->type()){
            case VT_INT:
                curfun->bytecode()->addInsn(BC_LOADIVAR);
                curfun->bytecode()->addInt16(vid);
                break;
            case VT_DOUBLE:
                curfun->bytecode()->addInsn(BC_LOADDVAR);
                curfun->bytecode()->addInt16(vid);
                break;
            case VT_STRING:
                curfun->bytecode()->addInsn(BC_LOADSVAR);
                curfun->bytecode()->addInt16(vid);
                break;
            default:
                throw runtime_error("Bad load var type");
                break;
            }
        }

        virtual void visitStoreNode(StoreNode *n){
            int16_t varid = (*code)->get_var_by_name(n->var()->name());
            if (n->op() != tASSIGN){
                switch (n->op()){
                case tINCRSET:
                    switch(n->var()->type()){
                    case VT_INT:
                        curfun->bytecode()->addInsn(BC_LOADIVAR);
                        curfun->bytecode()->addInt16(varid);
                        break;
                    case VT_DOUBLE:
                        curfun->bytecode()->addInsn(BC_LOADDVAR);
                        curfun->bytecode()->addInt16(varid);
                        break;
                    default:
                        throw runtime_error("Bad store var type");
                        break;
                    }
                    break;
                case tDECRSET:
                    switch(n->var()->type()){
                    case VT_INT:
                        curfun->bytecode()->addInsn(BC_LOADIVAR);
                        curfun->bytecode()->addInt16(varid);
                        break;
                    case VT_DOUBLE:
                        curfun->bytecode()->addInsn(BC_LOADDVAR);
                        curfun->bytecode()->addInt16(varid);
                        break;
                    default:
                        throw runtime_error("Bad store var type");
                        break;
                    }
                    break;
                default:
                    throw runtime_error("Bad store token type");
                    break;
                }
            }
            n->value()->visit(this);
            switch (n->var()->type()){
            case VT_INT:
                if (top_type == VT_DOUBLE){
                    curfun->bytecode()->addInsn(BC_D2I);
                }else if (top_type == VT_STRING){
                    curfun->bytecode()->addInsn(BC_S2I);
                } else if (top_type != VT_INT){
                    throw runtime_error("Bad store var type. Can't convert value to int");
                }
                break;
            case VT_DOUBLE:
                if (top_type == VT_INT){
                    curfun->bytecode()->addInsn(BC_I2D);
                }else if (top_type != VT_DOUBLE){
                    throw runtime_error("Bad store var type. Can't convert value to double");
                }
                break;
            case VT_STRING:
                if (top_type != VT_STRING){
                    throw runtime_error("Bad store var type. Can't convert value to string");
                }
                break;
            default:
                throw runtime_error("Bad store var type.");
                break;
            }
            if (n->op() != tASSIGN){
                if (n->op() == tINCRSET){
                    if (n->var()->type() == VT_INT){
                        curfun->bytecode()->addInsn(BC_IADD);
                    }else{
                        curfun->bytecode()->addInsn(BC_DADD);

                    }
               }else{
                    if (n->var()->type() == VT_INT){
                        curfun->bytecode()->addInsn(BC_ISUB);
                    }else{
                        curfun->bytecode()->addInsn(BC_DSUB);
                    }
                }
            }

            switch (n->var()->type()){
            case VT_INT:
                curfun->bytecode()->addInsn(BC_STOREIVAR);
                curfun->bytecode()->addInt16(varid);
                break;
            case VT_DOUBLE:
                curfun->bytecode()->addInsn(BC_STOREDVAR);
                curfun->bytecode()->addInt16(varid);
                break;
            case VT_STRING:
                curfun->bytecode()->addInsn(BC_STORESVAR);
                curfun->bytecode()->addInt16(varid);
                break;
            default:
                throw runtime_error("Bad store var type.");
                break;
            }
        }

        virtual void visitForNode(ForNode *n){
            Label end(curfun->bytecode());
            if (!n->inExpr()->isBinaryOpNode() || n->inExpr()->asBinaryOpNode()->kind() != tRANGE){
                throw runtime_error("Invalid for expression");
            }
            n->inExpr()->asBinaryOpNode()->left()->visit(this);
            if (top_type != VT_INT) {
                throw runtime_error("Invalid var type in for expression");
            }

            int varid = (*code)->get_var_by_name(n->var()->name());
            curfun->bytecode()->addInsn(BC_STOREIVAR);
            curfun->bytecode()->addInt16(varid);

            n->inExpr()->asBinaryOpNode()->right()->visit(this);
            if (top_type != VT_INT) {
                throw runtime_error("Invalid type in range expression");
            }

            Label start(curfun->bytecode());
            curfun->bytecode()->bind(start);
            // condition
            curfun->bytecode()->addInsn(BC_LOADIVAR);
            curfun->bytecode()->addInt16(varid);
            curfun->bytecode()->addBranch(BC_IFICMPG, end);
            curfun->bytecode()->addInsn(BC_POP);
            // body
            n->body()->visit(this);
            // increment and jump back
            curfun->bytecode()->addInsn(BC_LOADIVAR);
            curfun->bytecode()->addInt16(varid);
            curfun->bytecode()->addInsn(BC_ILOAD1);
            curfun->bytecode()->addInsn(BC_IADD);
            curfun->bytecode()->addInsn(BC_STOREIVAR);
            curfun->bytecode()->addInt16(varid);
            curfun->bytecode()->addBranch(BC_JA, start);
            curfun->bytecode()->bind(end);
            curfun->bytecode()->addInsn(BC_POP);
            curfun->bytecode()->addInsn(BC_POP);

		}
        virtual void visitWhileNode(WhileNode *n){
            Label end(curfun->bytecode());
            Label start(curfun->bytecode());
            curfun->bytecode()->bind(start);
            n->whileExpr()->visit(this);
            curfun->bytecode()->addInsn(BC_ILOAD0);
            curfun->bytecode()->addBranch(BC_IFICMPE, end);
            curfun->bytecode()->addInsn(BC_POP);
            curfun->bytecode()->addInsn(BC_POP);
            n->loopBlock()->visit(this);
            curfun->bytecode()->addBranch(BC_JA, start);
            curfun->bytecode()->bind(end);
            curfun->bytecode()->addInsn(BC_POP);
            curfun->bytecode()->addInsn(BC_POP);
		}

        virtual void visitIfNode(IfNode *n){
            Label _else(curfun->bytecode());
            Label end(curfun->bytecode());

            n->ifExpr()->visit(this);
            curfun->bytecode()->addInsn(BC_ILOAD0);
            curfun->bytecode()->addBranch(BC_IFICMPE, _else);
            curfun->bytecode()->addInsn(BC_POP);
            curfun->bytecode()->addInsn(BC_POP);
            n->thenBlock()->visit(this);
            curfun->bytecode()->addBranch(BC_JA, end);

            curfun->bytecode()->bind(_else);
            curfun->bytecode()->addInsn(BC_POP);
            curfun->bytecode()->addInsn(BC_POP);
            if (n->elseBlock()) {
                n->elseBlock()->visit(this);
            }
            curfun->bytecode()->bind(end);
		}

        virtual void visitBlockNode(BlockNode *n){
            Scope* curscope = n->scope();
            Scope::VarIterator scope_vars(curscope);
            int varcount = 0;
            //(*code)->fresh_abs_var_context();
            while(scope_vars.hasNext()){
                AstVar * av = scope_vars.next();
                (*code)->present_var(av->name(), av->type());
                ++varcount;
            }
            curfun->setLocalsNumber(curfun->localsNumber() + varcount);

            Scope::FunctionIterator scope_funs(curscope);
            vector<pair<int16_t, AstFunction*> > fresh_functions;
            while(scope_funs.hasNext()){
                AstFunction* af = scope_funs.next();
                BytecodeFunction* bf = new BytecodeFunction(af);
                fresh_functions.push_back(make_pair((*code)->addFunction(bf), af));
            }


			for (uint i = 0; i < n->nodes(); ++i){
				n->nodeAt(i)->visit(this);
            }
            for (uint i = 0; i < fresh_functions.size(); ++i){
                BytecodeFunction * bf = (BytecodeFunction*)(*code)->functionById(fresh_functions[i].first);
                BytecodeFunction * tmp = curfun;
                curfun = bf;
                fresh_functions[i].second->node()->visit(this);
                curfun = tmp;
            }
		}


        virtual void visitReturnNode(ReturnNode *n){
            if (n->returnExpr()){
                n->returnExpr()->visit(this);
                top_type = convert_type(top_type, curfun->returnType());
            }

            curfun->bytecode()->addInsn(BC_RETURN);
        }

        virtual void visitCallNode(CallNode *n){
            TranslatedFunction* f = (*code)->functionByName(n->name());

			for(uint i = 0; i < n->parametersNumber(); ++i){
				n->parameterAt(i)->visit(this);
                convert_type(top_type, f->parameterType(i));
			}

            curfun->bytecode()->addInsn(BC_CALL);
            curfun->bytecode()->addInt16(f->id());

            top_type = f->returnType();
        }

        virtual void visitNativeCallNode(NativeCallNode *n){
            void* func = dlsym(RTLD_DEFAULT, n->nativeName().c_str());
            if (!func){
                throw runtime_error((std::string("Can't find native function ") + n->nativeName().c_str()).c_str());
            }
            void * compiled_func;
            //compile
            {
                vector<uint> ints;
                vector<uint> doubles;
                vector<uint> stack_args;


                for (size_t i = 1; i < n->nativeSignature().size(); ++i) {
                    switch(n->nativeSignature()[i].first) {
                        case VT_DOUBLE:
                            if (doubles.size() < 8) {
                                doubles.push_back(i);
                            } else {
                                stack_args.push_back(i);
                            }

                            break;
                        case VT_STRING:
                        case VT_INT:
                            if (ints.size() < 6) {
                                ints.push_back(i);
                            } else {
                                stack_args.push_back(i);
                            }
                            break;
                        default:
                            throw runtime_error("Bad type in function signature");
                            break;
                    }
                }

                X86Assembler assembler((*code)->getRuntime());

                static X86GpReg int_regs[6] = { rdi, rsi, rdx, rcx, r8, r9 };
                static X86XmmReg double_regs[8] = { xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7 };

                // rule of thumb: (used stack size + 8 % 16) == 0, i.e. (used stack slots & 1) != 0
                size_t stack_delta = (stack_args.size() + !(stack_args.size() & 1)) * sizeof(long long int);

                assembler.sub(rsp, imm(stack_delta));
                assembler.mov(r11, rdi); // first argument is a pointer to begin offset of arguments

                for (size_t i = 0; i < ints.size(); ++i) {
                    assembler.mov(int_regs[i], ptr(r11, ints[i] * sizeof(long long int)));
                }

                for (size_t i = 0; i < doubles.size(); ++i) {
                    assembler.movsd(double_regs[i], ptr(r11, doubles[i] * sizeof(long long int)));
                }

                for (size_t i = 0; i < stack_args.size(); ++i) {
                    assembler.mov(r10, ptr(r11, stack_args[i] * sizeof(long long int)));
                    assembler.mov(ptr(rsp, i * sizeof(long long int)), r10);
                }

                assembler.mov(rax, imm(reinterpret_cast<long long int>(func)));
                assembler.call(rax);

                assembler.add(rsp, stack_delta);
                assembler.ret();
                compiled_func = assembler.make();
            }
            if (!compiled_func){
                throw runtime_error("Can't build native function");
            }
            int16_t fid = (*code)->makeNativeFunction(n->nativeName(), n->nativeSignature(), compiled_func);
            (*code)->nativeScopeIds[fid] = curfun->scopeId();
            curfun->bytecode()->addInsn(BC_CALLNATIVE);
            curfun->bytecode()->addInt16(fid);
        }

        virtual void visitPrintNode(PrintNode *n){
			for (uint i = 0; i < n->operands(); ++i){
				n->operandAt(i)->visit(this);
                switch(top_type){
                case VT_DOUBLE:
                    curfun->bytecode()->addInsn(BC_DPRINT);
                    break;
                case VT_INT:
                    curfun->bytecode()->addInsn(BC_IPRINT);
                    break;
                case VT_STRING:
                    curfun->bytecode()->addInsn(BC_SPRINT);
                    break;
                default:
                    throw runtime_error("Bad print type.");
                    break;
                }
			}

		}

	};

    Status* BytecodeTranslatorImpl::translate(const string & program, Code* *code){
        return translateBytecode(program, (InterpreterCodeImpl**)(code));
    }
	
    Status* BytecodeTranslatorImpl::translateBytecode(const string & program, InterpreterCodeImpl* *code){
        Parser parser;
        Status* st = parser.parseProgram(program);
        if (!st->isOk()) return st;

        *code = new InterpreterCodeImpl();   
        TranslatorVisitor* tv = new TranslatorVisitor(new BytecodeFunction(parser.top()), code);
        try{
            parser.top()->node()->visit(tv);
        } catch (runtime_error re){
            delete tv;
            return Status::Error(re.what());
        }

        delete tv;
        return Status::Ok();
    }
	
	Translator* Translator::create(const string& impl){
	 if (impl == ""){
	   return new BytecodeTranslatorImpl();
	 } else {
	  return NULL;
	 }
	}
}  
