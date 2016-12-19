//
// Created by dsavvinov on 16.10.16.
//

#include "PrinterVisitor.h"
#include <algorithm>
namespace mathvm {

void PrinterVisitor::visitForNode(ForNode *node) {
    cout << getIndent() << "for (" << node->var()->name() << " in ";

    shouldIndent = false;
    node->inExpr()->visit(this);
    shouldIndent = true;

    cout << ") {" << endl;

    Indent();
    node->body()->visit(this);
    Outdent();
    cout << getIndent() << "}";
}

void PrinterVisitor::visitPrintNode(PrintNode *node) {
    cout << getIndent() << "print (";
    for (uint32_t i = 0; i < node->operands(); ++i) {
        shouldIndent = false;
        node->operandAt(i)->visit(this);

        if (i != node->operands() - 1) {
            cout << ", ";
        }
    }
    shouldIndent = true;
    cout << ");";
}

void PrinterVisitor::visitLoadNode(LoadNode *node) {
    cout << getIndent() << node->var()->name();
}

void PrinterVisitor::visitIfNode(IfNode *node) {
    // Condition
    cout << getIndent() << "if (";
    shouldIndent = false;
    node->ifExpr()->visit(this);
    shouldIndent = true;
    cout << ") {" << endl;

    // Then-block
    Indent();
    node->thenBlock()->visit(this);
    Outdent();
    cout << getIndent() << "}";

    // Else-block
    if (node->elseBlock()) {
        cout << " else {" << endl;
        Indent();
        node->elseBlock()->visit(this);
        Outdent();
        cout << getIndent() << "}";
    }
}

void PrinterVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    bool shouldSurroundLeft = isNeedingBrackets(node->left(), tokenPrecedence(node->kind()));
    bool shouldSurroundRight = isNeedingBrackets(node->right(), tokenPrecedence(node->kind()));
    cout << getIndent();

    if (shouldSurroundLeft) {
        cout << "(";
    }
    shouldIndent = false;
    node->left()->visit(this);
    if (shouldSurroundLeft) {
        cout << ")";
    }

    cout << " " << tokenOp(node->kind()) << " ";

    if (shouldSurroundRight) {
        cout << "(";
    }
    shouldIndent = false;
    node->right()->visit(this);
    if (shouldSurroundRight) {
        cout << ")";
    }

    shouldIndent = true;
}

void PrinterVisitor::visitCallNode(CallNode *node) {
    cout << getIndent();
    cout << node->name() << " (";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        shouldIndent = false;
        node->parameterAt(i)->visit(this);

        if (i != node->parametersNumber() - 1) {
            cout << ", ";
        }
    }
    shouldIndent = true;
    cout << ")";
}

void PrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    cout << getIndent();
    cout << fixed;
    cout.precision(15);
    cout << node->literal();
}

void PrinterVisitor::visitStoreNode(StoreNode *node) {
    cout << getIndent();
    cout << node->var()->name() << " " << tokenOp(node->op()) << " ";
    shouldIndent = false;
    node->value()->visit(this);
    shouldIndent = true;
    cout << ";";
}

void PrinterVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    cout << getIndent();
    cout << "\'";
    printLiteral(node->literal());
    cout << "\'";
}

void PrinterVisitor::visitWhileNode(WhileNode *node) {
    cout << getIndent() << "while (";
    shouldIndent = false;
    node->whileExpr()->visit(this);
    shouldIndent = true;
    cout << ") {" << endl;

    Indent();
    node->loopBlock()->visit(this);
    Outdent();
    cout << getIndent() << "}";
}

void PrinterVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    cout << getIndent() << node->literal();
}

void PrinterVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    // Pretty-print: do not print redundant braces in obvious cases at least
    bool shouldSurround = isNeedingBrackets(node->operand(), tokenPrecedence(node->kind()));

    cout << getIndent() << tokenOp(node->kind());
    shouldIndent = false;
    if (shouldSurround) {
        cout << "(";
    }
    node->operand()->visit(this);
    if (shouldSurround) {
        cout << ")";
    }
    shouldIndent = true;
}

void PrinterVisitor::visitNativeCallNode(NativeCallNode *node) {
    cout << getIndent() << "native "
         << "\'" << node->nativeName() << "\'"
         << ";" << endl;
}

void PrinterVisitor::visitBlockNode(BlockNode *node) {
    // Check if it is native call block, because it's a degenerate case
    if (node->nodes() > 0 && node->nodeAt(0)->isNativeCallNode()) {
        shouldIndent = false;
        node->nodeAt(0)->visit(this);
        shouldIndent = true;
        return;
    }

    printScope(node->scope());

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        cout << endl;
    }
}

void PrinterVisitor::visitReturnNode(ReturnNode *node) {
    cout << getIndent() << "return ";
    shouldIndent = false;
    node->visitChildren(this);
    shouldIndent = true;
    cout << ";";
}

void PrinterVisitor::visitFunctionNode(FunctionNode *node) {
    bool isNative = node->body()->nodeAt(0)->isNativeCallNode();

    // Signature
    cout << getIndent() << "function" << " " << typeToName(node->returnType()) << " " << node->name() << " (";

    // Args list
    // iterate from 1 to skip return type
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        VarType const & curType = node->parameterType(i);
        string const & varName = node->parameterName(i);

        cout << typeToName(curType) << " " << varName;

        if (i != node->parametersNumber() - 1) {
            cout << ", ";
        }
    }

    cout << ") " << (isNative ? "" : "{\n");

    // Body
    Indent();
    node->body()->visit(this);
    Outdent();
    cout << getIndent() << (isNative ? "" : "}");
}

void PrinterVisitor::printScope(Scope *s) {
    Scope::VarIterator varIter(s);
    bool hasDeclarations = false;
    while (varIter.hasNext()) {
        hasDeclarations = true;
        AstVar * curVar = varIter.next();
        cout << getIndent() << typeToName(curVar->type()) << " " << curVar->name() << ";" << endl;
    }

    /* Pretty-printing: separate list of declarations by empty line,
       but don't print ugly empty line if there are no declarations at all
     */
    if (hasDeclarations) {
        cout << endl;
    }

    /* FunctionIterator iterates over functions in reverse order
       of their declarations, so we have to collect them
       and then sort so that earliest function is printed first.
     */
    vector <AstFunction*> funDeclarations {};
    Scope::FunctionIterator funIter(s);
    while (funIter.hasNext()) {
        AstFunction * curFun = funIter.next();
        funDeclarations.push_back(curFun);
    }

    std::sort(funDeclarations.begin(), funDeclarations.end(),
                [](AstFunction * lhs, AstFunction * rhs) -> bool {
                   return lhs->node()->position() < rhs->node()->position();
                });

    for (uint32_t i = 0; i < funDeclarations.size(); ++i) {
        AstFunction * curFun = funDeclarations[i];
        curFun->node()->visit(this);
        cout << endl;
        // Pretty-printing: separate each function definition with empty line
        cout << endl;
    }
}

void PrinterVisitor::printLiteral(string const &s) {
    for (uint32_t i = 0; i < s.length(); ++i) {
        // process escape-chars
        if (s[i] == '\n')
            cout << "\\n";
        else
            cout << s[i];
    }
}

bool PrinterVisitor::isNeedingBrackets(AstNode *node, int thisPrecedence) {
    // Don't surround in most trivial cases at least
    if (node->isLoadNode() ||
            node->isDoubleLiteralNode() ||
            node->isIntLiteralNode() ||
            node->isStringLiteralNode() ||
            node->isCallNode()
            ) {
        return false;
    }

    // Conservatively, surround everything else
    return true;
}
}