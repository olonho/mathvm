//
// Created by Владислав Калинин on 20/11/2018.
//

#include "Context.h"

using namespace mathvm;

vector<BytecodeFunction *> Context::functionList{};

unordered_map<string, uint16_t> Context::constantsById{};

BytecodeFunction *Context::getFunction(string name) {
    if (functionsById.find(name) != functionsById.end()) {
        return functionList[functionsById[name]];
    }
    if (parent == nullptr) {
        return nullptr;
    }
    return parent->getFunction(name);
}

uint16_t Context::getId() {
    return id;
}

uint16_t Context::makeStringConstant(string literal) {
    uint16_t id = static_cast<unsigned short>(constantsById.size());
    constantsById[literal] = id;
    return id;
}

Context *Context::getParent() {
    return parent;
}

Context *Context::getVarContext(string name) {
    auto *result = root->getVarContext(name);
    if (result != nullptr) {
        return result;
    }
    if (parent == nullptr) {
        return nullptr;
    }
    return parent->getVarContext(name);
}

uint16_t Context::VarNumber() {
    return static_cast<uint16_t>(varList.size());
}

SubContext *Context::subContext() {
    return currentSubContext;
}

void Context::createAndMoveToLowerLevel() {
    if (root == nullptr) {
        root = new SubContext(this);
        currentSubContext = root;
    } else {
        currentSubContext = currentSubContext->addChild();
    }
}

void Context::moveToUperLevel() {
    currentSubContext = currentSubContext->getParent();
}

void Context::nextSubContext() {
    if (currentSubContext == nullptr) {
        currentSubContext = root;
    } else if (currentSubContext->childsIterator()->hasNext()) {
        currentSubContext = currentSubContext->childsIterator()->next();
    } else {
        moveToUperLevel();
    }
}

Context *Context::addChild() {
    auto *child = new Context(this);
    childs.push_back(child);
    return child;
}

uint16_t Context::getVarId(string name) {
    return root->getVarId(name);
}

Context *Context::getChildAt(int ind) {
    return childs[ind];
}

void Context::destroySubContext() {
    delete root;
}

void SubContext::addVar(Var *var) {
    ownContext->varList.push_back(var);
    variablesById[var->name()] = static_cast<unsigned short>(ownContext->varList.size() - 1);
}

void SubContext::addFun(AstFunction *func) {
    if (parent != nullptr) {
        throw CompileError("Function must be declared in main scope");
    }
    auto *byteCodeFunction = new BytecodeFunction(func);
    uint16_t functionId = static_cast<uint16_t>(functionList.size());
    byteCodeFunction->assignId(functionId);
    functionList.push_back(byteCodeFunction);
    ownContext->functionsById[func->name()] = functionId;
}

SubContext *SubContext::getParent() {
    return parent;
}

SubContext *SubContext::addChild() {
    auto *child = new SubContext(ownContext, this);
    childs.push_back(child);
    return child;
}

Context *SubContext::getVarContext(string name) {
    if (variablesById.find(name) != variablesById.end()) {
        return this;
    }
    if (parent == nullptr) {
        if (ownContext->getParent() != nullptr) {
            return ownContext->getParent()->getVarContext(name);
        } else {
            return nullptr;
        }
    }
    return parent->getVarContext(name);
}

uint16_t SubContext::getVarId(string name) {
    return variablesById[name];
}

SubContext::ChildsIterator *SubContext::childsIterator() {
    return iter;
}

BytecodeFunction *SubContext::getFunction(string name) {
    return ownContext->getFunction(name);
}

uint16_t SubContext::getId() {
    return ownContext->getId();
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




