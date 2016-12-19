#pragma once

#include "visitors.h"
#include "helpers.h"

#include <iostream>
#include <cstdint>

namespace mathvm {
    namespace details {
        struct BytecodePrinter : public AstBaseVisitor {
            BytecodePrinter();

            virtual ~BytecodePrinter();

            #define VISITOR_FUNCTION(type, name) \
            virtual void visit##type(type* node);

            FOR_NODES(VISITOR_FUNCTION)
            #undef VISITOR_FUNCTION

            Status* translateToBytecode(Code* code, AstFunction* top);
        private:
            Bytecode* getCurrentBytecode();
            void handleCmp(BinaryOpNode* node);
            void handleArithmeticOp(BinaryOpNode* node);
            void handleLogicOp(BinaryOpNode* node);
            void handleBitwiseOp(BinaryOpNode* node);
            void handleModuloOp(BinaryOpNode* node);

            void castOperandTypes(VarType lType, VarType rType);
            void castTopType(VarType targetType);

            void initVariables(Scope* scope);
            void initFunctions(Scope* scope);

            void loadVar(const AstVar* var);
            void storeVar(const AstVar* var);
        private:
            Code* _code;
            VarType _topType;
            ScopeData* _currentScopeData;
        };
    } // end namespace details

    struct BytecodeTranslator : public Translator {
        BytecodeTranslator();
        
        ~BytecodeTranslator();

        virtual Status* translate(const string& program, Code** code);
    };

} //end namespace mvm
