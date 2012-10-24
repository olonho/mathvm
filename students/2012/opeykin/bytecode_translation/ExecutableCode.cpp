/*
 * ExecutableCode.cpp
 *
 *  Created on: Oct 25, 2012
 *      Author: alex
 */

#include "ExecutableCode.h"

namespace mathvm {

ExecutableCode::ExecutableCode() {
	// TODO Auto-generated constructor stub

}

ExecutableCode::~ExecutableCode() {
	// TODO Auto-generated destructor stub
}

Status* ExecutableCode::execute(vector<Var*>& vars) {
	return new Status;
}

} /* namespace mathvm */
