#ifndef CODETRANSLATOR_H
#define	CODETRANSLATOR_H


#include "ast.h"
#include <stack>


#include "Utilities.h"

namespace mathvm {


    class VarToContext {
    private:
        uint16_t myContextId;
        std::map<AstVar const *, ContextInfo*> myVarToContext;
        VarToContext* myParent;
        uint16_t myNextId;
        
    public:
        VarToContext(uint16_t context, VarToContext* parent) : myContextId(context), myParent(parent) {
            myNextId  =  (!myParent || myParent->context() != myContextId) ? 0 : parent->nextId();       
        }

        bool contains(AstVar const *key) {
            return myVarToContext.count(key) > 0;
        }

        ContextInfo  get(AstVar const *key) {
            if (contains(key)) return *myVarToContext[key];
            
            if(myParent == 0) 
                throw ExceptionInfo("ContextInfo: can't find var(") << key->name() << ")";    
            return myParent->get(key);
           
        }

        uint16_t add(AstVar const *key) {
            if(myVarToContext.count(key)) 
                cerr << "Warining: attempt to add an existing key(" << key->name() << ")";
            myVarToContext[key] = new ContextInfo(myNextId, myContextId);
            return myNextId++;
        }

        uint16_t nextId() const {
            return myNextId;
        }

        uint16_t context() const {
            return myContextId;
        }


    };


    class CodeTranslator: public mathvm::AstVisitor {
    private: // private methods
        VarType getType(CustomDataHolder* nodePtr) {
            if(myTypeMap->count(nodePtr))
                return (*myTypeMap)[nodePtr];
            return VT_INVALID;
        }
        
        bool handleConditionExpression(VarType exprType) {
            if(exprType == VT_VOID && exprType != VT_INVALID )
                return 0;
    
            if(exprType == VT_DOUBLE) bytecode()->addInsn(BC_D2I);
            if(exprType == VT_STRING) bytecode()->addInsn(BC_S2I); // может быть
            return 1;
        } 
        
        void visitCalculativeOp(BinaryOpNode* node);
        void visitComparativeOp(BinaryOpNode* node);
        void visitAndOp(BinaryOpNode* node);
        void visitOrOp(BinaryOpNode* node);
        
    private: // private fields
        Code* _code;
        AstTypeMap *myTypeMap; 
        BytecodeFunction* myCurrentFunction;

        std::stack<VarToContext*> _varScopes; 
        VarToContext* myCurrentContext;
        

    public:
        CodeTranslator(Code* code);
        virtual ~CodeTranslator();

        void translate(AstFunction* top, AstTypeMap *typeMap);

        #define VISITOR_FUNCTION(type, name) \
        virtual void visit##type(type* node);

        FOR_NODES(VISITOR_FUNCTION)
        #undef VISITOR_FUNCTION

    private:
        Bytecode* bytecode() {
            if(myCurrentFunction == 0) return 0;
            return myCurrentFunction->bytecode();
        }

        void processFunction(AstFunction* top);

        void storeLocal(VarType type, uint16_t id);
        void store(const AstVar* var);

        void load(const AstVar* var);
        void loadLocal(VarType type, uint16_t id);      
    };

}


#endif

