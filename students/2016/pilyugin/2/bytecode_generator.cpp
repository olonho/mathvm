#include <stack>
#include <dlfcn.h>
#include "mathvm.h"
#include "parser.h"
#include "generator_exception.h"
#include "generator_scope.h"
#include "operations.h"

namespace mathvm {

class BytecodeGenerator : public AstVisitor {
public:
    BytecodeGenerator(Code* code) :
            code_(code) {}

    Status* generate(AstFunction* top) {
        BytecodeFunction* bytecodeTop = new BytecodeFunction(top);
        code_->addFunction(bytecodeTop);
        addFunction(top, bytecodeTop);
        return Status::Ok();
    }

    void visitBinaryOpNode(BinaryOpNode* node) override {
        TokenKind kind = node->kind();

        VarType resultType = VT_INVALID;
        if (isLogicalOp(kind)) {
            resultType = addLogicalOp(node);
        } else if (isBitwiseOp(kind)) {
            resultType = addBitwiseOp(node);
        } else if (isCompareOp(kind)) {
            resultType = addComparisonOp(node);
        } else if (isArithmeticOp(kind)) {
            resultType = addArithmeticOp(node);
        }
        tosType_ = resultType;
    }

    void visitUnaryOpNode(UnaryOpNode* node) override {
        switch (node->kind()) {
            case tSUB:
                node->operand()->visit(this);
                bytecode()->addInsn((tosType_ == VT_INT) ? BC_INEG : BC_DNEG);
                break;
            case tNOT:
                addNotOp(node);
                break;
            default:
                throw BytecodeGeneratorException("Illegal unary operator");
        }
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) override {
        bytecode()->addInsn(BC_DLOAD);
        bytecode()->addDouble(node->literal());
        tosType_ = VT_DOUBLE;
    }

    void visitIntLiteralNode(IntLiteralNode* node) override {
        bytecode()->addInsn(BC_ILOAD);
        bytecode()->addInt64(node->literal());
        tosType_ = VT_INT;
    }

    void visitStringLiteralNode(StringLiteralNode* node) override {
        uint16_t const id = code_->makeStringConstant(node->literal());
        bytecode()->addInsn(BC_SLOAD);
        bytecode()->addUInt16(id);
        tosType_ = VT_STRING;
    }

    void visitPrintNode(PrintNode* node) override {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            bytecode()->addInsn(printInsn(tosType_));
        }
    }

    void visitLoadNode(LoadNode* node) override {
        loadVar(node->var());
    }

    void visitStoreNode(StoreNode* node) override {
        node->value()->visit(this);
        VarType type = node->var()->type();
        castTosType(type);
        if (node->op() == tINCRSET) {
            loadVar(node->var());
            bytecode()->addInsn(type == VT_INT ? BC_IADD : BC_DADD);
        }
        if (node->op() == tDECRSET) {
            loadVar(node->var());
            bytecode()->addInsn(type == VT_INT ? BC_ISUB : BC_DSUB);
        }
        storeVar(node->var());
    }

    void visitForNode(ForNode* node) override {
        BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();
        inExpr->left()->visit(this);
        castTosType(VT_INT);
        if (node->var()->type() != VT_INT) {
            throw BytecodeGeneratorException("Invalid for expression");
        }
        storeVar(node->var());

        Label l1 = Label(bytecode());
        bytecode()->bind(l1);
        inExpr->right()->visit(this);
        castTosType(VT_INT);
        loadVar(node->var());
        Label l2 = Label(bytecode());
        bytecode()->addBranch(BC_IFICMPG, l2);

        node->body()->visit(this);

        loadVar(node->var());
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_IADD);
        storeVar(node->var());
        bytecode()->addBranch(BC_JA, l1);
        bytecode()->bind(l2);

        tosType_ = VT_VOID;
    }

    void visitIfNode(IfNode* node) override {
        Label unlessLabel(bytecode());
        node->ifExpr()->visit(this);
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPE, unlessLabel);

        node->thenBlock()->visit(this);

        if (node->elseBlock() != 0) {
            Label afterElse(bytecode());
            bytecode()->addBranch(BC_JA, afterElse);
            bytecode()->bind(unlessLabel);
            node->elseBlock()->visit(this);
            bytecode()->bind(afterElse);
        } else {
            bytecode()->bind(unlessLabel);
        }
    }

    void visitWhileNode(WhileNode* node) override {
        Label l2(bytecode()->currentLabel());
        node->whileExpr()->visit(this);

        Label l1(bytecode());
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPE, l1);

        node->loopBlock()->visit(this);
        bytecode()->addBranch(BC_JA, l2);
        bytecode()->bind(l1);
    }

    void visitFunctionNode(FunctionNode* node) override {
        if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
        } else {
            node->body()->visit(this);
        }
        VarType type = node->returnType();
        tosType_ = type;
    }

    void visitCallNode(CallNode* node) override {
        BytecodeFunction* function = (BytecodeFunction*) code_->functionByName(node->name());
        if (!function) {
            throw BytecodeGeneratorException("No function found: " + node->name());
        }
        if (node->parametersNumber() != function->parametersNumber()) {
            throw BytecodeGeneratorException("Wrong parameters number");
        }

        for (int i = node->parametersNumber() - 1; i >= 0; --i) {
            node->parameterAt((uint32_t) i)->visit(this);
            castTosType(function->parameterType((uint32_t) i));
        }
        bytecode()->addInsn(BC_CALL);
        bytecode()->addUInt16(function->id());
        VarType type = function->returnType();
        tosType_ = type;
    }

    void visitNativeCallNode(NativeCallNode* node) override {
        void* code = dlsym((void*) -2/*RTLD_DEFAULT*/, node->nativeName().c_str());
        if (!code) {
            throw BytecodeGeneratorException("Invalid native function: " + node->nativeName());
        }
        uint16_t func_id = code_->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);
        bytecode()->addInsn(BC_CALLNATIVE);
        bytecode()->addUInt16(func_id);

        VarType type = node->nativeSignature()[0].first;
        tosType_ = type;
    }

    void visitBlockNode(BlockNode* node) override {
        Scope::VarIterator varIterator(node->scope());
        while (varIterator.hasNext()) {
            scope_->addVar(varIterator.next());
        }

        Scope::FunctionIterator funcIterator(node->scope());
        while (funcIterator.hasNext()) {
            AstFunction* astFunction = funcIterator.next();

            if (code_->functionByName(astFunction->name())) {
                throw BytecodeGeneratorException("Duplicated function");
            } else {
                code_->addFunction(new BytecodeFunction(astFunction));
            }
        }

        funcIterator = Scope::FunctionIterator(node->scope());
        while (funcIterator.hasNext()) {
            AstFunction* astFunction = funcIterator.next();
            addFunction(astFunction, (BytecodeFunction*) code_->functionByName(astFunction->name()));
        }

        for (uint32_t i = 0; i < node->nodes(); ++i) {
            node->nodeAt(i)->visit(this);
        }
        tosType_ = VT_VOID;
    }

    void visitReturnNode(ReturnNode* node) override {
        VarType type = scope_->bytecodeFunction()->returnType();
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
            castTosType(type);
        }
        bytecode()->addInsn(BC_RETURN);
        tosType_ = type;
    }

private:
    Code* code_;
    VarType tosType_;
    GeneratorScope* scope_;

    void addFunction(AstFunction* astFunction, BytecodeFunction* bytecodeFunction) {
        GeneratorScope* newScope = new GeneratorScope(bytecodeFunction, astFunction->scope(), scope_);
        scope_ = newScope;

        for (uint32_t i = 0; i < astFunction->parametersNumber(); ++i) {
            AstVar* var = astFunction->scope()->lookupVariable(astFunction->parameterName(i), false);
            storeVar(var);
        }
        astFunction->node()->visit(this);
        bytecodeFunction->setLocalsNumber(scope_->localsNumber());
        bytecodeFunction->setScopeId(scope_->scopeId());
        scope_ = newScope->parent();
        delete newScope;
    }

    Bytecode* bytecode() {
        return scope_->bytecodeFunction()->bytecode();
    }

    VarType addLogicalOp(BinaryOpNode* node) {
        node->left()->visit(this);
        castTosType(VT_INT);

        bytecode()->addInsn(BC_ILOAD0);
        Instruction compareInsn = node->kind() == tOR ? BC_IFICMPNE : BC_IFICMPE;

        Label l1(bytecode());
        bytecode()->addBranch(compareInsn, l1);
        node->right()->visit(this);
        castTosType(VT_INT);

        Label l2(bytecode());
        bytecode()->addBranch(BC_JA, l2);
        bytecode()->bind(l1);
        bytecode()->addInsn(node->kind() == tOR ? BC_ILOAD1 : BC_ILOAD0);
        bytecode()->bind(l2);
        return VT_INT;
    }

    VarType addBitwiseOp(BinaryOpNode* node) {
        node->right()->visit(this);
        VarType leftType = tosType_;
        node->left()->visit(this);
        VarType rightType = tosType_;
        if (leftType != VT_INT || rightType != VT_INT) {
            throw BytecodeGeneratorException("Invalid type for bitwise operation");
        }
        bytecode()->addInsn(bitInsn(node->kind()));
        return VT_INT;
    }

    VarType addComparisonOp(BinaryOpNode* node) {
        node->right()->visit(this);
        VarType rightType = tosType_;
        node->left()->visit(this);
        VarType leftType = tosType_;
        VarType resultType = castForBinaryOp(leftType, rightType);

        bytecode()->addInsn(resultType == VT_DOUBLE ? BC_DCMP : BC_ICMP);
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addInsn(BC_SWAP);
        Label success = Label(bytecode());
        Label after = Label(bytecode());
        bytecode()->addBranch(compareInsn(node->kind()), success);
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_JA, after);
        bytecode()->bind(success);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->bind(after);
        return VT_INT;
    }

    VarType addArithmeticOp(BinaryOpNode* node) {
        node->right()->visit(this);
        VarType rightType = tosType_;
        node->left()->visit(this);
        VarType leftType = tosType_;
        bool mod = node->kind() == tMOD;
        if (mod && (leftType != VT_INT || rightType != VT_INT)) {
            throw BytecodeGeneratorException("Invalid type for mod operation");
        }
        VarType resultType = castForBinaryOp(leftType, rightType);
        bytecode()->addInsn(arithmeticInsn(node->kind(), resultType));
        return resultType;
    }

    void addNotOp(UnaryOpNode* node) {
        node->operand()->visit(this);
        if (tosType_ != VT_INT) {
            throw BytecodeGeneratorException("Invalid type for not operation");
        }
        bytecode()->addInsn(BC_ILOAD0);
        Label l1 = Label(bytecode());
        bytecode()->addBranch(BC_IFICMPE, l1);
        bytecode()->addInsn(BC_ILOAD0);
        Label l2 = Label(bytecode());
        bytecode()->addBranch(BC_JA, l2);
        bytecode()->bind(l1);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->bind(l2);
        tosType_ = VT_INT;
    }

    void storeVar(const AstVar* var) {
        processVar(var, localStoreInsn(var->type()), contextStoreInsn(var->type()));
    }

    void loadVar(const AstVar* var) {
        processVar(var, localLoadInsn(var->type()), contextLoadInsn(var->type()));
    }

    void processVar(const AstVar* var, Instruction localInsn, Instruction contextInsn) {
        VarInfo varInfo = scope_->findVar(var->name());
        if (varInfo.scopeId == scope_->scopeId()) {
            bytecode()->addInsn(localInsn);
        } else {
            bytecode()->addInsn(contextInsn);
            bytecode()->addUInt16(varInfo.scopeId);
        }
        bytecode()->addUInt16(varInfo.varId);
        VarType type = var->type();
        tosType_ = type;
    }

    void castTosType(VarType to) {
        VarType from = tosType_;
        if (from == to) {
            return;
        }
        if (from == VT_INT && to == VT_DOUBLE) {
            bytecode()->addInsn(BC_I2D);
            return;
        }
        if (from == VT_DOUBLE && to == VT_INT) {
            bytecode()->addInsn(BC_D2I);
            return;
        }
        throw BytecodeGeneratorException("Invalid typecast");
    }

    VarType castForBinaryOp(VarType left, VarType right) {
        if (left == right) {
            return left;
        }
        if (right == VT_DOUBLE && left == VT_INT) {
            bytecode()->addInsn(BC_I2D);
            return VT_DOUBLE;
        }
        if (right == VT_INT && left == VT_DOUBLE) {
            bytecode()->addInsn(BC_SWAP);
            bytecode()->addInsn(BC_I2D);
            bytecode()->addInsn(BC_SWAP);
            return VT_DOUBLE;
        }
        throw BytecodeGeneratorException("Invalid typecast for binary operation");
    }
};

class BytecodeTranslator : public Translator {
public:
    Status* translate(const string& program, Code** code) override {
        Parser parser;

        Status* status = parser.parseProgram(program);
        if (status != NULL && status->isError()) {
            return status;
        }

        BytecodeGenerator generator(*code);
        return generator.generate(parser.top());
    }
};

Translator* Translator::create(const string& impl) {
    return new BytecodeTranslator();
}
}