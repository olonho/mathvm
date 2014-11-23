#include "SimpleInterpreter.hpp"

namespace mathvm {
    template<typename R>
    R nativeCaller(void *func) {
        return ((R (*)()) func)();
    }

    template<typename R, typename... T>
    R nativeCaller(void *func, T... a) {
        return ((R (*)(T...)) func)(a...);
    }

    template<uint16_t index, int size, typename R, typename T>
    struct Collector {
        static R run(SimpleInterpreter *inter, void* f, T a) {
            auto p = inter->loadVariable(index);
            switch (p._type) {
                case VT_DOUBLE:
                    return Collector<index + 1, size, R, decltype(tuple_cat(a, make_tuple(p.getDoubleValue())))>::
                            run(inter, f, tuple_cat(a, make_tuple(p.getDoubleValue())));
                case VT_INT:
                    return Collector<index + 1, size, R, decltype(tuple_cat(a, make_tuple(p.getIntValue())))>::
                            run(inter, f, tuple_cat(a, make_tuple(p.getIntValue())));
                case VT_STRING:
                    return Collector<index + 1, size, R, decltype(tuple_cat(a, make_tuple(p.getStringValue())))>::
                            run(inter, f, tuple_cat(a, make_tuple(p.getStringValue())));
                default:
                    throw InterpretationError("Unknown type of " + to_string(index) + " function parameter");
            }
        }
    };

    template<typename R, typename T>
    struct Collector<1, 1, R, T> {
        static R run(SimpleInterpreter *inter, void* f, T a) {
            return nativeCaller<R>(f, get<0>(a));
        }
    };

    template<typename R, typename T>
    struct Collector<2, 2, R, T> {
        static R run(SimpleInterpreter *inter, void* f, T a) {
            return nativeCaller<R>(f, get<0>(a), get<1>(a));
        }
    };

    template<typename R, typename T>
    struct Collector<3, 3, R, T> {
        static R run(SimpleInterpreter *inter, void* f, T a) {
            return nativeCaller<R>(f, get<0>(a), get<1>(a), get<2>(a));
        }
    };

    template<typename R, typename T>
    struct Collector<4, 4, R, T> {
        static R run(SimpleInterpreter *inter, void* f, T a) {
            return nativeCaller<R>(f, get<0>(a), get<1>(a), get<2>(a), get<3>(a));
        }
    };

    template<typename R>
    R runner(SimpleInterpreter *p, void *func, size_t paramsCount) {
        switch (paramsCount) {
            case 0:
                return nativeCaller<R>(func);
            case 1:
                return Collector<0, 1, R, decltype(make_tuple())>::run(p, func, make_tuple());
            case 2:
                return Collector<0, 2, R, decltype(make_tuple())>::run(p, func, make_tuple());
            case 3:
                return Collector<0, 3, R, decltype(make_tuple())>::run(p, func, make_tuple());
            case 4:
                return Collector<0, 4, R, decltype(make_tuple())>::run(p, func, make_tuple());
            default:
                throw InterpretationError("Non-jit native caller can't process " + to_string(paramsCount) + " function parameters. Call AmJit insted");
        }
    }

    void SimpleInterpreter::callNativeFunctionViaTemplateMagic(void* f, size_t params, VarType returnType) {
        switch (returnType) {
            case VT_VOID:
                runner<void>(this, f, params);
                break;
            case VT_DOUBLE:
                pushVariable(runner<double>(this, f, params));
                break;
            case VT_INT:
                pushVariable(runner<signedIntType>(this, f, params));
                break;
            case VT_STRING:
                pushVariable(runner<char const*>(this, f, params));
                break;
            default:
                throw InterpretationError("Wrong native function return type");
        }
    }
}