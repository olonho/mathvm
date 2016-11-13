#include "../../../../include/mathvm.h"
#include "../../../../vm/parser.h"
#include "../../../../include/visitors.h"
#include "IdentityTranslator.h"

namespace mathvm {

    Translator *Translator::create(const string &impl) {
        if (impl == "printer") {
            return new IdentityTranslator();
        }

        return 0;
    }

}

