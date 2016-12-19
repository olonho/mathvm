#pragma once

#include <sstream>

#include "ast.h"
#include "visitors.h"

namespace mathvm
{
    class AstPrinter : public AstBaseVisitor
    {
    public: 
        AstPrinter();
        virtual ~AstPrinter();

    public:
    #define VISITOR_FUNCTION(type, name) \
        virtual void visit##type(type* node) override;

        FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION

        std::string getResult() const;

    private:
        static std::string escapeChar(char symbol);
        std::string getIndent() const;
        void validateBlockStart();
        void validateBlockEnd();
        void printComma(size_t index);
    
    private:
        static const char SPACE = ' ';

        int32_t _indent_count;
        std::stringstream _output;
    };
}