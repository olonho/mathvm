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

            cerr << "can't get max type bw " << l << " and " << r << endl;
           return VT_INVALID; 
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
            //visit scope first

            curfun->setScopeId((*code)->fresh_abs_var_context());
            for (uint i = 0 ; i < n->parametersNumber(); ++i){
                (*code)->present_var(n->parameterName(i), n->parameterType(i));
            }
			n->visitChildren(this);
            if (n->name() == "<top>"){
                curfun->bytecode()->addInsn(BC_STOP);
			}
            //(*code)->addFunction(curfun);


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
                    cerr << "for int add insn" << n->kind() << endl;
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
                default:
                    cerr << "for int add insn" << n->kind() << endl;
                }
            } else {
                cerr << "Double opp with bad type: " << m_t << endl;
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
                            cerr << "add type for unary - " << top_type << endl;
                    }
                    break;
                default:
                    cerr << "add unary op: " << n->kind() << endl;
            }
		}

        virtual void visitStringLiteralNode(StringLiteralNode *n){
            cerr << "string literal: " << n->literal() << endl;

            curfun->bytecode()->addInsn(BC_SLOAD);
            curfun->bytecode()->addInt16((*code)->makeStringConstant(n->literal().c_str()));

            top_type = VT_STRING;
		}

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *n){
            cerr << "double literal: "<< n->literal() << endl;
            curfun->bytecode()->addInsn(BC_DLOAD);
            curfun->bytecode()->addDouble(n->literal());

            top_type = VT_DOUBLE;
		}
        virtual void visitIntLiteralNode(IntLiteralNode *n){
            cerr <<"int literal: "<< n->literal() << endl;
            curfun->bytecode()->addInsn(BC_ILOAD);
            curfun->bytecode()->addInt64(n->literal());
            top_type = VT_INT;

		}
        virtual void visitLoadNode(LoadNode *n){
            cerr << "load node: " << n->var()->name() << endl;
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
                break; //todo throw error
            }
        }

        virtual void visitStoreNode(StoreNode *n){
			n->value()->visit(this);
            int16_t varid = (*code)->get_var_by_name(n->var()->name());
            //todo switch on n->op()
            cerr << "store node: " << n->var()->name() << endl;
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
                break; //todo throw error
            }
        }

        virtual void visitForNode(ForNode *n){
            Label end(curfun->bytecode());

            Label start(curfun->bytecode());
            curfun->bytecode()->bind(start);
            n->inExpr()->visit(this);
            curfun->bytecode()->addInsn(BC_ILOAD0);
            curfun->bytecode()->addBranch(BC_IFICMPE, end);
			n->body()->visit(this);
            curfun->bytecode()->bind(end);

		}
        virtual void visitWhileNode(WhileNode *n){
            Label end(curfun->bytecode());
            Label start(curfun->bytecode());
            curfun->bytecode()->bind(start);
            n->whileExpr()->visit(this);
            curfun->bytecode()->addInsn(BC_ILOAD0);
            curfun->bytecode()->addBranch(BC_IFICMPE, end);
            n->loopBlock()->visit(this);
            curfun->bytecode()->bind(end);
		}

        virtual void visitIfNode(IfNode *n){
            Label _else(curfun->bytecode());
            Label end(curfun->bytecode());

            n->ifExpr()->visit(this);
            curfun->bytecode()->addInsn(BC_ILOAD0);
            curfun->bytecode()->addBranch(BC_IFICMPE, _else);

            n->thenBlock()->visit(this);
            curfun->bytecode()->addBranch(BC_JA, end);

            curfun->bytecode()->bind(_else);
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

            //todo check for scope
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
            }
            curfun->bytecode()->addInsn(BC_RETURN);
		}
        virtual void visitCallNode(CallNode *n){
            TranslatedFunction* f = (*code)->functionByName(n->name());

			for(uint i = 0; i < n->parametersNumber(); ++i){
				n->parameterAt(i)->visit(this);
                int16_t vid = (*code)->get_var_by_name(f->parameterName(i), f->scopeId());
                switch(f->parameterType(i)){
                case VT_DOUBLE:
                    curfun->bytecode()->addInsn(BC_STOREDVAR);
                    curfun->bytecode()->addInt16(vid);
                    break;
                case VT_INT:
                    curfun->bytecode()->addInsn(BC_STOREIVAR);
                    curfun->bytecode()->addInt16(vid);
                    break;
                case VT_STRING:
                    curfun->bytecode()->addInsn(BC_STORESVAR);
                    curfun->bytecode()->addInt16(vid);
                    break;
                default:
                    break; //todo throw error
                }
			}

            curfun->bytecode()->addInsn(BC_CALL);
            curfun->bytecode()->addInt16(f->id());
		}
        virtual void visitNativeCallNode(NativeCallNode *n){
            void* func = dlsym(RTLD_DEFAULT, n->nativeName().c_str());
            if (!func) {} //todo throw exc
            int16_t fid = (*code)->makeNativeFunction(n->nativeName(), n->nativeSignature(), func);
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
                    break; //todo throw error
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
        parser.top()->node()->visit(tv);
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
