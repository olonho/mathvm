#ifndef PROJECT_INTERPRETER_H
#define PROJECT_INTERPRETER_H

#include "interpreter_code_impl.h"
#include "translate_to_bytecode.h"
#include <vector>
#include <ostream>

namespace mathvm
{
    /**
     * Stack machine!
     */
    class BytecodeInterpreter {
    public:
        BytecodeInterpreter(InterpreterCodeImpl* code, std::ostream& out): _code(code), _out(out) {
            auto fun_it = Code::FunctionIterator(code);
            uint32_t max_fun_ctx = 0;
            while (fun_it.hasNext()) {
                auto f = fun_it.next();
                if (f->scopeId() > max_fun_ctx) {
                    max_fun_ctx = f->scopeId();
                }
            }
            _contexts.resize(max_fun_ctx + 1);
            auto fun = dynamic_cast<BytecodeFunction*>(_code->functionById(0));
            push_fun(fun);
        }
        Status* run();

        template<typename T>
        T get_topmost_var_by_id(int32_t id) {
            return static_cast<T>(_contexts[0].back()[id]);
        }

        template<typename T>
        void set_topmost_var_by_id(int32_t id, T val) {
            _contexts[0].back()[id] = StackVal(val);
        }
    private:
        union StackVal {
        public:
            StackVal(int64_t iv): iv(iv) {}
            StackVal(double dv): dv(dv) {}
            StackVal(const char* str_ref): str_ref(str_ref) {}
            StackVal(): iv(0) {}

            ~StackVal() {
                str_ref = nullptr;
            }

            double getDouble() const {
                return dv;
            }

            int64_t getInt() const {
                return iv;
            }

            const char* getStrRef() const {
                return str_ref;
            }

            explicit operator int64_t() const { return getInt(); }
            explicit operator double() const { return getDouble(); }
            explicit operator const char*() const { return getStrRef(); }
        private:
            double dv;
            int64_t iv;
            const char* str_ref;
        };

        InterpreterCodeImpl *_code;
        std::ostream& _out;
        /**
         * Due to possible recursion we have to use vector of vectors of vector of variables -_-
         * index in root vector == context id. Number of contexts equal to function nesting depth.
         * So for function it's context is known after translation and equal to distance to root scope.
         * (`function a { function b { function c {} } }` --> ctx(<top>) = 0, ctx(a) = 1; ctx(b) = 2, ...)
         * So local variables of function with context = ctx, in case it was called, pushed on the top
         * of the stack `_contexts[ctx]`.
         */
        std::vector<std::vector<std::vector<StackVal>>> _contexts;
        std::vector<BytecodeFunction*> _bytecode_funs;
        std::vector<StackVal> _stack;
        std::vector<uint32_t> _call_stack;

        void interpret();

        Bytecode& bc();

        void push_fun(BytecodeFunction* bf) {
            _bytecode_funs.push_back(bf);
            _contexts[bf->scopeId()].push_back(std::vector<StackVal>(bf->localsNumber()));
        }

        void pop_fun() {
            assert(_contexts.size() > cur_fun_ctx());
            assert(!_contexts[cur_fun_ctx()].empty());

            _contexts[cur_fun_ctx()].pop_back();
            _bytecode_funs.pop_back();
        }

        uint16_t cur_fun_ctx() {
            assert(!_bytecode_funs.empty());

            return _bytecode_funs.back()->scopeId();
        }

        void push_local(uint32_t i) {
            assert(!_bytecode_funs.empty());
            assert(cur_fun_ctx() < _contexts.size());
            assert(i < _contexts[cur_fun_ctx()].back().size());

            _stack.push_back(_contexts[cur_fun_ctx()].back()[i]);
        }

        void push_from_context(uint32_t ctx, uint32_t id) {
            assert(ctx < _contexts.size() && !_contexts[ctx].empty());
            assert(id < _contexts[ctx].back().size());

            _stack.push_back(_contexts[ctx].back()[id]);
        }

        template<typename T>
        void push_value(T t) {
            _stack.push_back(StackVal(t));
        }

        template<typename T>
        T pop_value() {
            assert(!_stack.empty());

            T t = static_cast<T>(_stack.back());
            _stack.pop_back();
            return t;
        }

        void store_local(uint32_t i) {
            auto sv = _stack.back();
            _stack.pop_back();
            _contexts[cur_fun_ctx()].back()[i] = sv;
        }

        void store_to_context(uint32_t ctx, uint32_t id) {
            assert(!_stack.empty());
            assert(ctx < _contexts.size());
            assert(!_contexts[ctx].empty());
            assert(_contexts[ctx].back().size() > id);

            auto sv = _stack.back();
            _stack.pop_back();
            _contexts[ctx].back()[id] = sv;
        }
    };

    /**
     * Translator to be used for running virtual stack machine
     */
    class BytecodeInterpreterTranslator: public Translator {
    public:
        BytecodeInterpreterTranslator(std::ostream& out): _out(out) {}
        Status *translate(const std::string &program, Code **code) override {
            auto bytecode_gen = BytecodeGenTranslator();
            auto status = bytecode_gen.translate(program, code);
            if (status->isError()) {
                return status;
            }
            auto vm = BytecodeInterpreter(dynamic_cast<InterpreterCodeImpl*>(*code), _out);
            return vm.run();
        }
    private:
        ostream& _out;
    };
}

#endif //PROJECT_INTERPRETER_H
