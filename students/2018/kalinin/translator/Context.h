//
// Created by Владислав Калинин on 20/11/2018.
//

#ifndef MATHVM_CONTEXT_H
#define MATHVM_CONTEXT_H

#include <cstdint>
#include "../../../../include/mathvm.h"
#include "../../../../include/ast.h"
#include <unordered_map>

namespace mathvm {

    class Context {
        class ChildsIterator;

        unordered_map<string, uint16_t> variablesById{};
        vector<Var *> varList{};
        unordered_map<string, uint16_t> functionsById{};
        static vector<BytecodeFunction *> functionList;
        unordered_map<string, uint16_t> constantsById{};

        static vector<Context *> contextList;
        Context *parent{};
        vector<Context *> childs{};

        uint16_t id{};
        ChildsIterator *iter{};
        static Context *instanse;

    private:
        Context() {
            init(nullptr);
        }

        explicit Context(Context *parentContext) {
            init(parentContext);
        }

        void init(Context *parentContext);

    public:
        static Context *getRoot();

        Context *addChild();

        Context *getLastChildren();

        uint16_t getId();

        Context *getVarContext(string name);

        void addVar(Var *var);

        uint16_t VarNumber();

        uint16_t getVarId(string name);

        BytecodeFunction *getFunction(string name);

        void addFun(AstFunction *func);

        uint16_t makeStringConstant(string literal);

        Context *getParentContext();

        ChildsIterator *childsIterator();

    private:
        class ChildsIterator {
            friend Context;
            vector<Context *> *childs{};
            uint32_t count = 0;

        private:
            explicit ChildsIterator(vector<Context *> *childs) : childs(childs) {};

        public:
            bool hasNext() {
                return count < childs->size() - 1;
            }

            Context *next() {
                Context *res = (*childs)[count];
                count++;
                return res;
            }
        };
    };

}

#endif //MATHVM_CONTEXT_H
