#ifndef MATHVM_ASTPRINTER_H
#define MATHVM_ASTPRINTER_H

#include "ast.h"
#include "visitors.h"

namespace mathvm {
    struct AstPrinterVisitor : public AstVisitor {
        AstPrinterVisitor(std::ostream& os = std::cout);
        ~AstPrinterVisitor();
#define VISITOR_FUNCTION(type, name) \
        void visit##type(type* node) override;

        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    private:
        void visitAstVar(const AstVar* var);
        void visitScope(Scope* scope);
        void visitAstFunction(AstFunction* func);
        std::ostream& _os;
        size_t _offset;
        size_t _curOffset;
    };

    struct AstPrinterTranslator : Translator {
        Status* translate(const std::string &program, Code **code) override;
    };
}
#endif //MATHVM_ASTPRINTER_H
