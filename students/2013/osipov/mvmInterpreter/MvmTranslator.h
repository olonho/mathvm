/* 
 * File:   MvmTranslator.h
 * Author: stasstels
 *
 * Created on January 3, 2014, 4:56 PM
 */

#ifndef MVMTRANSLATOR_H
#define	MVMTRANSLATOR_H

#include <stdexcept>
#include <cstdlib>
#include <tr1/memory>

#include "mathvm.h"
#include "ast.h"

namespace mathvm {
    using namespace std;

    struct VarMap;

    struct Val {
        u_int16_t id;
        VarType type;
        string name;
        u_int16_t scopeId;

        void store(Bytecode* dst);

        static Val define(VarMap& vars, uint16_t scopeId, string const& name, VarType type);

        static Val define(VarMap& vars, uint16_t scopeId, AstVar* var) {
            return define(vars, scopeId, var -> name(), var -> type());
        }

        static void unbound(VarMap& vars, Val const& v);
    };

    struct Fun {
        u_int16_t id;
        Signature sign;
        Bytecode* body;
        bool isNative;
    };

    struct VarMap {
        typedef map<string, vector<Val> >::iterator iterator;
        typedef map<string, vector<Val> >::const_iterator c_iterator;

        VarMap() : nextId(new uint16_t(0)) {
        }

        map<string, vector<Val> > varMap;
        std::tr1::shared_ptr<uint16_t> nextId;
    };

    typedef map<string, Fun> FunMap;

    void scopeEnter(Bytecode* nextIns, uint16_t scopeId) {
        nextIns -> addInsn(BC_ILOAD1);
        nextIns -> addInsn(BC_LOADIVAR);
        nextIns -> addUInt16(scopeId);
        nextIns -> addInsn(BC_IADD);
        nextIns -> addInsn(BC_STOREIVAR);
        nextIns -> addUInt16(scopeId);
    }

    void scopeExit(Bytecode* nextIns, uint16_t scopeId) {
        nextIns -> addInsn(BC_LOADIVAR);
        nextIns -> addUInt16(scopeId);
        nextIns -> addInsn(BC_ILOADM1);
        nextIns -> addInsn(BC_IADD);
        nextIns -> addInsn(BC_STOREIVAR);
        nextIns -> addUInt16(scopeId);
    }

    class MvmBytecode : public Code {
        Bytecode* bytecode;

    public:

        MvmBytecode() : bytecode(new Bytecode()) {
        }

        ~MvmBytecode() {
            delete bytecode;
        }

        virtual Status* execute(vector<Var*>& vars) {
            return 0;
        }

        virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0) {
            Code::disassemble(out, filter);
            bytecode -> dump(out);
        }

        Bytecode* getBytecode() {
            return bytecode;
        }

    };

    class MvmTranslator : public Translator {
    public:

        MvmTranslator();

        virtual ~MvmTranslator() {
        }

        virtual Status* translate(const string& program, Code**code);
    };

    class MvmTranslateVisitor : public AstVisitor {
    public:

        void accept(uint16_t _scopeId, MvmBytecode* _code, Bytecode* _nextIns, FunMap& _funcs, VarMap& _vars, VarType _returnType, BlockNode* node) {
            scopeId = _scopeId;
            code = _code;
            nextIns = _nextIns;
            vars = _vars;
            funcs = _funcs;
            returnType = _returnType;
            lastType = VT_INVALID;

            vector<Val> localVars;

            Scope::VarIterator vIt(node -> scope());
            while (vIt.hasNext()) {
                localVars.push_back(Val::define(vars, scopeId, vIt.next()));
            }

            for (Scope::FunctionIterator fIt(node -> scope()); fIt.hasNext();) {
                AstFunction* astFun = fIt.next();
                Fun fun = {0, astFun -> node() -> signature(), 0, false};
                if (astFun -> node() -> body() -> nodeAt(0) -> isNativeCallNode()) {
                    NativeCallNode* native = (NativeCallNode*) astFun -> node() -> body() -> nodeAt(0);
                    fun.id = code -> makeNativeFunction(native -> nativeName(), native -> nativeSignature(), 0);
                    fun.isNative = true;
                } else {
                    BytecodeFunction* bcFun = new BytecodeFunction(astFun);
                    fun.id = _code -> addFunction(bcFun);
                    fun.body = bcFun -> bytecode();
                }
                funcs[astFun -> node() -> name()] = fun;
            }

            for (Scope::FunctionIterator fIt(node -> scope()); fIt.hasNext();) {
                fIt.next() -> node() -> visit(this);
            }

            for (uint32_t i = 0; i < node -> nodes(); ++i) {
                node -> nodeAt(i) -> visit(this);
            }

            for (vector<Val>::iterator it = localVars.begin(); it != localVars.end(); ++it) {
                Val::unbound(vars, *it);
            }

        }

        virtual void visitStringLiteralNode(StringLiteralNode* node) {
            //            cout << "StringLiteralNode " << node -> literal() << endl;
            uint16_t str = code -> makeStringConstant(node -> literal());
            nextIns -> addInsn(BC_SLOAD);
            nextIns -> addUInt16(str);
            lastType = VT_STRING;
        }

        virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
            //            cout << "DoubleLiteralNode " << node->literal() << endl;
            nextIns -> addInsn(BC_DLOAD);
            nextIns -> addDouble(node -> literal());
            lastType = VT_DOUBLE;
        }

        virtual void visitIntLiteralNode(IntLiteralNode* node) {
            //            cout << "IntLiteralNode " << node->literal() << endl;
            nextIns -> addInsn(BC_ILOAD);
            nextIns -> addInt64(node -> literal());
            lastType = VT_INT;
        }

        virtual void visitPrintNode(PrintNode* node) {
            //            cout << "PrintCallNode" << endl;
            for (uint32_t i = 0; i < node -> operands(); ++i) {
                node -> operandAt(i) -> visit(this);
                switch (lastType) {
                    case VT_STRING: nextIns -> addInsn(BC_SPRINT);
                        break;
                    case VT_DOUBLE: nextIns -> addInsn(BC_DPRINT);
                        break;
                    case VT_INT: nextIns -> addInsn(BC_IPRINT);
                        break;
                    default: throw std::runtime_error("Can't print expression of this type");
                }
            }
        }

        virtual void visitBlockNode(BlockNode* node) {
            //            cout << "BlockNode " << endl;
            MvmTranslateVisitor().accept(scopeId, code, nextIns, funcs, vars, returnType, node);
        }

        virtual void visitFunctionNode(FunctionNode* node) {
            //            cout << "FunctionNode " << std::endl;
            Fun& fun = funcs[node -> name()];

            if (fun.isNative) {
                return;
            }

            Val newScope = Val::define(vars, scopeId, "$scopeId", VT_INT);

            scopeEnter(fun.body, newScope.id);
            for (uint32_t i = 0; i < node -> parametersNumber(); ++i) {
                Val p = Val::define(vars, newScope.id, node -> parameterName(i), node->parameterType(i));
                p.store(fun.body);
            }
            MvmTranslateVisitor().accept(scopeId, code, nextIns, funcs, vars, node -> returnType(), node -> body());
            scopeExit(fun.body, newScope.id);
        }



    private:

        u_int16_t scopeId;
        MvmBytecode* code;
        Bytecode* nextIns;
        VarMap vars;
        FunMap funcs;
        VarType returnType;
        VarType lastType;

    };



}

#endif	/* MVMTRANSLATOR_H */

