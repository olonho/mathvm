#include <ostream>
#include <string>
//#include <algorithm>
#include <numeric>
#include <cassert>

#include "mathvm.h"
#include "ast.h"
#include "visitors.h"
#include "parser.h"

namespace mathvm {


// TODO migrate to inheriting from AstDumper,
// Now there are vtable problems (as it's not pure virtual base).
class AstPrinter : public AstVisitor {
#define SPACE std::string(" ")
#define COMMA std::string("'")
    static constexpr int TABSTOP = 4;

public:
    AstPrinter(std::ostream &os = std::cout)
        : _outputStream(os)
        , _level(0)
    {}

    void dump(AstNode *rootNode) {
        rootNode->visitChildren(this);
    }

    void visitBinaryOpNode(BinaryOpNode *node) override {
        print("(");
        node->left()->visit(this);
        print(SPACE + tokenOp(node->kind()) + SPACE);
        node->right()->visit(this);
        print(")");
    }

    void visitUnaryOpNode(UnaryOpNode *node) override {
        print(tokenOp(node->kind()));
        node->operand()->visit(this);
    }

    void visitStringLiteralNode(StringLiteralNode *node) override {
        const std::string escapedString = escapeString(node->literal());
        print(COMMA + escapedString + COMMA);
    }


    void visitDoubleLiteralNode(DoubleLiteralNode *node) override {
        print(std::to_string(node->literal()));
    }

    void visitIntLiteralNode(IntLiteralNode *node) override {
        print(std::to_string(node->literal()));
    }

    void visitLoadNode(LoadNode *node) override {
        print(node->var()->name());
    }

    void visitStoreNode(StoreNode *node) override {
        print(node->var()->name());
        print(SPACE + tokenOp(node->op()) + SPACE);
        node->value()->visit(this);
    }

    void visitForNode(ForNode *node) override {
        print("for (");
        print(node->var()->name());
        print(" in ");
        node->inExpr()->visit(this);
        print(") ");
        node->body()->visit(this);
    }

    void visitWhileNode(WhileNode *node) override {
        print("while (");
        node->whileExpr()->visit(this);
        print(") ");
        node->loopBlock()->visit(this);
    }

    void visitIfNode(IfNode *node) override {
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

    void visitBlockNode(BlockNode *node) override {
        if (_level > 0) {
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
        for (size_t i = 0; i < nodesCount; ++i) {
            AstNode *innerNode = node->nodeAt(i);
            indent();
            innerNode->visit(this);
            if (isOneLiner(innerNode)) {
                print(";");
            }
            print("\n");
        }

        decreaseLevel();

        if (_level > 0) {
            indent();
            print("}\n");
        }
    }

    void visitFunctionNode(FunctionNode *node) override {
        print("function" + SPACE);
        print(typeToName(node->returnType()) + SPACE);
        print(node->name());
        print("(");

        // print formal parameters.
        const size_t parametersCount = node->parametersNumber();
        for (size_t i = 0; i < parametersCount; ++i) {
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

    void visitReturnNode(ReturnNode *node) override {
        print("return");
        if (node->returnExpr() != nullptr) {
            print(SPACE);
            node->returnExpr()->visit(this);
        }
    }

    void visitCallNode(CallNode *node) override {
        print(node->name() + "(");
        const size_t parametersCount = node->parametersNumber();
        for (size_t i = 0; i < parametersCount; ++i) {
            node->parameterAt(i)->visit(this);
            if (i + 1 < parametersCount) {
                print("," + SPACE);
            }
        }
        print(")");
    }
    void visitNativeCallNode(NativeCallNode *node) override {
        print("native '");
        print(node->nativeName());
        print("';\n");
    }

    void visitPrintNode(PrintNode *node) override {
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

private:
    std::ostream & _outputStream;
    size_t _level;

    bool isOneLiner(AstNode *node) {
        return node->isLoadNode() ||
                node->isStoreNode() ||
                node->isReturnNode() ||
                node->isCallNode() ||
                node->isNativeCallNode() ||
                node->isPrintNode();
    }

    void increaseLevel() {
        _level++;
    }

    void decreaseLevel() {
        assert(_level > 0);
        _level--;
    }

    void print(const std::string &expr, bool withIndent = false) {
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
            _outputStream << expr.substr(startPos);
        }
        else {
            _outputStream << expr;
        }

        lastCharacter = expr.back();
    }

    void indent() {
        std::string tab = "";

        // notCounting global scope's level.
        const size_t tabLevels = _level - 1;
        for (size_t i = 0; i < tabLevels; ++i) {
            for (size_t j = 0; j < TABSTOP; ++j) {
                tab += SPACE;
            }
        }
        print(tab);
    }

    string escapeString(const std::string &s) {
        auto binOp = [](const std::string &acc, char nextCharacter) {
            std::string nextReplacement = "";
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
};



class TranslatorImpl : public Translator {
public:
    TranslatorImpl() {}

    Status *translate(const string &program, Code **code) override {
        Parser parser;
        Status *parseStatus = parser.parseProgram(program);
        assert(parseStatus->isOk());
        delete parseStatus;
        AstPrinter *astPrinter = new AstPrinter();
        astPrinter->dump(parser.top()->node());

        return Status::Ok();
    }
};

Translator* Translator::create(const string &impl) {
    assert(impl == "printer");
    return new TranslatorImpl();
}
}
