#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "mathvm.h"
#include <vector>

namespace mathvm {

    namespace utils {

        inline VarType resultType(VarType left, VarType right) {
            if (left == right) {
                return left;
            }
            switch (left) {
                case VT_DOUBLE: return VT_DOUBLE;
                case VT_INT:
                    if (right == VT_DOUBLE) {
                        return VT_DOUBLE;
                    }
                    if (right == VT_STRING) {
                        return VT_INT;
                    }
                    break;
                case VT_STRING:
                    if (right == VT_INT && right == VT_DOUBLE) {
                        return right;
                    }
                default: return VT_INVALID;
            }
            return VT_INVALID;
        }

        inline std::vector<Instruction> insnConvert(VarType from, VarType to) {
            // to maybe either VT_INT or VT_DOUBLE (see resultType)
            std::vector<Instruction> result;
            if (from == VT_INT && to == VT_DOUBLE) {
                result.emplace_back(BC_I2D);
                return result;
            }

            if (from == VT_STRING && to == VT_INT) {
                result.emplace_back(BC_S2I);
                return result;
            }

            if (from == VT_STRING && to == VT_DOUBLE) {
                result.emplace_back(BC_S2I);
                result.emplace_back(BC_I2D);
                return result;
            }
            result.emplace_back(BC_LAST);
            return result;
        }

        inline Instruction localScopeStoreInsn(VarType type, uint16_t localId) {
            switch (type) {
                case VT_INT:
                    switch (localId) {
                        case 0: return BC_STOREIVAR0;
                        case 1: return BC_STOREIVAR1;
                        case 2: return BC_STOREIVAR2;
                        case 3: return BC_STOREIVAR3;
                        default: return BC_STOREIVAR;
                    }
                case VT_DOUBLE:
                    switch (localId) {
                        case 0: return BC_STOREDVAR0;
                        case 1: return BC_STOREDVAR1;
                        case 2: return BC_STOREDVAR2;
                        case 3: return BC_STOREDVAR3;
                        default: return BC_STOREDVAR;
                    }
                case VT_STRING:
                    switch (localId) {
                        case 0: return BC_STORESVAR0;
                        case 1: return BC_STORESVAR1;
                        case 2: return BC_STORESVAR2;
                        case 3: return BC_STORESVAR3;
                        default: return BC_STORESVAR;
                    }
                default: return BC_LAST;
            }
        }

        inline Instruction outerScopeStoreInsn(VarType type) {
            switch (type) {
                case VT_INT: return BC_STORECTXIVAR;
                case VT_DOUBLE: return BC_STORECTXDVAR;
                case VT_STRING: return BC_STORECTXSVAR;
                default: return BC_LAST;
            }
        }

        inline Instruction localScopeLoadInsn(VarType type, uint16_t localId) {
            switch (type) {
                case VT_INT:
                    switch (localId) {
                        case 0: return BC_LOADIVAR0;
                        case 1: return BC_LOADIVAR1;
                        case 2: return BC_LOADIVAR2;
                        case 3: return BC_LOADIVAR3;
                        default: return BC_LOADIVAR;
                    }
                case VT_DOUBLE:
                    switch (localId) {
                        case 0: return BC_LOADDVAR0;
                        case 1: return BC_LOADDVAR1;
                        case 2: return BC_LOADDVAR2;
                        case 3: return BC_LOADDVAR3;
                        default: return BC_LOADDVAR;
                    }
                case VT_STRING:
                    switch (localId) {
                        case 0: return BC_LOADSVAR0;
                        case 1: return BC_LOADSVAR1;
                        case 2: return BC_LOADSVAR2;
                        case 3: return BC_LOADSVAR3;
                        default: return BC_LOADSVAR;
                    }
                default: return BC_LAST;
            }
        }

        inline Instruction outerScopeLoadInsn(VarType type) {
            switch (type) {
                case VT_INT: return BC_LOADCTXIVAR;
                case VT_DOUBLE: return BC_LOADCTXDVAR;
                case VT_STRING: return BC_LOADCTXSVAR;
                default: return BC_LAST;
            }
        }

        inline Instruction arithmeticInsn(VarType type, TokenKind kind) {
            if (type != VT_INT && type != VT_DOUBLE) {
                return BC_LAST;
            }

            switch (kind) {
                case tADD: return type == VT_INT ? BC_IADD : BC_DADD;
                case tSUB: return type == VT_INT ? BC_ISUB : BC_DSUB;
                case tMUL: return type == VT_INT ? BC_IMUL : BC_DMUL;
                case tDIV: return type == VT_INT ? BC_IDIV : BC_DDIV;
                case tMOD: return type == VT_INT ? BC_IMOD : BC_INVALID;
                default: return BC_LAST;
            }
        }

        inline Instruction bitwiseInsn(TokenKind kind) {
            switch (kind) {
                case tAOR: return BC_IAOR;
                case tAAND: return BC_IAAND;
                case tAXOR: return BC_IAXOR;
                default: return BC_LAST;
            }
        }

        inline Instruction comparisonInsn(TokenKind kind) {
            switch (kind) {
                case tEQ: return BC_IFICMPE;
                case tNEQ: return BC_IFICMPNE;
                case tGT: return BC_IFICMPG;
                case tGE: return BC_IFICMPGE;
                case tLT: return BC_IFICMPL;
                case tLE: return BC_IFICMPLE;
                default: return BC_LAST;
            }
        }
    }
}
#endif //INSTRUCTION_H
