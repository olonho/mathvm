#pragma once

#include <mathvm.h>
#include <ast.h>

enum BinaryOperationType {
  ARITHMETIC,
  LOGICAL,
  BITWISE,
  COMPARISON,
  UNKNOWN
};

mathvm::Instruction getLoadVarInstructionOuterScope(mathvm::VarType type);

mathvm::Instruction getLoadVariableInstructionLocalScope(mathvm::VarType type, uint16_t localId);

mathvm::Instruction getStoreVariableInstructionOuterScope(mathvm::VarType type);

mathvm::Instruction getStoreVariableInstructionLocalScope(mathvm::VarType type, uint16_t localId);

mathvm::Instruction getSumInstruction(mathvm::VarType type);

mathvm::Instruction getSubInstruction(mathvm::VarType type);

mathvm::Instruction getArithmeticBinaryInstruction(mathvm::VarType type, mathvm::TokenKind kind);

mathvm::Instruction getBitwiseBinaryInstruction(mathvm::TokenKind kind);

mathvm::Instruction getComparisonBinaryInstruction(mathvm::TokenKind kind);

BinaryOperationType getBinaryOperationType(mathvm::TokenKind kind);
