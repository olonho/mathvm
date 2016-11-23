//
// Created by wimag on 20.11.16.
//

#include <cassert>
#include "context_storage.h"

void mathvm::context_storage::enter_function(mathvm::BytecodeFunction *function) {
    functions.push(function);
    assert(contexts.size() < UINT16_MAX);
    contexts.push_back(context(function->id()));
}

void mathvm::context_storage::exit_function() {
    BytecodeFunction* function = functions.top();
    context local_context = contexts.back();
    function->setLocalsNumber(local_context.size());
    functions.pop();
    contexts.pop_back();
}

context_entry mathvm::context_storage::register_variable(mathvm::AstVar *astVar) {
    return contexts.back().add_var(astVar->name());
}

context_entry mathvm::context_storage::register_variable(const string& name) {
    return contexts.back().add_var(name);
}

mathvm::VarType mathvm::context_storage::tos_type() {
    return types.empty()? VT_INVALID : types.top();
}

void mathvm::context_storage::pop_type() {
    types.pop();
}

void mathvm::context_storage::top_insn(mathvm::Instruction insn) {
    functions.top()->bytecode()->addInsn(insn);
}

mathvm::Bytecode* mathvm::context_storage::get_bytecode() {
    return functions.top()->bytecode();
}

void mathvm::context_storage::push_type(mathvm::VarType type) {
    types.push(type);
}

mathvm::VarType mathvm::context_storage::top_2_consensus() {
    VarType second = types.top();
    types.pop();
    VarType first = types.top();
    if(first == second){
        types.push(second);
        return first;
    }
    if(first == VT_DOUBLE && second == VT_INT){
        get_bytecode()->addInsn(BC_I2D);
        types.push(VT_DOUBLE);
        return VT_DOUBLE;
    }
    if(first == VT_INT && second == VT_DOUBLE){
        get_bytecode()->addInsn(BC_SWAP);
        get_bytecode()->addInsn(BC_I2D);
        get_bytecode()->addInsn(BC_SWAP);
        types.pop();
        types.push(VT_DOUBLE);
        types.push(VT_DOUBLE);
        return VT_DOUBLE;
    }
    assert(false);
}

context_entry mathvm::context_storage::find_var(std::string name) {
    for (auto it = contexts.rbegin(); it != contexts.rend(); ++it){
        if(it->has_var(name)){
            return it->get_var_entry(name);
        }
    }
    assert(false);
}

bool mathvm::context_storage::in_tompost_context(context_entry entry) {
    return contexts.back().getId() == entry.context_id;
}

void mathvm::context_storage::cast_top_to(mathvm::VarType type) {
    VarType current = tos_type();
    if(current == type){
        return;
    }
    assert(current != VT_STRING);
    assert(type != VT_STRING);
    if(current == VT_DOUBLE){
        get_bytecode()->addInsn(BC_D2I);
        return;
    }else if(current == VT_INT){
        get_bytecode()->addInsn(BC_I2D);
        return;
    }
    assert(false);
}

mathvm::BytecodeFunction *mathvm::context_storage::topmost_function() {
    return functions.top();
}

bool context_entry::operator<(const context_entry &other) const{
    return context_id == other.context_id ? var_id < other.var_id : context_id < other.context_id;
}

bool context::has_var(const std::string &name) const{
    auto it = entries.find(name);
    return it != entries.end();
}

context_entry context::get_var_entry(const std::string &name){
    assert(has_var(name)); // check if entry exists
    return entries[name];

}

context_entry context::add_var(const std::string &name) {
    assert(!has_var(name));
    assert(entries.size() < UINT16_MAX);
    context_entry entry{id, (uint16_t) entries.size()};
    entries[name] = entry;
    return entry;
}

uint16_t context::size() {
    assert(entries.size() < UINT16_MAX);
    return (uint16_t) entries.size();
}

uint16_t context::getId() const {
    return id;
}
