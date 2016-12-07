#include "print_translator.h"

#include <ostream>
#include <sstream>
#include <map>
#include <numeric>

#include "ast.h"
#include "mathvm.h"
#include "parser.h"

namespace mathvm {

namespace {

class PrintVisitor: public AstVisitor {

public:
    PrintVisitor(AstNode* root) {
        root->visit(this);
    }

    std::string getSourceCode() const {
        return _source_code.str();
    }

#define VISITOR_FUNCTION(type, name) \
      virtual void visit##type(type* node);

      FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION

private:
    std::stringstream _source_code;
    size_t _scope_level = 0;

    static const std::string BEGIN;
    static const std::string COMMA;
    static const std::string END;
    static const std::string IDENT;
    static const std::string RPAREN;
    static const std::string SEP;
    static const std::string SEMICOLON;
    static const std::string LPAREN;
    static const std::string QUOTE;

    static std::string get_ident(size_t level) {
        std::string result;
        for (size_t i = 0; i < level; ++i) {
            result += IDENT;
        }
        return result;
    }

    static std::string safeEscape(char c) {
        static std::map<char, std::string> safeReplacement = {
            {'\'', "\\'"}, {'\"', "\\\""}, {'\?', "\\?"}, {'\\', "\\\\"},
            {'\a', "\\a"}, {'\b', "\\b"}, {'\f', "\\f"}, {'\n', "\\n"},
            {'\r', "\\r"}, {'\t', "\\t"}, {'\v', "\\v"}
        };

        return safeReplacement.count(c) ? safeReplacement[c] : std::string(1, c);
    }
};

void PrintVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    _source_code << LPAREN;
    node->left()->visit(this);
    _source_code << SEP << tokenOp(node->kind()) << SEP;
    node->right()->visit(this);
    _source_code << RPAREN;
}

void PrintVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    _source_code << tokenOp(node->kind());
    node->operand()->visit(this);
}

void PrintVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    const auto& str = node->literal();
    const auto& escaped = std::accumulate(str.begin(), str.end(), std::string(), 
        [](std::string acc, char c){ return acc += safeEscape(c); });

    _source_code << QUOTE << escaped << QUOTE;
}

void PrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    _source_code << node->literal();
}

void PrintVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    _source_code << node->literal();
}

void PrintVisitor::visitLoadNode(LoadNode *node) {
    _source_code << node->var()->name();
}

void PrintVisitor::visitStoreNode(StoreNode *node) {
    _source_code << node->var()->name() << SEP << tokenOp(node->op()) << SEP;
    node->value()->visit(this);
}

void PrintVisitor::visitForNode(ForNode *node) {
    _source_code << "for" << SEP << LPAREN << node->var()->name() << SEP << "in" << SEP;
    node->inExpr()->visit(this);
    _source_code << RPAREN;
    node->body()->visit(this);
}

void PrintVisitor::visitWhileNode(WhileNode *node) {
    _source_code << "while" << SEP << LPAREN;
    node->whileExpr()->visit(this);
    _source_code << RPAREN;
    node->loopBlock()->visit(this);
}

void PrintVisitor::visitIfNode(IfNode *node) {
    _source_code << "if" << SEP << LPAREN;
    node->ifExpr()->visit(this);
    _source_code << RPAREN << SEP;
    node->thenBlock()->visit(this);
    if (auto elseBlock = node->elseBlock()) {
        _source_code << SEP << "else" << SEP;
        elseBlock->visit(this);
    }
}

void PrintVisitor::visitBlockNode(BlockNode *node) {
    auto isNotRootScope = _scope_level != 0;
    const auto& ident = get_ident(_scope_level);

    if (isNotRootScope) {
        _source_code << BEGIN << std::endl;
    }
    ++_scope_level;

    // variables
    Scope::VarIterator var_it(node->scope());
    while (var_it.hasNext()) {
        auto var = var_it.next();
        _source_code << ident << typeToName(var->type()) << SEP;
        _source_code << var->name() << SEMICOLON << std::endl;
    }

    // functions
    Scope::FunctionIterator func_it(node->scope());
    while (func_it.hasNext()) {
        auto function = func_it.next();
        _source_code << ident;
        function->node()->visit(this);
        _source_code << std::endl;
    }

    // statements
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        _source_code << ident;
        auto statement = node->nodeAt(i);
        statement->visit(this);
        auto need_semicolon =  !statement->isBlockNode() 
                            && !statement->isWhileNode() 
                            && !statement->isIfNode() 
                            && !statement->isForNode();
        if (need_semicolon) {
            _source_code << SEMICOLON;
        }
        _source_code << std::endl;
    }

    if (isNotRootScope) {
        _source_code << get_ident(_scope_level - 2) << END;
    }

    --_scope_level;
}

void PrintVisitor::visitFunctionNode(FunctionNode *node) {
    _source_code << "function" << SEP << typeToName(node->returnType()) << SEP;
    _source_code << node->name() << LPAREN;

    auto print_parameter = [&](uint32_t i) {
        _source_code << typeToName(node->parameterType(i)) << SEP << node->parameterName(i);
    };
    auto n_args = node->parametersNumber();
    for (uint32_t i = 0; i + 1 < n_args; ++i) {
        print_parameter(i);
        _source_code << COMMA << SEP;
    }
    if (n_args) {
       print_parameter(n_args - 1);
    }

    _source_code << RPAREN << SEP;

    auto body = node->body();
    auto is_native = body->nodes() != 0 && body->nodeAt(0)->isNativeCallNode();

    if (is_native) {
        auto native_call = body->nodeAt(0)->asNativeCallNode();
        _source_code << "native" << SEP << QUOTE << native_call->nativeName() << QUOTE;
        _source_code << SEMICOLON;
        return;
    }

    body->visit(this);
}

void PrintVisitor::visitReturnNode(ReturnNode *node) {
    _source_code << "return";
    if (auto expr = node->returnExpr()) {
        _source_code << SEP;
        expr->visit(this);
    }
}

void PrintVisitor::visitCallNode(CallNode *node) {
    _source_code << node->name() << LPAREN;
    auto n_args = node->parametersNumber();
    for (uint32_t i = 0; i + 1 < n_args; ++i) {
        node->parameterAt(i)->visit(this);
        _source_code << COMMA << SEP;
    }
    if (n_args) {
       node->parameterAt(n_args - 1)->visit(this);
    }
    _source_code << RPAREN;
}

void PrintVisitor::visitNativeCallNode(NativeCallNode *node) {
    _source_code << "native" << SEP << node->nativeName();
}

void PrintVisitor::visitPrintNode(PrintNode *node) {
    _source_code << "print" << LPAREN;
    auto n_args = node->operands();
    for (uint32_t i = 0; i + 1 < n_args; ++i) {
        node->operandAt(i)->visit(this);
        _source_code << COMMA << SEP;
    }
    if (n_args) {
        node->operandAt(n_args - 1)->visit(this);
    }
    _source_code << RPAREN;
}

const std::string PrintVisitor::BEGIN = "{";
const std::string PrintVisitor::COMMA = ",";
const std::string PrintVisitor::END = "}";
const std::string PrintVisitor::IDENT = "\t";
const std::string PrintVisitor::RPAREN = ")";
const std::string PrintVisitor::SEP = " ";
const std::string PrintVisitor::SEMICOLON = ";";
const std::string PrintVisitor::LPAREN = "(";
const std::string PrintVisitor::QUOTE = "'";

} // anonymous namespace



Status* PrintTranslator::translate(const std::string &program, Code* *code) {
    Parser parser;
    const auto status = parser.parseProgram(program);

    if (status->isError()) {
        return status;
    }

    auto astTop = parser.top();
    _out << PrintVisitor(astTop->node()->body()).getSourceCode();

    return status;
}

} // namespace mathvm
