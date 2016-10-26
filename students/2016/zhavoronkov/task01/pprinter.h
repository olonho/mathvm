#pragma once

#include "visitors.h"

#include <iostream>
#include <cstdint>

namespace mathvm {

    using std :: ostream;
    using std :: string;

    namespace details {
        class PrettyPrinter : public AstBaseVisitor {
        public:
            PrettyPrinter(ostream& out, uint32_t indent);

            virtual ~PrettyPrinter();

            #define VISITOR_FUNCTION(type, name) \
            virtual void visit##type(type* node);

            FOR_NODES(VISITOR_FUNCTION)
            #undef VISITOR_FUNCTION

        private:
            string escapeChar(char c);
            void printScope(Scope* scope);
            void indent();
            void separator();
        private:
            ostream& _out;
            uint32_t _indent;
            bool _format = true;
        };
    } // end namespace details

    struct PrinterTranslatorImpl : public Translator {
        PrinterTranslatorImpl(std :: ostream& out);
        virtual ~PrinterTranslatorImpl();

        virtual Status* translate(const string& program, Code** code);

    private:
        details::PrettyPrinter _pprinter;
    };

} //end namespace mvm
