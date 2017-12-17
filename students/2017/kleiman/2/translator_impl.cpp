#include "include/ast_printing.h"
#include "include/mathvm.h"
#include "ast.h"
#include "parser.h"
#include <iostream>
#include <unordered_map>
#include "include/context.h"
#include <cmath>

namespace mathvm {
    class TranslateCode : public AstVisitor
    {
        InterpreterCodeImpl* _code;
        VarType _type;

        public:
        TranslateCode(InterpreterCodeImpl* code)
            : _code(code)
        {
        }

        uint16_t visitAstFunction(AstFunction* func)
        {
            BytecodeFunction* bFunc = new BytecodeFunction(func);
            uint16_t id = _code->addFunction(bFunc);
            _code->addContext(id);
            if (id == 0) {
                func->node()->visit(this);
            }
            return id;
        }

        void visitBinaryOpNode(BinaryOpNode* node)
        {
            auto kind = node->kind();
            auto bcode = _code->getFunctionCode();
            if (kind == tADD || kind == tSUB || kind == tMUL || kind == tDIV || kind == tMOD)
            {
                node->left()->visit(this);
                VarType leftType = _type;
                node->right()->visit(this);
                VarType rightType = _type;
                auto common = commonType(leftType, rightType);
                if (common == VT_INVALID)
                {
                    bcode->addInsn(BC_INVALID);
                    return;
                }
                Instruction insn;
                switch(kind)
                {
                    case tSUB: insn = (common == VT_INT) ? BC_ISUB : BC_DSUB; break;
                    case tADD: insn = (common == VT_INT) ? BC_IADD : BC_DADD; break;
                    case tMUL: insn = (common == VT_INT) ? BC_IMUL : BC_DMUL; break;
                    case tDIV: insn = (common == VT_INT) ? BC_IDIV : BC_DDIV; break;
                    case tMOD: insn = (common == VT_INT) ? BC_IMOD : BC_INVALID; break;
                    default: insn = BC_INVALID; break;
                }
                if (rightType != common) 
                {
                    castFromTo(rightType, common);
                }
                if (leftType != common) {
                    bcode->addInsn(BC_SWAP);
                    castFromTo(leftType, common);
                    bcode->addInsn(BC_SWAP);
                }
                bcode->addInsn(insn);
                _type = common;
            }
            else if (kind == tOR || kind == tAND)
            {
                Label aTrue(bcode);
                Label aFalse(bcode);
                if (kind == tAND) {
                    bcode->addInsn(BC_ILOAD0);
                    node->left()->visit(this);
                    castFromTo(_type, VT_INT);
                    _type = VT_INT;
                    bcode->addBranch(BC_IFICMPE, aFalse);
                    bcode->addInsn(BC_POP);
                    bcode->addInsn(BC_POP);
                    bcode->addInsn(BC_ILOAD0);
                    node->right()->visit(this);
                    castFromTo(_type, VT_INT);
                    _type = VT_INT;
                    bcode->addBranch(BC_IFICMPE, aFalse);
                    bcode->addInsn(BC_POP);
                    bcode->addInsn(BC_POP);
                    bcode->addInsn(BC_ILOAD1);
                    bcode->addBranch(BC_JA, aTrue);
                    bcode->bind(aFalse);
                    bcode->addInsn(BC_POP);
                    bcode->addInsn(BC_POP);
                    bcode->addInsn(BC_ILOAD0);
                    bcode->bind(aTrue);
                    _type = VT_INT;
                    return;
                }
                bcode->addInsn(BC_ILOAD0);
                node->left()->visit(this);
                castFromTo(_type, VT_INT);
                _type = VT_INT;
                bcode->addBranch(BC_IFICMPNE, aTrue);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_ILOAD0);
                node->right()->visit(this);
                castFromTo(_type, VT_INT);
                _type = VT_INT;
                bcode->addBranch(BC_IFICMPNE, aTrue);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_ILOAD0);
                bcode->addBranch(BC_JA, aFalse);
                bcode->bind(aTrue);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_ILOAD1);
                bcode->bind(aFalse);
                _type = VT_INT;
            }
            else if (kind == tAOR || kind == tAXOR || kind == tAAND)
            {
                node->left()->visit(this);
                VarType left = _type;
                castFromTo(left, VT_INT);
                _type = VT_INT;
                node->right()->visit(this);
                VarType right = _type;
                castFromTo(right, VT_INT);
                _type = VT_INT;
                Instruction insn;
                switch (kind) {
                    case tAOR: insn = BC_IAOR; break;
                    case tAAND: insn = BC_IAAND; break;
                    case tAXOR: insn = BC_IAXOR; break;
                    default: insn = BC_INVALID; break;
                }
                bcode->addInsn(insn);
                _type = VT_INT;
            }
            else if (kind == tEQ || kind == tNEQ || kind == tGT || kind == tGE || kind == tLT || kind == tLE)
            {
                node->left()->visit(this);
                VarType left = _type;
                node->right()->visit(this);
                VarType right = _type;
                VarType common = commonType(left, right);
                if (right != common) {
                    castFromTo(right, common);
                }

                if (left != common) {
                    bcode->addInsn(BC_SWAP);
                    castFromTo(left, common);
                    bcode->addInsn(BC_SWAP);
                }
                if (common == VT_DOUBLE) {
                    bcode->addInsn(BC_DCMP);
                } else {
                    bcode->addInsn(BC_ICMP);
                }
                Instruction insn;
                switch (kind) {
                    case tEQ: insn = BC_IFICMPE; break;
                    case tNEQ: insn = BC_IFICMPNE; break;
                    case tGT: insn = BC_IFICMPG; break;
                    case tGE: insn = BC_IFICMPGE; break;
                    case tLT: insn = BC_IFICMPL; break;
                    case tLE: insn = BC_IFICMPLE; break;
                    default: insn = BC_INVALID;
                        break;
                }
                Label setTrue(bcode);
                Label setFalse(bcode);
                bcode->addInsn(BC_ILOAD0);
                bcode->addBranch(insn, setTrue);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_ILOAD0);
                bcode->addBranch(BC_JA, setFalse);
                bcode->bind(setTrue);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_ILOAD1);
                bcode->bind(setFalse);
                _type = VT_INT;
            }
            else
            {
                bcode->addInsn(BC_INVALID);
            }
        }

        void visitUnaryOpNode(UnaryOpNode* node)
        {
            node->operand()->visit(this);
            auto ot = _type;
            auto kind = node->kind();
            auto bcode = _code->getFunctionCode();
            if (kind == tNOT)
            {
                castFromTo(ot, VT_INT);
                _type = VT_INT;
                Label t(bcode);
                Label f(bcode);
                bcode->addInsn(BC_ILOAD0);
                bcode->addBranch(BC_IFICMPE, t);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_ILOAD0);
                bcode->addBranch(BC_JA, f);
                bcode->bind(t);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_POP);
                bcode->addInsn(BC_ILOAD1);
                bcode->bind(f);
            }
            else if (kind == tSUB)
            {
                if (ot == VT_DOUBLE)
                {
                    bcode->addInsn(BC_DNEG);
                }
                if (ot == VT_INT)
                {
                    bcode->addInsn(BC_INEG);
                }
            }
            else
            {
                bcode->addInsn(BC_INVALID);
            }
        }

        void visitStringLiteralNode(StringLiteralNode* node)
        {
            const std::string& lit = node->literal();
            auto bcode = _code->getFunctionCode();
            _type = VT_STRING;
            if (lit.empty())
            {
                bcode->addInsn(BC_SLOAD0);
            }
            else
            {
                uint16_t id = _code->makeStringConstant(lit);
                bcode->addInsn(BC_SLOAD);
                bcode->addUInt16(id);
            }
        }

        void visitIntLiteralNode(IntLiteralNode* node)
        {
            int64_t lit = node->literal();
            auto bcode = _code->getFunctionCode();
            _type = VT_INT;
            switch(lit)
            {
                case 0:
                    bcode->addInsn(BC_ILOAD0);
                    break;
                case 1:
                    bcode->addInsn(BC_ILOAD1);
                    break;
                case -1:
                    bcode->addInsn(BC_ILOADM1);
                    break;
                default:
                    bcode->addInsn(BC_ILOAD);
                    bcode->addInt64(lit);
                    break;
            }
        }

        void visitDoubleLiteralNode(DoubleLiteralNode* node)
        {
            double lit = node->literal();
            double eps = 1e-10;
            auto bcode = _code->getFunctionCode();
            _type = VT_DOUBLE;
            if (fabs(lit) < eps)
            {
                bcode->addInsn(BC_DLOAD0);
                return;
            }
            if (fabs(lit + 1.) < eps)
            {
                bcode->addInsn(BC_DLOADM1);
                return;
            }
            if (fabs(lit - 1.) < eps)
            {
                bcode->addInsn(BC_DLOAD1);
                return;
            }
            bcode->addInsn(BC_DLOAD);
            bcode->addDouble(lit);
        }

        void visitLoadNode(LoadNode* node)
        {
            loadVarToTOS(node->var());
        }

        void visitStoreNode(StoreNode* node)
        {
            node->value()->visit(this);
            TokenKind kind = node->op();
            auto bcode = _code->getFunctionCode();
            if (kind == tINCRSET || kind == tDECRSET || kind == tASSIGN)
            {
                if (kind == tINCRSET || kind == tDECRSET)
                {
                    VarType vt = node->var()->type();
                    castFromTo(_type, vt);
                    _type = vt;
                    loadVarToTOS(node->var());
                    Instruction insn;
                    if (vt != VT_DOUBLE && vt != VT_INT)
                    {
                        insn = BC_INVALID;
                        bcode->addInsn(insn);
                        return;
                    }
                    if (kind == tINCRSET) {
                        kind = tADD;
                    } else {
                        kind = tSUB;
                    }
                    switch(kind)
                    {
                        case tSUB: insn = (vt == VT_INT) ? BC_ISUB : BC_DSUB; break;
                        case tADD: insn = (vt == VT_INT) ? BC_IADD : BC_DADD; break;
                        default: insn = BC_INVALID; break;
                    }
                    bcode->addInsn(BC_SWAP);
                    bcode->addInsn(insn);
                }
                storeVarToTOS(node->var());
            }
            else
            {
                bcode->addInsn(BC_INVALID);
            }
        }

        void visitBlockNode(BlockNode* node)
        {
            int count = 0;
            for (Scope::VarIterator it(node->scope()); it.hasNext(); )
            {
                auto var = it.next();
                auto sp = std::make_shared<Var>(var->type(), var->name());
                _code->addVar(sp);
                count++;
            }
            uint16_t currentContext = _code->getCurrentContext();
            std::vector<uint16_t> scope;
            for (Scope::FunctionIterator it(node->scope()); it.hasNext(); )
            {
                scope.push_back(visitAstFunction(it.next()));
                _code->setCurrentContext(currentContext);
            }
            int i = 0;
            for (Scope::FunctionIterator it(node->scope()); it.hasNext(); )
            {
                _code->setCurrentContext(scope[i]);
                i++;
                it.next()->node()->visit(this);
            }
            _code->setCurrentContext(currentContext);
            for (size_t i = 0; i < node->nodes(); ++i)
            {
                AstNode *cld = node->nodeAt(i);
                cld->visit(this);
            }     
        }

        void visitNativeCallNode(NativeCallNode* node)
        {

        }

        void visitForNode(ForNode* node)
        {
            const auto v = node->var();
            auto binNode = node->inExpr()->asBinaryOpNode();
            auto bcode = _code->getFunctionCode();
            if (v->type() != VT_INT || binNode->kind() != tRANGE)
            {
                bcode->addInsn(BC_INVALID);
                return;
            }
            binNode->left()->visit(this);
            castFromTo(_type, VT_INT);
            _type = VT_INT;
            storeVarToTOS(v);
            binNode->right()->visit(this);
            castFromTo(_type, VT_INT);
            _type = VT_INT;
            Label begin(bcode);
            Label end(bcode);
            bcode->bind(begin);
            loadVarToTOS(v);
            bcode->addBranch(BC_IFICMPL, end);
            bcode->addInsn(BC_POP);
            node->body()->visit(this);
            bcode->addInsn(BC_ILOAD1);
            loadVarToTOS(v);
            bcode->addInsn(BC_IADD);
            storeVarToTOS(v);
            bcode->addBranch(BC_JA, begin);
            bcode->bind(end);
            bcode->addInsn(BC_POP);
            bcode->addInsn(BC_POP);
        }

        void visitWhileNode(WhileNode* node)
        {
            auto bcode = _code->getFunctionCode();
            Label begin(bcode);
            Label end(bcode);
            bcode->bind(begin);
            node->whileExpr()->visit(this);
            bcode->addInsn(BC_ILOAD0);
            bcode->addBranch(BC_IFICMPE, end);
            bcode->addInsn(BC_POP);
            bcode->addInsn(BC_POP);
            node->loopBlock()->visit(this);
            bcode->addBranch(BC_JA, begin);
            bcode->bind(end);
            bcode->addInsn(BC_POP);
            bcode->addInsn(BC_POP);
        }

        void visitIfNode(IfNode* node)
        {
            auto bcode = _code->getFunctionCode();
            Label first(bcode);
            Label second(bcode);
            node->ifExpr()->visit(this);
            bcode->addInsn(BC_ILOAD0);
            bcode->addBranch(BC_IFICMPE, second);
            bcode->addInsn(BC_POP);
            bcode->addInsn(BC_POP);
            node->thenBlock()->visit(this);
            bcode->addBranch(BC_JA, first);
            bcode->bind(second);
            bcode->addInsn(BC_POP);
            bcode->addInsn(BC_POP);
            if (node->elseBlock() != nullptr) 
            {
                node->elseBlock()->visit(this);
            }
            bcode->bind(first);
        }

        void visitReturnNode(ReturnNode* node)
        {
            auto vt = _code->functionById(_code->getCurrentContext())->returnType();
            if (node->returnExpr() != nullptr) 
            {
                node->returnExpr()->visit(this);
                castFromTo(_type, vt);
                _type= vt;
            }
            auto bcode = _code->getFunctionCode();
            bcode->addInsn(BC_RETURN);
            if (vt != VT_VOID && vt != VT_INVALID) 
            {
                _type = vt;
            }
        }

        void visitFunctionNode(FunctionNode* node)
        {
            auto bcode = _code->getFunctionCode();
            for (int32_t i = node->parametersNumber() - 1; i >= 0; --i)
            {
                auto sp = std::make_shared<Var>(node->parameterType(i), node->parameterName(i));
                uint16_t id = _code->addVar(sp);
                switch (node->parameterType(i)) {
                    case VT_INT:
                        bcode->addInsn(BC_STOREIVAR);
                        bcode->addUInt16(id);
                        break;
                    case VT_DOUBLE:
                        bcode->addInsn(BC_STOREDVAR);
                        bcode->addUInt16(id);
                        break;
                    case VT_STRING:
                        bcode->addInsn(BC_STORESVAR);
                        bcode->addUInt16(id);
                        break;
                    default:
                        break;
                }
            }
            node->body()->visit(this);
        }

        void visitCallNode(CallNode* node)
        {
            auto func = _code->functionByName(node->name());
            auto bcode = _code->getFunctionCode();
            for (size_t i = 0; i < node->parametersNumber(); ++i)
            {
                node->parameterAt(i)->visit(this);
                castFromTo(_type, func->parameterType(i));
                _type = func->parameterType(i);
            }
            bcode->addInsn(BC_CALL);
            bcode->addUInt16(func->id());
            switch(func->returnType())
            {
                case VT_DOUBLE:
                case VT_INT:
                case VT_STRING:
                    _type = func->returnType();
                    break;
                default: break;
            }
        }

        void visitPrintNode(PrintNode* node)
        {
            auto bcode = _code->getFunctionCode();
            for (size_t i = 0; i < node->operands(); ++i)
            {
                node->operandAt(i)->visit(this);
                switch(_type)
                {
                    case VT_DOUBLE:
                        bcode->addInsn(BC_DPRINT);
                        break;
                    case VT_INT:
                        bcode->addInsn(BC_IPRINT);
                        break;
                    case VT_STRING:
                        bcode->addInsn(BC_SPRINT);
                        break;
                    default:
                        bcode->addInsn(BC_INVALID);
                        break;
                }
            }
            _type = VT_VOID;
        }
    private:
        VarType commonType(VarType first, VarType second)
        {
            if (first == second)
            {
                return second;
            }
            if (first == VT_INT && second == VT_DOUBLE)
            {
                return VT_DOUBLE;
            }
            if (first == VT_INT && second == VT_STRING)
            {
                return VT_INT;
            }
            if (first == VT_STRING && (second == VT_INT || second == VT_DOUBLE))
            {
                return second;
            }
            if (first == VT_DOUBLE)
            {
                return first;
            }
            return VT_INVALID;
        }

        void storeVarToTOS(const AstVar* var)
        {
            auto idx = _code->ContextAndIDByName(var->name());
            auto context_id = _code->getCurrentContext();
            auto currentContext = _code->getCurrentContext();
            _type = VT_VOID;
            Instruction insn;
            if (currentContext == idx.first)
            {
                switch(var->type())
                {
                    case VT_INT:
                        insn = BC_STOREIVAR;
                        break;
                    case VT_DOUBLE:
                        insn = BC_STOREDVAR;
                        break;
                    case VT_STRING:
                        insn = BC_STORECTXSVAR;
                        break;
                    default:
                        insn = BC_STORESVAR;
                        break;
                }
            }
            else
            {
                switch(var->type())
                {
                    case VT_INT:
                        insn = BC_STORECTXIVAR;
                        break;
                    case VT_DOUBLE:
                        insn = BC_STORECTXDVAR;
                        break;
                    case VT_STRING:
                        insn = BC_STORECTXSVAR;
                        break;
                    default:
                        insn = BC_INVALID;
                        break;
                }
            }
            auto bcode = _code->getFunctionCode();
            bcode->addInsn(insn);
            if (context_id != idx.first)
            {
                bcode->addUInt16(idx.first);
                bcode->addUInt16(idx.second);
            }
            else
            {
                bcode->addUInt16(idx.second);
            }
        }
        void loadVarToTOS(const AstVar* var)
        {
            auto idx = _code->ContextAndIDByName(var->name());
            auto context_id = _code->getCurrentContext();
            _type = var->type();
            Instruction insn;
            if (context_id == idx.first)
            {
                switch(_type)
                {
                    case VT_DOUBLE:
                        insn = BC_LOADDVAR;
                        break;
                    case VT_INT:
                        insn = BC_LOADIVAR;
                        break;
                    case VT_STRING:
                        insn = BC_LOADSVAR;
                        break;
                    default:
                        insn = BC_INVALID;
                        break;
                }
            }
            else
            {
                switch(_type)
                {
                    case VT_DOUBLE:
                        insn = BC_LOADCTXDVAR;
                        break;
                    case VT_INT:
                        insn = BC_LOADCTXIVAR;
                        break;
                    case VT_STRING:
                        insn = BC_LOADCTXSVAR;
                        break;
                    default:
                        insn = BC_INVALID;
                        break;
                }
            }
            Bytecode* bcode = _code->getFunctionCode();
            bcode->addInsn(insn);
            if (context_id != idx.first)
            {
                bcode->addUInt16(idx.first);
                bcode->addUInt16(idx.second);
            }
            else
            {
                bcode->addUInt16(idx.second);
            }
        }

        void castFromTo(VarType from, VarType to)
        {
            if (from == to)
            {
                return;
            }
            Bytecode* bcode = _code->getFunctionCode();
            if (from == VT_STRING && to == VT_INT)
            {
                bcode->addInsn(BC_S2I);
                return;
            }
            if (from == VT_STRING && to == VT_DOUBLE)
            {
                bcode->addInsn(BC_S2I);
                bcode->addInsn(BC_I2D);
                return;
            }
            if (from == VT_INT && to == VT_DOUBLE)
            {
                bcode->addInsn(BC_I2D);
                return;
            }
            bcode->addInsn(BC_INVALID);
        }
    };

    Translator* Translator::create(const string& impl) 
    {
        if (impl == "" || impl == "intepreter") {
            return new BytecodeTranslatorImpl();
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        if (impl == "printer") {
            return new ProgramTranslatorImpl();
        }
        assert(false);
        return 0;
    }

    Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code)
    {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status->isError())
            return status;
        *code = new InterpreterCodeImpl();
        TranslateCode translateVisitor(static_cast<InterpreterCodeImpl*>(*code));
        translateVisitor.visitAstFunction(parser.top());
        return status;
    }
}
