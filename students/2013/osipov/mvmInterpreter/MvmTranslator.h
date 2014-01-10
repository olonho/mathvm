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

#include "MvmInterpreter.h"

namespace mathvm {
    using namespace std;

    struct Val;
    struct Fun;
    struct VarMap;
    typedef map<string, Fun> FunMap;

    void scopeEnter(Bytecode* nextIns, uint16_t scopeId);
    void scopeExit(Bytecode* nextIns, uint16_t scopeId);

    struct Val {
        u_int16_t id;
        VarType type;
        string name;
        u_int16_t scopeId;

        void store(Bytecode* dst);
        void load(Bytecode* dst);
        void unbound(VarMap& vars);

        static Val define(VarMap& vars, uint16_t scopeId, string const& name, VarType type);

        static Val define(VarMap& vars, uint16_t scopeId, AstVar* var) {
            return define(vars, scopeId, var -> name(), var -> type());
        }

        static Val defScope(VarMap& vars, Bytecode* nextIns);


        static Val fromScope(const VarMap& vars, const string& name);
    };

    struct Fun {
        u_int16_t id;
        Signature sign;
        Bytecode* body;
        bool isNative;

        static Fun fromScope(const FunMap& funcs, const string& name) {
            FunMap::const_iterator it = funcs.find(name);
            if (it == funcs.end()) {
                throw runtime_error("Function not found");
            }
            return it -> second;
        }
    };

    struct VarMap {
        typedef map<string, vector<Val> >::iterator iterator;
        typedef map<string, vector<Val> >::const_iterator c_iterator;

        VarMap() : nextId(new uint16_t(0)) {
        }

        map<string, vector<Val> > varMap;
        std::tr1::shared_ptr<uint16_t> nextId;
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
                it -> unbound(vars);
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

            Val newScope = Val::defScope(vars, nextIns);

            scopeEnter(fun.body, newScope.id);
            for (uint32_t i = 0; i < node -> parametersNumber(); ++i) {
                Val p = Val::define(vars, newScope.id, node -> parameterName(i), node->parameterType(i));
                p.store(fun.body);
            }
            MvmTranslateVisitor().accept(scopeId, code, nextIns, funcs, vars, node -> returnType(), node -> body());
        }

        virtual void visitStoreNode(StoreNode* node) {
            //            cout << "StoreNode: " << node -> var() -> name() << " Type: " << typeToName(node -> var() -> type()) << endl;
            Val v = Val::fromScope(vars, node -> var() -> name());
            if (node -> op() == tINCRSET || node -> op() == tDECRSET) {
                v.load(nextIns);
            }
            visitExpression(node -> value(), v.type);
            switch (node -> op()) {
                case tASSIGN: break;
                case tINCRSET:
                    nextIns -> addInsn(v.type == VT_INT ? BC_IADD : BC_DADD);
                    break;
                case tDECRSET:
                    nextIns -> addInsn(v.type == VT_INT ? BC_ISUB : BC_DSUB);
                    break;
                default:
                    throw runtime_error("Unknown Operator");
            }
            v.store(nextIns);
        }

        void visitExpression(AstNode* expr, VarType expected) {
            expr -> visit(this);
            checkType(lastType, expected);
        }

        virtual void visitLoadNode(LoadNode* node) {
            //            cout << "LoadNode: " << node -> var() -> name() << " Type: " << typeToName(node -> var() -> type()) << endl;
            Val v = Val::fromScope(vars, node -> var() -> name());
            v.load(nextIns);
            lastType = node -> var() -> type();
        }

        virtual void visitCallNode(CallNode* node) {
            //            cout << "CallNode " << node -> name() << endl;
            Fun fun = Fun::fromScope(funcs, node -> name());
            if (fun.sign.size() - 1 != node -> parametersNumber()) {
                throw runtime_error("Invalid signature");
            }
            for (int i = node -> parametersNumber() - 1; i >= 0; --i) {
                node -> parameterAt(i) -> visit(this);
                checkType(lastType, fun.sign[i + 1].first);
            }
            nextIns -> addInsn(fun.isNative ? BC_CALLNATIVE : BC_CALL);
            nextIns -> addUInt16(fun.id);
            lastType = fun.sign[0].first;
        }

        virtual void visitIfNode(IfNode* node) {
            //            cout << "IfNode " << std::endl;
            Label alter(nextIns);
            Label end(nextIns);


            visitExpression(node -> ifExpr(), VT_INT);
            nextIns -> addInsn(BC_ILOAD0);
            nextIns -> addBranch(BC_IFICMPE, alter);

            node -> thenBlock() -> visit(this);
            // Jump to end
            nextIns -> addBranch(BC_JA, end);

            nextIns -> bind(alter);
            if (node->elseBlock()) {
                node->elseBlock()->visit(this);
            }
            nextIns -> bind(end);
        }

        virtual void visitWhileNode(WhileNode* node) {
            //            cout << "WhileNode " << endl;
            Label start(nextIns -> currentLabel());
            Label end(nextIns);

            // Condition
            visitExpression(node -> whileExpr(), VT_INT);
            nextIns -> addInsn(BC_ILOAD0);
            nextIns -> addBranch(BC_IFICMPE, end);
            // Body
            node -> loopBlock() -> visit(this);
            nextIns -> addBranch(BC_JA, start);

            // Bind loop end 
            nextIns -> bind(end);
        }

        virtual void visitForNode(ForNode* node) {
            //            cout << "ForNode " << endl;

            Val iter = Val::fromScope(vars, node -> var() -> name());
            Val from = Val::define(vars, scopeId, "$from", VT_INT);
            Val to = Val::define(vars, scopeId, "$to", VT_INT);


            BinaryOpNode* range = (BinaryOpNode*) node -> inExpr();
            if (range -> kind() != tRANGE) {
                throw runtime_error("Expected range expression");

            }

            // From
            visitExpression(range -> left(), VT_INT);
            from.store(nextIns);
            //To
            visitExpression(range -> right(), VT_INT);
            to.store(nextIns);

            //For iter = From
            from.load(nextIns);
            iter.store(nextIns);

            // Labels
            Label start(nextIns -> currentLabel());
            Label end(nextIns);

            // iter <= To
            iter.load(nextIns);
            to.load(nextIns);
            nextIns -> addBranch(BC_IFICMPG, end);

            // Body
            MvmTranslateVisitor().accept(scopeId, code, nextIns, funcs, vars, returnType, node -> body());

            // iter += 1

            iter.load(nextIns);
            nextIns -> addInsn(BC_ILOAD1);
            nextIns -> addInsn(BC_IADD);
            iter.store(nextIns);

            // Start
            nextIns -> addBranch(BC_JA, start);

            // Bind labels && clean scope
            nextIns -> bind(end);
            from.unbound(vars);
            to.unbound(vars);
        }

        virtual void visitReturnNode(ReturnNode* node) {
            //            cout << "ReturnNode " << endl;
            if (node -> returnExpr()) {
                visitExpression(node -> returnExpr(), returnType);
            } else {
                checkType(returnType, VT_VOID);
            }
            scopeExit(nextIns, scopeId);
            nextIns -> addInsn(BC_RETURN);
        }

        virtual void visitUnaryOpNode(UnaryOpNode* node) {
            //            cout << "UnaryOpNode " <<  endl;

            node -> operand() -> visit(this);
            VarType type = lastType;
            switch (node -> kind()) {
                case tSUB:
                {
                    if (type != VT_INT && type != VT_DOUBLE) {
                        throw runtime_error("Operand must be INT or DOUBLE");
                    }
                    nextIns -> addInsn(type == VT_INT ? BC_INEG : BC_DNEG);
                    break;
                }
                case tNOT:
                {
                    checkType(type, VT_INT);
                    flipIntBCGen();
                    break;
                }

                default:
                    throw runtime_error("Unknown Unary Operation");
            }
        }

        virtual void visitBinaryOpNode(BinaryOpNode* node) {
            //            cout << "BinaryOpNode " <<  endl;
            node -> left()-> visit(this);
            VarType left = lastType;
            node -> right() -> visit(this);
            VarType right = lastType;
            VarType type = inferType(left, right);

            switch (node -> kind()) {
                case tGT:
                case tGE:
                case tLT:
                case tLE:
                case tEQ:
                case tNEQ:
                {
                    if (type == VT_DOUBLE) {
                        nextIns -> addInsn(BC_DCMP);
                        nextIns -> addInsn(BC_ILOAD0);
                    }
                    genOrderOpsBc(node);
                    break;
                }
                case tOR:
                case tAND:
                {
                    checkType(left, VT_INT);
                    checkType(right, VT_INT);
                    genAndOrBc(node);
                    break;
                }
                case tADD:
                case tSUB:
                case tMUL:
                case tDIV:
                {
                    genArithmOpsBc(node, type);
                    break;
                }
                case tMOD:
                case tAOR:
                case tAXOR:
                case tAAND:
                {
                    checkType(left, VT_INT);
                    checkType(right, VT_INT);
                    switch (node -> kind()) {
                        case tMOD:
                            nextIns -> addInsn(BC_IMOD);
                            break;
                        case tAOR:
                            nextIns -> addInsn(BC_IAOR);
                            break;
                        case tAXOR:
                            nextIns -> addInsn(BC_IAXOR);
                            break;
                        case tAAND:
                            nextIns -> addInsn(BC_IAAND);
                            break;
                        default: throw runtime_error("Unknown Binary Operation");
                            ;
                    }
                    lastType = VT_INT;
                    break;
                }
                case tINCRSET:
                case tDECRSET:
                {
                    genAssignOpsBc(node, left, right, type);
                    break;
                }
                default:
                    throw runtime_error("Unknown Binary Operation");
            }
        }

    private:

        void flipIntBCGen() {
            Label alter(nextIns);
            Label end(nextIns);
            nextIns -> addInsn(BC_ILOAD0);
            nextIns -> addBranch(BC_IFICMPE, alter);
            nextIns -> addInsn(BC_ILOAD0);
            nextIns -> addBranch(BC_JA, end);
            nextIns -> bind(alter);
            nextIns -> addInsn(BC_ILOAD1);
            nextIns -> bind(end);
        }

        void genAndOrBc(BinaryOpNode* node) {
            switch (node -> kind()) {
                case tOR: nextIns -> addInsn(BC_IADD);
                    break;
                case tAND: nextIns -> addInsn(BC_IMUL);
                    break;
                default: runtime_error("Unknown Binary Operation");
            }
            Label alter(nextIns);
            Label end(nextIns);
            nextIns -> addInsn(BC_ILOAD0);
            nextIns -> addBranch(BC_IFICMPE, alter);
            nextIns -> addInsn(BC_ILOAD1);
            nextIns -> addBranch(BC_JA, end);
            nextIns -> bind(alter);
            nextIns -> addInsn(BC_ILOAD0);
            nextIns -> bind(end);
            lastType = VT_INT;
        }

        void genOrderOpsBc(BinaryOpNode* node) {
            Label alter(nextIns);
            Label end(nextIns);
            switch (node -> kind()) {
                case tLT:
                    nextIns -> addBranch(BC_IFICMPL, alter);
                    break;
                case tLE:
                    nextIns -> addBranch(BC_IFICMPLE, alter);
                    break;
                case tGT:
                    nextIns -> addBranch(BC_IFICMPG, alter);
                    break;
                case tGE:
                    nextIns -> addBranch(BC_IFICMPGE, alter);
                    break;
                case tEQ:
                    nextIns -> addBranch(BC_IFICMPE, alter);
                    break;
                case tNEQ:
                    nextIns -> addBranch(BC_IFICMPNE, alter);
                    break;
                default: throw runtime_error("Unknown binary operator");
            }
            nextIns -> addInsn(BC_ILOAD0);
            nextIns -> addBranch(BC_JA, end);
            nextIns -> bind(alter);
            nextIns -> addInsn(BC_ILOAD1);
            nextIns -> bind(end);
            lastType = VT_INT;
        }

        void genAssignOpsBc(BinaryOpNode* node, VarType left, VarType right, VarType type) {
            if (!(node -> left() -> isLoadNode())) {
                throw runtime_error("lvalue expected on left side");
            }
            LoadNode* lvalue = (LoadNode*) node -> left();
            Val v = Val::fromScope(vars, lvalue -> var() -> name());
            if (type != VT_INT || type != VT_DOUBLE) {
                throw runtime_error("Operand must be INT or DOUBLE");
            }
            if (left == VT_INT && right != VT_INT) {
                throw runtime_error("Incompatible types in assignment");
            }
            if (left == VT_DOUBLE) {
                if (right == VT_INT) {
                    nextIns -> addInsn(BC_I2D);
                } else if (right != VT_DOUBLE) {
                    throw runtime_error("Incompatible types in assignment");
                }
            }
            switch (node -> kind()) {
                case tINCRSET:
                    nextIns -> addInsn(left == VT_INT ? BC_IADD : BC_DADD);
                    break;
                case tDECRSET:
                    nextIns -> addInsn(left == VT_INT ? BC_ISUB : BC_DSUB);
                    break;
                default: throw runtime_error("Unknown Binary Operation");
            }
            v.store(nextIns);
            v.load(nextIns);
            lastType = left;
        }

        void genArithmOpsBc(BinaryOpNode* node, VarType type) {
            switch (node -> kind()) {
                case tADD: nextIns -> addInsn(type == VT_INT ? BC_IADD: BC_DADD);
                    break;
                case tSUB: nextIns -> addInsn(type == VT_INT ? BC_ISUB: BC_DSUB);
                    break;
                case tMUL: nextIns -> addInsn(type == VT_INT ? BC_IMUL: BC_DMUL);
                    break;
                case tDIV: nextIns -> addInsn(type == VT_INT ? BC_IDIV: BC_DDIV);
                    break;
                default: throw runtime_error("Unknown Unary Operation");
            }
            lastType = type;
        }

        void checkType(VarType a, VarType b) {
            if (a != b) {
                throw runtime_error("Type check failed!");
            }
        }

        VarType inferType(VarType left, VarType right) {
            if (left == VT_INT && right == VT_INT) return VT_INT;
            if (left == VT_DOUBLE && right == VT_DOUBLE) return VT_DOUBLE;
            if (left == VT_DOUBLE && right == VT_INT) {
                nextIns -> addInsn(BC_I2D);
                return VT_DOUBLE;
            }
            if (left == VT_INT && right == VT_DOUBLE) {
                nextIns -> addInsn(BC_SWAP);
                nextIns -> addInsn(BC_I2D);
                nextIns -> addInsn(BC_SWAP);
                return VT_DOUBLE;
            }
            throw runtime_error("Can't infer operands type");
        }

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

