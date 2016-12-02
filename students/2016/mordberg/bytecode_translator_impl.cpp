#include "mathvm.h"
#include "interpreter_code_impl.h"
#include "parser.h"
#include "ast_node_type_resolver.h"

#include <stack>

namespace mathvm {

namespace {

class BytecodeGeneratorVisitor: public AstVisitor {
public:
    BytecodeGeneratorVisitor(AstFunction* astTop, InterpreterCodeImpl* *code)
        : _interpreterCode(code), _type_resolver(astTop) {
        // astTop->visit(this);
    }

    Status* getStatus() const {
        return Status::Ok();
    }

#define VISITOR_FUNCTION(type, name) \
      virtual void visit##type(type* node);

      FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION

private:
    InterpreterCodeImpl* *_interpreterCode;
    Status* _status;
    sdt::stack<Scope*> _scopes;
    type_resolver::AstNodeTypeResolver _type_resolver;
    Bytecode* _code;

    static Instruction getZeroInstruction(VarType type) {
        switch (type) {
            case VT_INT:
                return BC_ILOAD0;
            case VT_DOUBLE:
                return BC_DLOAD0;
            case VT_STRING:
                return BC_SLOAD0;
            default:
                return BC_INVALID;
        }  
    }

    void generateBooleanAndOr(BinaryOpNode* node) {
        auto l = node->left();
        auto r = node->right();
        auto lt = _resolver[l];
        auto rt = _resolver[r];

        auto kind = node->kind();

    }

    void generateIntegerAndOrXorMod(BinaryOpNode* node) {

    }

    void generateComparisonOp(BinaryOpNode* node) {

    }

    void generateArithmeticOp(BinaryOpNode* node) {

    }

    void visit(AstNode* node) {
        if (!_status) {
            node->visit(this);
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
            _status = Status::Error("Invalid binary operator", node->position);
    }
}

void BytecodeGeneratorVisitor::visitUnaryOpNode(UnaryOpNode *node) {

}

void BytecodeGeneratorVisitor::visitStringLiteralNode(StringLiteralNode *node) {

}

void BytecodeGeneratorVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {

}

void BytecodeGeneratorVisitor::visitIntLiteralNode(IntLiteralNode *node) {

}

void BytecodeGeneratorVisitor::visitLoadNode(LoadNode *node) {

}

void BytecodeGeneratorVisitor::visitStoreNode(StoreNode *node) {

}

void BytecodeGeneratorVisitor::visitForNode(ForNode *node) {

}

void BytecodeGeneratorVisitor::visitWhileNode(WhileNode *node) {

}

void BytecodeGeneratorVisitor::visitIfNode(IfNode *node) {

}

void BytecodeGeneratorVisitor::visitBlockNode(BlockNode *node) {

}

void BytecodeGeneratorVisitor::visitFunctionNode(FunctionNode *node) {

}

void BytecodeGeneratorVisitor::visitReturnNode(ReturnNode *node) {

}

void BytecodeGeneratorVisitor::visitCallNode(CallNode *node) {

}

void BytecodeGeneratorVisitor::visitNativeCallNode(NativeCallNode *node) {

}

void BytecodeGeneratorVisitor::visitPrintNode(PrintNode *node) {

}

} // anonymous namespace

Status *BytecodeTranslatorImpl::translateBytecode(const string& program, InterpreterCodeImpl* *code) {
    Parser parser;
    auto status = parser.parseProgram(program);

    if (!status->isError()) {
        auto astTop = parser.top();
        auto visitor = BytecodeGeneratorVisitor(astTop, code);
        return visitor.getStatus();
    }

    return status;
}

Status *BytecodeTranslatorImpl::translate(const std::string &program, Code* *code) {
    auto interpreterCode = new InterpreterCodeImpl();
    *code = interpreterCode;
    return translateBytecode(program, &interpreterCode);
}

} // namespace mathvm