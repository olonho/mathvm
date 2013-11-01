#ifndef CODEIMPL_H_
#define CODEIMPL_H_

#include <mathvm.h>
#include <ast.h>
#include <visitors.h>
#include <parser.h>


namespace mathvm {

struct CodeImpl: Code {

	Status* execute(vector<Var*>& vars) {
		return NULL;
	}

};

}
#endif /* CODEIMPL_H_ */
