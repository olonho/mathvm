#pragma once

#include "includes.h"

#include <sstream>
#include <map>

namespace mathvm {

//inline Instruction getStoreVarInsn(VarType type) {
//    switch (type) {
//        case VT_DOUBLE:
//            return BC_STOREDVAR;
//        case VT_INT:
//            return BC_STOREIVAR;
//        case VT_STRING:
//            return BC_STORESVAR;
//        default:
//            return BC_INVALID;
//    }
//}

    inline Instruction getStoreCtxVarInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_STORECTXDVAR;
            case VT_INT:
                return BC_STORECTXIVAR;
            case VT_STRING:
                return BC_STORECTXSVAR;
            default:
                return BC_INVALID;
        }
    }

//inline Instruction getLoadVarInsn(VarType type) {
//    switch (type) {
//        case VT_DOUBLE:
//            return BC_LOADDVAR;
//        case VT_INT:
//            return BC_LOADIVAR;
//        case VT_STRING:
//            return BC_LOADSVAR;
//        default:
//            return BC_INVALID;
//    }
//}

    inline Instruction getLoadCtxVarInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_LOADCTXDVAR;
            case VT_INT:
                return BC_LOADCTXIVAR;
            case VT_STRING:
                return BC_LOADCTXSVAR;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getLoadInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_DLOAD;
            case VT_INT:
                return BC_ILOAD;
            case VT_STRING:
                return BC_SLOAD;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getAddInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_DADD;
            case VT_INT:
                return BC_IADD;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getSubInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_DSUB;
            case VT_INT:
                return BC_ISUB;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getMulInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_DMUL;
            case VT_INT:
                return BC_IMUL;
            default:
                return BC_INVALID;
        }
    }


    inline Instruction getDivInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_DDIV;
            case VT_INT:
                return BC_IDIV;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getModInsn(VarType type) {
        switch (type) {
            case VT_INT:
                return BC_IMOD;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getNegInsn(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_DNEG;
            case VT_INT:
                return BC_INEG;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getOrInsn(VarType type) {
        switch (type) {
            case VT_INT:
                return BC_IAOR;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getAndInsn(VarType type) {
        switch (type) {
            case VT_INT:
                return BC_IAAND;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getXorInsn(VarType type) {
        switch (type) {
            case VT_INT:
                return BC_IAXOR;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getStoreVar0(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_STOREDVAR0;
            case VT_INT:
                return BC_STOREIVAR0;
            case VT_STRING:
                return BC_STORESVAR0;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getStoreVar1(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_STOREDVAR1;
            case VT_INT:
                return BC_STOREIVAR1;
            case VT_STRING:
                return BC_STORESVAR1;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getLoadVar1(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return BC_LOADDVAR1;
            case VT_INT:
                return BC_LOADIVAR1;
            case VT_STRING:
                return BC_LOADSVAR1;
            default:
                return BC_INVALID;
        }
    }

    inline Instruction getCast(VarType from, VarType to) {
        if (from == VT_DOUBLE && to == VT_INT) {
            return BC_D2I;
        }
        else
        if (from == VT_INT && to == VT_DOUBLE) {
            return BC_I2D;
        }
        else
        if (from == VT_STRING && to == VT_INT) {
            return BC_S2I;
        }
        else {
            std::cout << "ERROR: unsupported cast" << std::endl;
            exit(42);
        }
    }

    inline Instruction getPrint(VarType type) {
        switch (type) {
            case VT_INT:
                return BC_IPRINT;
            case VT_DOUBLE:
                return BC_DPRINT;
            case VT_STRING:
                return BC_SPRINT;
            default:
                return BC_INVALID;
        }
    }
}

