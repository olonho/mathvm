/*
 * BCLabel.h
 *
 *  Created on: Jan 9, 2014
 *      Author: sam
 */

#ifndef BCLABEL_H_
#define BCLABEL_H_

namespace mathvm {

class BCLabel {
	Label label;
	Bytecode* code;
public:
	BCLabel(mathvm::Bytecode* c) :
			label(c), code(c) {
	}
	~BCLabel() {
		if (!label.isBound()) {
			code->bind(label);
		}
	}
	Label& operator()() {
		return label;
	}
};

} /* namespace mathvm */

#endif /* LABEL_H_ */
