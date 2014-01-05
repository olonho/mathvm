#include "mathvm.h"
#include "ast.h"

#include <stdio.h>

#include <iostream>

using namespace std;

namespace mathvm {

    Var::Var(VarType type, const string& name) :
    _type(type) {
        _name = string(name);
        switch (type) {
            case VT_DOUBLE:
                setDoubleValue(0.0);
                break;
            case VT_INT:
                setIntValue(0);
                break;
            case VT_STRING:
                setStringValue(0);
                break;
            default:
                assert(false);
        }
    }

    void Var::print() {
        switch (_type) {
            case VT_DOUBLE:
                cout << getDoubleValue();
                break;
            case VT_INT:
                cout << getIntValue();
                break;
            case VT_STRING:
                cout << getStringValue();
                break;
            default:
                assert(false);
        }
    }



    const string Code::empty_string;

    Code::Code() {
        _constants.push_back(empty_string);
    }

    Code::~Code() {
        for (uint32_t i = 0; i < _functions.size(); i++) {
            delete _functions[i];
        }
    }

    uint16_t Code::addFunction(TranslatedFunction* function) {
        uint16_t id = _functions.size();
        _functions.push_back(function);
        _functionById[function->name()] = id;
        function->assignId(id);
        return id;
    }

    TranslatedFunction* Code::functionById(uint16_t id) const {
        if (id >= _functions.size()) {
            return 0;
        }
        return _functions[id];
    }

    TranslatedFunction* Code::functionByName(const string& name) const {
        FunctionMap::const_iterator it = _functionById.find(name);
        if (it == _functionById.end()) {
            return 0;
        }
        return functionById((*it).second);
    }

    uint16_t Code::makeStringConstant(const string& str) {
        ConstantMap::iterator it = _constantById.find(str);
        if (it != _constantById.end()) {
            return (*it).second;
        }
        uint16_t id = _constants.size();
        _constantById[str] = id;
        _constants.push_back(str);
        return id;
    }

    uint16_t Code::makeNativeFunction(const string& name, const Signature& signature, const void* address) {
        NativeMap::iterator it = _nativeById.find(name);
        if (it != _nativeById.end()) {
            return (*it).second;
        }
        uint16_t id = _natives.size();
        _nativeById[name] = id;
        _natives.push_back(NativeFunctionDescriptor(name, signature, address));
        return id;
    }

    const string& Code::constantById(uint16_t id) const {
        if (id >= _constants.size()) {
            assert(false);
            return _constants[0];
        }
        return _constants[id];
    }

    const void* Code::nativeById(uint16_t id,
            const Signature** signature,
            const string** name) const {
        if (id >= _natives.size()) {
            return 0;
        }
        const NativeFunctionDescriptor& nfd = _natives[id];
        *name = &nfd.name();
        *signature = &nfd.signature();
        return nfd.code();
    }

    void Code::disassemble(ostream& out, FunctionFilter* filter) {
        for (uint32_t i = 0; i < _functions.size(); i++) {
            TranslatedFunction* function = _functions[i];
            bool match = filter ? filter->matches(function) : true;
            if (match) {
                out << "function [" << function->id() << "] "
                        << typeToName(function->returnType())
                        << " " << function->name() << "(";
                for (uint32_t i = 0; i < function->parametersNumber(); i++) {
                    out << typeToName(function->parameterType(i));
                    if (i + 1 < function->parametersNumber()) {
                        out << ", ";
                    }
                }
                out << ")" << endl;
                function->disassemble(out);
            }
        }
    }

    TranslatedFunction::TranslatedFunction(AstFunction* function) :
    _id(INVALID_ID),
    _locals(0), _params(function->parametersNumber()),
    _name(function->name()) {
        _signature.push_back(SignatureElement(function->returnType(), "return"));
        for (uint32_t i = 0; i < function->parametersNumber(); i++) {
            _signature.push_back(SignatureElement(function->parameterType(i), function->parameterName(i)));
        }
    }

    TranslatedFunction::TranslatedFunction(const string& name, const Signature& signature) :
    _id(INVALID_ID), _locals(0), _params(signature.size() - 1), _name(name),
    _signature(signature) {
    }

    TranslatedFunction::~TranslatedFunction() {
    }

    void ErrorInfoHolder::error(uint32_t position, const char* format, ...) {
        va_list args;
        va_start(args, format);
        verror(position, format, args);
    }

    void ErrorInfoHolder::verror(uint32_t position, const char* format, va_list args) {
        _position = position;
        vsnprintf(_msgBuffer, sizeof (_msgBuffer), format, args);
        va_end(args);
        throw this;
    }

    uint8_t typeToSize(VarType type) {
        if (type == VT_INT)
            return sizeof (int64_t);
        if (type == VT_DOUBLE)
            return sizeof (double);
        if (type == VT_STRING)
            return sizeof (uint16_t);
        if (type == VT_VOID)
            return 0;
        assert(false);
    }


}
