#ifndef BCTRANSLATOR_H
#define BCTRANSLATOR_H

#include "mathvm.h"
#include "ast.h"

#include <map>
#include <utility>
#include <string>

using std::map;
using std::pair;
using std::make_pair;
using std::string;

namespace mathvm {

struct bytecode_translator: public Translator, public AstVisitor {
    bytecode_translator() {}

    Status* translate(const string &program, Code** code);

private:
    void translate_function(AstFunction* f);
    void convert_tos_type(VarType to);
    void convert_tos_to_bool();

    VarType make_num_type_cast(VarType ltype, VarType rtype, VarType to_type, bool soft = false);

    void create_math_operation(TokenKind op);
    void create_comparison_operation(TokenKind op);
    void create_logic_operation(TokenKind op);

    void enter_scope(BytecodeFunction* f) { current_scope = new TScope(f, current_scope); }

    void exit_scope() {
        TScope* p_scope = current_scope->parent;
        delete current_scope;
        current_scope = p_scope;
    }

    Bytecode* bytecode() { return current_scope->function->bytecode(); }

    void load_var(const AstVar* var) {
        process_var(var, true);
        tos_type = var->type();
    }

    void process_var(const AstVar* var, bool loading = false);

    void store_var(const AstVar* var) {
        process_var(var, false);
    }

    #define VISITOR_FUNCTION(type, name) void visit##type(type* node);
        FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION

    struct TScope {
        TScope(BytecodeFunction* f, TScope* p = 0):
            parent(p),
            function(f)
        {}

        pair<uint16_t, TScope*> get_var(const AstVar* v) {
            map<const AstVar*, uint16_t>::iterator vi = vars.find(v);
            if(vi != vars.end())
                return std::make_pair(vi->second, this);
            else {
                if(!parent) throw string(v->name() + " not found");
                return parent->get_var(v);
            }
        }

        void add_var(AstVar* v) { vars.insert(make_pair(v, vars.size())); }

        TScope* parent;
        BytecodeFunction* function;
        map<const AstVar*, uint16_t> vars;
    };

    Code* code;
    TScope* current_scope;
    VarType tos_type;
};

}

#endif // BCTRANSLATOR_H
