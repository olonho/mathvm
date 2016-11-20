#ifndef CODEPRINTER_H
#define CODEPRINTER_H

#include "ast.h"
#include "visitors.h"
#include "context.h"
#include "generator_exception.h"

namespace mathvm
{

    class CodeGenerator : public AstBaseVisitor 
    {
    public:
        CodeGenerator(Code* input);
        virtual ~CodeGenerator();
    public:
    #define VISITOR_FUNCTION(type, name) \
        virtual void visit##type(type* node) override;

        FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION
    
    public:
        Status* addFunction(AstFunction* function);
    
    private:
        // Binary operation handlers
        void handleLogicalOp(BinaryOpNode* node);
        void handleBitOp(BinaryOpNode* node);
        void handleArithmeticOp(BinaryOpNode* node);
        void handleCompareOp(BinaryOpNode* node);
        std::pair<VarType, VarType> visitTwoArguments(BinaryOpNode* node);
        
        // Unary operation handlers
        void handleNegOp(UnaryOpNode* node);
        void handleNotOp(UnaryOpNode* node);
        
        // Cast methods
        void castTypeTOS(VarType target);
        VarType castBinaryOp(VarType lhs, VarType rhs);
        
        // Getters
        Bytecode* getBytecode();
    
    private:
        Code* _code;
        Context* _context;
        VarType _tosType;
    };
}

#endif /* CODEPRINTER_H */

