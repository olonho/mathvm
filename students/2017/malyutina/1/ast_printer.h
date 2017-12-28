//
// Created by kate on 23.09.17.
//

#ifndef MATHVM_AST_PRINTER_H_H
#define MATHVM_AST_PRINTER_H_H

#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"

namespace mathvm {

    namespace ast_printer {

        class ast_printer : public Translator {
        public:
            virtual  ~ast_printer();
            virtual Status *translate(const string &program, Code **code) override;
        };
    }

    namespace printer {
        using namespace std;

        class Printer : public AstVisitor {
        private:
            ostream &out;
            int level;
        public:
            Printer();

            ~Printer();

            void visitForNode(ForNode *node);

            void visitPrintNode(PrintNode *node);

            void visitLoadNode(LoadNode *node);

            void visitIfNode(IfNode *node);

            void visitIntLiteralNode(IntLiteralNode *node);

            void visitDoubleLiteralNode(DoubleLiteralNode *node);

            void visitStringLiteralNode(StringLiteralNode *node);

            void visitWhileNode(WhileNode *node);

            void visitBinaryOpNode(BinaryOpNode *node);

            void visitUnaryOpNode(UnaryOpNode *node);

            void visitNativeCallNode(NativeCallNode *node);

            void visitFunctionNode(FunctionNode *node);

            void visitReturnNode(ReturnNode *node);

            void visitStoreNode(StoreNode *node);

            void visitCallNode(CallNode *node);

            void visitBlockNode(BlockNode *node);

        private:

            void makeLevel() ;

            bool notScopeNode(AstNode *node) ;

            void printScope(Scope *scope);

            string escape(const string &str) ;
        };
    }
}


#endif //MATHVM_AST_PRINTER_H_H
