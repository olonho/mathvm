#include "interpret.h"


template<class Ret, class ... Args>
struct execSymbol {
    static Ret exec(void * addr, Args ... args)
    {
        Ret (*addr2)(Args...) = (Ret (*)(Args...))addr;
        Ret ret = addr2(args...);
        return ret;
    }
};



template<unsigned SZ, class Ret>
struct ParamsReducer {
    template <class ... Args>
    static Ret reduce(Code * code, void * addr, std::vector<Var*> v,  Args...  rest)
    {
        if (v.empty()) {
            return execSymbol<Ret, Args...>::exec(addr, rest...);
        }
        Var * last =  v.back();
        v.pop_back();
        if (last->type() == VT_INT) {
            return ParamsReducer<SZ-1, Ret>::reduce(code, addr, v, last->getIntValue(), rest...);
        }
        if (last->type() == VT_DOUBLE) {
            return ParamsReducer<SZ-1, Ret>::reduce(code, addr, v, last->getDoubleValue(), rest...);
        }
        return ParamsReducer<SZ-1, Ret>::reduce(code, addr, v, last->getStringValue(), rest...);
    }
};
template<class Ret>
struct ParamsReducer<0, Ret> {
    template <class ... Args>
    static Ret reduce(Code * code, void * addr, std::vector<Var*> v,  Args...  rest)
    {
        return execSymbol<Ret, Args...>::exec(addr, rest...);
    }
};

#define MAX_ARGS 5


void PassedCode::callnative(uint16_t id) {
    const Signature* sign;
    const string* nm;
    void * addr = (void*)nativeById(id, &sign, &nm);
    vector<Var*> params;
    for (size_t i = 1; i < sign->size(); i++) {
        Var * v = new Var((*sign)[i].first , (*sign)[i].second);
        if ((*sign)[i].first == VT_INT) {
            v->setIntValue(tos.popInt());
        }
        else if ((*sign)[i].first == VT_DOUBLE) {
            v->setDoubleValue(tos.popDouble());
        } else {
            uint16_t str_id = tos.popStringId();
            v->setStringValue(constantById(str_id).c_str());
        }
        params.push_back(v);
    }
    if ((*sign)[0].first == VT_INT) {
        tos.pushInt(ParamsReducer<MAX_ARGS, int64_t>::reduce(this, addr, params));
    } else if ((*sign)[0].first == VT_DOUBLE) {
        tos.pushDouble(ParamsReducer<MAX_ARGS, double>::reduce(this, addr, params));
    } else {
        const char * res = ParamsReducer<MAX_ARGS, const char *>::reduce(this, addr, params);
        tos.pushStringId(makeStringConstant(string(res)));
    }
    for (auto it : params) {
        Var * v = it;
        delete v;
    }
}

//=================================== */

#define TAKE_INSTR(code, index)\
    ({\
        Instruction i = code->getInsn(index);\
        index += sizeof(uint8_t);\
        (i);\
    })

#define TAKE_VAR(code, index, type, var)\
    do {\
        var = code->get##type(index);\
        index += sizeof(var);\
    } while (0);

#define TAKE_VAR2(code, index, type, var, type2, var2)\
    do {\
        TAKE_VAR(code, index, type, var);\
        TAKE_VAR(code, index, type2, var2);\
    } while (0);


bool ByteCodeIterator::NextInstr(PassedCode * callee) {
        Instruction i = TAKE_INSTR(code, index);

        switch (i) {
        case BC_RETURN:
            return true;
            break;
        case BC_STOP:
            callee->stop();
            return true;
            break;
        case BC_DUMP:
            callee->dump();
            break;

        case BC_BREAK:
            break;

        case BC_POP:
             callee->pop();
             break;
        case BC_SWAP:
             callee->swap();
             break;
        case BC_I2D:
            callee->i2d();
            break;
        case BC_D2I:
            callee->d2i();
            break;
        case BC_S2I:
            callee->s2i();
            break;


        case BC_SLOAD: {
            uint16_t id;
            TAKE_VAR(code, index, UInt16, id);
            callee->sload(id);
        }
            break;
        case BC_ILOAD: {
            int64_t lit;
            TAKE_VAR(code, index, Int64, lit);
            callee->iload(lit);
        }
            break;
        case BC_DLOAD: {
            double lit;
            TAKE_VAR(code, index, Double, lit);
            callee->dload(lit);
        }
            break;


        case BC_SPRINT:
            callee->sprint();
            break;
        case BC_IPRINT:
            callee->iprint();
            break;
        case BC_DPRINT:
            callee->dprint();
            break;

        case BC_ILOADM1:
            callee->iloadconst(-1);
            break;
        case BC_DLOADM1:
            callee->dloadconst(-1);
            break;

        case BC_ILOAD0:
            callee->iloadconst(0);
            break;
        case BC_DLOAD0:
            callee->dloadconst(0);
            break;
        case BC_SLOAD0:
            callee->sloadempty();
            break;

        case BC_ILOAD1:
            callee->iloadconst(1);
            break;
        case BC_DLOAD1:
            callee->dloadconst(1);
            break;


        case BC_IMUL:
            callee->imul();
            break;
        case BC_DMUL:
            callee->dmul();
            break;
        case BC_IADD:
            callee->iadd();
            break;
        case BC_DADD:
            callee->dadd();
            break;
        case BC_ISUB:
            callee->isub();
            break;
        case BC_DSUB:
            callee->dsub();
            break;
        case BC_IDIV:
            callee->idiv();
            break;
        case BC_DDIV:
            callee->ddiv();
            break;
        case BC_IAAND:
            callee->iand();
            break;
        case BC_IAOR:
            callee->ior();
            break;
        case BC_IAXOR:
            callee->ixor();
            break;
        case BC_INEG:
            callee->ineg();
            break;
        case BC_DNEG:
            callee->dneg();
            break;
        case BC_ICMP:
            callee->icmp();
            break;
        case BC_DCMP:
            callee->dcmp();
            break;
        case BC_IMOD:
            callee->imod();
            break;

#define VARFF(F) {\
           uint16_t id = code->getInt16(index); \
           index += sizeof(id); \
           callee->F(id);\
}\

        case BC_LOADIVAR:
            VARFF(loadivar)
            ;
            break;
        case BC_LOADDVAR:
            VARFF(loaddvar)
            ;
            break;
        case BC_LOADSVAR:
            VARFF(loadsvar)
            ;
            break;
        case BC_STOREIVAR:
            VARFF(storeivar)
            ;
            break;
        case BC_STOREDVAR:
            VARFF(storedvar)
            ;
            break;
        case BC_STORESVAR:
            VARFF(storesvar)
            ;
            break;

#undef VARFF

#define VARFF(F) {\
           uint16_t cid = code->getUInt16(index); \
           index += sizeof (cid);\
           uint16_t vid = code->getUInt16(index); \
           index += sizeof(vid); \
           callee->F(cid, vid);\
}\

        case BC_LOADCTXIVAR:
            VARFF(loadctxivar)
            ;
            break;
        case BC_LOADCTXDVAR:
            VARFF(loadctxdvar)
            ;
            break;
        case BC_LOADCTXSVAR:
            VARFF(loadctxsvar)
            ;
            break;
        case BC_STORECTXIVAR:
            VARFF(storectxivar)
            ;
            break;
        case BC_STORECTXDVAR:
            VARFF(storectxdvar)
            ;
            break;
        case BC_STORECTXSVAR:
            VARFF(storectxsvar)
            ;
            break;

#undef VARFF

#define SP(level)\
        case BC_LOADIVAR##level:\
            callee->loadspecvar(level, VT_INT);\
            break;\
        case BC_STOREIVAR##level:\
            callee->storespecvar(level, VT_INT);\
            break;\
        case BC_LOADDVAR##level:\
            callee->loadspecvar(level, VT_DOUBLE);\
            break;\
        case BC_STOREDVAR##level:\
            callee->storespecvar(level, VT_DOUBLE);\
            break;\
        case BC_LOADSVAR##level:\
            callee->loadspecvar(level, VT_STRING);\
            break;\
        case BC_STORESVAR##level:\
            callee->storespecvar(level, VT_STRING);\
            break;\

            SP(0);
            SP(1);
            SP(2);
            SP(3);

#undef SP

#define VARFF(F, VAL) {\
           int16_t cid = code->getInt16(index); \
           index += sizeof(cid) ; \
           int16_t off = callee->F(cid, VAL);\
           if (off) \
               index += off - sizeof(cid); \
}\

        case BC_IFICMPNE:
            VARFF(ificmpnot, 0);
            break;
        case BC_IFICMPE:
            VARFF(ificmp, 0);
            break;

        case BC_IFICMPG:
            VARFF(ificmp, 1);
            break;
        case BC_IFICMPGE:
            VARFF(ificmpnot, -1);
            break;

        case BC_IFICMPL:
            VARFF(ificmp, -1);
            break;
        case BC_IFICMPLE:
            VARFF(ificmpnot, 1);
            break;

#undef VARFF


        case BC_JA: {
            int16_t cid = code->getInt16(index);
            index += sizeof(cid) ;
            int16_t off = callee->ja(cid);
            if (off)
               index += off - sizeof(cid);
        }
            break;
        case BC_CALL:
        {
            uint16_t cid = code->getUInt16(index);
            index += sizeof(cid) ;
            callee->call(cid);
        }
        break;
        case BC_CALLNATIVE:
        {
            uint16_t cid = code->getUInt16(index);
            index += sizeof(cid) ;
            callee->callnative(cid);
        }
        break;

        default:
            break;
        }
        return false;
}


ScopedCode::~ScopedCode(){
        for (auto it : scopeMap) {
            delete it.second;
        }
}
