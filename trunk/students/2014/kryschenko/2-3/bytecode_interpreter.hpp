#ifndef BYTECODE_INTERPRETER_HPP
#define BYTECODE_INTERPRETER_HPP

#include <vector>

#include "mathvm.h"

namespace mathvm {

    class StackObj {
        union {
            int64_t i_;
            double d_;
            uint16_t ui_;
        } data_;
        VarType type_;

    public:
        StackObj() :
            type_(VT_INVALID) {

        }
        static StackObj create(int64_t i) {
            StackObj var;
            var.data_.i_ = i;
            var.type_ = VT_INT;
            return var;
        }
        static StackObj create(double d) {
            StackObj var;
            var.data_.d_ = d;
            var.type_ = VT_DOUBLE;
            return var;
        }
        static StackObj create(uint16_t ui) {
            StackObj var;
            var.data_.ui_ = ui;
            var.type_ = VT_STRING;
            return var;
        }
        int64_t  getAsInt() {
            return data_.i_;
        }
        double getAsDouble() {
            return data_.d_;
        }
        uint16_t getAsUInt16() {
            return data_.ui_;
        }

    };

    class InterpreterScopeContext {
        BytecodeFunction* bf_;
        std::vector<StackObj> scope_vars;
        uint32_t bci_;
        InterpreterScopeContext* parent_;


    public:
        InterpreterScopeContext(BytecodeFunction* bf, InterpreterScopeContext* parent = NULL) :
                bf_(bf), scope_vars(bf->localsNumber()), bci_(0), parent_(parent) {
        }

        ~InterpreterScopeContext() {
        }


        Instruction getInstruction() {
            return bc()->getInsn(bci_++);
        }

        int64_t getInt() {
            int64_t res = bc()->getInt64(bci_);
            bci_ += sizeof(int64_t);
            return res;
        }

        uint16_t getUInt16() {
            uint16_t res = bc()->getUInt16(bci_);
            bci_ += sizeof(uint16_t);
            return res;
        }

        int16_t getInt16() {
            int16_t res = bc()->getInt16(bci_);
            bci_ += sizeof(int16_t);
            return res;
        }

        double getDouble() {
            double res = bc()->getDouble(bci_);
            bci_ += sizeof(double);
            return res;
        }

        StackObj getVar() {
            uint16_t  var_id = getUInt16();
            if (scope_vars.size() < var_id) {
                throw new std::runtime_error("Uncorrect var");
            }
            return scope_vars[var_id];
        }

        StackObj getVarById(uint16_t var_id) {
            if (scope_vars.size() < var_id) {
                throw new std::runtime_error("Uncorrect var");
            }
            return scope_vars[var_id];
        }

        void storeVar(StackObj var) {
            uint16_t  var_id = getUInt16();
            if (scope_vars.size() < var_id) {
                throw new std::runtime_error("Uncorrect var");
            }
            scope_vars[var_id] = var;
        }

        void storeVarById(StackObj var, uint16_t var_id) {
            if (scope_vars.size() < var_id) {
                throw new std::runtime_error("Uncorrect var");
            }
            scope_vars[var_id] = var;
        }

        StackObj getContextVar() {
            uint16_t context_id = getUInt16();
            uint16_t var_id = getUInt16();
            return getContextVar(context_id, var_id);
        }

        StackObj getContextVar(uint16_t context_id, uint16_t var_id) {
            if (context_id == bf_->scopeId()) {
                if (scope_vars.size() < var_id) {
                    throw new std::runtime_error("Uncorrect var");
                }
                return scope_vars[var_id];
            } else if (parent_ != NULL) {
                return parent_->getContextVar(context_id, var_id);
            } else {
                throw new std::runtime_error("Uncorrect var");
            }
        }

        void storeContextVar(StackObj var) {
            uint16_t context_id = getUInt16();
            uint16_t  var_id = getUInt16();
            storeContextVar(var, context_id, var_id);
        }

        void storeContextVar(StackObj var, uint16_t context_id, uint16_t var_id) {
            if (context_id == bf_->scopeId()) {
                if (scope_vars.size() < var_id) {
                    throw new runtime_error("Uncorrect var");
                }
                scope_vars[var_id] = var;
            } else if (parent_ != NULL) {
                parent_->storeContextVar(var, context_id, var_id);
            } else {
                throw new std::runtime_error("Uncorrect Var");
            }

        }

        void jump() {
            int16_t offset = getInt16();
            bci_ += offset - sizeof(int16_t);
        }

        Bytecode* bc() {
            return bf_->bytecode();
        }

        uint32_t bci() {
            return bci_;
        }

        InterpreterScopeContext* getParent() {
            return parent_;
        }


    };

    class InterpreterCode : public Code {
        InterpreterScopeContext* context_;
        std::vector<StackObj> stack_;

    public:
        InterpreterCode() :
            context_(NULL) {
        }
        ~InterpreterCode() {
            if (context_ != NULL) {
                delete context_;
            }
        }
        /*virtual*/ Status* execute(std::vector<Var*>& vars);
    private:
        bool executeInstruction(Instruction ins);
        std::pair<StackObj, StackObj> getOperandsForBinOp();

        Bytecode* bc() {
            return context_->bc();
        }
    };
}

#endif