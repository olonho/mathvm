#ifndef BYTECODETRASLATOR_H
#define	BYTECODETRASLATOR_H

#include "mathvm.h"
#include "visitors.h"
#include "bytecodeCode.h"
#include <string>
#include <stack>
#include <map>
#include <set>

namespace mathvm {

    class BytecodeTranslator : public Translator {
        Status* translateBytecode(
                const string& program,
                Code** code);

    public:

        BytecodeTranslator() {
        }

        virtual ~BytecodeTranslator() {
        }

        virtual Status* translate(const string& program, Code** code);
        
    };

    class BytecodeAstVisitor : public AstVisitor {
        friend BytecodeTranslator;
        BytecodeCode* code;
        vector<uint16_t> functionsStack;
        vector<uint16_t> contextsStack;
        BytecodeFunction* currentFunction;
        Status* status;
        set<TokenKind> logicKinds;
        set<TokenKind> logicCompareKinds;
        map<TokenKind, Instruction> logicKindToJump;

    public:

        BytecodeAstVisitor(BytecodeCode* code_) : code(code_), status(NULL) {
            logicCompareKinds.insert(tEQ);
            logicCompareKinds.insert(tNEQ);
            logicCompareKinds.insert(tGT);
            logicCompareKinds.insert(tGE);
            logicCompareKinds.insert(tLT);
            logicCompareKinds.insert(tLE);
            logicKinds.insert(tAND);
            logicKinds.insert(tOR);

            logicKindToJump[tEQ] = BC_IFICMPE;
            logicKindToJump[tNEQ] = BC_IFICMPNE;
            logicKindToJump[tGT] = BC_IFICMPG;
            logicKindToJump[tGE] = BC_IFICMPGE;
            logicKindToJump[tLT] = BC_IFICMPL;
            logicKindToJump[tLE] = BC_IFICMPLE;
        }

        void addTrueFalseJumpRegion(Instruction jumpInsn);

        inline void setTrueJump(uint16_t to) {
            setJump(trueIdUnsettedPos, to);
        }

        inline void setFalseJump(uint16_t to) {
            setJump(falseIdUnsettedPos, to);
        }
        
        inline void addJump(uint16_t to) {
            addId(0);
            setJump(current() - 2, to);
        }

        inline void setJump(uint16_t jumpId, uint16_t to) {
            currentBytecode()->setInt16(jumpId, (uint16_t) to - jumpId);
        }

        inline void addTypedSwap(VarType type) {
            if (type == VT_INT)
                addInsn(BC_ISWAP);
            if (type == VT_DOUBLE)
                addInsn(BC_DSWAP);
            if (type == VT_STRING)
                addInsn(BC_SSWAP);
        }

        void visitAst(AstFunction*);
        bool beforeVisit();

#define VISITOR_FUNCTION(type, name) \
        virtual void visit##type##_(type* node); \
        inline virtual void visit##type(type* node){ \
                if(beforeVisit()) \
                        return; \
                visit##type##_(node); \
        } 
        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

        void visitBinaryLogicOpNode(BinaryOpNode* node);
        void fillAstFunction(AstFunction*, BytecodeFunction*);

    private:

        inline Bytecode* currentBytecode() {
            return currentFunction->bytecode();
        }

        inline void addInsn(Instruction insn) {
            currentBytecode()->addInsn(insn);
        }

        inline void add(uint8_t b) {
            currentBytecode()->add(b);
        }

        inline void addId(uint16_t id) {
            currentBytecode()->addInt16(id);
        }

        inline uint32_t current() {
            return currentBytecode()->current();
        }

        uint16_t allocateVar(AstVar& var);

        map<uint16_t, map<string, uint16_t> > contextVarIds;
        map<uint16_t, map<string, uint16_t> > functionParamIds;

        stack<VarType> typesStack;

        void addTypedOpInsn(VarType type, TokenKind op);

        inline VarType topType() {
            return typesStack.top();
        }

        inline uint16_t findVarLocal(const string& name) {
            return findVar(name, true).second;
        }

        pair<uint16_t, uint16_t> findVar(const string& name, bool onlyCurrentContext = false);

        void loadVar(const AstVar* var);

        inline void ensureType(VarType td, uint16_t truePos,
                uint16_t falsePos) {
            ensureType(topType(), td, truePos, falsePos);
        }
        
        inline void addCastSpace() {
            // check ensureType() after
            addInsn(BC_INVALID); // basic cast
//            addInsn(BC_INVALID); // or jump id
//            addInsn(BC_INVALID); // if logic
        }
        
        inline void ensureType(VarType ts, VarType td,
                uint16_t truePos, uint16_t falsePos) {
            addCastSpace();
            // always look at addCastSpace()
            ensureType(ts, td, current() - 1, truePos, falsePos);
        }

        void ensureType(VarType ts, VarType td, uint32_t pos,
                uint16_t truePos, uint16_t falsePos);



        uint16_t currentContext; // aka function
        uint16_t trueIdUnsettedPos;
        uint16_t falseIdUnsettedPos;
        
        


    };




}

#endif

