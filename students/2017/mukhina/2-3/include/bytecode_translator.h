#ifndef INC_1_BYTECODE_TRANSLATOR_H
#define INC_1_BYTECODE_TRANSLATOR_H

#include <stack>
#include "mathvm.h"
#include "ast.h"

namespace mathvm {
    class BytecodeFunctionImpl;
    typedef map<const AstVar*, uint16_t> AstVarsIds;
    typedef map<TranslatedFunction*, uint16_t> FunctionsIds;
    typedef map<TranslatedFunction*, AstFunction*> FunctionsMap;
    typedef map<Scope*, uint16_t> ScopesIds;

    struct AstBytecodeVisitor : AstVisitor {
    private:
        Bytecode* _bytecode = new Bytecode();
        Code *_code = nullptr;
        ScopesIds* _scopes;
        AstVarsIds* _varsIds;
        FunctionsIds* _functionsIds;
        map<uint16_t, map<const AstVar*, uint16_t>>* _varsPerScopes;
        stack<VarType> typesInStack;

        void booleanBinOperation(BinaryOpNode* node);
        void relationBinOperation(BinaryOpNode* node);
        void mathBinOperation(BinaryOpNode* node);
        void toOneType();
        #define VISITOR_FUNCTION(type, name)            \
                        void visit##type(type* node) override;
                FOR_NODES(VISITOR_FUNCTION)
        #undef VISITOR_FUNCTION
    public:
        AstBytecodeVisitor(Code *code,
                           ScopesIds* scopes,
                           AstVarsIds* varsIds,
                           FunctionsIds* functionsIds,
                           map<uint16_t, map<const AstVar*, uint16_t>>* varsByScopes)
                : _code(code),
                  _scopes(scopes),
                  _varsIds(varsIds),
                  _functionsIds(functionsIds),
                  _varsPerScopes(varsByScopes){};
        virtual ~AstBytecodeVisitor() = default;;
        Bytecode *getBytecode(TranslatedFunction *function, AstFunction *astFunction);
    };

    struct BytecodeTranslator : Translator {
    private:
        map<uint16_t, map<const AstVar*, uint16_t>>* _varsPerScopes =
                new map<uint16_t, map<const AstVar*, uint16_t>>();
        map<uint16_t, vector<uint16_t>>* _scopesChilds =
                new map<uint16_t, vector<uint16_t>>();
        ScopesIds* _scopes = new ScopesIds();
        AstVarsIds* _varsIds = new AstVarsIds();
        FunctionsIds* _functionsIds = new FunctionsIds();
        FunctionsMap* _astFunctions = new FunctionsMap();
        Code * _code = nullptr;

        uint16_t extractScopesAndVars(Scope *initialScope);
        void extractFunctions(Scope* initialScope);
        map<const string, uint16_t>* getTopScopeVarsNames();
    public:
        virtual ~BytecodeTranslator(){
            delete(_scopes);
            delete(_varsIds);
            delete(_functionsIds);
            delete(_varsPerScopes);
            delete(_astFunctions);
        }
        virtual Status* translate(const string& program, Code** code) override;
    };


    class BytecodeFunctionImpl : public BytecodeFunction {
    private:
        Bytecode* _bytecode = nullptr;
    public:
        explicit BytecodeFunctionImpl(AstFunction* function) :
                BytecodeFunction(function) {}
        Bytecode* getBytecode() const {
            return _bytecode;
        }

        void setBytecode(Bytecode* bytecode) {
            _bytecode = bytecode;
        }

        virtual ~BytecodeFunctionImpl() {
            delete(_bytecode);
        }
    };
}
#endif //INC_1_BYTECODE_TRANSLATOR_H
