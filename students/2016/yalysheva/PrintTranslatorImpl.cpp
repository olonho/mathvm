//
// Created by natalia on 15.10.16.
//

#include "PrintTranslatorImpl.h"
//#include "parser.h"
//#include "visitors.h"

// for build in ide:
#include "../../../vm/parser.h"
#include "../../../include/visitors.h"

namespace mathvm {

struct PrintVisitor : public AstBaseVisitor {

    PrintVisitor(ostream &outs, const TokenList &list) : _outs(outs), _tokens(list)
    { }

    virtual ~PrintVisitor() {
    }

    virtual void visitForNode(ForNode *node) {
        printNodeValue(node);
        _outs << " " << tokenOp(tLPAREN)
              << node->var()->name()
              << " " << "in" << " ";
        node->inExpr()->visit(this);
        _outs << tokenOp(tRPAREN);
        node->body()->visit(this);
    }

    virtual void visitPrintNode(PrintNode *node) {
//        printNodeValue(node);    // didn't work because of parser.cpp:213 (_currentTokenIndex)
        _outs << "print"
              << tokenOp(tLPAREN);
        for (size_t i = 0; i < node->operands(); ++i) {
            if (i != 0)
                _outs << tokenOp(tCOMMA) << " ";
            node->operandAt(i)->visit(this);
        }
        _outs << tokenOp(tRPAREN);
    }

    virtual void visitLoadNode(LoadNode *node) {
        _outs << node->var()->name();
    }

    virtual void visitIfNode(IfNode *node) {
        printNodeValue(node);
        _outs << " " << tokenOp(tLPAREN);
        node->ifExpr()->visit(this);
        _outs << tokenOp(tRPAREN);
        node->thenBlock()->visit(this);

        if(node->elseBlock()) {
            _outs << " " << "else";
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        node->left()->visit(this);
        _outs << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        _outs << node->literal();
    }

    virtual void visitStoreNode(StoreNode *node) {
        _outs << node->var()->name()
              << " " << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        _outs << "'" << escapeString(node->literal()) << "'";
    }

    virtual void visitWhileNode(WhileNode *node) {
        _outs << endl;
        printNodeValue(node);
        _outs << " " << tokenOp(tLPAREN);
        node->whileExpr()->visit(this);
        _outs << tokenOp(tRPAREN);
        node->loopBlock()->visit(this);
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) {
        _outs << node->literal();
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        _outs << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        printNodeValue(node);
        _outs << " '" << node->nativeName() << "'";
    }

    virtual void visitBlockNode(BlockNode *node) {
        if (_nesting != 0)
            _outs << " " << tokenOp(tLBRACE) << endl;
        ++_nesting;

        Scope *scope = node->scope();

        Scope::VarIterator varIterator = Scope::VarIterator(scope);
        while (varIterator.hasNext()) {
            AstVar *var = varIterator.next();
            printIndent();
            _outs << typeToName(var->type())
                  << " " << var->name()
                  << tokenOp(tSEMICOLON) << endl;
        }

        Scope::FunctionIterator functionIterator  = Scope::FunctionIterator(scope);
        while (functionIterator.hasNext()) {
            AstFunction *function = functionIterator.next();
            printIndent();
            function->node()->visit(this);
            _outs << endl;
        }

        for (size_t i = 0; i < node->nodes(); ++i) {
            AstNode *node_i = node ->nodeAt(i);
            printIndent();
            node_i->visit(this);
            if (!(node_i->isForNode()
             || node_i->isIfNode()
             || node_i->isWhileNode()
             || node_i->isBlockNode()
             || node_i->isFunctionNode()))
                _outs << tokenOp(tSEMICOLON) << endl;
        }

        --_nesting;
        if (_nesting != 0) {
            printIndent();
            _outs << tokenOp(tRBRACE) << endl;
        }
    }

    virtual void visitFunctionNode(FunctionNode *node) {
        if (node->name() != AstFunction::top_name) {
            printNodeValue(node);
            _outs << " " << typeToName(node->returnType())
                  << " " << node->name()
                  << tokenOp(tLPAREN);

            for (size_t i = 0; i < node->parametersNumber(); ++i) {
                if (i != 0)
                    _outs << tokenOp(tCOMMA) << " ";
                _outs << typeToName(node->parameterType(i))
                      << " " << node->parameterName(i);
            }
            _outs << tokenOp(tRPAREN);
        }

        if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
            _outs << endl;
            return;
        }

        node->body()->visit(this);
    }

    virtual void visitReturnNode(ReturnNode *node) {
//        printNodeValue(node);  didn't work in case of no return statement in source (in which case tokenIndex=0)
        _outs << "return";
        if (node->returnExpr()) {
            _outs << " ";
            node->returnExpr()->visit(this);
        }
    }

    virtual void visitCallNode(CallNode *node) {
        _outs << node->name()
              << tokenOp(tLPAREN);
        for (size_t i = 0; i < node->parametersNumber(); ++i) {
            if (i != 0)
                _outs << ", ";
            node->parameterAt(i)->visit(this);
        }
        _outs << tokenOp(tRPAREN);

    }

private:

    void printIndent() {
        for(size_t i = 0; i < _nesting-1; ++i)
            _outs << "    ";
    }

    void printNodeValue(AstNode *node) {
        _outs << _tokens.valueAt(node->position());
    }

    string escapeChar(char c) {
        switch(c) {
            case '\a':
                return "\\a";
            case '\b':
                return "\\b";
            case '\f':
                return "\\f";
            case '\n':
                return "\\n";
            case '\r':
                return "\\r";
            case '\t':
                return "\\t";
            case '\v':
                return "\\v";
            case '\\':
                return "\\\\";
            case '\'':
                return "\\'";
            case '\"':
                return "\\\"";
            case '\?':
                return "\\?";
            default:
                return string(1, c);
        }
    }

    string escapeString(const string &str) {
            string result;
            for (char c : str) {
                result += escapeChar(c);
            }
        return result;
        }

        ostream & _outs;
        TokenList _tokens;
        uint32_t _nesting = 0;
};

PrintTranslatorImpl::~PrintTranslatorImpl() {
}

Status *PrintTranslatorImpl::translate(const std::string &program, Code **code) {

    Parser parser;
    Status *status = parser.parseProgram(program);

    if (!status->isOk())
        return status;

    PrintVisitor visitor(cout, parser.tokens());
    parser.top()->node()->visit(&visitor);

    return Status::Ok();
}

} //mathvm namespace

