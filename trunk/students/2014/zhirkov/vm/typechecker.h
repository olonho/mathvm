#pragma once

#include <stack>
#include <map>
#include "ir/ir.h"
#include "../../../../vm/parser.h"
#include "ast_printer.h"
#include "ast_metadata.h"
#include "common.h"
#include "ast_utils.h"
#include "ast_analyzer.h"
#include <map>

namespace mathvm {

    class TypeChecker : AstAnalyzer<std::map<AstNode *, VarType>, AstAnalyzerContext> {
    public:
        TypeChecker(AstFunction const *top) : AstAnalyzer(top) {
        }

        void setType(AstNode *node, VarType type) {
            (*_result)[node] = type;
        }

        VarType getType(AstNode *node) {
            if (_result->find(node) == _result->end()) return VT_VOID;
            return (*_result)[node];
        }

        virtual void visitBinaryOpNode(BinaryOpNode *node);

        virtual void visitUnaryOpNode(UnaryOpNode *node);

        virtual void visitStringLiteralNode(StringLiteralNode *node);

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *node);

        virtual void visitIntLiteralNode(IntLiteralNode *node);

        virtual void visitLoadNode(LoadNode *node);

        virtual void visitBlockNode(BlockNode *node);

        virtual void visitFunctionNode(FunctionNode *node);

        virtual void visitReturnNode(ReturnNode *node);

        virtual void visitCallNode(CallNode *node);


        virtual void visitAstFunction(AstFunction*);

        virtual ~TypeChecker() {
        }
    };


}