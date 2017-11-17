#include <numeric>
#include "ast_printer.h"

namespace mathvm {
#define SPACE std::string(" ")
#define COMMA std::string("'")

    AstPrinter::AstPrinter(std::ostream &os)
        : out_(os)
        , level_(0)
    {}

    void AstPrinter::dump(AstNode *rootNode) {
        rootNode->visitChildren(this);
    }

    void AstPrinter::visitBinaryOpNode(BinaryOpNode *node) {
        print("(");
        node->left()->visit(this);
        print(SPACE + tokenOp(node->kind()) + SPACE);
        node->right()->visit(this);
        print(")");
    }

    void AstPrinter::visitUnaryOpNode(UnaryOpNode *node) {
        print(tokenOp(node->kind()));
        node->operand()->visit(this);
    }

    void AstPrinter::visitStringLiteralNode(StringLiteralNode *node) {
        const std::string escapedString = escapeString(node->literal());
        print(COMMA + escapedString + COMMA);
    }


    void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        print(std::to_string(node->literal()));
    }

    void AstPrinter::visitIntLiteralNode(IntLiteralNode *node) {
        print(std::to_string(node->literal()));
    }

    void AstPrinter::visitLoadNode(LoadNode *node) {
        print(node->var()->name());
    }

    void AstPrinter::visitStoreNode(StoreNode *node) {
        print(node->var()->name());
        print(SPACE + tokenOp(node->op()) + SPACE);
        node->value()->visit(this);
    }

    void AstPrinter::visitForNode(ForNode *node) {
        print("for (");
        print(node->var()->name());
        print(" in ");
        node->inExpr()->visit(this);
        print(") ");
        node->body()->visit(this);
    }

    void AstPrinter::visitWhileNode(WhileNode *node) {
        print("while (");
        node->whileExpr()->visit(this);
        print(") ");
        node->loopBlock()->visit(this);
    }

    void AstPrinter::visitIfNode(IfNode *node) {
        print("if" + SPACE + "(");
        node->ifExpr()->visit(this);
        print(")" + SPACE);
        node->thenBlock()->visit(this);
        if (node->elseBlock() != nullptr) {
            indent();
            print("else" + SPACE);
            node->elseBlock()->visit(this);
        }
    }

    void AstPrinter::visitBlockNode(BlockNode *node) {
        if (level_ > 0) {
            print("{\n");
        }

        increaseLevel();

        // iterate scope variable declarations.
        Scope::VarIterator varDeclarationIterator{node->scope()};
        while (varDeclarationIterator.hasNext()) {
            AstVar *variable = varDeclarationIterator.next();
            VarType variableType = variable->type();
            indent();
            print(typeToName(variableType));
            print(SPACE);
            print(variable->name());
            print(";\n");
        }

        // iterate function definitions.
        Scope::FunctionIterator functionIterator{node->scope()};
        while (functionIterator.hasNext()) {
            indent();
            functionIterator.next()->node()->visit(this);
        }

        const size_t nodesCount = node->nodes();
        for (uint32_t i = 0; i < nodesCount; ++i) {
            AstNode *innerNode = node->nodeAt(i);
            indent();
            innerNode->visit(this);
            if (isOneLiner(innerNode)) {
                print(";");
            }
            print("\n");
        }

        decreaseLevel();

        if (level_ > 0) {
            indent();
            print("}\n");
        }
    }

    void AstPrinter::visitFunctionNode(FunctionNode *node) {
        print("function" + SPACE);
        print(typeToName(node->returnType()) + SPACE);
        print(node->name());
        print("(");

        // print formal parameters.
        const uint32_t parametersCount = node->parametersNumber();
        for (uint32_t i = 0; i < parametersCount; ++i) {
            VarType parameterType = node->parameterType(i);
            const string & parameterName = node->parameterName(i);
            print(typeToName(parameterType) + SPACE);
            print(parameterName);
            if (i + 1 < parametersCount) {
                print("," + SPACE);
            }
        }
        print(")" + SPACE);

        // Special case for native calls.
        if (node->body()->nodes() == 2 && node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
        }
        else {
            node->body()->visit(this);
        }
    }

    void AstPrinter::visitReturnNode(ReturnNode *node) {
        print("return");
        if (node->returnExpr() != nullptr) {
            print(SPACE);
            node->returnExpr()->visit(this);
        }
    }

    void AstPrinter::visitCallNode(CallNode *node) {
        print(node->name() + "(");
        const uint32_t parametersCount = node->parametersNumber();
        for (uint32_t i = 0; i < parametersCount; ++i) {
            node->parameterAt(i)->visit(this);
            if (i + 1 < parametersCount) {
                print("," + SPACE);
            }
        }
        print(")");
    }
    void AstPrinter::visitNativeCallNode(NativeCallNode *node) {
        print("native '");
        print(node->nativeName());
        print("';\n");
    }

    void AstPrinter::visitPrintNode(PrintNode *node) {
        print("print(");
        const size_t operandsCount = node->operands();
        for (size_t i = 0; i < operandsCount; ++i) {
            node->operandAt(i)->visit(this);
            if (i + 1 < operandsCount) {
                print("," + SPACE);
            }
        }
        print(")");
    }

    bool AstPrinter::isOneLiner(AstNode *node) {
        return node->isLoadNode() ||
                node->isStoreNode() ||
                node->isReturnNode() ||
                node->isCallNode() ||
                node->isNativeCallNode() ||
                node->isPrintNode();
    }

    void AstPrinter::increaseLevel() {
        level_++;
    }

    void AstPrinter::decreaseLevel() {
        assert(level_ > 0);
        level_--;
    }

    void AstPrinter::print(const std::string &expr, bool withIndent) {
        static char lastCharacter = '\0';
        if (withIndent) {
            indent();
        }
        if (expr.empty()) {
            return;
        }

        if (lastCharacter == '\n') {
            size_t startPos = 0;
            while (startPos < expr.size() && expr.at(startPos) == '\n') {
                startPos++;
            }
            out_ << expr.substr(startPos);
        }
        else {
            out_ << expr;
        }

        lastCharacter = expr.back();
    }

    void AstPrinter::indent() {
        std::string tab;

        // notCounting global scope's level.
        const size_t tabLevels = level_ - 1;
        for (size_t i = 0; i < tabLevels; ++i) {
            for (size_t j = 0; j < TABSTOP; ++j) {
                tab += SPACE;
            }
        }
        print(tab);
    }

    string AstPrinter::escapeString(const std::string &s) {
        auto binOp = [](const std::string &acc, char nextCharacter) {
            std::string nextReplacement;
            switch(nextCharacter) {
                case '\\':
                    nextReplacement += "\\\\";
                    break;
                case '\n':
                    nextReplacement += "\\n";
                    break;
                case '\t':
                    nextReplacement += "\\t";
                    break;
                case '\r':
                    nextReplacement += "\\r";
                    break;
                default:
                    nextReplacement += nextCharacter;
            }

            return acc + nextReplacement;
        };

        return std::accumulate(s.cbegin(), s.cend(), std::string(), binOp);
    }
#undef SPACE
#undef ENDL
}
