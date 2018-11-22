//
// Created by Владислав Калинин on 20/11/2018.
//

#include "Context.h"

using namespace mathvm;

void Context::addVar(Var *var) {
    varList.push_back(var);
    variablesById[var->name()] = static_cast<unsigned short>(varList.size() - 1);
}

void Context::addFun(AstFunction *func) {
    auto *byteCodeFunction = new BytecodeFunction(func);
    uint16_t functionId = static_cast<uint16_t>(functionList.size());
    byteCodeFunction->assignId(functionId);
    functionList.push_back(byteCodeFunction);
    functionsById[func->name()] = functionId;
}

Context *Context::instanse = nullptr;

vector<Context *> Context::contextList{};

vector<BytecodeFunction *> Context::functionList{};

Context *Context::getParentContext() {
    return parent;
}

uint16_t Context::getId() {
    return id;
}

Context *Context::getLastChildren() {
    return childs.back();
}

Context *Context::addChild() {
    auto *child = new Context(this);
    childs.push_back(child);
    return child;
}

uint16_t Context::VarNumber() {
    return static_cast<uint16_t>(variablesById.size());
}

BytecodeFunction *Context::getFunction(string name) {
    if (parent == nullptr) {
        return nullptr;
    }
    if (functionsById.find(name) != functionsById.end()) {
        return functionList[functionsById[name]];
    }
    return parent->getFunction(name);
}

Context *Context::getVarContext(string name) {
    if (parent == nullptr) {
        return nullptr;
    }
    if (variablesById.find(name) != variablesById.end()) {
        return this;
    }
    return parent->getVarContext(name);
}

uint16_t Context::getVarId(string name) {
    return variablesById[name];
}

Context *Context::getRoot() {
    if (instanse == nullptr) {
        instanse = new Context();
    }
    return instanse;
}

void Context::init(Context *parentContext) {
    parent = parentContext;
    id = static_cast<uint16_t>(contextList.size());
    iter = new ChildsIterator(&childs);
    contextList.push_back(this);
}

Context::ChildsIterator *Context::childsIterator() {
    return iter;
}

uint16_t Context::makeStringConstant(string literal) {
    uint16_t id = static_cast<unsigned short>(constantsById.size());
    constantsById[literal] = id;
    return id;
}

vector<StackContext *> StackContext::contextList{};

void StackContext::setInt16(int ind, uint16_t value) {
    (*variables)[ind] = value;
}

void StackContext::setInt64(int ind, uint64_t value) {
    (*variables)[ind] = value;
}

void StackContext::setDouble(int ind, double value) {
    (*variables)[ind] = value;
}

uint16_t StackContext::getInt16(int ind) {
    return (*variables)[ind].i16;
}

uint64_t StackContext::getInt64(int ind) {
    return (*variables)[ind].i;
}

double StackContext::getDouble(int ind) {
    return (*variables)[ind].d;
}




