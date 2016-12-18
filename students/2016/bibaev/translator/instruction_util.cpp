

#include "instruction_util.h"

using namespace mathvm;

Instruction getLoadVarInstructionOuterScope(VarType type) {
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

Instruction getLoadVariableInstructionLocalScope(VarType type, uint16_t localId) {
  switch (type) {
    case VT_DOUBLE:
      switch (localId) {
        case 0:
          return BC_LOADDVAR0;
        case 1:
          return BC_LOADDVAR1;
        case 2:
          return BC_LOADDVAR2;
        case 3:
          return BC_LOADDVAR3;
        default:
          return BC_LOADDVAR;
      }
    case VT_INT:
      switch (localId) {
        case 0:
          return BC_LOADIVAR0;
        case 1:
          return BC_LOADIVAR1;
        case 2:
          return BC_LOADIVAR2;
        case 3:
          return BC_LOADIVAR3;
        default:
          return BC_LOADIVAR;
      }
    case VT_STRING:
      switch (localId) {
        case 0:
          return BC_LOADSVAR0;
        case 1:
          return BC_LOADSVAR1;
        case 2:
          return BC_LOADSVAR2;
        case 3:
          return BC_LOADSVAR3;
        default:
          return BC_LOADSVAR;
      }
    default:
      return BC_INVALID;
  }
}

Instruction getStoreVariableInstructionOuterScope(VarType type) {
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

Instruction getStoreVariableInstructionLocalScope(VarType type, uint16_t localId) {
  switch (type) {
    case VT_DOUBLE:
      switch (localId) {
        case 0:
          return BC_STOREDVAR0;
        case 1:
          return BC_STOREDVAR1;
        case 2:
          return BC_STOREDVAR2;
        case 3:
          return BC_STOREDVAR3;
        default:
          return BC_STOREDVAR;
      }
    case VT_INT:
      switch (localId) {
        case 0:
          return BC_STOREIVAR0;
        case 1:
          return BC_STOREIVAR1;
        case 2:
          return BC_STOREIVAR2;
        case 3:
          return BC_STOREIVAR3;
        default:
          return BC_STOREIVAR;
      }
    case VT_STRING:
      switch (localId) {
        case 0:
          return BC_STORESVAR0;
        case 1:
          return BC_STORESVAR1;
        case 2:
          return BC_STORESVAR2;
        case 3:
          return BC_STORESVAR3;
        default:
          return BC_STORESVAR;
      }
    default:
      return BC_INVALID;
  }
}

Instruction getSumInstruction(VarType type) {
  switch (type) {
    case VT_DOUBLE:
      return BC_DADD;
    case VT_INT:
      return BC_IADD;
    default:
      return BC_INVALID;
  }
}

Instruction getSubInstruction(VarType type) {
  switch (type) {
    case VT_DOUBLE:
      return BC_DSUB;
    case VT_INT:
      return BC_ISUB;
    default:
      return BC_INVALID;
  }
}

Instruction getArithmeticBinaryInstruction(VarType type, TokenKind kind) {
  if (type != VT_INT && type != VT_DOUBLE) {
    return BC_INVALID;
  }

  switch (kind) {
    case tADD:
      return type == VT_INT ? BC_IADD : BC_DADD;
    case tSUB:
      return type == VT_INT ? BC_ISUB : BC_DSUB;
    case tMUL:
      return type == VT_INT ? BC_IMUL : BC_DMUL;
    case tDIV:
      return type == VT_INT ? BC_IDIV : BC_DDIV;
    case tMOD:
      return type == VT_INT ? BC_IMOD : BC_INVALID;
    default:
      return BC_INVALID;
  }
}

Instruction getBitwiseBinaryInstruction(TokenKind kind) {
  switch (kind) {
    case tAOR:
      return BC_IAOR;
    case tAAND:
      return BC_IAAND;
    case tAXOR:
      return BC_IAXOR;
    default:
      return BC_INVALID;
  }
}

Instruction getComparisonBinaryInstruction(TokenKind kind) {
  switch (kind) {
    case tEQ:
      return BC_IFICMPE;
    case tNEQ:
      return BC_IFICMPNE;
    case tGT:
      return BC_IFICMPG;
    case tGE:
      return BC_IFICMPGE;
    case tLT:
      return BC_IFICMPL;
    case tLE:
      return BC_IFICMPLE;
    default:
      return BC_INVALID;
  }
}

BinaryOperationType getBinaryOperationType(TokenKind kind) {
  switch (kind) {
    case tADD:
    case tSUB:
    case tMUL:
    case tDIV:
    case tMOD:
      return ARITHMETIC;
    case tOR:
    case tAND:
      return LOGICAL;
    case tAOR:
    case tAAND:
    case tAXOR:
      return BITWISE;
    case tEQ:
    case tNEQ:
    case tGT:
    case tGE:
    case tLT:
    case tLE:
      return COMPARISON;
    default:
      return UNKNOWN;
  }
}


