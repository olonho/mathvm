/* 
 * File:   PrinterVisitor.h
 * Author: yarik
 *
 * Created on October 2, 2012, 6:18 PM
 */

#ifndef PRINTERVISITOR_H
#define	PRINTERVISITOR_H

#include <visitors.h>
#include <stack>
#include <map>
#include <stdlib.h>
#include <mathvm.h>
#include "CodeImpl.h"


using namespace mathvm;
using std::stack;




const bool DEBUG_MODE = true;

void WR_ERROR(const char*);
void WR_DEBUG(const char*);



class TranslatorVisitor: public AstVisitor {
public:
    TranslatorVisitor(Code* *code);
    virtual ~TranslatorVisitor();


    void translate(AstFunction*);

    
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node );
FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    
private:

    SmartCode* _code;

    stack<VarType> varStack;



    void storeVar(const AstVar*);
    void loadVar(const AstVar*);

    Instruction getNegInsn(VarType type) {
       	switch (type) {
       		case VT_INT:
       			return BC_INEG;
       			break;
       		case VT_DOUBLE:
       		    return BC_DNEG;
       		    break;
       		default:
       			assert(false);
       			break;
       	}
    }

    Instruction getMinusInsn(VarType type) {
    	switch (type) {
    		case VT_INT:
    			return BC_ISUB;
    			break;
    		case VT_DOUBLE:
    		    return BC_DSUB;
    		    break;
    		default:
    			assert(false);
    			break;
    	}
    }

    Instruction getDivInsn(VarType type) {
       	switch (type) {
       		case VT_INT:
       			return BC_IDIV;
       			break;
       		case VT_DOUBLE:
       		    return BC_DDIV;
       		    break;
       		default:
       			assert(false);
       			break;
      	}
    }

    Instruction getPlusInsn(VarType type) {
    	switch (type) {
    		case VT_INT:
     			return BC_IADD;
     			break;
     		case VT_DOUBLE:
     		    return BC_DADD;
    		    break;
     		default:
    			assert(false);
     			break;
  	    }
    }


    Instruction getStoreCTXInsn(VarType type) {
        switch (type) {
        	case VT_INT:
        		return BC_STORECTXIVAR;
        		break;
        	case VT_DOUBLE:
        	    return BC_STORECTXDVAR;
        	    break;
         	case VT_STRING:
         		return BC_STORECTXSVAR;
         		break;
         	default:
        		assert(false);
         		break;
      	}
     }


    Instruction getLoadCTXInsn(VarType type) {
      	switch (type) {
      		case VT_INT:
      			return BC_LOADCTXIVAR;
     			break;
            case VT_DOUBLE:
                return BC_LOADCTXDVAR;
                break;
            case VT_STRING:
            	return BC_LOADCTXSVAR;
            	break;
            default:
            	assert(false);
            	break;
        }
    }

    Instruction getCMPInsn(VarType type) {
       	switch (type) {
       		case VT_INT:
       			return BC_ICMP;
      			break;
            case VT_DOUBLE:
                return BC_DCMP;
                break;
            default:
              	assert(false);
              	break;
        }
    }

    Instruction getLoad1Insn(VarType type) {
       	switch (type) {
       		case VT_INT:
       			return BC_ILOAD1;
      			break;
            case VT_DOUBLE:
                return BC_DLOAD1;
                break;
            default:
              	assert(false);
               	break;
        }
    }



    void visitBlockNodeBody(BlockNode*);
    void visitScopeVars(Scope*);
    void visitScopeFuns(Scope*);
    void visitScopeAttr(Scope*);
    void ifCondLoad1Else0();



};

#endif	/* PRINTERVISITOR_H */

