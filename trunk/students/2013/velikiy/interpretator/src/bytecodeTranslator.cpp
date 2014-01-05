#include "bytecodeTranslator.h"
#include "mathvm.h"
#include "parser.h"
#include "bytecodeCode.h"
#include "ast.h"
#include "AsmJit/Build.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace std;

namespace mathvm {

    Status* BytecodeTranslator::translate(const string& program, Code** code_) {
        Parser parser;
        Status* status = parser.parseProgram(program);

        if (status != NULL && status->isError()) {
            return status;
        }

        BytecodeCode* code = new BytecodeCode();
        
        *code_ = code;
        
        BytecodeAstVisitor visitor(code);
        visitor.visitAst(parser.top());
        
//        cout << "size::" << code->globalVars()->size() << endl;
        
        return visitor.status;
    }

    void BytecodeAstVisitor::visitAst(AstFunction* fun) {
        size_t bci = 0;
        Scope::VarIterator varIt(fun->node()->body()->scope());
        
        while (varIt.hasNext()) {
            AstVar* var = varIt.next();
            bci++; // load instruction
            string name(var->name());
            code->globalVars()->insert(make_pair(name, bci));
            bci += typeToSize(var->type());
        }
//        cout << "my size:" << code->globalVars()->size() << endl;
        
        
        visitAstFunction(fun);
    }

    void BytecodeAstVisitor::visitAstFunction(AstFunction* function) {
        BytecodeFunction* fun = new BytecodeFunction(function);

        map<string, uint16_t> paramIds;

        BytecodeFunction* prevFunction = currentFunction;
        uint16_t prevContext = currentContext;

        currentContext = code->addFunction(fun);
        currentFunction = fun;

        functionsStack.push_back(currentContext);
        contextsStack.push_back(currentContext);

        for (int i = 0; i < function->parametersNumber(); i++) {
            if (function->parameterType(i) == VT_DOUBLE) {
                addInsn(BC_DLOAD);
                paramIds[function->parameterName(i)] = current();
                currentBytecode()->addDouble(0);
            }
            if (function->parameterType(i) == VT_INT) {
                addInsn(BC_ILOAD);
                paramIds[function->parameterName(i)] = current();
                currentBytecode()->addInt64(0);
            }
            if (function->parameterType(i) == VT_STRING) {
                addInsn(BC_SLOAD);
                paramIds[function->parameterName(i)] = current();
                addId(0);
            }
        }

        functionParamIds[currentContext] = paramIds;
        contextVarIds[currentContext] = map<string, uint16_t>();

        function->node()->visit(this);

        currentFunction = prevFunction;
        currentContext = prevContext;

        functionsStack.pop_back();
        contextsStack.pop_back();

    }

    void BytecodeAstVisitor::visitFunctionNode_(FunctionNode* node) {
        node->body()->visit(this);
    }

    void BytecodeAstVisitor::visitBlockNode_(BlockNode* node) {

        Scope::VarIterator varIt(node->scope());
        while (varIt.hasNext()) {
            AstVar* var = varIt.next();
            allocateVar(*var);
        }

        Scope::FunctionIterator funIt(node->scope());
        while (funIt.hasNext()) {
            AstFunction* fun = funIt.next();
            visitAstFunction(fun);
        }

        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
        }

    }

    uint16_t BytecodeAstVisitor::allocateVar(AstVar& var) {
        uint32_t beginIndex = current() + 1;
        contextVarIds[currentContext][var.name()] = beginIndex;
        if (var.type() == VT_DOUBLE) {
            addInsn(BC_DLOAD);
            currentBytecode()->addDouble(0);
            return beginIndex;
        }
        if (var.type() == VT_INT) {
            addInsn(BC_ILOAD);
            currentBytecode()->addInt64(0);
            return beginIndex;
        }
        if (var.type() == VT_STRING) {
            addInsn(BC_SLOAD);
            addId(0);
            return beginIndex;
        }
        assert(false);
    }

    void BytecodeAstVisitor::addTypedOpInsn(VarType type, TokenKind op) {
        uint32_t codeLenBefore = currentBytecode()->length();
        if (op == tADD) {
            if (type == VT_INT)
                addInsn(BC_IADD);
            if (type == VT_DOUBLE)
                addInsn(BC_DADD);
        }

        if (op == tSUB) {
            if (type == VT_INT)
                addInsn(BC_ISUB);
            if (type == VT_DOUBLE)
                addInsn(BC_DSUB);
        }

        if (op == tMUL) {
            if (type == VT_INT)
                addInsn(BC_IMUL);
            if (type == VT_DOUBLE)
                addInsn(BC_DMUL);
        }

        if (op == tDIV) {
            if (type == VT_INT)
                addInsn(BC_IDIV);
            if (type == VT_DOUBLE)
                addInsn(BC_DDIV);
        }

        if (op == tAAND) {
            if (type == VT_INT)
                addInsn(BC_IAAND);
        }

        if (op == tAXOR) {
            if (type == VT_INT)
                addInsn(BC_IAXOR);
        }

        if (op == tAOR) {
            if (type == VT_INT)
                addInsn(BC_IAOR);
        }

        assert((currentBytecode()->length()) != codeLenBefore);
    }

    void BytecodeAstVisitor::ensureType(VarType ts, VarType td, uint32_t pos) {
        if (ts == td || td == VT_VOID)
            return;
        if (ts == VT_INT && td == VT_DOUBLE) {
            int funId = currentFunction->id();
            int vv = currentBytecode()->get(pos);
            currentBytecode()->set(pos, BC_I2D);
            vv = currentBytecode()->get(pos);
            return;
        }
        if (ts == VT_DOUBLE && td == VT_INT) {
            currentBytecode()->set(pos, BC_D2I);
            return;
        }
        if (ts == VT_STRING && td == VT_INT) {
            currentBytecode()->set(pos, BC_S2I);
            return;
        }
        throw invalid_argument("Can't convert type");
    }

    void BytecodeAstVisitor::addTrueFalseJumpRegion(Instruction jumpInsn) {
        addInsn(jumpInsn);
        trueIdUnsettedPos = current();
        addId(0);
        addInsn(BC_JA);
        falseIdUnsettedPos = current();
        addId(0);
    }

    void BytecodeAstVisitor::visitBinaryLogicOpNode(BinaryOpNode* node) {

        if (node->kind() == tAND || node->kind() == tOR) {
            node->left()->visit(this);
            uint16_t leftTrueIdUnsettedPos = trueIdUnsettedPos;
            uint16_t leftFalseIdUnsettedPos = falseIdUnsettedPos;

            uint16_t rightBeginId = current();
            node->right()->visit(this);
            uint16_t rightTrueIdUnsettedPos = trueIdUnsettedPos;
            uint16_t rightFalseIdUnsettedPos = falseIdUnsettedPos;

            addTrueFalseJumpRegion(BC_JA);

            if (node->kind() == tAND) {
                setJump(leftTrueIdUnsettedPos, rightBeginId);
                // jump located before jump id
                setJump(leftFalseIdUnsettedPos, falseIdUnsettedPos - 1);
            } else {
                setJump(leftTrueIdUnsettedPos, trueIdUnsettedPos - 1);
                setJump(leftFalseIdUnsettedPos, rightBeginId);
            }

            setJump(rightTrueIdUnsettedPos, trueIdUnsettedPos - 1);
            setJump(rightFalseIdUnsettedPos, falseIdUnsettedPos - 1);
            return;
        }

        assert(logicKindToJump.find(node->kind()) != logicKindToJump.end());
        node->left()->visit(this);
        node->right()->visit(this);
        addTrueFalseJumpRegion(logicKindToJump[node->kind()]);

    }

    void BytecodeAstVisitor::visitBinaryOpNode_(BinaryOpNode* node) {
        if (logicKinds.find(node->kind()) != logicKinds.end()) {
            visitBinaryLogicOpNode(node);
            return;
        }
        if (node->kind() == tRANGE) {
            node->left()->visit(this);
            node->right()->visit(this);
            return;
        }

        node->left()->visit(this);
        VarType leftType = topType();
        uint32_t leftCastPos = current();
        addInsn(BC_INVALID); // there will be type cast

        node->right()->visit(this);
        VarType rightType = topType();

        VarType maxType = max(leftType, rightType);
        ensureType(leftType, maxType, leftCastPos);
        ensureType(rightType, maxType);
        addTypedOpInsn(maxType, node->kind());
        typesStack.push(maxType);
    }

    void BytecodeAstVisitor::visitForNode_(ForNode* node) {
        uint16_t topVar = 0;
        if (node->var()->type() == VT_INT) {
            addInsn(BC_ILOAD);
            topVar = current();
            currentBytecode()->addInt64(0);
        }

        if (node->var()->type() == VT_DOUBLE) {
            addInsn(BC_DLOAD);
            topVar = current();
            currentBytecode()->addDouble(0);
        }

        assert(topVar != 0);

        // TODO: check if var type and expression different
        node->inExpr()->visit(this);

        if (node->var()->type() == VT_INT) {
            addInsn(BC_STOREIVAR);
            addId(topVar);
            addInsn(BC_STOREIVAR);
        }
        if (node->var()->type() == VT_DOUBLE) {
            addInsn(BC_STOREDVAR);
            addId(topVar);
            addInsn(BC_STOREDVAR);
        }
        //        addId(astVarsContext[node->var()]);
        addId(findVarLocal(node->var()->name()));


        uint16_t forConditionId = current();

        if (node->var()->type() == VT_INT) {
            addInsn(BC_LOADIVAR);
            addId(findVarLocal(node->var()->name()));
            addInsn(BC_LOADIVAR);
            addId(topVar);
        }

        if (node->var()->type() == VT_DOUBLE) {
            addInsn(BC_LOADDVAR);
            addId(findVarLocal(node->var()->name()));
            addInsn(BC_LOADDVAR);
            addId(topVar);
        }

        addTrueFalseJumpRegion(BC_IFICMPLE);

        uint16_t bodyBegin = current();
        node->body()->visit(this);

        if (node->var()->type() == VT_INT) {
            addInsn(BC_LOADIVAR);
            addId(findVarLocal(node->var()->name()));
            addInsn(BC_ILOAD1);
            addInsn(BC_IADD);
            addInsn(BC_STOREIVAR);
            addId(findVarLocal(node->var()->name()));
        }
        if (node->var()->type() == VT_DOUBLE) {
            addInsn(BC_LOADDVAR);
            addId(findVarLocal(node->var()->name()));
            addInsn(BC_DLOAD1);
            addInsn(BC_DADD);
            addInsn(BC_STOREDVAR);
            addId(findVarLocal(node->var()->name()));
        }
        addInsn(BC_JA);
        addId(0);
        setJump(current() - 2, forConditionId);

        setTrueJump(bodyBegin);
        setFalseJump(current());
        addInsn(BC_INVALID);

    }

    void BytecodeAstVisitor::visitWhileNode_(WhileNode* node) {

        uint16_t whileCondition = current();
        node->whileExpr()->visit(this);

        uint16_t bodyBegin = current();
        node->loopBlock()->visit(this);
        addInsn(BC_JA);
        addId(0);
        setJump(current() - 2, whileCondition);

        setTrueJump(bodyBegin);
        setFalseJump(current());
        addInsn(BC_INVALID);

        typesStack.push(VT_VOID);
    }

    void BytecodeAstVisitor::visitIfNode_(IfNode* node) {

        addInsn(BC_JA);
        uint32_t ifBeginId = current();
        addId(0);
        node->thenBlock()->visit(this);
        addInsn(BC_JA);
        uint32_t thenEndId = current();
        addId(0);
        uint32_t elseBeginId = current();
        if (node->elseBlock() != NULL)
            node->elseBlock()->visit(this);
        addInsn(BC_JA);
        uint32_t elseEndId = current();
        addId(0);

        uint32_t ifExprBeginId = current();
        setJump(ifBeginId, ifExprBeginId);

        node->ifExpr()->visit(this);
        setTrueJump(ifBeginId + 2);
        setFalseJump(elseBeginId);

        uint32_t ifExprEndId = current();
        setJump(thenEndId, (uint16_t) ifExprEndId);
        setJump(elseEndId, (uint16_t) ifExprEndId);
        addInsn(BC_INVALID); // just idle

    }

    pair<uint16_t, uint16_t> BytecodeAstVisitor::findVar(const string& name, bool onlyCurrentContext) {

        size_t stackI = contextsStack.size() - 1;

        while (true) {
            uint16_t cctx = contextsStack[stackI];
            if (contextVarIds[cctx].find(name) != contextVarIds[cctx].end()) {
                return make_pair(cctx, contextVarIds[cctx][name]);
            }
            if (functionParamIds[cctx].find(name) != functionParamIds[cctx].end()) {
                return make_pair(cctx, functionParamIds[cctx][name]);
            }

            if (onlyCurrentContext)
                break;

            if (stackI == 0)
                break;
            stackI--;

        }

        if (onlyCurrentContext)
            throw logic_error("cant find name " + name + "[local]");
        throw logic_error("cant find name " + name);

        return make_pair(0, 0);

    }

    void BytecodeAstVisitor::loadVar(const AstVar* var) {

        //        cout << "load var " << var->name() << " :: " << (void*) var << endl;
        //        cout << "owner " << (void*) var->owner() << endl;

        pair<uint16_t, uint16_t> ids = findVar(var->name());

        if (var->type() == VT_DOUBLE) {
            if (ids.first != contextsStack.back())
                addInsn(BC_LOADCTXDVAR);
            else
                addInsn(BC_LOADDVAR);
        }
        if (var->type() == VT_INT) {
            if (ids.first != contextsStack.back())
                addInsn(BC_LOADCTXIVAR);
            else
                addInsn(BC_LOADIVAR);
        }
        if (var->type() == VT_STRING) {
            if (ids.first != contextsStack.back())
                addInsn(BC_LOADCTXSVAR);
            else
                addInsn(BC_LOADSVAR);
        }

        if (ids.first != contextsStack.back())
            addId(ids.first);
        addId(ids.second);

        typesStack.push(var->type());
    }

    void BytecodeAstVisitor::visitLoadNode_(LoadNode* node) {
        loadVar(node->var());
    }

    void BytecodeAstVisitor::visitCallNode_(CallNode* node) {
        TranslatedFunction* fun = code->functionByName(node->name());
        if (fun == NULL) {
            status = new Status("Undefined function call", node->position());
            return;
        }
        if (node->parametersNumber() != fun->parametersNumber()) {
            stringstream ss;
            ss << "Parameters number mismatch: " << node->parametersNumber()
                    << " vs " << fun->parametersNumber();
            status = new Status(ss.str());
            return;
        }
        assert(fun);
        
        for (int i = node->parametersNumber() - 1; i >= 0; i--) {
            node->parameterAt(i)->visit(this);
            ensureType(fun->parameterType(i));
        }
        addInsn(BC_CALL);
        addId(fun->id());
        typesStack.push(fun->returnType());
    }

    // TODO: 

    void BytecodeAstVisitor::visitNativeCallNode_(NativeCallNode* node) {
        typesStack.push(node->nativeSignature()[0].first);
    }

    void BytecodeAstVisitor::visitPrintNode_(PrintNode* node) {
        for (size_t i = 0; i < node->operands(); i++) {
            AstNode* opn = node->operandAt(i);
            opn->visit(this);

            if (topType() == VT_INT) {
                addInsn(BC_IPRINT);
                continue;
            }
            if (topType() == VT_DOUBLE) {
                addInsn(BC_DPRINT);
                continue;
            }
            if (topType() == VT_STRING) {
                addInsn(BC_SPRINT);
                continue;
            }
            assert(false);
        }
        typesStack.push(VT_VOID);
    }

    void BytecodeAstVisitor::visitReturnNode_(ReturnNode* node) {
        if (node->returnExpr() != NULL) {
            node->returnExpr()->visit(this);
            if (currentFunction->returnType() != VT_VOID)
                ensureType(currentFunction->returnType());
        }
        addInsn(BC_RETURN);
    }

    void BytecodeAstVisitor::visitStoreNode_(StoreNode* node) {
        pair<uint16_t, uint16_t> ids = findVar(node->var()->name());
        node->value()->visit(this);
        if (node->op() == tINCRSET || node->op() == tDECRSET) {
            ensureType(topType(), node->var()->type());
            loadVar(node->var());
        }
        if (node->op() == tINCRSET) {
            addTypedOpInsn(node->var()->type(), tADD);
            goto STORE_TO_VAR;
        }

        if (node->op() == tDECRSET) {
            addTypedSwap(node->var()->type());
            addTypedOpInsn(node->var()->type(), tSUB);
            goto STORE_TO_VAR;
        }

        if (node->op() == tEQ)
            goto STORE_TO_VAR;

STORE_TO_VAR:
        ensureType(node->var()->type());
        const AstVar* var = node->var();
        if (var->type() == VT_DOUBLE) {
            if (ids.first != contextsStack.back())
                addInsn(BC_STORECTXDVAR);
            else
                addInsn(BC_STOREDVAR);
        }
        if (var->type() == VT_INT) {
            if (ids.first != contextsStack.back())
                addInsn(BC_STORECTXIVAR);
            else
                addInsn(BC_STOREIVAR);
        }
        if (var->type() == VT_STRING) {
            if (ids.first != contextsStack.back())
                addInsn(BC_STORECTXSVAR);
            else
                addInsn(BC_STORESVAR);
        }
        if (ids.first != contextsStack.back())
            currentBytecode()->addInt16(ids.first);
        currentBytecode()->addInt16(ids.second);
    }

    void BytecodeAstVisitor::visitDoubleLiteralNode_(DoubleLiteralNode* node) {
        addInsn(BC_DLOAD);
        currentBytecode()->addDouble(node->literal());
        typesStack.push(VT_DOUBLE);
    }

    void BytecodeAstVisitor::visitIntLiteralNode_(IntLiteralNode* node) {
        addInsn(BC_ILOAD);
        currentBytecode()->addInt64(node->literal());
        typesStack.push(VT_INT);
    }

    void BytecodeAstVisitor::visitStringLiteralNode_(StringLiteralNode* node) {
        addInsn(BC_SLOAD);
        uint16_t strId = code->makeStringConstant(node->literal());
        addId(strId);
        typesStack.push(VT_STRING);
    }

    void BytecodeAstVisitor::visitUnaryOpNode_(UnaryOpNode* node) {
        if (node->kind() == tNOT) {
            node->visitChildren(this);
            swap(trueIdUnsettedPos, falseIdUnsettedPos);
            return;
        }
        if (node->kind() == tSUB) {
            node->visitChildren(this);
            if (topType() == VT_INT) {
                addInsn(BC_ILOAD);
                currentBytecode()->addInt64(-1);
                addInsn(BC_IMUL);
                return;
            }
            if (topType() == VT_DOUBLE) {
                addInsn(BC_DLOAD);
                currentBytecode()->addDouble(-1.0);
                addInsn(BC_DMUL);
                return;
            }
        }

        assert(false);
    }

    bool BytecodeAstVisitor::beforeVisit() {
        if (status == NULL)
            return false;
        return true;
    }

}

