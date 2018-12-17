#pragma once

#include <mathvm.h>
#include <ast.h>
#include <unordered_map>
#include "utils.hpp"

namespace mathvm {

    union Val {
        double doubleValue;
        int64_t intValue;
        uint16_t stringId;

        Val() : intValue(0) {};

        explicit Val(double val) : doubleValue(val) {}

        explicit Val(int64_t val) : intValue(val) {}

        explicit Val(uint16_t id) : stringId(id) {}
    };

    union NativeVal {
        double doubleValue;
        int64_t intValue;
        char const* stringValue;

        NativeVal() : intValue(0) {};

        explicit NativeVal(double val) : doubleValue(val) {}

        explicit NativeVal(int64_t val) : intValue(val) {}

        explicit NativeVal(char const* val) : stringValue(val) {}
    };

    struct CallContext {
        explicit CallContext(BytecodeFunction* function)
            : function_(function) {}

        CallContext(CallContext const& other) = delete;
        CallContext(CallContext&& other) = default;

        void declareVar(string const& name) {
            if (variableNameToId_.find(name) != variableNameToId_.end()) {
                throw std::runtime_error(
                    "Variable with name " + name + " is already defined!"
                );
            }

            variableNameToId_.emplace(name, variables_.size());
            variables_.emplace_back();
        }

        uint16_t scopeId() const {
            return function_->scopeId();
        }

        Bytecode* bytecode() {
            return function_->bytecode();
        }

        uint16_t findVarIndex(const string& name) const {
            auto index = variableNameToId_.find(name);
            if (index != variableNameToId_.end()) {
                return index->second;
            }

            throw std::runtime_error("No variable with name " + name + " in this scope!");
        }

        bool containsVar(const string& name) const {
            auto index = variableNameToId_.find(name);
            return index != variableNameToId_.end();
        }

        VarType tosType() const {
            return tosType_;
        }

        VarType returnType() const {
            return function_->returnType();
        }

        void setTosType(VarType param) {
            tosType_ = param;
        }

        uint16_t varsNumber() const {
            return uint16_t(variables_.size());
        }

    private:
        BytecodeFunction* function_;
        vector <Val> variables_;
        unordered_map <string, uint16_t> variableNameToId_;
        VarType tosType_{VT_INVALID};
    };

    struct ExecutionContext {
        explicit ExecutionContext(BytecodeFunction* function)
            : function_(function),
              variables_(function->localsNumber()) {}

        ExecutionContext(ExecutionContext const&) = delete;
        ExecutionContext(ExecutionContext&&) = default;

        Bytecode* bytecode() {
            return function_->bytecode();
        }

        Instruction readInstruction() {
            return bytecode()->getInsn(idx_++);
        }

        uint16_t readStringId() {
            uint16_t id = bytecode()->getUInt16(idx_);
            idx_ += sizeof(id);

            return id;
        }

        int64_t readInt() {
            int64_t result = bytecode()->getInt64(idx_);
            idx_ += sizeof(result);

            return result;
        }

        double readDouble() {
            double result = bytecode()->getDouble(idx_);
            idx_ += sizeof(result);

            return result;
        }

        uint16_t readUInt16() {
            auto index = bytecode()->getUInt16(idx_);
            idx_ += sizeof(index);

            return index;
        }

        int16_t readInt16() {
            auto index = bytecode()->getInt16(idx_);
            idx_ += sizeof(index);

            return index;
        }

        Val& getVar(uint16_t index) {
            return variables_[index];
        }

        template <uint16_t index>
        Val& getVar() {
            return variables_[index];
        }

        uint32_t instructionIndex() const {
            return idx_;
        }

        void moveInstructionIndexByOffset(int16_t position) {
            assert(idx_ + position - 2 < bytecode()->length());
            idx_ += position - 2;
        }

        uint16_t scopeId() const {
            return function_->scopeId();
        }

        uint16_t localsNumber() {
            return uint16_t(variables_.size());
        }

    private:
        BytecodeFunction* function_;
        std::vector<Val> variables_;
        uint32_t idx_{0};
    };

}