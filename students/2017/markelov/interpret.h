#ifndef STUDENTS_2017_MARKELOV_INTERPRET_H_
#define STUDENTS_2017_MARKELOV_INTERPRET_H_

#include "parser.h"
#include "mathvm.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stack>
#include <vector>
#include <set>
#include <tuple>
#include <memory>
#include "code.h"

//#define INTERPRET_DEBUG

#ifdef INTERPRET_DEBUG
#define DI(arg) std::cout << arg
#else
#define DI(arg)
#endif


namespace mathvm {


class Tos: public vector<uint8_t> {
    uint8_t get(uint32_t index) const {
        return (*this)[index];
    }

    void add(uint8_t b) {
        push_back(b);
    }
    void pop() {
        pop_back();
    }
    template<class T> T top() const {
        union {
            T val;
            uint8_t bits[sizeof(T)];
        } u;
        uint32_t index = size() - sizeof(T);
        for (uint32_t i = 0; i < sizeof(u.bits); i++) {
            u.bits[i] = get(index + i);
        }
        return u.val;
    }
    template<class T> void popTyped() {
        for (uint32_t i = 0; i < sizeof(T); i++) {
            pop();
        }
    }
    template<class T> void push(T d) {
        union {
            T val;
            uint8_t bits[sizeof(T)];
        } u;

        u.val = d;
        for (uint32_t i = 0; i < sizeof(u.bits); i++) {
            add(u.bits[i]);
        }
    }
public:
    int64_t topInt() {
        return top<int64_t>();
    }
    uint16_t topStringId() {
        return top<uint16_t>();
    }
    double topDouble() {
        return top<double>();
    }
    int64_t popInt() {
        int64_t v = topInt();
        popTyped<int64_t>();
        return v;
    }
    uint16_t popStringId() {
        uint16_t v = topStringId();
        popTyped<uint16_t>();
        return v;
    }
    double popDouble() {
        double v = topDouble();
        popTyped<double>();
        return v;
    }
    void pushInt(int64_t v ) {
        push(v);
    }
    void pushStringId(uint16_t v ) {
        push(v);
    }
    void pushDouble(double v ) {
        push(v);
    }
};

class PassedCode;
class ByteCodeIterator {
    Bytecode * code;
public:
    uint32_t index;
    ByteCodeIterator(Bytecode * code) :
            code(code), index(0) {
    }
    bool hasNext() {
        return index < code->length();
    }
    bool NextInstr(PassedCode * callee);
};


class PassedCode: public ScopedCode {
    class StopException : public std::exception {
    };
    class MyVar : public Var {
        uint16_t sid;
    public:
        using Var::Var;
        uint16_t * getSid() {
            return &sid;
        }
    };
    struct RuntimeScope {
        std::map<uint16_t, MyVar> var;
        RuntimeScope * prev;
        CodeScope * myscope;
        Var* getVar(VarType t, uint16_t id) {
            if (!var.count(id)) {
                var.insert(std::make_pair(id, MyVar(t, "")));
            }
            return (Var*)(&var.at(id));
        }
        RuntimeScope (RuntimeScope * prev, CodeScope * myscope) :
            prev(prev), myscope(myscope) {
        }
    };
    Tos tos;
    std::map<uint16_t, RuntimeScope*> var_scope;

    std::vector<Var*> userTopScopeVars;

#define SPEC_VARS_N 5

    std::shared_ptr<Var> spec_vars[SPEC_VARS_N];

    Var * getVarOuter(uint16_t sid, uint16_t vid, VarType t) {
        return var_scope[sid]->getVar(t, vid);
    }
    Var * getVar(uint16_t vid, VarType t) {
        return getVarOuter(0, vid, t);
    }

    void inhiriteRunScope(RuntimeScope* scope) {
        uint16_t sc = scope->myscope->scopeid;
        var_scope[sc] = new RuntimeScope(scope, scope->myscope);
    }

    void makeRunScope(CodeScope * scope) {
        uint16_t sc = scope->scopeid;
        var_scope[sc] = new RuntimeScope(nullptr, scope);
    }

    void pushRunScope(uint16_t id) {
        if (var_scope.count(id)) {
            inhiriteRunScope(var_scope[id]);
        } else {
            makeRunScope(getScope(id));
        }
    }

    void revertScope(uint16_t sc) {

        RuntimeScope * deleted = var_scope[sc];

        var_scope[sc] = var_scope[sc]->prev;
        if (var_scope[sc] == nullptr) {
            var_scope.erase(var_scope.find(sc));
        }

        delete deleted;
    }


    void setVarFromAnother(Var *var, Var * got) {
        switch (var->type()) {
                        case VT_INT:
                            var->setIntValue(got->getIntValue());
                            break;
                        case VT_DOUBLE:
                            var->setDoubleValue(got->getDoubleValue());
                            break;
                        case VT_STRING:
                            var->setStringValue(got->getStringValue());
                            break;
                        default:
                            break;
                        }
    }

    void checkPopedBlockTop(uint16_t sc) {
        if (var_scope[sc]->myscope->isGlobal) {
            for (auto var : userTopScopeVars) {
                auto ids = var_scope[sc + 1]->myscope->outerVarId(var->name());
                Var * got = var_scope[sc + 1]->getVar(var->type(), std::get<1>(ids));
                setVarFromAnother(var, got);
            }
        }
    }
    void checkPushedBlockTop(uint16_t sc) {
        if (var_scope[sc]->myscope->isGlobal) {
            for (auto var : userTopScopeVars) {
                auto ids = var_scope[sc + 1]->myscope->outerVarId(var->name());
                Var * got = var_scope[sc + 1]->getVar(var->type(), std::get<1>(ids));
                setVarFromAnother(got, var);
            }
        }
    }

    void push_level(uint16_t sc) {
            for (auto dep : getEdges(sc)) {
                pushRunScope(dep);
            }
            pushRunScope(sc);
            checkPushedBlockTop(sc);
        }


    void pop_level(uint16_t sc) {
        checkPopedBlockTop(sc);
        for (auto dep : getEdges(sc)) {
            revertScope(dep);
        }
        revertScope(sc);
    }

public:
    PassedCode() {
    }
    virtual ~PassedCode() {
        for (auto it : var_scope) {
            RuntimeScope * scope = it.second;
            delete scope;
        }
    }
    void sprint(void)  {
        uint16_t id = tos.popStringId();
        const string& str = constantById(id);
        cout << str;
    }

    void iprint(void)  {
        const int64_t lit = tos.popInt();
        cout << lit;
    }
    void dprint(void)  {
        double lit = tos.popDouble();
        cout << lit;
    }
    void sload(uint16_t id)  {
        tos.pushStringId(id);
    }

    void i2d() {
        int64_t v = tos.popInt();
        tos.pushDouble((double) v);
    }
    void d2i() {
        tos.pushInt((int64_t)tos.popDouble());
    }
    void s2i() {
        tos.pushInt(tos.popStringId());
    }
    void swap() {
        uint16_t a = tos.popStringId();
        uint16_t b = tos.popStringId();
        tos.pushStringId(a);
        tos.pushStringId(b);
    }
    void dump() {
    }
    void iload(int64_t lit)  {
        tos.pushInt(lit);
    }
    void dload(double lit)  {
        tos.pushDouble(lit);
    }
    void imul(void)  {
        tos.pushInt(tos.popInt() * tos.popInt());
    }
    void dmul(void)  {
        tos.pushDouble(tos.popDouble() * tos.popDouble());
    }
    void idiv(void)  {
        tos.pushInt(tos.popInt() / tos.popInt());
    }
    void imod(void)  {
        tos.pushInt(tos.popInt() % tos.popInt());
    }
    void ddiv(void)  {
        tos.pushDouble(tos.popDouble() / tos.popDouble());
    }
    void iadd(void)  {
        tos.pushInt(tos.popInt() + tos.popInt());
    }
    void dadd(void)  {
        tos.pushDouble(tos.popDouble() + tos.popDouble());
    }
    void isub(void)  {
        tos.pushInt(tos.popInt() - tos.popInt());
    }
    void dsub(void)  {
        tos.pushDouble(tos.popDouble() - tos.popDouble());
    }
    void ior(void)  {
        tos.pushInt(tos.popInt() | tos.popInt());
    }
    void ixor(void)  {
        tos.pushInt(tos.popInt() ^ tos.popInt());
    }
    void iand(void)  {
        tos.pushInt(tos.popInt() & tos.popInt());
    }
    void ineg(void)  {
        tos.pushInt(-tos.popInt());
    }
    void dneg(void)  {
        tos.pushInt(-tos.popInt());
    }

    void loadivar(uint16_t id) {
        Var * var = getVar(id, VT_INT);
        tos.pushInt(var->getIntValue());
    }
    void loaddvar(uint16_t id) {
        Var * var = getVar(id, VT_DOUBLE);
        tos.pushDouble(var->getDoubleValue());
    }
    void storeivar(uint16_t id) {
        Var * var = getVar(id, VT_INT);
        var->setIntValue(tos.popInt());
    }
    void storedvar(uint16_t id) {
        Var * var = getVar(id, VT_DOUBLE);
        var->setDoubleValue(tos.popDouble());
    }

    void storesvar(uint16_t id) {
        MyVar * var = (MyVar*)getVar(id, VT_STRING);
        uint16_t sid = tos.popStringId();
        (*(var->getSid())) = sid;
        var->setStringValue(constantById(sid).c_str());
    }

    void loadsvar(uint16_t vid) {
        MyVar * var = (MyVar*)getVar(vid, VT_STRING);
        tos.pushStringId(*(var->getSid()));
    }

    void loadctxivar(uint16_t cid, uint16_t vid) {
        Var * var = getVarOuter(cid, vid, VT_INT);
        tos.pushInt(var->getIntValue());
    }
    void loadctxdvar(uint16_t cid, uint16_t vid) {
        Var * var = getVarOuter(cid, vid, VT_DOUBLE);
        tos.pushDouble(var->getDoubleValue());
    }
    void loadctxsvar(uint16_t cid, uint16_t vid) {
        MyVar * var = (MyVar*)getVarOuter(cid, vid, VT_STRING);
        tos.pushStringId(*(var->getSid()));
    }
    void storectxivar(uint16_t cid, uint16_t vid) {
        Var * var = getVarOuter(cid, vid, VT_INT);
        var->setIntValue(tos.popInt());
    }
    void storectxdvar(uint16_t cid, uint16_t vid) {
        Var * var = getVarOuter(cid, vid, VT_DOUBLE);
        var->setDoubleValue(tos.popDouble());
    }
    void storectxsvar(uint16_t cid, uint16_t vid) {
        MyVar * var = (MyVar*)getVarOuter(cid, vid, VT_STRING);
        uint16_t id = tos.popStringId();
        (*(var->getSid())) = id;
        var->setStringValue(constantById(id).c_str());
    }

    void loadspecvar(uint16_t id, VarType t) {
        Var * var = spec_vars[ id].get();
        if (t == VT_INT)
            tos.pushInt(var->getIntValue());
        else
            tos.pushDouble(var->getDoubleValue());
    }

    void storespecvar(uint16_t id, VarType t) {
        spec_vars[id] = std::make_shared<Var>(t, "");
        Var * var = spec_vars [ id].get();
        if (t == VT_INT)
            var->setIntValue(tos.popInt());
        else
            var->setDoubleValue(tos.popDouble());
    }

    template<class T> int64_t cmp(T l, T r)  {
        return l == r ? 0 : (l < r) ? -1 : 1;
    }

    int64_t cmpInt() {
        int64_t l = tos.popInt();
        int64_t r = tos.popInt();
        int64_t res = cmp(l, r);
        return res;
    }

    void icmp() {
        tos.pushInt(cmpInt());
    }

    void dcmp() {
        int64_t l = tos.popDouble();
        int64_t r = tos.popDouble();
        tos.pushDouble(cmp(l, r));
    }


    int16_t ificmp(int16_t off, int val)  {
        int64_t r = cmpInt();
        if (r == val) {
            return off;
        }
        return 0;
    }

    int16_t ificmpnot(int16_t off, int val)  {
        int64_t r = cmpInt();
        if (r != val) {
            return off;
        }
        return 0;
    }

    int16_t ja(int16_t off)  {
        return off;
    }

    void iloadconst(int64_t c) {
        tos.pushInt(c);
    }

    void dloadconst(double c) {
        tos.pushDouble(c);
    }

    void pop()  {
        tos.popStringId();
    }

    void call(uint16_t id)  {
        TranslatedFunction * f = functionById(id);
        assert (f != nullptr);
        bfunction(f);
    }

    void callnative(uint16_t id) ;

    void sloadempty() {
        tos.pushStringId(makeStringConstant(""));
    }

    void stop() {
        throw StopException();
    }

    //=====================================
    void bfunction(TranslatedFunction * f) {
        push_level(f->scopeId());
        BytecodeFunction * bfunc = dynamic_cast<BytecodeFunction *>(f);
        ByteCodeIterator it(bfunc->bytecode());

#ifdef INTERPRET_DEBUG
        bfunc->bytecode()->dump(std::cout);
        std::cout << "==================" << endl;
#endif
        while (it.hasNext()) {
            bool returned = false;

            try {
                returned = it.NextInstr(this);
            } catch (StopException &e) {
                pop_level(f->scopeId());
                throw e;
            }

            if (returned)
                break;

        }
        pop_level(f->scopeId());
    }
    Status * execute(vector<Var*>& vars) override {
        userTopScopeVars = vars;
        TranslatedFunction * f = functionByName(AstFunction::top_name);
        assert (f != nullptr);
        bfunction(f);
        return Status::Ok();
    }
};

}


#endif
