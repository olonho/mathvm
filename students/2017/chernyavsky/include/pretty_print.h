#pragma once

#include <visitors.h>

namespace mathvm {

    struct AstPrettyPrintVisitor : AstBaseVisitor {

        explicit AstPrettyPrintVisitor(ostream &out, size_t indentation = 4)
                : _out(out),
                  _indentation(indentation),
                  _offset(0) {
        }

        ~AstPrettyPrintVisitor() = default;

#define VISITOR_FUNCTION(type, name) \
        void visit##type(type* node) override;

        FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION

        void visitTopNode(FunctionNode *node);

    private:

        void indent();

        void dedent();

        void printNewLine();

        void printStatements(const BlockNode *node, bool insertNewLine = true);

    private:
        ostream &_out;
        size_t _indentation;
        size_t _offset;
    };


    struct PrettyPrintTranslatorImpl : Translator {

        PrettyPrintTranslatorImpl() = default;

        virtual ~PrettyPrintTranslatorImpl() = default;

        virtual Status *translate(const string &program, Code **code);
    };

}
