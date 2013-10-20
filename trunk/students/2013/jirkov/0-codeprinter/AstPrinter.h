#pragma once


#include "visitors.h"

namespace mathvm {
    class AstPrinter : public AstBaseVisitor 
    {
        public:

            AstPrinter() : m_indentLevel(0) { }

            virtual ~AstPrinter() { }

#define VISITOR_FUNCTION(type, name) \
            virtual void visit##type(type* node); 

            FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION



        private:
            const static size_t INDENT_UNIT = 2; 
            size_t m_indentLevel; 

            void widerIndent() 
            {
                m_indentLevel += INDENT_UNIT;
            }

            void tighterIndent() 
            {
                if ( m_indentLevel != 0 ) m_indentLevel -= INDENT_UNIT;
            }

            std::string indent() const 
            {
                return std::string( m_indentLevel, ' ' );
            }

            void goDeeper( BlockNode*, bool );
    };

}
