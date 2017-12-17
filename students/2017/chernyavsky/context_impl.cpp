#include <ast.h>
#include "context.h"

namespace mathvm {

    Context::Context(BytecodeFunction* func, Context* parent, Scope* scope)
            : _function(func)
            , _parent(parent)
            , _bytecodePos(0)
            , _tosType(VT_VOID)
            , _locals(func->localsNumber())
            , _scopeId(0) {
        if (parent) {
            _scopeId = parent->getId() + 1;
        }

        if (scope) {
            Scope::VarIterator it = Scope::VarIterator(scope);
            while (it.hasNext()) {
                addVar(it.next()->name());
            }
        }
    }

    BytecodeFunction* Context::getFunction() {
        return _function;
    }

    Context* Context::getParent() {
        return _parent;
    }

    Bytecode* Context::getBytecode() {
        return _function->bytecode();
    }

    uint32_t Context::getBytecodePosition() {
        return _bytecodePos;
    }

    VarType Context::getTosType() {
        return _tosType;
    }

    void Context::setTosType(VarType type) {
        _tosType = type;
    }

    uint64_t Context::getId() {
        return _scopeId;
    }

    Val Context::getVar(id_t varId) {
        if (varId >= getLocalsNumber()) {
            throw std::runtime_error("Invalid var id");
        }
        return _locals[varId];
    }

    id_t Context::getVarId(const string& name) {
        return _idByName[name];
    }

    Val* Context::findVar(const string& name) {
        auto it = _idByName.find(name);
        if (it == _idByName.end()) {
            return nullptr;
        }
        return &_locals[it->second];
    }

    void Context::setVar(id_t varId, Val newVal) {
        if (varId >= getLocalsNumber()) {
            throw std::runtime_error("Invalid var id");
        }
        _locals[varId] = newVal;
    }

    void Context::addVar(const string& name) {
        if (_idByName.find(name) != _idByName.end()) {
            throw std::runtime_error("Var already exists");
        }

        if (_idByName.size() > std::numeric_limits<uint16_t>::max()) {
            throw std::runtime_error("Context overflow");
        }

        id_t id = getLocalsNumber();
        _idByName[name] = id;
        if (_locals.size() <= id) {
            _locals.resize(id + 1);
        }
    }

    id_t Context::getLocalsNumber() {
        return static_cast<id_t>(_locals.size());
    }

    double Context::readDouble() {
        double d = _function->bytecode()->getDouble(_bytecodePos);
        _bytecodePos += sizeof(double);
        return d;
    }

    int64_t Context::readInt() {
        int64_t i = _function->bytecode()->getInt64(_bytecodePos);
        _bytecodePos += sizeof(int64_t);
        return i;
    }

    id_t Context::readId() {
        uint16_t s = _function->bytecode()->getUInt16(_bytecodePos);
        _bytecodePos += sizeof(uint16_t);
        return s;
    }

    int16_t Context::readOffset() {
        auto s = _function->bytecode()->getTyped<int16_t>(_bytecodePos);
        _bytecodePos += sizeof(int16_t);
        return s;
    }

    Instruction Context::readInsn() {
        return _function->bytecode()->getInsn(_bytecodePos++);
    }

    void Context::jump(int16_t offset) {
        _bytecodePos += offset - 2;
    }

}
