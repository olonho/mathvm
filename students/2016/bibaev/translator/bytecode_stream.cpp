#include "bytecode_stream.h"

using namespace mathvm;

BytecodeStream::BytecodeStream(Bytecode* bytecode)
    : _bytecode(bytecode), _pointer(0) {
}

Instruction BytecodeStream::readInstruction() {
  return _bytecode->getInsn(_pointer++);
}

uint16_t BytecodeStream::readUInt16() {
  uint16_t result = _bytecode->getUInt16(_pointer);
  _pointer += sizeof(uint16_t);
  return result;
}

double BytecodeStream::readDouble() {
  double result = _bytecode->getDouble(_pointer);
  _pointer += sizeof(double);
  return result;
}

int64_t BytecodeStream::readInt64() {
  int64_t result = _bytecode->getInt64(_pointer);
  _pointer += sizeof(int64_t);
  return result;
}

uint32_t BytecodeStream::currentOffset() const {
  return _pointer;
}

bool BytecodeStream::hasNext() {
  return currentOffset() < length();
}

uint32_t BytecodeStream::length() const {
  return _bytecode->length();
}

int16_t BytecodeStream::readInt16() {
  int16_t result = _bytecode->getInt16(_pointer);
  _pointer += sizeof(int16_t);
  return result;
}

void BytecodeStream::jump(int16_t offset) {
  _pointer += offset;
}
