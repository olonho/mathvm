#include <string>
#include <algorithm>
#include "include/bytecode_translator.h"
#include "parser.h"
#include "include/interpreter_code.h"

namespace mathvm {

    map<uint16_t, map<uint16_t, Var*>>* inverseAstVarsByScopesMap(const map<uint16_t, map<const AstVar *, uint16_t>>* initialMap,
                                                                  map<uint16_t, Var*>* varIds) {
        auto resultMap = new map<uint16_t, map<uint16_t, Var*>>();
        for (auto mapElem : *initialMap) {
            for (auto elem : mapElem.second) {
                (*resultMap)[mapElem.first][elem.second] = new Var(elem.first->type(), elem.first->name());
                (*varIds)[elem.second] = (*resultMap)[mapElem.first][elem.second];
            }
        }
        return resultMap;
    };

    Status* BytecodeTranslator::translate(const string& program, Code** code) {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status->isOk()) {
            extractScopesAndVars(parser.top()->scope());
            auto varsIds = new map<uint16_t, Var*>();
            auto inverseVarsByScopes = inverseAstVarsByScopesMap(_varsPerScopes, varsIds);
            _code = new InterpreterCode(varsIds, inverseVarsByScopes, getTopScopeVarsNames(), _scopesChilds);
            *code = this->_code;
            extractFunctions(parser.top()->scope());


            for (auto func : *_astFunctions) {
                auto visitor = AstBytecodeVisitor(_code, _scopes, _varsIds, _functionsIds, _varsPerScopes);
                Bytecode* bytecode = visitor.getBytecode(func.first, func.second);
                dynamic_cast<BytecodeFunctionImpl*>(func.first)->setBytecode(bytecode);
            }
        }
        return status;
    }

    map<const string, uint16_t>* BytecodeTranslator::getTopScopeVarsNames() {
        auto * varsFromOuter = new map<const string, uint16_t>();
        for (auto mapElem : (*_varsPerScopes)[1]) {
            varsFromOuter->insert(make_pair(mapElem.first->name(), mapElem.second));
        }
        return varsFromOuter;
    };

    uint16_t BytecodeTranslator::extractScopesAndVars(Scope *initialScope) {
        auto initialScopeId = uint16_t(_scopes->size());
        (*_scopes)[initialScope] = initialScopeId;
        (*_varsPerScopes)[initialScopeId] = map<const AstVar*, uint16_t >();
        Scope::VarIterator varIterator(initialScope);
        while (varIterator.hasNext()) {
            AstVar* astVar = varIterator.next();
            auto varId = uint16_t(_varsIds->size());
            (*_varsIds)[astVar] = varId;
            (*_varsPerScopes)[initialScopeId][astVar] = varId;
        }
        uint32_t childScopeNumber = initialScope->childScopeNumber();
        for (uint32_t i = 0; i < childScopeNumber; i++) {
            uint16_t childScopeId = extractScopesAndVars(initialScope->childScopeAt(i));
            (*_scopesChilds)[initialScopeId].push_back(childScopeId);
        }
        return initialScopeId;
    }

    void BytecodeTranslator::extractFunctions(Scope* initialScope) {
        unsigned long childScopeNumber = initialScope->childScopeNumber();
        for (uint32_t i = 0; i < childScopeNumber; i++) {
            extractFunctions(initialScope->childScopeAt(i));
        }
        Scope::FunctionIterator functionIterator(initialScope);
        while(functionIterator.hasNext()) {
            AstFunction* nextFunction = functionIterator.next();
            auto * bytecodeFunction = new BytecodeFunctionImpl(nextFunction);
            (*_astFunctions)[bytecodeFunction] = nextFunction;
            uint16_t functionId = _code->addFunction(bytecodeFunction);
            (*_functionsIds)[bytecodeFunction] = functionId;
            bytecodeFunction->setScopeId((*_scopes)[nextFunction->scope()]);
        }
    }

    Bytecode* AstBytecodeVisitor::getBytecode(TranslatedFunction* function, AstFunction* astFunction) {
        uint32_t parsNumber = function->parametersNumber();
        if (parsNumber > 0) {
            for (uint32_t i = parsNumber - 1; i >= 0; i--) {
                switch (function->parameterType(i)) {
                    case VarType::VT_DOUBLE :
                        _bytecode->addInsn(Instruction::BC_STORECTXDVAR);
                        break;
                    case VarType::VT_INT :
                        _bytecode->addInsn(Instruction::BC_STORECTXIVAR);
                        break;
                    case VarType::VT_STRING:
                        _bytecode->addInsn(Instruction::BC_STORECTXSVAR);
                        break;
                    default:
                        cerr << "Wrong function parameter type" << endl;
                }
                _bytecode->addUInt16(function->scopeId());
                const string &parName = function->parameterName(i);
                uint16_t varId = (*_varsIds)[astFunction->scope()->lookupVariable(parName)];
                _bytecode->addUInt16(varId);
                if (i == 0) break;
            }
        }
        astFunction->node()->visitChildren(this);
        if (_bytecode->getInsn(_bytecode->current() - 1) != BC_LAST) {
            _bytecode->addInsn(BC_LAST);
        }
        return _bytecode;
    }

    void AstBytecodeVisitor::booleanBinOperation(BinaryOpNode *node) {
        switch(node->kind()) {
            case TokenKind::tOR :
            case TokenKind::tAOR :
                _bytecode->addInsn(Instruction::BC_IAOR);
                break;
            case TokenKind::tAND :
            case TokenKind::tAAND :
                _bytecode->addInsn(Instruction::BC_IAAND);
                break;
            case TokenKind::tAXOR :
                _bytecode->addInsn(Instruction::BC_IAXOR);
                break;
            default:
            cerr << "Wrong boolean operation";
        }
    }

    void AstBytecodeVisitor::relationBinOperation(BinaryOpNode *node) {
        toOneType();
        Label labelToLoadTrue(_bytecode);
        Label labelToEnd(_bytecode);
        if (typesInStack.top() == VT_DOUBLE) {
            _bytecode->addInsn(BC_DCMP);
            _bytecode->addInsn(BC_LOADIVAR0);
            _bytecode->addInsn(BC_SWAP);
        }
        switch(node->kind()) {
            case TokenKind::tEQ :
                _bytecode->addBranch(Instruction::BC_IFICMPE, labelToLoadTrue);
                break;
            case TokenKind::tNEQ :
                _bytecode->addBranch(Instruction::BC_IFICMPNE, labelToLoadTrue);
                break;
            case TokenKind::tGT :
                _bytecode->addBranch(Instruction::BC_IFICMPG, labelToLoadTrue);
                break;
            case TokenKind::tGE :
                _bytecode->addBranch(Instruction::BC_IFICMPGE, labelToLoadTrue);
                break;
            case TokenKind::tLT :
                _bytecode->addBranch(Instruction::BC_IFICMPL, labelToLoadTrue);
                break;
            case TokenKind::tLE :
                _bytecode->addBranch(Instruction::BC_IFICMPLE, labelToLoadTrue);
                break;
            default:
            cerr << "Wrong relation operation" << endl;
        }
        _bytecode->addInsn(Instruction::BC_ILOAD0);
        _bytecode->addBranch(Instruction::BC_JA, labelToEnd);
        _bytecode->bind(labelToLoadTrue);
        _bytecode->addInsn(Instruction::BC_ILOAD1);
        _bytecode->bind(labelToEnd);
        typesInStack.pop();
        typesInStack.push(VT_INT);
    }

    void AstBytecodeVisitor::mathBinOperation(BinaryOpNode *node) {
        toOneType();
        VarType resultType = typesInStack.top();
        if (resultType == VarType::VT_INT) {
            switch(node->kind()) {
                case TokenKind::tADD :
                    _bytecode->addInsn(Instruction::BC_IADD);
                    break;
                case TokenKind::tSUB :
                    _bytecode->addInsn(Instruction::BC_ISUB);
                    break;
                case TokenKind::tMUL :
                    _bytecode->addInsn(Instruction::BC_IMUL);
                    break;
                case TokenKind::tDIV :
                    _bytecode->addInsn(Instruction::BC_IDIV);
                    break;
                case TokenKind::tMOD :
                    _bytecode->addInsn(Instruction::BC_IMOD);
                    break;
                case TokenKind::tINCRSET :
                    break;
                case TokenKind::tDECRSET :
                    break;
                default:
                    cerr << "Wrong math bin operation";
            }
        } else if (resultType == VarType::VT_DOUBLE){
                switch (node->kind()) {
                case TokenKind::tADD :
                    _bytecode->addInsn(Instruction::BC_DADD);
                break;
                case TokenKind::tSUB :
                    _bytecode->addInsn(Instruction::BC_DSUB);
                break;
                case TokenKind::tMUL :
                    _bytecode->addInsn(Instruction::BC_DMUL);
                break;
                case TokenKind::tDIV :
                    _bytecode->addInsn(Instruction::BC_DDIV);
                break;
                case TokenKind::tINCRSET :
                    break;
                case TokenKind::tDECRSET :
                    break;
                default:
                    cerr << "Wrong math bin operation";
            }
        }
    }

    void AstBytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
        node->right()->visit(this);
        node->left()->visit(this);
        switch(node->kind()) {
            case TokenKind::tOR :
            case TokenKind::tAND :
            case TokenKind::tAOR :
            case TokenKind::tAAND :
            case TokenKind::tAXOR :
                booleanBinOperation(node);
                break;
            case TokenKind::tEQ :
            case TokenKind::tNEQ :
            case TokenKind::tGT :
            case TokenKind::tGE :
            case TokenKind::tLT :
            case TokenKind::tLE :
                relationBinOperation(node);
                break;
            case TokenKind::tADD :
            case TokenKind::tSUB :
            case TokenKind::tMUL :
            case TokenKind::tDIV :
            case TokenKind::tMOD :
            case TokenKind::tINCRSET :
            case TokenKind::tDECRSET :
                mathBinOperation(node);
                break;
            default:
            cerr << "Wrong binary operation" << endl;
        }
    }

    void AstBytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node) {
        node->operand()->visit(this);
        VarType currentType = typesInStack.top();
        switch(node->kind()) {
            case TokenKind::tNOT :
                _bytecode->addInsn(Instruction::BC_ILOAD1);
                _bytecode->addInsn(Instruction::BC_IAXOR);
                break;
            case TokenKind::tSUB :
                if (currentType == VarType::VT_INT) {
                    _bytecode->addInsn(Instruction::BC_INEG);
                } else if (currentType == VarType::VT_DOUBLE) {
                    _bytecode->addInsn(Instruction::BC_DNEG);
                }
                break;
            default:
            cerr << "Wrong operation to unary node";
        }
    }

    void AstBytecodeVisitor::visitStringLiteralNode(StringLiteralNode* node) {
        _bytecode->addInsn(Instruction::BC_SLOAD);
        uint16_t stringId = _code->makeStringConstant(node->literal());
        _bytecode->addUInt16(stringId);
        typesInStack.push(VarType::VT_STRING);
    }

    void AstBytecodeVisitor::visitIntLiteralNode(IntLiteralNode* node) {
        _bytecode->addInsn(Instruction::BC_ILOAD);
        _bytecode->addTyped(node->literal());
        typesInStack.push(VarType::VT_INT);
    }

    void AstBytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        _bytecode->addInsn(Instruction::BC_DLOAD);
        _bytecode->addDouble(node->literal());
        typesInStack.push(VarType::VT_DOUBLE);
    }

    void AstBytecodeVisitor::visitLoadNode(LoadNode* node) {
        switch(node->var()->type()) {
            case VarType::VT_DOUBLE:
                _bytecode->addInsn(Instruction::BC_LOADCTXDVAR);
                break;
            case VarType::VT_INT:
                _bytecode->addInsn(Instruction::BC_LOADCTXIVAR);
                break;
            case VarType::VT_STRING:
                _bytecode->addInsn(Instruction::BC_LOADCTXSVAR);
                break;
            default:
            cerr << "Wrong loadNode var type";
        }
        uint16_t ownerScopeId = _scopes->at(node->var()->owner());
        _bytecode->addUInt16(_scopes->at(node->var()->owner()));
        uint16_t index = (*_varsPerScopes)[ownerScopeId][node->var()];
        _bytecode->addUInt16(index);
        typesInStack.push(node->var()->type());
    }

    void AstBytecodeVisitor::toOneType() {
        VarType type1 = typesInStack.top();
        typesInStack.pop();
        VarType type2 = typesInStack.top();
        typesInStack.pop();
        if (type1 == VT_INT) {
            if (type2 == VT_INT) {
                typesInStack.push(VT_INT);
            } else if (type2 == VT_DOUBLE) {
                _bytecode->addInsn(BC_I2D);
                typesInStack.push(VT_DOUBLE);
            }
        } else if (type1 == VT_DOUBLE) {
            if (type2 == VT_DOUBLE) {
                typesInStack.push(VT_DOUBLE);
            } else if (type2 == VT_INT){
                _bytecode->addInsn(BC_SWAP);
                _bytecode->addInsn(BC_I2D);
                _bytecode->addInsn(BC_SWAP);
                typesInStack.push(VT_DOUBLE);
            }
        }
    }

    void AstBytecodeVisitor::visitStoreNode(StoreNode* node) {
        node->value()->visit(this);
        VarType valueType = typesInStack.top();
        uint16_t  ownerScopeId = _scopes->at(node->var()->owner());
        auto index = (*_varsPerScopes)[ownerScopeId][node->var()];

        if (node->op() == TokenKind::tINCRSET || node->op() == TokenKind::tDECRSET) {
            switch(node->var()->type()) {
                case VarType::VT_DOUBLE:
                    _bytecode->addInsn(BC_LOADCTXDVAR);
                    break;
                case VarType::VT_INT:
                    _bytecode->addInsn(BC_LOADCTXIVAR);
                    break;
                case VarType::VT_STRING:
                    _bytecode->addInsn(BC_LOADCTXSVAR);
                    break;
                default:
                    cerr << "Wrong var type for store";
            }
            _bytecode->addUInt16(ownerScopeId);
            _bytecode->addUInt16(index);
            typesInStack.push(node->var()->type());
            toOneType();
            if (node->op() == tINCRSET) {
                switch(typesInStack.top()) {
                    case VT_DOUBLE:
                        _bytecode->addInsn(BC_DADD);
                        break;
                    case VT_INT:
                        _bytecode->addInsn(BC_IADD);
                        break;
                    default:
                        cerr << "Wrong variable type for addition";
                }
            } else if (node->op() == tDECRSET) {
                switch(typesInStack.top()) {
                    case VT_DOUBLE:
                        _bytecode->addInsn(BC_DSUB);
                        break;
                    case VT_INT:
                        _bytecode->addInsn(BC_ISUB);
                        break;
                    default:
                        cerr << "Wrong variable type for subtraction";
                }
            }
        }
        switch (valueType) {
            case VarType::VT_DOUBLE:
                _bytecode->addInsn(Instruction::BC_STORECTXDVAR);
                break;
            case VarType::VT_INT:
                _bytecode->addInsn(Instruction::BC_STORECTXIVAR);
                break;
            case VarType::VT_STRING:
                _bytecode->addInsn(Instruction::BC_STORECTXSVAR);
                break;
            default:
            cerr << "Wrong value type";
        }
        typesInStack.pop();
        _bytecode->addUInt16(ownerScopeId);
        _bytecode->addUInt16(index);
    }

    void AstBytecodeVisitor::visitBlockNode(BlockNode* node) {
        uint32_t nodesNumber = node->nodes();
        for (uint32_t i = 0; i < nodesNumber; i++) {
            (node->nodeAt(i))->visit(this);
        }
    }

    void AstBytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {
    }

    void AstBytecodeVisitor::visitForNode(ForNode* node) {
        Label labelToForBody(_bytecode);
        Label labelToEnd(_bytecode);
        BinaryOpNode* binNode = node->inExpr()->asBinaryOpNode();
        binNode->right()->visit(this);
        binNode->left()->visit(this);
        _bytecode->bind(labelToForBody);
        switch (node->var()->type()) {
            case VarType::VT_INT:
                _bytecode->addInsn(Instruction::BC_STORECTXIVAR);
                break;
            default:
            cerr << "Wrong var type for forNode";
        }
        uint16_t ownerScopeId = _scopes->at(node->var()->owner());
        uint16_t varId = _varsIds->at(node->var());
        _bytecode->addUInt16(ownerScopeId);
        _bytecode->addUInt16(varId);
        binNode->right()->visit(this);
        switch (node->var()->type()) {
            case VarType::VT_INT:
                _bytecode->addInsn(Instruction::BC_LOADCTXIVAR);
                break;
            default:
                cerr << "Wrong var type for forNode";
        }
        _bytecode->addUInt16(ownerScopeId);
        _bytecode->addUInt16(varId);

        node->body()->visit(this);
        _bytecode->addInsn(BC_ILOAD1);
        _bytecode->addInsn(BC_IADD);
        _bytecode->addInsn(BC_SWAP);
        switch (node->var()->type()) {
            case VarType::VT_INT:
                _bytecode->addInsn(Instruction::BC_LOADCTXIVAR);
                break;
            default:
                cerr << "Wrong var type for forNode";
        }
        _bytecode->addUInt16(ownerScopeId);
        _bytecode->addUInt16(varId);
        _bytecode->addInsn(BC_ILOAD1);
        _bytecode->addInsn(BC_IADD);
        _bytecode->addBranch(Instruction::BC_IFICMPLE, labelToForBody);
        _bytecode->addInsn(BC_POP);
        _bytecode->addInsn(BC_POP);
    }

    void AstBytecodeVisitor::visitWhileNode(WhileNode* node) {
        Label labelToWhileCondition(_bytecode);
        Label labelToWhileBody(_bytecode);
        Label labelToEnd(_bytecode);
        _bytecode->bind(labelToWhileCondition);
        node->whileExpr()->visit(this);
        _bytecode->addInsn(Instruction::BC_ILOAD1);
        _bytecode->addBranch(Instruction::BC_IFICMPE, labelToWhileBody);
        _bytecode->addBranch(Instruction::BC_JA, labelToEnd);
        _bytecode->bind(labelToWhileBody);
        node->loopBlock()->visit(this);
        _bytecode->addBranch(BC_JA, labelToWhileCondition);
        _bytecode->bind(labelToEnd);
    }

    void AstBytecodeVisitor::visitIfNode(IfNode* node) {
        AstNode* elseBlock = node->elseBlock();
        Label labelToThen(_bytecode);
        Label labelToEnd(_bytecode);
        node->ifExpr()->visit(this);
        _bytecode->addInsn(Instruction::BC_ILOAD1);
        _bytecode->addBranch(Instruction::BC_IFICMPE, labelToThen);
        if (elseBlock) {
            elseBlock->visit(this);
        }
        _bytecode->addBranch(Instruction::BC_JA, labelToEnd);
        _bytecode->bind(labelToThen);
        node->thenBlock()->visit(this);
        _bytecode->bind(labelToEnd);
    }

    void AstBytecodeVisitor::visitReturnNode(ReturnNode* node) {
        AstNode* returnNode = node->returnExpr();
        if (returnNode) {
            returnNode->visit(this);
        }
        _bytecode->addInsn(BC_LAST);
    }

    void AstBytecodeVisitor::visitFunctionNode(FunctionNode* node) {
    }

    void AstBytecodeVisitor::visitCallNode(CallNode* node) {
        auto function = _code->functionByName(node->name());
        uint16_t funId = (*_functionsIds)[function];
        uint32_t parsNumber = node->parametersNumber();
        for (uint32_t i = 0; i < parsNumber; i++) {
            node->parameterAt(i)->visit(this);
        }
        _bytecode->addInsn(Instruction::BC_CALL);
        _bytecode->addUInt16(funId);
        if (function->returnType() != VarType::VT_VOID) {
            typesInStack.push(function->returnType());
        }
    }

    void AstBytecodeVisitor::visitPrintNode(PrintNode* node) {
        uint32_t operandsNumber = node->operands();
        for (uint32_t i = 0; i < operandsNumber; i++) {
            node->operandAt(i)->visit(this);
            if (typesInStack.top() == VarType::VT_DOUBLE) {
                _bytecode->addInsn(Instruction::BC_DPRINT);
            } else if (typesInStack.top() == VarType::VT_INT) {
                _bytecode->addInsn(Instruction::BC_IPRINT);
            } else if (typesInStack.top() == VarType::VT_STRING) {
                _bytecode->addInsn(Instruction::BC_SPRINT);
            }
            typesInStack.pop();
        }
    }
}
