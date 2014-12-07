#pragma once
#include <iostream>
#include "../../../../vm/parser.h"
#include "ast_printer.h"
#include "common.h"

namespace mathvm {

    class AstPrinterVisitor : public AstVisitor {

    private:
        void indent();

        void _enter();

        void _leave();

        void functionDeclaration(Scope *scope);

        void variableDeclaration(Scope *scope);

        std::ostream& _out;
        size_t _indent;
        uint8_t _spacesForIndent;

    public:
        AstPrinterVisitor(
                std::ostream &out = std::cout,
                const uint8_t indentSpaces = 3) :
                _out(out),
                _indent(0),
                _spacesForIndent(indentSpaces) { }

        virtual void enterBlock(BlockNode *node);

        FOR_NODES(VISITOR_FUNCTION)

    };



}