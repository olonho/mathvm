/*
 * astinfo.h
 *
 *  Created on: Oct 27, 2012
 *      Author: user
 */

#ifndef ASTINFO_H_
#define ASTINFO_H_

#include "visitors.h"
#include "mathvm.h"
#include "ast.h"

namespace mathvm {

VarType* getNodeType(AstNode* node);
void setNodeType(AstNode* node, VarType* type);
void setErrorMessage(AstNode* node, string message);

class AstInfo {

public:
	AstInfo(VarType* type);
	~AstInfo() {}

	VarType* getType();
	string* getErrorMessage();
	void setErrorMessage(string* error_message);

private:
	VarType* _type;
	string* _error_message;

};

}


#endif /* ASTINFO_H_ */
