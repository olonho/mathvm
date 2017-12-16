#include "include/interpreter_code.h"

namespace mathvm {
    mathvm::Status *InterpreterCode::execute(std::vector<mathvm::Var *> &vars) {
        const int topnostScopeId = 1;

        for (Var *var : vars) {
            uint16_t varId = (*_varsFromTopmostScope)[var->name()];
            switch (var->type()) {
                case VarType::VT_STRING:
                    (*_varsByScopes)[topnostScopeId][varId]->setStringValue(var->getStringValue());
                    break;
                case VarType::VT_INT:
                    (*_varsByScopes)[topnostScopeId][varId]->setIntValue(var->getIntValue());
                    break;
                case VarType::VT_DOUBLE:
                    (*_varsByScopes)[topnostScopeId][varId]->setDoubleValue(var->getDoubleValue());
                    break;
                default:
                    std::cerr << "Wrong type name" << std::endl;
            }
        }

        BytecodeFunctionImpl* func = dynamic_cast<BytecodeFunctionImpl*>(functionByName("<top>"));
        insnStack.push(pair<Bytecode*, uint32_t>(func->getBytecode(), 0));
        auto * newScope = new  map<uint16_t, Var*>();
        for (auto elem : (*_varsByScopes)[topnostScopeId]) {
            (*newScope)[elem.first] = new Var(elem.second->type(), elem.second->name());
            switch (elem.second->type()) {
                case VarType::VT_STRING:
                    (*newScope)[elem.first]->setStringValue(elem.second->getStringValue());
                    break;
                case VarType::VT_INT:
                    (*newScope)[elem.first]->setIntValue(elem.second->getIntValue());
                    break;
                case VarType::VT_DOUBLE:
                    (*newScope)[elem.first]->setDoubleValue(elem.second->getDoubleValue());
                    break;
                default:
                    std::cerr << "Wrong type name" << std::endl;
            }
        }
        (*curCopiesOfScopes)[1].push(newScope);
        for (uint16_t childScope : (*_functionsScopesChilds)[topnostScopeId]) {
            copyScope(childScope);
        }
        executeCurBytecode(topnostScopeId);

        for (Var *var : vars) {
            uint16_t varId = (*_varsFromTopmostScope)[var->name()];
            switch (var->type()) {
                case VarType::VT_STRING:
                    var->setStringValue((*curCopiesOfScopes)[topnostScopeId].top()->at(varId)->getStringValue());
                    break;
                case VarType::VT_INT:
                    var->setIntValue((*curCopiesOfScopes)[topnostScopeId].top()->at(varId)->getIntValue());
                    break;
                case VarType::VT_DOUBLE:
                    var->setDoubleValue((*curCopiesOfScopes)[topnostScopeId].top()->at(varId)->getDoubleValue());
                    break;
                default:
                    std::cerr << "Wrong type name" << std::endl;
            }
        }
        finalizeFuncExecution(topnostScopeId);
        return Status::Ok();
    }

    void InterpreterCode::executeCurBytecode(uint16_t scopeId) {
        Bytecode *currBytecode = insnStack.top().first;
        uint32_t *currBytecodePtr = &insnStack.top().second;
        Instruction nextInsn = currBytecode->getInsn(*currBytecodePtr);
        while (nextInsn != Instruction::BC_LAST) {
            *currBytecodePtr += 1;
            switch (nextInsn) {
                #define CASE_INSN(b, d, l) case Instruction::BC_##b:   execute##b(); break;
                                FOR_BYTECODES(CASE_INSN)
                #undef CASE_INSN
                default:
                    break;
            }
            nextInsn = currBytecode->getInsn(*currBytecodePtr);
        }
        if (scopeId != 1) {
            finalizeFuncExecution(scopeId);
        }
        insnStack.pop();
    }

    void InterpreterCode::finalizeFuncExecution(uint16_t scopeId) {
        for (auto elem : *((*curCopiesOfScopes)[scopeId].top())) {
            delete(elem.second);
        }
        delete((*curCopiesOfScopes)[scopeId].top());
        (*curCopiesOfScopes)[scopeId].pop();
        for (uint16_t childScope : (*_functionsScopesChilds)[scopeId])
        {
            finalizeFuncExecution(childScope);
        }
    }

    void InterpreterCode::executeINVALID() {
        assert(false);
    }

    void InterpreterCode::executeDLOAD() {
        Var newVar(VT_DOUBLE, "");
        newVar.setDoubleValue(insnStack.top().first->getDouble(insnStack.top().second));
        dataStack.push(newVar);
        insnStack.top().second += sizeof(double);
    }

    void InterpreterCode::executeILOAD() {
        Var newVar(VT_INT, "");
        newVar.setIntValue(insnStack.top().first->getInt64(insnStack.top().second));
        dataStack.push(newVar);
        insnStack.top().second += sizeof(int64_t);
    }

    void InterpreterCode::executeSLOAD() {
        Var newVar(VT_STRING, "");
        uint16_t stringId = insnStack.top().first->getUInt16(insnStack.top().second);
        newVar.setStringValue(constantById(stringId).c_str());
        dataStack.push(newVar);
        insnStack.top().second += sizeof(uint16_t);
    }

    void InterpreterCode::executeDLOAD0() {
        Var newVar(VT_DOUBLE, "");
        newVar.setDoubleValue(0.0);
        dataStack.push(newVar);
    }

    void InterpreterCode::executeILOAD0() {
        Var newVar(VT_INT, "");
        newVar.setIntValue(0);
        dataStack.push(newVar);
    }

    void InterpreterCode::executeSLOAD0() {
        Var newVar(VT_STRING, "");
        newVar.setStringValue("");
        dataStack.push(newVar);
    }

    void InterpreterCode::executeDLOAD1() {
        Var newVar(VT_DOUBLE, "");
        newVar.setDoubleValue(1.0);
        dataStack.push(newVar);
    }

    void InterpreterCode::executeILOAD1() {
        Var newVar(VT_INT, "");
        newVar.setIntValue(1);
        dataStack.push(newVar);
    }

    void InterpreterCode::executeDLOADM1() {
        Var newVar(VT_DOUBLE, "");
        newVar.setDoubleValue(-1.0);
        dataStack.push(newVar);
    }

    void InterpreterCode::executeILOADM1() {
        Var newVar(VT_INT, "");
        newVar.setDoubleValue(-1);
        dataStack.push(newVar);
    }

    void InterpreterCode::executeDADD() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_DOUBLE, "");
        resVar.setDoubleValue(var1.getDoubleValue() + var2.getDoubleValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeIADD() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var1.getIntValue() + var2.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeDSUB() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_DOUBLE, "");
        resVar.setDoubleValue(var1.getDoubleValue() - var2.getDoubleValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeISUB() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var1.getIntValue() - var2.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeDMUL() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_DOUBLE, "");
        resVar.setDoubleValue(var1.getDoubleValue() * var2.getDoubleValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeIMUL() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var1.getIntValue() * var2.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeDDIV() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_DOUBLE, "");
        resVar.setDoubleValue(var1.getDoubleValue() / var2.getDoubleValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeIDIV() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var1.getIntValue() / var2.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeIMOD() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var2.getIntValue() % var1.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeDNEG() {
        Var var = dataStack.top();
        dataStack.pop();
        var.setDoubleValue(-var.getDoubleValue());
        dataStack.push(var);
    }

    void InterpreterCode::executeINEG() {
        Var var = dataStack.top();
        dataStack.pop();
        var.setIntValue(-var.getIntValue());
        dataStack.push(var);
    }

    void InterpreterCode::executeIAOR() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var2.getIntValue() | var1.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeIAAND() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var2.getIntValue() & var1.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeIAXOR() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var resVar(VT_INT, "");
        resVar.setIntValue(var2.getIntValue() ^ var1.getIntValue());
        dataStack.push(resVar);
    }

    void InterpreterCode::executeIPRINT() {
        Var var = dataStack.top();
        dataStack.pop();
        cout << var.getIntValue();
    }

    void InterpreterCode::executeDPRINT() {
        Var var = dataStack.top();
        if (var.type() == VT_INT) {
            executeI2D();
        }
        var = dataStack.top();
        dataStack.pop();
        cout << var.getDoubleValue();
    }

    void InterpreterCode::executeSPRINT() {
        Var var = dataStack.top();
        dataStack.pop();
        cout << var.getStringValue();
    }

    void InterpreterCode::executeI2D() {
        Var var = dataStack.top();
        dataStack.pop();
        Var newVar(VT_DOUBLE, "");
        newVar.setDoubleValue(var.getIntValue());
        dataStack.push(newVar);
    }

    void InterpreterCode::executeD2I() {
        Var var = dataStack.top();
        dataStack.pop();
        Var newVar(VT_INT, "");
        newVar.setIntValue(int64_t(var.getDoubleValue()));
        dataStack.push(newVar);
    }

    void InterpreterCode::executeS2I() {
        Var var = dataStack.top();
        dataStack.pop();
        Var newVar(VT_INT, "");
        newVar.setIntValue(stoi(var.getStringValue()));
        dataStack.push(newVar);
    }

    void InterpreterCode::executeSWAP() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        dataStack.push(var1);
        dataStack.push(var2);
    }

    void InterpreterCode::executePOP() {
        dataStack.pop();
    }

    void InterpreterCode::executeLOADDVAR0() {
        Var* var = (*_valuesByIds)[0];
        assert(var->type() == VT_DOUBLE);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADDVAR1() {
        Var* var = (*_valuesByIds)[1];
        assert(var->type() == VT_DOUBLE);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADDVAR2() {
        Var* var = (*_valuesByIds)[2];
        assert(var->type() == VT_DOUBLE);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADDVAR3() {
        Var* var = (*_valuesByIds)[3];
        assert(var->type() == VT_DOUBLE);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADIVAR0() {
        Var* var = (*_valuesByIds)[0];
        assert(var->type() == VT_INT);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADIVAR1() {
        Var* var = (*_valuesByIds)[1];
        assert(var->type() == VT_INT);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADIVAR2() {
        Var* var = (*_valuesByIds)[2];
        assert(var->type() == VT_INT);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADIVAR3() {
        Var* var = (*_valuesByIds)[3];
        assert(var->type() == VT_INT);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADSVAR0() {
        Var* var = (*_valuesByIds)[0];
        assert(var->type() == VT_STRING);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADSVAR1() {
        Var* var = (*_valuesByIds)[1];
        assert(var->type() == VT_STRING);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADSVAR2() {
        Var* var = (*_valuesByIds)[2];
        assert(var->type() == VT_STRING);
        dataStack.push(*var);
    }

    void InterpreterCode::executeLOADSVAR3() {
        Var* var = (*_valuesByIds)[3];
        assert(var->type() == VT_STRING);
        dataStack.push(*var);
    }



    void InterpreterCode::executeSTOREDVAR0() {
        storeVar(0, VT_DOUBLE);
    }

    void InterpreterCode::storeVar(int varId, VarType varType) {
        Var var = dataStack.top();
        assert(var.type() == varType);
        assert((*_valuesByIds)[varId]->type() == varType);
        dataStack.pop();
        switch(varType) {
            case VT_DOUBLE:
                (*_valuesByIds)[varId]->setDoubleValue(var.getDoubleValue());
                break;
            case VT_INT:
                (*_valuesByIds)[varId]->setIntValue(var.getIntValue());
                break;
            case VT_STRING:
                (*_valuesByIds)[varId]->setStringValue(var.getStringValue());
                break;
            default:
                cerr << "Wrong var type";
        }
    }

    void InterpreterCode::executeSTOREDVAR1() {
        storeVar(1, VT_DOUBLE);
    }

    void InterpreterCode::executeSTOREDVAR2() {
        storeVar(2, VT_DOUBLE);
    }

    void InterpreterCode::executeSTOREDVAR3() {
        storeVar(3, VT_DOUBLE);
    }

    void InterpreterCode::executeSTOREIVAR0() {
        storeVar(0, VT_INT);
    }

    void InterpreterCode::executeSTOREIVAR1() {
        storeVar(1, VT_INT);
    }

    void InterpreterCode::executeSTOREIVAR2() {
        storeVar(2, VT_INT);
    }

    void InterpreterCode::executeSTOREIVAR3() {
        storeVar(3, VT_INT);
    }

    void InterpreterCode::executeSTORESVAR0() {
        storeVar(0, VT_STRING);
    }

    void InterpreterCode::executeSTORESVAR1() {
        storeVar(1, VT_STRING);
    }

    void InterpreterCode::executeSTORESVAR2() {
        storeVar(2, VT_STRING);
    }

    void InterpreterCode::executeSTORESVAR3() {
        storeVar(3, VT_STRING);
    }



    void InterpreterCode::executeLOADDVAR() {
        uint16_t varId = insnStack.top().first->getUInt16(insnStack.top().second);
        Var var(*(*_valuesByIds)[varId]);
        assert(var.type() == VT_DOUBLE);
        dataStack.push(var);
        insnStack.top().second += sizeof(uint16_t);
    }

    void InterpreterCode::executeLOADIVAR() {
        uint16_t varId = insnStack.top().first->getUInt16(insnStack.top().second);
        Var var(*(*_valuesByIds)[varId]);
        assert(var.type() == VT_INT);
        dataStack.push(var);
        insnStack.top().second += sizeof(uint16_t);
    }

    void InterpreterCode::executeLOADSVAR() {
        uint16_t varId = insnStack.top().first->getUInt16(insnStack.top().second);
        Var var(*(*_valuesByIds)[varId]);
        assert(var.type() == VT_STRING);
        dataStack.push(var);
        insnStack.top().second += sizeof(uint16_t);
    }

    void InterpreterCode::executeSTOREDVAR() {
        storeVar(VT_DOUBLE);
    }

    void InterpreterCode::storeVar(VarType varType) {
        uint16_t varId = insnStack.top().first->getUInt16(insnStack.top().second);
        storeVar(varId, varType);
    }

    void InterpreterCode::executeSTOREIVAR() {
        storeVar(VT_INT);
    }

    void InterpreterCode::executeSTORESVAR() {
        storeVar(VT_STRING);
    }

    void InterpreterCode::executeLOADCTXDVAR() {
        uint16_t scopeId = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        uint16_t varIdInScope = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        Var var = *((*curCopiesOfScopes)[scopeId].top()->at(varIdInScope));
        assert(var.type() == VT_DOUBLE);
        dataStack.push(var);
    }

    void InterpreterCode::executeLOADCTXIVAR() {
        uint16_t scopeId = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        uint16_t varIdInScope = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        Var var = *((*curCopiesOfScopes)[scopeId].top()->at(varIdInScope));
        assert(var.type() == VT_INT);
        dataStack.push(var);
    }

    void InterpreterCode::executeLOADCTXSVAR() {
        uint16_t scopeId = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        uint16_t varIdInScope = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        Var var = *((*curCopiesOfScopes)[scopeId].top()->at(varIdInScope));
        assert(var.type() == VT_STRING);
        dataStack.push(var);
    }

    void InterpreterCode::storeVarByCtx(VarType varType) {
        uint16_t scopeId = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        uint16_t varId = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        Var var = dataStack.top();
        assert((*_varsByScopes)[scopeId][varId]->type()== varType);
        Var* varToChange = (*curCopiesOfScopes)[scopeId].top()->at(varId);
        switch(varType) {
            case VT_DOUBLE:
                if (var.type() == VT_INT) {
                    executeI2D();
                }
                var = dataStack.top();
                varToChange->setDoubleValue(var.getDoubleValue());
                break;
            case VT_INT:
                varToChange->setIntValue(var.getIntValue());
                break;
            case VT_STRING:
                varToChange->setStringValue(var.getStringValue());
                break;
            default:
                cerr << "Wrong var type";
        }
        dataStack.pop();
    }

    void InterpreterCode::executeSTORECTXDVAR() {
        storeVarByCtx(VT_DOUBLE);
    }

    void InterpreterCode::executeSTORECTXIVAR() {
        storeVarByCtx(VT_INT);
    }

    void InterpreterCode::executeSTORECTXSVAR() {
        storeVarByCtx(VT_STRING);
    }

    void InterpreterCode::executeDCMP() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var newVar(VT_INT, "");
        newVar.setIntValue((var1.getDoubleValue() > var2.getDoubleValue()) -
                                   (var1.getDoubleValue() < var2.getDoubleValue()));
        dataStack.push(newVar);
    }

    void InterpreterCode::executeICMP() {
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        Var newVar(VT_INT, "");
        newVar.setIntValue((var1.getIntValue() > var2.getIntValue()) -
                           (var1.getIntValue() < var2.getIntValue()));
        dataStack.push(newVar);
    }

    void InterpreterCode::executeJA() {
        int16_t offset = insnStack.top().first->getInt16(insnStack.top().second);
        insnStack.top().second += offset;
    }

    void InterpreterCode::executeIFICMPNE() {
        int16_t offset = insnStack.top().first->getInt16(insnStack.top().second);
        insnStack.top().second += sizeof(int16_t);
        Var var1 = dataStack.top();
        executeSWAP();
        Var var2 = dataStack.top();
        executeSWAP();
        if (var1.getIntValue() != var2.getIntValue()) {
            insnStack.top().second += offset;
        }
    }

    void InterpreterCode::executeIFICMPE() {
        int16_t offset = insnStack.top().first->getInt16(insnStack.top().second);
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        if (var1.getIntValue() == var2.getIntValue()) {
            insnStack.top().second += offset;
        } else {
            insnStack.top().second += sizeof(int16_t);
        }
    }

    void InterpreterCode::executeIFICMPG() {
        int16_t offset = insnStack.top().first->getInt16(insnStack.top().second);
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        if (var1.getIntValue() > var2.getIntValue()) {
            insnStack.top().second += offset;
        } else {
            insnStack.top().second += sizeof(int16_t);
        }
    }

    void InterpreterCode::executeIFICMPGE() {
        int16_t offset = insnStack.top().first->getInt16(insnStack.top().second);
        insnStack.top().second += sizeof(int16_t);
        Var var1 = dataStack.top();
        executeSWAP();
        Var var2 = dataStack.top();
        executeSWAP();
        if (var1.getIntValue() >= var2.getIntValue()) {
            insnStack.top().second += offset;
        }
    }

    void InterpreterCode::executeIFICMPL() {
        int16_t offset = insnStack.top().first->getInt16(insnStack.top().second);
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        if (var1.getIntValue() < var2.getIntValue()) {
            insnStack.top().second += offset;
        } else {
            insnStack.top().second += sizeof(int16_t);
        }
    }

    void InterpreterCode::executeIFICMPLE() {
        int16_t offset = insnStack.top().first->getInt16(insnStack.top().second);
        Var var1 = dataStack.top();
        dataStack.pop();
        Var var2 = dataStack.top();
        dataStack.pop();
        if (var1.getIntValue() <= var2.getIntValue()) {
            insnStack.top().second += offset;
        } else {
            insnStack.top().second += sizeof(int16_t);
        }
    }

    void InterpreterCode::executeDUMP() {
        Var var = dataStack.top();
        if (var.type() == VT_INT) {
            cout << var.getIntValue();
        } else if (var.type() == VT_DOUBLE) {
            cout << var.getDoubleValue();
        } else if (var.type() == VT_STRING) {
            cout << var.getStringValue();
        }
    }

    void InterpreterCode::executeSTOP() {
    }

    void InterpreterCode::executeCALL() {
        uint16_t funId = insnStack.top().first->getUInt16(insnStack.top().second);
        insnStack.top().second += sizeof(uint16_t);
        BytecodeFunctionImpl* func = dynamic_cast<BytecodeFunctionImpl*>(functionById(funId));
        uint16_t scopeId = func->scopeId();
        copyScope(scopeId);
        insnStack.push(pair<Bytecode*, uint32_t>(func->getBytecode(), 0));
        executeCurBytecode(scopeId);
    }

    void InterpreterCode::copyScope(uint16_t scopeId) {
        map<uint16_t, Var*>* newScope = new  map<uint16_t, Var*>();
        for (auto elem : (*_varsByScopes)[scopeId]) {
            (*newScope)[elem.first] = new Var(elem.second->type(), elem.second->name());
        }
        (*curCopiesOfScopes)[scopeId].push(newScope);
        for (uint16_t childScope : (*_functionsScopesChilds)[scopeId]) {
            copyScope(childScope);
        }
    }

    void InterpreterCode::executeCALLNATIVE() {

    }

    void InterpreterCode::executeRETURN() {
    }

    void InterpreterCode::executeBREAK() {
    }
}

