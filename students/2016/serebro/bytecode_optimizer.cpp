//
// Created by andy on 11/23/16.
//

#include <algorithm>
#include "bytecode_optimizer.h"

namespace mathvm
{

void BytecodeOptimizer::removeEmptyBlocks() {
    vector<BytecodeScope*> q;

    for (auto &p : _meta.funcToScope) {
        q.push_back(p.second);
    }

    while (!q.empty()) {
        BytecodeScope* s = q.back();
        q.pop_back();

        s->children.erase(std::remove_if(s->children.begin(), s->children.end(),
                                         [](BytecodeScope* ch) {return ch->empty(); }), s->children.end());
        for (BytecodeScope* ch: s->children) {
            q.push_back(ch);
        }
    }
}

}



