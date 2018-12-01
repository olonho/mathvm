#pragma once

namespace utils {

    template<typename Iterator, typename Consumer>
    void iterate(Iterator& it, Consumer const& p) {
        while (it.hasNext()) {
            p(it.next());
        }
    }

}