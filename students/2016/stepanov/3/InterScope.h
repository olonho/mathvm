//
// Created by user on 11/30/16.
//

#ifndef VM_AF_3_INTERSCOP11E_H
#define VM_AF_3_INTERSCOP11E_H

#include "StackItem.h"
#include <stack>
#include "../../../../include/mathvm.h"


namespace mathvm {

    extern std::stack<size_t> scopeOffsets[UINT16_MAX];

    class InterScope {
    private:
        Bytecode *bytecode;
        BytecodeFunction *bf;
    public:
        //[TODO] REWRITE
        Status *status = nullptr;
        uint32_t IP = 0;
        std::size_t variableOffset;
        InterScope *parent;

        InterScope(BytecodeFunction *bf, InterScope *parent = nullptr){
            this->bf = bf;
            this->parent = parent;
            variableOffset = variables.size();
            variables.resize(variableOffset + bf->localsNumber(), STACK_EMPTY);
            scopeOffsets[bf->scopeId()].push(variableOffset);
            bytecode = bf->bytecode();
        }

        ~InterScope(){
            variables.resize(variableOffset);
            scopeOffsets[bf->scopeId()].pop();
            delete (status);
        }

        Instruction next(){
            return bytecode->getInsn(IP++);
        }

        inline uint16_t nextUint16t(){
            const uint32_t currentIP = IP;
            IP += sizeof(uint16_t);
            return bytecode->getUInt16(currentIP);
        }

        inline void skipUint16t(){
            IP += sizeof(uint16_t);
        }

        inline int64_t nextInt(){
            const uint32_t currentIP = IP;
            IP += sizeof(int64_t);
            return bytecode->getInt64(currentIP);
        }

        inline double nextDouble(){
            const uint32_t currentIP = IP;
            IP += sizeof(double);
            return bytecode->getDouble(currentIP);
        }

        inline void jump(){
            const uint32_t currentIP = IP;
            IP += bytecode->getInt16(currentIP);
#ifdef MY_DEBUG
            std::cout << (currentIP-1) << "|: jump to " << IP << std::endl;
#endif
        }

        StackItem *variableLookup(uint16_t scope, uint16_t variable){
            if (bf->scopeId() == scope) {
                return &(variables[variableOffset + variable]);
            } else {
                return &(variables[scopeOffsets[scope].top() + variable]);
            }
        }
    };


};


#endif //VM_AF_3_INTERSCOPE_H

