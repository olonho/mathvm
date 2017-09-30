#include "printer_translator.h"

namespace mathvm {
#define VISITOR_FUNCTION(type, name)                        \
    void AstDumper::visit##type(type* node) { (void) node; }                \

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    Status* AstPrinterTranslatorImpl::translate(const string &program, Code **code) {
        Parser* parser = new Parser();
        Status* answer = parser->parseProgram(program);
        if (!answer->isError()) {
            PrettyPrintVisitor* visitor = new PrettyPrintVisitor();
            parser->top()->node()->visitChildren(visitor);
            delete(visitor);
        } else {
            std::cout << "Parsing error: can't print AST!" << std::endl;
        }
        delete(parser);
        return answer;
    }

    void PrettyPrintVisitor::printIndent() {
        std::string space = std::string(indent, ' ');
        std::cout << space.c_str();
    }

    void PrettyPrintVisitor::incIndent() {
        indent += 4;
    }

    void PrettyPrintVisitor::decIndent() {
        indent -= 4;
    }

    const char* PrettyPrintVisitor::to_string(const VarType& var) const {
        switch (var) {
            case VT_VOID: return "void";
            case VT_DOUBLE: return "double";
            case VT_INT: return "int";
            case VT_STRING: return "string";
            default: return "UNKNOWN_TYPE";
        }
    }

    const char* PrettyPrintVisitor::correctStrLiteral(const std::string& str) {
      std::map<char, std::string> spec = {{'\n', "\\n"}, {'\r', "\\r"}, {'\t', "\\t"}};
      std::string literal = "";
      for (char c: str) {
          if (spec.find(c) != spec.end())
              literal += spec.at(c);
          else
              literal += c;
      }
      return literal.c_str();
    }

    bool PrettyPrintVisitor::needSemicolon(const AstNode* node) {
        return !(node->isForNode() || node->isWhileNode() || node->isIfNode() || node->isFunctionNode());
    }

    void PrettyPrintVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        std::cout << '(';
        node->left()->visit(this);
        std::cout << ' ' << tokenOp(node->kind()) << ' ';
        node->right()->visit(this);
        std::cout << ')';
    }

    void PrettyPrintVisitor::visitBlockNode(BlockNode *node) {
        Scope::VarIterator varIter(node->scope());
        while (varIter.hasNext()) {
          AstVar* var = varIter.next();
          printIndent();
          std::cout << to_string(var->type()) << ' ' << (var->name()).c_str() << ';' << std::endl;
        }

        Scope::FunctionIterator funIter(node->scope());
        while (funIter.hasNext()) {
          AstFunction* fun = funIter.next();
          fun->node()->visit(this);
        }

        int count = node->nodes();
        int ind = 0;
        do {
            printIndent();
            node->nodeAt(ind)->visit(this);
            if (needSemicolon(node->nodeAt(ind++)))
                std::cout << ';' << std::endl;
        } while(ind < count);
    }

    void PrettyPrintVisitor::visitCallNode(CallNode *node) {
        std::cout << node->name().c_str() << '(';
        int count = node->parametersNumber();
        if (count > 0)
            node->parameterAt(0)->visit(this);
        for (int i = 1; i < count; ++i) {
            std::cout << ", ";
            node->parameterAt(i)->visit(this);
        }
        std::cout << ')';
    }

    void PrettyPrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        std::string literal = std::to_string(node->literal());
        std::cout << literal.c_str();
    }

    void PrettyPrintVisitor::visitForNode(ForNode *node) {
        std::cout << "for (" << (node->var()->name()).c_str() << " in ";
        node->inExpr()->visit(this);
        std::cout << ") {" << std::endl;
        incIndent();
        node->body()->visit(this);
        decIndent();
        printIndent();
        std::cout << '}' << std::endl;
    }

    void PrettyPrintVisitor::visitFunctionNode(FunctionNode *node) {
        printIndent();
        std::cout << "function " << to_string(node->returnType()) << " " << (node->name()).c_str() << '(';
        int count = node->parametersNumber();
        if (count > 0)
            std::cout << to_string(node->parameterType(0)) << " " << (node->parameterName(0)).c_str();
        for (int i = 1; i < count; ++i) {
            std::cout << ", ";
            std::cout << to_string(node->parameterType(i)) << " " << (node->parameterName(i)).c_str();
        }
        std::cout << ") {" << std::endl;
        incIndent();
        node->visitChildren(this);
        decIndent();
        printIndent();
        std::cout << '}' << std::endl;
    }

    void PrettyPrintVisitor::visitIfNode(IfNode *node) {
        std::cout << "if (";
        node->ifExpr()->visit(this);
        std::cout << ") {" << std::endl;
        incIndent();
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            decIndent();
            printIndent();
            std::cout << "} else {" << std::endl;
            incIndent();
            node->elseBlock()->visit(this);
        }
        decIndent();
        printIndent();
        std::cout << '}' << std::endl;
    }

    void PrettyPrintVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        std::string literal = std::to_string(node->literal());
        std::cout << literal.c_str();
    }

    void PrettyPrintVisitor::visitLoadNode(LoadNode *node) {
        std::cout << (node->var()->name()).c_str();
    }

    void PrettyPrintVisitor::visitNativeCallNode(NativeCallNode *node) {
        std::cout << "native '" << (node->nativeName()).c_str();
    }

    void PrettyPrintVisitor::visitPrintNode(PrintNode *node) {
        std::cout << "print(";
        int count = node->operands();
        if (count > 0)
            node->operandAt(0)->visit(this);
        for (int i = 1; i < count; ++i) {
            std::cout << ", ";
            node->operandAt(i)->visit(this);
        }
        std::cout << ')';
    }

    void PrettyPrintVisitor::visitReturnNode(ReturnNode *node) {
        std::cout << "return";
        if (node->returnExpr()) {
            std::cout << " ";
            node->returnExpr()->visit(this);
        }
    }

    void PrettyPrintVisitor::visitStoreNode(StoreNode *node) {
        std::cout << (node->var()->name()).c_str() << ' ' << tokenOp(node->op()) << ' ';
        node->visitChildren(this);
    }

    void PrettyPrintVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        std::cout << "'";
        std::cout << correctStrLiteral(node->literal()) << "'";
    }

    void PrettyPrintVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        std::cout << tokenOp(node->kind());
        node->visitChildren(this);
    }

    void PrettyPrintVisitor::visitWhileNode(WhileNode *node) {
        std::cout << "while (";
        node->whileExpr()->visit(this);
        std::cout << ") {" << std::endl;
        incIndent();
        node->loopBlock()->visit(this);
        decIndent();
        printIndent();
        std::cout << '}' << std::endl;
    }
}

