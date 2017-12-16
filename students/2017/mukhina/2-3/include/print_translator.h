#ifndef TRANSLATOR_IMPL_H
#define TRANSLATOR_IMPL_H

#include "mathvm.h"
#include "ast.h"


namespace mathvm {
    struct AstPrintVisitor : AstVisitor {
#define VISITOR_FUNCTION(type, name)            \
		void visit##type(type* node) override;
        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    private:
        void printOffset();
        void print(const string& str);
        int currentOffsetNumber = 0;
        string offset = "    ";
    };

    struct AstPrintTranslator : Translator {
        virtual ~AstPrintTranslator(){}
        virtual Status* translate(const string& program, Code** code) override;
    };
}
#endif //TRANSLATOR_IMPL_H
