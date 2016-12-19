#include "mathvm.h"
#include "interpreter_code_impl.h"
#include "parser.h"
#include "ast_node_type_resolver.h"
#include "bytecode_translator_context.h"

#include <stack>
#include <vector>
#include <dlfcn.h>
#include <memory>
#include <iostream>

namespace mathvm {

namespace {

class BytecodeGeneratorVisitor: public AstVisitor {
public:

    using StatusPtr = std::shared_ptr<Status>;

    BytecodeGeneratorVisitor(AstFunction* astTop, InterpreterCodeImpl* code)
        : _code(code), _status(Status::Ok()), _context(nullptr), _type_resolver(astTop) {
        BytecodeFunction* bytecodeFunction = new BytecodeFunction(astTop);
        _code->addFunction(bytecodeFunction);
        try {
            traverseAstFunction(astTop);
        } catch (ContextLookupException& e) {
            _status.reset(Status::Error(e.what(), e.get_position()));
        }
    }

    ~BytecodeGeneratorVisitor() {
        delete _context;
    }

    StatusPtr getStatus() const {
        return _status;
    }

#define VISITOR_FUNCTION(type, name) \
      virtual void visit##type(type* node);

      FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION

private:
    InterpreterCodeImpl* _code;
    StatusPtr _status;
    BytecodeTranslatorContext* _context;
    type_resolver::AstNodeTypeResolver _type_resolver;

    Bytecode* get_bytecode() {
        return _context->get_bytecode_function()->bytecode();
    }

    void traverseAstFunction(AstFunction *astFunction) {
        const auto& name = astFunction->name();
        auto scope = astFunction->scope();
        auto fun = static_cast<BytecodeFunction*>(_code->functionByName(name));
        auto newContext = new BytecodeTranslatorContext(_context, fun, scope);

        _context = newContext;

        for (uint32_t i = 0; i < astFunction->parametersNumber(); ++i) {
            const auto& variableName = astFunction->parameterName(i);
            auto variable = astFunction->scope()->lookupVariable(variableName, false);
            loadVariable(variable, astFunction->node()->position());
        }

        visit(astFunction->node());

        fun->setScopeId(_context->get_id());
        fun->setLocalsNumber(_context->get_locals_count());

        _context = _context->get_parent();

        delete newContext;
    }

    void cast(VarType from, VarType to, uint32_t position) {
        if (from == to) {
            return;
        }
        auto bytecode = get_bytecode();

        if (from == VT_INT && to == VT_DOUBLE) {
            bytecode->addInsn(BC_I2D);
            return;
        }
        if (from == VT_DOUBLE && to == VT_INT) {
            bytecode->addInsn(BC_D2I);
            return;
        }
        if (from == VT_STRING && to == VT_INT) {
            bytecode->addInsn(BC_S2I);
        }

        if (from == VT_STRING && to == VT_DOUBLE) {
            bytecode->addInsn(BC_S2I);
            bytecode->addInsn(BC_I2D);
            return;
        }

        _status.reset(Status::Error("Cannot perform cast", position));
    }

    void generateBooleanAndOr(BinaryOpNode* node) {
        static std::map<VarType, Instruction> zeroInstructions = {
            {VT_INT,    BC_ILOAD0}, 
            {VT_DOUBLE, BC_DLOAD0}, 
            {VT_STRING, BC_SLOAD0}
        };
        auto bytecode = get_bytecode();
        auto l = node->left();
        auto r = node->right();
        auto lt = _type_resolver(l);
        auto rt = _type_resolver(r);
        auto kind = node->kind();

        auto minEvaluation = kind == tOR ? BC_ILOAD1 : BC_ILOAD0;
        auto maxEvaluation = kind == tOR ? BC_ILOAD0 : BC_ILOAD1;
        auto cmpInstruction = kind == tOR ? BC_IFICMPE : BC_IFICMPNE;

        visit(l);
        if (!zeroInstructions.count(lt)) {
            _status.reset(Status::Error("Invalid left operamd type of BinOp", l->position()));
            return;
        }
        bytecode->addInsn(zeroInstructions[lt]);
        Label longLabel(bytecode);
        bytecode->addBranch(cmpInstruction, longLabel);

        bytecode->addInsn(minEvaluation);

        Label endLabel(bytecode);
        bytecode->addBranch(BC_JA, endLabel);
        longLabel.bind(bytecode->current());
        
        visit(r);
        if (!zeroInstructions.count(rt)) {
            _status.reset(Status::Error("Invalid right operamd type of BinOp", r->position()));
            return;
        }

        Label shortLabel(bytecode);
        bytecode->addBranch(cmpInstruction, shortLabel);
        bytecode->addInsn(minEvaluation);
        bytecode->addBranch(BC_JA, endLabel);
        bytecode->bind(shortLabel);
        bytecode->addInsn(maxEvaluation);
        bytecode->bind(endLabel);

    }

    void generateIntegerAndOrXorMod(BinaryOpNode* node) {
        static const std::map<TokenKind , Instruction> instructions = {
            {tAOR, BC_IAOR}, 
            {tAAND, BC_IAAND}, 
            {tAXOR, BC_IAXOR},
            {tMOD, BC_IMOD}
        };

        auto l = node->left();
        auto r = node->right();
        auto lt = _type_resolver(l);
        auto rt = _type_resolver(r);

        visit(r);
        visit(l);

        if (lt != VT_INT || rt != VT_INT) {
            _status.reset(Status::Error("Arguments must be integers", node -> position()));
            return;
        }

        get_bytecode()->addInsn(instructions.at(node->kind()));
    }

    void generateComparisonOp1(BinaryOpNode* node) {
        static const std::map<TokenKind, Instruction> returnTrueInstructions = {
            {tEQ,  BC_ILOAD0},
            {tNEQ, BC_ILOAD0},
            {tGE,  BC_ILOADM1},
            {tLE,  BC_ILOAD1},
            {tLT,  BC_ILOADM1},
            {tGT,  BC_ILOAD1}
        };
        static const std::map<TokenKind, Instruction> returnFalseInstructions = {
            {tEQ,  BC_IFICMPNE},
            {tNEQ, BC_IFICMPE},
            {tGE,  BC_IFICMPE},
            {tLE,  BC_IFICMPE},
            {tLT,  BC_IFICMPNE},
            {tGT,  BC_IFICMPNE}
        };

        auto bytecode = get_bytecode();
        auto l = node->left();
        auto r = node->right();
        auto lt = _type_resolver(l);
        auto rt = _type_resolver(r);
        auto kind = node->kind();

        if (rt == VT_STRING || lt == VT_STRING) {
            _status.reset(Status::Error("Cannot compare strings", node->position()));
            return;
        }
        auto commonType = (rt == VT_DOUBLE || lt == VT_DOUBLE) ? VT_DOUBLE : VT_INT;

        visit(r);
        cast(rt, commonType, r->position());

        visit(l);
        cast(lt, commonType, l->position());

        bytecode->addInsn(commonType == VT_DOUBLE ? BC_DCMP : BC_ICMP);

        if (!returnTrueInstructions.count(kind)) {
            _status.reset(Status::Error("Corrupted AST. Unknown comparison operator", node->position()));
            return;
        }
        bytecode->addInsn(returnTrueInstructions.at(kind));
        Label falseLabel(bytecode);
        bytecode->addBranch(returnFalseInstructions.at(kind), falseLabel);
        bytecode->addInsn(BC_ILOAD1);
        Label trueLabel(bytecode);
        bytecode->addBranch(BC_JA, trueLabel);
        bytecode->bind(falseLabel);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->bind(trueLabel);
        std::cout << "----\n";
    }

    void generateComparisonOp(BinaryOpNode* node) {

        static const std::map<TokenKind, Instruction> instructions = {
                {tEQ,  BC_IFICMPE},
                {tNEQ, BC_IFICMPNE},
                {tGE,  BC_IFICMPLE},
                {tLE,  BC_IFICMPGE},
                {tLT,  BC_IFICMPG},
                {tGT,  BC_IFICMPL}
        };

        auto kind = node->kind();
        if (!instructions.count(kind)) {
            _status.reset(Status::Error("Corrupted AST. Unknown comparison operator", node->position()));
            return;
        }

        auto bytecode = get_bytecode();
        auto l = node->left();
        auto r = node->right();
        auto lt = _type_resolver(l);
        auto rt = _type_resolver(r);

        if (rt == VT_STRING || lt == VT_STRING) {
            _status.reset(Status::Error("Cannot compare strings", node->position()));
            return;
        }
        auto commonType = (rt == VT_DOUBLE || lt == VT_DOUBLE) ? VT_DOUBLE : VT_INT;

        visit(r);
        cast(rt, commonType, r->position());

        visit(l);
        cast(lt, commonType, l->position());

        bytecode->addInsn(commonType == VT_DOUBLE ? BC_DCMP : BC_ICMP);

        Label trueLabel(bytecode);
        Label exitLabel(bytecode);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(instructions.at(kind), trueLabel);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, exitLabel);
        bytecode->bind(trueLabel);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(exitLabel);
    }

    void generateArithmeticOp(BinaryOpNode* node) {
        static const std::map<TokenKind, std::pair<Instruction, Instruction>> instructions = {
            {tADD, {BC_IADD, BC_DADD}},
            {tSUB, {BC_ISUB, BC_DSUB}},
            {tDIV, {BC_IDIV, BC_DDIV}},
            {tMUL, {BC_IMUL, BC_DMUL}}
        };

        auto l = node->left();
        auto r = node->right();
        auto lt = _type_resolver(l);
        auto rt = _type_resolver(r);
        auto kind = node->kind();

        if (rt == VT_STRING || lt == VT_STRING) {
            _status.reset(Status::Error("Cannot perform arithmetic on strings", node -> position()));
            return;
        }
        auto commonType = (rt == VT_DOUBLE || lt == VT_DOUBLE) ? VT_DOUBLE : VT_INT;

        visit(r);
        cast(rt, commonType, r->position());

        visit(l);
        cast(lt, commonType, l->position());

        if (!instructions.count(kind)) {
            _status.reset(Status::Error("Corrupted AST. Unknown arithmetic operator", node->position()));
            return;
        }

        get_bytecode()->addInsn(commonType == VT_DOUBLE ? instructions.at(kind).second : instructions.at(kind).first);
    }


void loadVariable(const AstVar* var, uint32_t position) {
    static std::map<VarType, std::vector<Instruction> > loadLocalVar = {
        {VT_INT,    {BC_LOADIVAR0, BC_LOADIVAR1, BC_LOADIVAR2, BC_LOADIVAR3, BC_LOADIVAR}},
        {VT_DOUBLE, {BC_LOADDVAR0, BC_LOADDVAR1, BC_LOADDVAR2, BC_LOADDVAR3, BC_LOADDVAR}},
        {VT_STRING, {BC_LOADSVAR0, BC_LOADSVAR1, BC_LOADSVAR2, BC_LOADSVAR3, BC_LOADSVAR}},
    };

    static std::map<VarType, Instruction> loadContextVar = {
        {VT_INT,    BC_LOADCTXIVAR},
        {VT_DOUBLE, BC_LOADCTXDVAR},
        {VT_STRING, BC_LOADCTXSVAR},
    };


    auto bytecode = get_bytecode();

    auto name = var->name();
    auto type = var->type();

    if (!loadContextVar.count(type)) {
        _status.reset(Status::Error("Corrupted AST. Bad type to load", position));
        return;
    }

    auto contextId = _context->get_context_id(name, position);
    auto varId     = _context->get_var_id(name, position);

    auto isLocal = contextId == _context -> get_id();
    if (isLocal) {
        if (varId < 4) {
            bytecode->addInsn(loadLocalVar.at(type)[varId]);
        } else {
            bytecode->addInsn(loadLocalVar.at(type)[4]);
            bytecode->addInt16(varId);
        }
    } else {
        bytecode->addInsn(loadContextVar.at(type));
        bytecode->addInt16(contextId);
        bytecode->addInt16(varId);
    }
}

void storeVariable(const AstVar* variable, uint32_t position) {
    static std::map<VarType, std::vector<Instruction> > storeLocalVar = {
        {VT_INT,    {BC_STOREIVAR0, BC_STOREIVAR1, BC_STOREIVAR2, BC_STOREIVAR3, BC_STOREIVAR}},
        {VT_DOUBLE, {BC_STOREDVAR0, BC_STOREDVAR1, BC_STOREDVAR2, BC_STOREDVAR3, BC_STOREDVAR}},
        {VT_STRING, {BC_STORESVAR0, BC_STORESVAR1, BC_STORESVAR2, BC_STORESVAR3, BC_STORESVAR}},
    };

    static std::map<VarType, Instruction> storeContextVar = {
        {VT_INT,    BC_STORECTXIVAR},
        {VT_DOUBLE, BC_STORECTXDVAR},
        {VT_STRING, BC_STORECTXSVAR},
    };

    auto bytecode = get_bytecode();

    auto name = variable->name();
    auto type = variable->type();

    auto contextId = _context->get_context_id(name, position);
    auto varId     = _context->get_var_id(name, position);

    auto isLocal = contextId == _context -> get_id();

    if (isLocal) {
        if (varId < 4) {
            bytecode->addInsn(storeLocalVar[type][varId]);
        } else {
            bytecode->addInsn(storeLocalVar[type][4]);
            bytecode->addInt16(varId);
        }
    } else {
        bytecode->addInsn(storeContextVar[type]);
        bytecode->addInt16(contextId);
        bytecode->addInt16(varId);
    }
}

    void visit(AstNode* node) {
        if (_status->isOk() && node) {
            node->visit(this);
        }
    }

    void visitChildren(AstNode* node) {
        if (_status->isOk() && node) {
            node->visitChildren(this);
        }
    }
};

void BytecodeGeneratorVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    switch (node->kind()) {
        case tAND:
        case tOR:
            generateBooleanAndOr(node);
            break;
        case tAAND:
        case tAOR:
        case tAXOR:
        case tMOD:
            generateIntegerAndOrXorMod(node);
            break;
        case tEQ:
        case tGT:
        case tGE:
        case tLE:
        case tLT:
        case tNEQ:
            generateComparisonOp(node);
            break;
        case tADD:
        case tDIV:
        case tMUL:
        case tSUB:
            generateArithmeticOp(node);
            break;
        case tCOMMA:
            break;
        default:
            _status.reset(Status::Error("Invalid binary operator", node->position()));
    }
}

void BytecodeGeneratorVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    auto operand = node->operand();
    visit(operand);
    auto type = _type_resolver(operand);
    auto pos = operand->position();
    if (type != VT_INT && type != VT_DOUBLE) {
        _status.reset(Status::Error("Invalid type of unary operation operand.", pos));
        return;
    }
    auto kind = node->kind();
    auto bytecode = get_bytecode();

    switch (kind) {
        case tSUB:
            if (type == VT_DOUBLE) {
                bytecode->addInsn(BC_DNEG);
            } else {
                cast(type, VT_INT, pos);
                bytecode->addInsn(BC_INEG);
            }
        break;
        case tNOT: {
            if (type != VT_INT) {
                _status.reset(Status::Error("Invalid type of NOT operation (must be VT_INT).", pos));
                return;
            }
            bytecode->addInsn(BC_ILOAD0);
            Label zeroLabel(bytecode);
            bytecode->addBranch(BC_IFICMPE, zeroLabel);
            bytecode->addInsn(BC_POP);
            bytecode->addInsn(BC_ILOAD0);
            Label endLabel(bytecode);
            bytecode->addBranch(BC_JA, endLabel);
            bytecode->bind(zeroLabel);
            bytecode->addInsn(BC_POP);
            bytecode->addInsn(BC_ILOAD1);
            bytecode->bind(endLabel);
            break;
        }
        default:
          _status.reset(Status::Error("Corrupted AST node: invalid operation for UnaryOp node", pos));
    }
}

void BytecodeGeneratorVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    auto bytecode = get_bytecode();
    const auto& str = node->literal();
    if (str.empty()) {
        bytecode->addInsn(BC_SLOAD0);
    } else {
        auto id = _code->makeStringConstant(str);
        bytecode->addInsn(BC_SLOAD);
        bytecode->addInt16(id);
    }
}

void BytecodeGeneratorVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    auto bytecode = get_bytecode();
    const auto value = node->literal();
    if (value == 0.) {
        bytecode->addInsn(BC_DLOAD0);
    } else if (value == 1.) {
        bytecode->addInsn(BC_DLOAD1);
    } else if (value == -1.) {
        bytecode->addInsn(BC_DLOADM1);
    } else {
        bytecode->addInsn(BC_DLOAD);
        bytecode->addDouble(value);
    }
}

void BytecodeGeneratorVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    auto bytecode = get_bytecode();
    const auto value = node->literal();
    if (value == 0) {
        bytecode->addInsn(BC_ILOAD0);
    } else if (value == 1) {
        bytecode->addInsn(BC_ILOAD1);
    } else if (value == -1) {
        bytecode->addInsn(BC_ILOADM1);
    } else {
        bytecode->addInsn(BC_ILOAD);
        bytecode->addInt64(value);
    }
}

void BytecodeGeneratorVisitor::visitLoadNode(LoadNode *node) {
    loadVariable(node->var(), node->position());
}

void BytecodeGeneratorVisitor::visitStoreNode(StoreNode *node) {
    auto variable = node->var();
    auto type = variable->type();
    auto bytecode = get_bytecode();
    auto operation = node->op();
    uint32_t pos = node->position();

    visitChildren(node);
    cast(_type_resolver(node->value()), type, pos);
    
    Instruction instruction;
    switch (operation) {
        case tASSIGN:
            storeVariable(variable, pos);
            break;
        case tINCRSET:
            loadVariable(variable, pos);
            if (type != VT_INT && type != VT_DOUBLE) {
                _status.reset(Status::Error("Incalid type for increment", pos));
                return;
            }
            instruction = type == VT_INT ? BC_IADD : BC_DADD;
            bytecode->addInsn(instruction);
            storeVariable(variable, pos);
            break;
        case tDECRSET:
          loadVariable(variable, node->position());
            if (type != VT_INT && type != VT_DOUBLE) {
                _status.reset(Status::Error("Incalid type for increment", pos));
                return;
            }
            instruction = type == VT_INT ? BC_ISUB : BC_DSUB;
            bytecode->addInsn(instruction);
            storeVariable(variable, pos);
            break;
        default:
          _status.reset(Status::Error("Corrupted AST node: invalid operation for store node", pos));
    }
}

void BytecodeGeneratorVisitor::visitForNode(ForNode *node) {
    auto iterVar = node->var();
    if (iterVar->type() != VT_INT) {
        _status.reset(Status::Error("index in for loop must be VT_INT", node->position()));
        return;
    }

    auto inExpr = node->inExpr();
    if (!inExpr->isBinaryOpNode() || inExpr->asBinaryOpNode()->kind() != tRANGE) {
        _status.reset(Status::Error("expression in for loop must be binary range operation", inExpr->position()));
        return;
    }

    auto rangeNode = inExpr->asBinaryOpNode();
    auto initNode = rangeNode->left();
    visit(initNode);
    auto type = _type_resolver(initNode);
    if (type != VT_DOUBLE && type != VT_INT) {
        _status.reset(Status::Error("init value in for loop must be a number", initNode->position()));
        return;
    }

    storeVariable(iterVar, node->position());

    auto termNode = rangeNode->right();
    visit(termNode);
    type = _type_resolver(termNode);
    if (type != VT_DOUBLE && type != VT_INT) {
        _status.reset(Status::Error("termination value in for loop must be a number", initNode->position()));
        return;
    }

    auto bytecode = get_bytecode();
    Label beginLabel(bytecode);
    Label endLabel(bytecode);

    bytecode->bind(beginLabel);

    cast(type, VT_INT, termNode->position());
    loadVariable(iterVar, node->position());

    bytecode->addBranch(BC_IFICMPG, endLabel);

    bytecode->addInsn(BC_POP);
    visit(node->body());

    bytecode->addInsn(BC_ILOAD1);
    loadVariable(iterVar, node->position());
    bytecode->addInsn(BC_IADD);
    storeVariable(iterVar, node->position());

    bytecode->addBranch(BC_JA, beginLabel);
    bytecode->bind(endLabel);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_POP);
}

void BytecodeGeneratorVisitor::visitWhileNode(WhileNode *node) {
    auto bytecode = get_bytecode();
    Label beginLabel(bytecode);
    Label endLabel(bytecode);

    bytecode->bind(beginLabel);
    auto whileExpr = node->whileExpr();
    visit(whileExpr);
    cast(_type_resolver(whileExpr), VT_INT, node->position());
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, endLabel);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_POP);

    visit(node->loopBlock());
    bytecode->addBranch(BC_JA, beginLabel);
    bytecode->bind(endLabel);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_POP);
}

void BytecodeGeneratorVisitor::visitIfNode(IfNode *node) {
    auto exprNode = node->ifExpr();
    visit(exprNode);
    cast(_type_resolver(exprNode), VT_INT, exprNode->position());
    auto bytecode = get_bytecode();
    
    Label elseLabel(bytecode);
    Label endLabel(bytecode);

    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, elseLabel);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_POP);

    visit(node->thenBlock());
    bytecode->addBranch(BC_JA, endLabel);

    bytecode->bind(elseLabel);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_POP);

    visit(node->elseBlock());

    bytecode->bind(endLabel);
}

void BytecodeGeneratorVisitor::visitBlockNode(BlockNode *node) {
    auto scope = node->scope();
    auto varIt = Scope::VarIterator(scope);
    while (varIt.hasNext()) {
        _context->add_var(varIt.next(), node->position());
    }

    auto funcIt = Scope::FunctionIterator(node->scope());
    while (funcIt.hasNext()) {
        auto astFunction = funcIt.next();
        if (_code->functionByName(astFunction->name())) {
            _status.reset(Status::Error("Function redefinition", node->position()));
        } else {
            auto byte_func = new BytecodeFunction(astFunction);
            _code->addFunction(byte_func);
        }
    }

    funcIt = Scope::FunctionIterator(node->scope());
    while (funcIt.hasNext()) {
        traverseAstFunction(funcIt.next());
    }

    visitChildren(node);
}

void BytecodeGeneratorVisitor::visitFunctionNode(FunctionNode *node) {
    visitChildren(node);
}

void BytecodeGeneratorVisitor::visitReturnNode(ReturnNode *node) {
    VarType returnType = _context->get_bytecode_function()->returnType();
    if (auto child = node->returnExpr()) {
        visit(child);
        auto exprType = _type_resolver(child);
        cast(exprType, returnType, node->position());
    }
    get_bytecode()->addInsn(BC_RETURN);
}

void BytecodeGeneratorVisitor::visitCallNode(CallNode *node) {
    const auto& name = node->name();
    auto function = (BytecodeFunction*) _code->functionByName(name);

    if (function->parametersNumber() != node->parametersNumber()) {
        _status.reset(Status::Error("Wrong arguments number", node->position()));
        return;
    }
    int32_t n = node->parametersNumber();
    for (int32_t i = n - 1; i >= 0; --i) {
        auto child = node->parameterAt(i);
        visit(child);
        cast(_type_resolver(child), function->parameterType(i), node->position());
    }

    auto bytecode = get_bytecode();

    bytecode->addInsn(BC_CALL);
    bytecode->addUInt16(function->id());
}

void BytecodeGeneratorVisitor::visitNativeCallNode(NativeCallNode *node) {
    if (void* nativeCode = dlsym(RTLD_DEFAULT, node->nativeName().c_str())) {
        auto id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), nativeCode);
        auto bytecode = get_bytecode();
        bytecode->addInsn(BC_CALLNATIVE);
        bytecode->addUInt16(id);
    } else {
        _status.reset(Status::Error("Native function not found", node->position()));
    }

}

void BytecodeGeneratorVisitor::visitPrintNode(PrintNode *node) {

    static std::map<VarType, Instruction> printInstrutions = {
        {VT_INT,    BC_IPRINT},
        {VT_DOUBLE, BC_DPRINT},
        {VT_STRING, BC_SPRINT},
    };

    for (uint32_t i = 0; i < node->operands(); ++i) {
        auto child = node->operandAt(i);
        visit(child);
        auto type = _type_resolver(child);
        if (printInstrutions.count(type)) {
            get_bytecode()->addInsn(printInstrutions[type]);
        } else {
            _status.reset(Status::Error("Invalid type of print argument", node->position()));
        }
    }
}

} // anonymous namespace

Status *BytecodeTranslatorImpl::translateBytecode(const string& program, InterpreterCodeImpl* *code) {
    Parser parser;
    auto status = parser.parseProgram(program);

    if (!status->isError()) {
        auto astTop = parser.top();
        auto visitor = BytecodeGeneratorVisitor(astTop, *code);

        auto translatorStatus = visitor.getStatus();
        if (translatorStatus->isError()) {
            return Status::Error(translatorStatus->getMsg(), translatorStatus->getPosition());
        } else {
            return translatorStatus->isOk()
                   ? Status::Ok()
                   : Status::Warning(translatorStatus->getMsg(), translatorStatus->getPosition());
        }
    }

    return status;
}

Status *BytecodeTranslatorImpl::translate(const std::string &program, Code* *code) {
    auto interpreterCode = new InterpreterCodeImpl(std::cout);
    *code = interpreterCode;
    return translateBytecode(program, &interpreterCode);
}

} // namespace mathvm