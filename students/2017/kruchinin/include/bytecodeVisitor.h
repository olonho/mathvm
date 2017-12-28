#pragma once

#include <iostream>
#include <stack>
#include <set>
#include <iostream>
#include <memory>

#include "ast.h"
#include "utils.h"
#include "scope.h"
#include "interpreter.h"

namespace mathvm {
    class BytecodeVisitor : public AstVisitor {
    public:
        BytecodeVisitor()
        : code(new InterpreterCodeImpl(std::cout))
        , scope(nullptr)
        , bytecode(nullptr)
        {}

        ~BytecodeVisitor() override {
            // TODO !!!
        }

        Status* process(AstFunction *pFunction) {
            try {
                code->addFunction(new BytecodeFunction(pFunction));
                visitAstFunction(pFunction);
            } catch (logic_error& e) {
                return Status::Error(e.what());
            }

            return Status::Ok();
        }

        Code* getCode() {
            code->setContext(scopes);
            return code.release();
        }
    private:
        unique_ptr<InterpreterCodeImpl> code;
        TranslatedScope* scope;
        Bytecode* bytecode;
        std::vector<TranslatedScope*> scopes;
        std::stack<VarType> typeStack;
        const std::set<TokenKind> logicalOp = { tOR, tAND };
        const std::set<TokenKind> arithmeticOp = {
                tADD, tSUB, tMUL, tDIV,
                tMOD, tAOR, tAAND, tAXOR
        };
        const std::set<TokenKind> compareOp = {
                tEQ, tNEQ, tGT,
                tGE, tLT, tLE
        };

        static VarType getCommonType(VarType type1, VarType type2) {
            if (type1 == type2) {
                return type1;
            }

            if (type1 == VT_VOID || type2 == VT_VOID ||
                type1 == VT_INVALID || type2 == VT_INVALID) {
                return VT_INVALID;
            }

            if (type1 == VT_DOUBLE || type2 == VT_DOUBLE) {
                return VT_DOUBLE;
            }

            if (type1 == VT_INT || type2 == VT_INT) {
                return VT_INT;
            }

            return VT_STRING;
        }

        bool isArithmeticOp(TokenKind kind) {
            return arithmeticOp.find(kind) != arithmeticOp.end();
        }

        bool isLogicalOp(TokenKind kind) {
            return logicalOp.find(kind) != logicalOp.end();
        }

        bool isCompareOp(TokenKind kind) {
            return compareOp.find(kind) != compareOp.end();
        }

        void convertInstruction(VarType src, VarType dst) {
            if (src == dst) {
                return;
            }

            if (src == VT_DOUBLE && dst == VT_INT) {
                bytecode->addInsn(BC_D2I);
                return;
            }

            if (src == VT_STRING && dst == VT_INT) {
                bytecode->addInsn(BC_S2I);
                return;
            }

            if (src == VT_INT && dst == VT_DOUBLE) {
                bytecode->addInsn(BC_I2D);
                return;
            }

            if (src == VT_STRING && dst == VT_DOUBLE) {
                bytecode->addInsn(BC_S2I);
                bytecode->addInsn(BC_I2D);
                return;
            }

            throw std::logic_error(string("Cannot convert type ") +
                                   typeToName(src) + " to type "
                                   + typeToName(dst));
        }

        void visitArithmeticNode(BinaryOpNode* node, VarType resultType) {
            if (resultType != VT_INT && resultType != VT_DOUBLE) {
                throw std::logic_error("Incompatible type for arithmetic operation");
            }

            switch (node->kind()) {
                case tADD:
                    bytecode->addInsn(resultType == VT_INT ? BC_IADD : BC_DADD);
                    break;
                case tSUB:
                    bytecode->addInsn(resultType == VT_INT ? BC_ISUB : BC_DSUB);
                    break;
                case tMUL:
                    bytecode->addInsn(resultType == VT_INT ? BC_IMUL : BC_DMUL);
                    break;
                case tDIV:
                    bytecode->addInsn(resultType == VT_INT ? BC_IDIV : BC_DDIV);
                    break;
                case tMOD:
                    if (resultType != VT_INT) {
                        throw std::logic_error("MOD instruction used with type distinct from INT");
                    }
                    bytecode->addInsn(BC_IMOD);
                    break;
                case tAOR:
                    if (resultType != VT_INT) {
                        throw std::logic_error("AOR instruction used with type distinct from INT");
                    }
                    bytecode->addInsn(BC_IAOR);
                    break;
                case tAAND:
                    if (resultType != VT_INT) {
                        throw std::logic_error("AAND instruction used with type distinct from INT");
                    }
                    bytecode->addInsn(BC_IAAND);
                    break;
                case tAXOR:
                    if (resultType != VT_INT) {
                        throw std::logic_error("AXOR instruction used with type distinct from INT");
                    }
                    bytecode->addInsn(BC_IAXOR);
                    break;
                default:
                    throw std::logic_error("Call visitArithmeticNode "
                                                   "with not arithmetic operation");
            }
        }

        void convertToBool() {
            Label end(bytecode);
            Label load0(bytecode);
            bytecode->addInsn(BC_ILOAD0);
            bytecode->addBranch(BC_IFICMPE, load0);

            bytecode->addInsn(BC_ILOAD1);
            bytecode->addBranch(BC_JA, end);

            bytecode->bind(load0);
            bytecode->addInsn(BC_ILOAD0);

            bytecode->bind(end);
        }

        void logicalNot() {
            bytecode->addInsn(BC_ILOAD1);
            bytecode->addInsn(BC_IAXOR);
        }

        void visitLogicalNode(BinaryOpNode* node, VarType resultType) {
            if (resultType != VT_INT) {
                throw std::logic_error("Incompatible type for logical operation");
            }

            convertToBool();
            bytecode->addInsn(BC_SWAP);
            convertToBool();
            bytecode->addInsn(BC_SWAP);

            switch (node->kind()) {
                case tOR:
                    bytecode->addInsn(BC_IAOR);
                    break;
                case tAND:
                    bytecode->addInsn(BC_IAAND);
                    break;
                default:
                    throw std::logic_error("Call visitLogicalNode"
                                                   "with not logical operation");
            }
        }

        void visitCompareNode(BinaryOpNode* node, VarType resultType) {
            if (resultType != VT_INT && resultType != VT_DOUBLE) {
                throw std::logic_error("Incompatible type in compare operation");
            }

            bytecode->addInsn(resultType == VT_INT ? BC_ICMP : BC_DCMP);

            switch (node->kind()) {
                case tEQ:
                    convertToBool();
                    logicalNot();
                    break;
                case tNEQ:
                    convertToBool();
                    break;
                case tGT:
                    bytecode->addInsn(BC_ILOAD1);
                    bytecode->addInsn(BC_ISUB);
                    convertToBool();
                    logicalNot();
                    break;
                case tGE:
                    bytecode->addInsn(BC_ILOAD1);
                    bytecode->addInsn(BC_IADD);
                    convertToBool();
                    break;
                case tLT:
                    bytecode->addInsn(BC_ILOAD1);
                    bytecode->addInsn(BC_IADD);
                    convertToBool();
                    logicalNot();
                    break;
                case tLE:
                    bytecode->addInsn(BC_ILOAD1);
                    bytecode->addInsn(BC_ISUB);
                    convertToBool();
                    break;
                default:
                    throw std::logic_error("Call visitCompareNode"
                                                   "with not compare operation");
            }
        }

        void switchScope(TranslatedScope *childScope) {
            scope = childScope;
            bytecode = childScope->getFunction()->bytecode();
        }

        void visitScope(Scope* scp) {
            Scope::VarIterator varIterator(scp, false);

            while (varIterator.hasNext()) {
                const AstVar* astVar = varIterator.next();
                scope->declareVariable(astVar->name(), astVar->type());
            }

            Scope::FunctionIterator functionIterator(scp, false);

            while (functionIterator.hasNext()) {
                AstFunction* astFunction = functionIterator.next();
                BytecodeFunction* bcFunction = new BytecodeFunction(astFunction);
                code->addFunction(bcFunction);
            }

            functionIterator = Scope::FunctionIterator(scp, false);

            while (functionIterator.hasNext()) {
                AstFunction *astFunction = functionIterator.next();
                visitAstFunction(astFunction);
            }
        }

        void loadVar(const AstVar* var) {
            loadVar(var->name(), var->type());
        }

        void loadVar(const string& varName, VarType type) {
            const uint16_t varId = scope->getVariableId(varName);
            const uint16_t scopeId = scope->getScopeId(varName);

            switch (type) {
                case VT_STRING:
                    bytecode->addInsn(BC_LOADCTXSVAR);
                    break;
                case VT_DOUBLE:
                    bytecode->addInsn(BC_LOADCTXDVAR);
                    break;
                case VT_INT:
                    bytecode->addInsn(BC_LOADCTXIVAR);
                    break;
                default:
                    throw std::logic_error("Inconsistent load type");
            }

            bytecode->addUInt16(scopeId);
            bytecode->addUInt16(varId);
        }

        void visitAstFunction(AstFunction* function) {
            auto bcFunction = static_cast<BytecodeFunction*>(
                    code->functionByName(function->name()));

            TranslatedScope* childScope = new TranslatedScope(scope, bcFunction);
            scopes.push_back(childScope);
            switchScope(childScope);

            for (uint32_t i = 0; i < bcFunction->parametersNumber(); i++) {
                const string& name = bcFunction->parameterName(i);
                const VarType type = bcFunction->parameterType(i);
                scope->declareVariable(name, type);
            }

            function->node()->visit(this);

            if (childScope->getParent() != nullptr) {
                switchScope(childScope->getParent());
            }
        }

        void storeFromStackToVar(const string& name, VarType type) {
            Instruction insn = (type == VT_INT) ? BC_STORECTXIVAR :
                               (type == VT_DOUBLE) ? BC_STORECTXDVAR :
                               BC_STORECTXSVAR;

            uint16_t varId = scope->getVariableId(name);
            uint16_t scopeId = scope->getScopeId(name);

            if (varId == TranslatedScope::INVALID_VAR_ID ||
                scopeId == TranslatedScope::INVALID_SCOPE_ID) {
                throw std::logic_error(string("No such variable ") + name);
            }


            bytecode->addInsn(insn);
            bytecode->addUInt16(scopeId);
            bytecode->addUInt16(varId);
        }

        void storeFromStackToVar(const AstVar *var) {
            storeFromStackToVar(var->name(), var->type());
        }

    public:
        void visitBinaryOpNode(BinaryOpNode* node) override {
            node->left()->visit(this);
            node->right()->visit(this);

            VarType rightType = typeStack.top();
            typeStack.pop();
            VarType leftType = typeStack.top();
            typeStack.pop();

            VarType resultType = getCommonType(leftType, rightType);

            if (resultType == VT_INVALID) {
                std::cerr << "Type check failed in binary node." << std::endl;
            }

            if (leftType != resultType) {
                bytecode->addInsn(BC_SWAP);
                convertInstruction(leftType, resultType);
                bytecode->addInsn(BC_SWAP);
            }

            if (rightType != resultType) {
                convertInstruction(rightType, resultType);
            }

            typeStack.push(resultType);

            if (isArithmeticOp(node->kind()))
            {
                visitArithmeticNode(node, resultType);
            }
            else if (isLogicalOp(node->kind()))
            {
                visitLogicalNode(node, resultType);
            }
            else if (isCompareOp(node->kind()))
            {
                visitCompareNode(node, resultType);
            }
            else {
                throw std::logic_error("Invalid node in visitBinaryOpNode");
            }
        }

        void visitUnaryOpNode(UnaryOpNode* node) override {
            node->operand()->visit(this);

            VarType resultType = typeStack.top();
            typeStack.pop();

            if (resultType == VT_STRING) {
                convertInstruction(VT_STRING, VT_INT);
            }

            if (resultType != VT_INT && resultType != VT_DOUBLE) {
                throw std::logic_error("Type check failed in unary node");
            }

            switch (node->kind()) {
                case tSUB:
                    bytecode->addInsn(resultType == VT_INT ? BC_INEG : BC_DNEG);
                    typeStack.push(resultType);
                    break;
                case tNOT:
                    convertToBool();
                    logicalNot();
                    typeStack.push(VT_INT);
                    return;
                default:
                    throw std::logic_error("Invalid node in visitUnaryOpNode");
            }
        }

        void visitStringLiteralNode(StringLiteralNode* node) override {
            typeStack.push(VT_STRING);
            const string &literal = node->literal();
            if (literal.empty()) {
                bytecode->addInsn(BC_SLOAD0);
                return;
            }

            bytecode->addInsn(BC_SLOAD);
            bytecode->addUInt16(code->makeStringConstant(literal));
        }

        void visitDoubleLiteralNode(DoubleLiteralNode* node) override {
            bytecode->addInsn(BC_DLOAD);
            bytecode->addDouble(node->literal());
            typeStack.push(VT_DOUBLE);
        }

        void visitIntLiteralNode(IntLiteralNode* node) override {
            bytecode->addInsn(BC_ILOAD);
            bytecode->addInt64(node->literal());
            typeStack.push(VT_INT);
        }

        void visitLoadNode(LoadNode* node) override {
            const AstVar *pVar = node->var();
            loadVar(pVar->name(), pVar->type());
            typeStack.push(pVar->type());
        }

        void visitStoreNode(StoreNode* node) override {
            node->value()->visit(this);
            VarType opType = typeStack.top();
            typeStack.pop();

            const AstVar* var = node->var();
            const VarType resultType = var->type();
            convertInstruction(opType, resultType);

            TokenKind kind = node->op();
            if (kind == tINCRSET || kind == tDECRSET) {
                if (resultType != VT_INT && resultType != VT_DOUBLE) {
                    throw logic_error("Increment or decrement operations "
                                              "are not allowed with types "
                                              "other than int and double.");
                }

                loadVar(var->name(), resultType);
                bytecode->addInsn(BC_SWAP);

                if (kind == tINCRSET) {
                    bytecode->addInsn(resultType == VT_INT ? BC_IADD : BC_DADD);
                } else {
                    bytecode->addInsn(resultType == VT_INT ? BC_ISUB : BC_DSUB);
                }
            }

            storeFromStackToVar(var);
        }

        void visitForNode(ForNode* node) override {
            const AstVar* var = node->var();
            if (var->type() != VT_INT) {
                throw std::logic_error(string("Cannot use variable with type ") +
                            typeToName(var->type()) + " in for range");
            }

            auto rangeNode = node->inExpr()->asBinaryOpNode();
            assert(rangeNode->kind() == tRANGE);

            rangeNode->left()->visit(this);
            convertInstruction(typeStack.top(), VT_INT);

            storeFromStackToVar(var);

            Label begin(bytecode), end(bytecode);
            bytecode->bind(begin);

            // load for variable
            loadVar(var);

            // load upper bound
            rangeNode->right()->visit(this);
            convertInstruction(typeStack.top(), VT_INT);

            // compare
            bytecode->addBranch(BC_IFICMPG, end);

            node->body()->visit(this);

            // load for variable and increment it
            loadVar(var);
            bytecode->addInsn(BC_ILOAD1);
            bytecode->addInsn(BC_IADD);
            storeFromStackToVar(var);

            bytecode->addBranch(BC_JA, begin);
            bytecode->bind(end);
        }

        void visitWhileNode(WhileNode* node) override {
            Label begin(bytecode), end(bytecode);

            bytecode->bind(begin);

            node->whileExpr()->visit(this);
            bytecode->addInsn(BC_ILOAD0);
            bytecode->addBranch(BC_IFICMPE, end);

            node->loopBlock()->visit(this);

            bytecode->addBranch(BC_JA, begin);

            bytecode->bind(end);

        }

        void visitIfNode(IfNode* node) override {
            Label els(bytecode), end(bytecode);

            node->ifExpr()->visit(this);

            bytecode->addInsn(BC_ILOAD1);
            bytecode->addBranch(BC_IFICMPNE, els);

            node->thenBlock()->visit(this);

            bytecode->addBranch(BC_JA, end);
            bytecode->bind(els);

            if (node->elseBlock() != nullptr) {
                node->elseBlock()->visit(this);
            }

            bytecode->bind(end);
        }

        void visitBlockNode(BlockNode* node) override {
            visitScope(node->scope());
            node->visitChildren(this);
        }

        void visitFunctionNode(FunctionNode* node) override {
            for (uint32_t i = 0; i < node->parametersNumber(); i++) {
                storeFromStackToVar(node->parameterName(i),
                                    node->parameterType(i));
            }
            node->visitChildren(this);
        }

        void visitReturnNode(ReturnNode* node) override {
            if (node->returnExpr() != nullptr) {
                node->returnExpr()->visit(this);
            } else {
                typeStack.push(VT_VOID);
            }

            const VarType returnType = scope->getFunction()->returnType();
            convertInstruction(typeStack.top(), returnType);
            typeStack.pop();
            typeStack.push(returnType);

            bytecode->addInsn(BC_RETURN);
        }

        void visitCallNode(CallNode* node) override {
            auto function = code->functionByName(node->name());

            for (uint32_t i = node->parametersNumber(); i != 0; i--) {
                node->parameterAt(i - 1)->visit(this);

                const VarType parameterType = function->parameterType(i - 1);
                convertInstruction(typeStack.top(), parameterType);
                typeStack.pop();
                typeStack.push(parameterType);
            }

            bytecode->addInsn(BC_CALL);
            bytecode->addUInt16(function->id());

            typeStack.push(function->returnType());
        }

        void visitNativeCallNode(NativeCallNode* node) override {
        }

        void visitPrintNode(PrintNode* node) override {
            for (uint32_t i = 0; i < node->operands(); i++) {
                node->operandAt(i)->visit(this);

                VarType type = typeStack.top();
                typeStack.pop();
                switch (type) {
                    case VT_INT:
                        bytecode->addInsn(BC_IPRINT);
                        break;
                    case VT_DOUBLE:
                        bytecode->addInsn(BC_DPRINT);
                        break;
                    case VT_STRING:
                        bytecode->addInsn(BC_SPRINT);
                        break;
                    default:
                        throw std::logic_error(string("Incompatible type ")
                                               + typeToName(type) + " in print function");
                }
            }

            typeStack.push(VT_VOID);
        }
    };
}