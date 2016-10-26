#pragma once
#include <ostream>
#include <visitors.h>

namespace mathvm {
    class AstPrinterVisitor : public AstBaseVisitor {
    public:
        AstPrinterVisitor(std::ostream& output)
                : os(output)
                , identNum(0) {}

#define TYPE_VISITOR_F(type, name)            \
    virtual void visit##type(type* node) override;

        FOR_NODES(TYPE_VISITOR_F)
#undef TYPE_VISITOR_F

    private:
        uint32_t scopeLevel(Scope const* scope);
        void print(const AstVar* var);
        void newLine();
        void print_symbol(char c);

        std::ostream& os;
        uint32_t identNum;
    }; // AstPrinterVisitor
} // mathvm