//
// Created by wimag on 23.11.16.
//

#include "execution_storage.h"

using namespace mathvm;

stack_var::operator int64_t() const {
    return data.i;
}

stack_var::operator double() const {
    return data.d;
}

stack_var::stack_var(int64_t value) : type_(VT_INT) {
    data.i = value;
}

stack_var::stack_var(double value) : type_(VT_DOUBLE){
    data.d = value;
}

stack_var::stack_var(const char *value) : type_(VT_STRING){
    data.s_ptr = value;
}

const VarType stack_var::type() {
    return type_;
}

stack_var::stack_var() : type_(VT_INVALID){}


stack_frame::stack_frame(BytecodeFunction *bytecodeFunction) : vars(bytecodeFunction->localsNumber()), function(bytecodeFunction), ip(0){

}

uint16_t stack_frame::get_context_id() {
    return function->id();
}

Bytecode *stack_frame::get_bytecode() {
    return function->bytecode();
}

stack_var stack_frame::get_var(uint16_t id) {
    assert(id < vars.size());
    //assert(vars[id].type() != VT_INVALID);
    return vars[id];
}



Instruction stack_frame::next_instruction() {
    return get_bytecode()->getInsn(ip++);
}

int64_t stack_frame::load_int() {
    int64_t res = get_bytecode()->getInt64(ip);
    ip += sizeof(int64_t);
    return res;
}

double stack_frame::load_double() {
    double res = get_bytecode()->getDouble(ip);
    ip += sizeof(double);
    return res;
}

uint16_t stack_frame::load_uint16() {
    uint16_t res = get_bytecode()->getUInt16(ip);
    ip += sizeof(uint16_t);
    return res;
}

void stack_frame::set_var(stack_var var, uint16_t id) {
    assert(id < vars.size());
    vars[id] = var;
}

void stack_frame::jump(int16_t offset) {
    ip += offset;
}


void execution_storage::enter_function(BytecodeFunction *function) {
    call_stack.push_back(stack_frame(function));
}

stack_var execution_storage::get_local_var(uint16_t id) {
    return call_stack.back().get_var(id);
}

stack_var execution_storage::get_context_var(uint16_t context_id, uint16_t var_id) {
    for(auto q = call_stack.rbegin(); q != call_stack.rend(); q++){
        if(q->get_context_id() == context_id){
            return q->get_var(var_id);
        }
    }
    assert(false);
}

stack_var execution_storage::pop_stack() {
    assert(!real_stack.empty());
    auto res = real_stack.top();
    real_stack.pop();
    return res;
}

void execution_storage::push_stack(stack_var var) {
    real_stack.push(var);
}

void execution_storage::exit_function() {
    assert(!call_stack.empty());
    call_stack.pop_back();
}

Instruction execution_storage::next_instruction() {
    assert(!call_stack.empty());
    return call_stack.back().next_instruction();
}

bool execution_storage::finished() {
    return call_stack.empty();
}

execution_storage::execution_storage(Code *code) : code(code){}

int64_t execution_storage::load_int() {
    assert(!call_stack.empty());
    return call_stack.back().load_int();
}

double execution_storage::load_double() {
    assert(!call_stack.empty());
    return call_stack.back().load_double();
}

uint16_t execution_storage::load_addr() {
    assert(!call_stack.empty());
    return call_stack.back().load_uint16();
}

const char *execution_storage::load_string() {
    assert(!call_stack.empty());
    uint16_t id = call_stack.back().load_uint16();
    return code->constantById(id).c_str();
}

void execution_storage::set_local_var(stack_var var, uint16_t id) {
    call_stack.back().set_var(var, id);
}

void execution_storage::set_context_var(stack_var var, uint16_t context_id, uint16_t var_id) {
    for(auto q = call_stack.rbegin(); q != call_stack.rend(); q++){
        if(q->get_context_id() == context_id){
            q->set_var(var, var_id);
            return;
        }
    }
    assert(false);
}

void execution_storage::jump(int16_t offset) {
    call_stack.back().jump(offset);
}

std::ostream &::mathvm::operator<<(ostream &stream, stack_var var) {
    switch (var.type()){
        case VT_INT:
            stream << (int64_t)var;
            break;
        case VT_DOUBLE:
            stream << (double) var;
            break;
        case VT_STRING:
            stream << (const char*) var;
            break;
        default:
            assert(false);
    }
}
