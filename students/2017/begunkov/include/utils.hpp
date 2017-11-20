#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <stack>
#include <map>
#include "mathvm.h"

template <typename T>
static T poptop(std::stack<T>& container)
{
    assert(!container.empty());

    T res = container.top();
    container.pop();
    return res;
}

template <typename T>
class mapInc
{
    std::map<T, uint16_t> mp;
    uint16_t lastNum = 0;

public:
    uint16_t operator[](T idx) {
        auto it = mp.find(idx);
        if (it == mp.end())
            return mp[idx] = lastNum++;
        return it->second;
    }
};

inline mathvm::Instruction opToBC(mathvm::TokenKind kind, bool integral)
{
    using namespace mathvm;
    if (integral) {
        switch (kind) {
            case tADD: return BC_IADD;
            case tSUB: return BC_ISUB;
            case tMUL: return BC_IMUL;
            case tDIV: return BC_IDIV;
            case tMOD: return BC_IMOD;
            case tAAND: return BC_IAAND;
            case tAOR: return BC_IAOR;
            case tAXOR: return BC_IAXOR;
            default: assert(false);
        }
    } else {
        switch (kind) {
            case tADD: return BC_DADD;
            case tSUB: return BC_DSUB;
            case tMUL: return BC_DMUL;
            case tDIV: return BC_DDIV;
            default: assert(false);
        }
    }
}

inline mathvm::Instruction orderReverese(mathvm::Instruction i)
{
    using namespace mathvm;

    switch (i) {
        case BC_IFICMPE: return BC_IFICMPE;
        case BC_IFICMPNE: return BC_IFICMPNE;
        case BC_IFICMPL: return BC_IFICMPGE;
        case BC_IFICMPG: return BC_IFICMPLE;
        case BC_IFICMPLE: return BC_IFICMPG;
        case BC_IFICMPGE: return BC_IFICMPL;
        default: assert(false);
    }
}

inline bool isCompareToken(mathvm::TokenKind token)
{
    using namespace mathvm;
    return tEQ <= token && token <= tLE;
}


#endif // UTILS_HPP__
