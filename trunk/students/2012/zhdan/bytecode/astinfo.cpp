/*
 * astinfo.cpp
 *
 *  Created on: Oct 27, 2012
 *      Author: user
 */

#include "astinfo.h"

namespace mathvm {

AstInfo::AstInfo(VarType type) {
	_type = type;
	_error_message = NULL;
}

VarType AstInfo::getType() {
	return _type;
}

string* AstInfo::getErrorMessage() {
	return _error_message;
}

void AstInfo::setErrorMessage(string* error_message) {
	_error_message = error_message;
}

VarType getNodeType(AstNode* node) {
	return ((AstInfo*) ((node->info())))->getType();
}

void setNodeType(AstNode* node, VarType type) {
	node->setInfo(new AstInfo(type));
}

void setErrorMessage(AstNode* node, string message) {
	AstInfo* info = (AstInfo*)node->info();
	if (info == NULL) {
		info = new AstInfo(VT_INVALID);
		node->setInfo(info);
	}
	info->setErrorMessage(&message);
}

}
