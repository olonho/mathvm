#include "my_utils.h"

#include <iostream>

using std::cout;

void DEBUG_MSG(string const& msg) {
    cout << "#DEBUG: " << msg << '\n';
}

void DEBUG_MSG(Scope* scope) {
    Scope::FunctionIterator funIt(scope);
    cout << "#DEBUG: Scope " <<  std::to_string(scope->functionsCount()) << endl;
    while(funIt.hasNext()) {
        AstFunction* fun = funIt.next();
        cout << fun->name() << endl;
    }
    if(scope->parent() != 0) {
        DEBUG_MSG(scope->parent());
    }
}
