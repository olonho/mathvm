#pragma once

#include <mathvm.h>

class BytecodeStream {
public:
  BytecodeStream(mathvm::Bytecode* bytecode);

  mathvm::Instruction readInstruction();

  uint16_t readUInt16();

  double readDouble();

  int64_t readInt64();

  uint32_t currentOffset() const;

  bool hasNext();

  uint32_t length() const;

  int16_t readInt16();

  void jump(int16_t offset);

private:
  mathvm::Bytecode* _bytecode;
  uint32_t _pointer;
};
