#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm{
    class AstPrinterVisitor: public AstBaseVisitor{
	public:
	AstPrinterVisitor(ostream & out):
	_tabLevel(0),
	_out(out){
	}
	
	void visitBinaryOpNode(BinaryOpNode * node){
		_out << "(";
		node->left()->visit(this);
		_out << tokenOp(node->kind());
		node->right()->visit(this);
		_out << ")";
	}
	
	void visitUnaryOpNode(UnaryOpNode * node){
		_out << tokenOp(node->kind());
		_out << "(";
		node->operand()->visit(this);
		_out << ")";
	}
	
	void visitStringLiteralNode(StringLiteralNode * node){
		_out << "\"";
		_out << escapeString(node->literal());
		_out << "\"";
	}

	void visitIntLiteralNode(IntLiteralNode * node){
		_out << node->literal();
	}

	void visitDoubleLiteralNode(DoubleLiteralNode * node){
		_out << showpoint << node->literal();
	}

	void visitLoadNode(LoadNode * node){
		_out << node->var()->name();
	}

	void visitStoreNode(StoreNode * node){
		_out << node->var()->name() << tokenOp(node->op());
		node->value()->visit(this);
		_out << ";";
	}

	void visitBlockNode(BlockNode * node){

		bool isMainScope = ++_tabLevel == 0;
		if(!isMainScope){
			_out << '{' << endl;
			printScopeVarDeclarations(node->scope());
		}


		
	        for (uint32_t i = 0; i < node->nodes(); ++i) {
                	_out << string(_tabLevel, '\t');
			node->nodeAt(i)->visit(this);
                
               		if (node->nodeAt(i)->isCallNode()) {
                    		_out << ";";
               		}

               		 _out << endl;
            	}

		--_tabLevel;
		if(!isMainScope){
			_out << string(_tabLevel, '\t') <<'}';
		}

	}

	void visitForNode(ForNode * node){
		_out << "for (" << node->var()->name() << " in ";
		node->inExpr()->visit(this);
		_out << ')';
		node->body()->visit(this);
	}
	
	void visitWhileNode(WhileNode * node){
		_out << "while (";
		node->whileExpr()->visit(this);
		_out << ")";
		node->loopBlock()->visit(this);
	}

	void visitIfNode(IfNode * node){
		_out << "if (";
		node->ifExpr()->visit(this);
		_out << ")";
		node->thenBlock()->visit(this);
		if(node->elseBlock()){
			_out << " else ";
			node->elseBlock()->visit(this);
		}
	}
	
	void visitReturnNode(ReturnNode * node){
		if(node->returnExpr()){
			_out << "return ";
			node->returnExpr()->visit(this);
			_out << ";";
		} else{
			_out << "return;";
		}
	}

	void visitFunctionNode(FunctionNode * node){
		if (node->name() != AstFunction::top_name) {
		_out << "function " << typeToName(node->returnType()) << ' ' << node->name() << " (";
        if(node->parametersNumber() > 0){
                for(uint32_t i = 0; i < node->parametersNumber() - 1; ++i){
                        _out << typeToName(node->parameterType(i)) << ' ';
			_out << node->parameterName(i);
                        _out << ", ";
                }

                        _out << typeToName(node->parameterType(node->parametersNumber()-1)) << ' ';
                        _out << node->parameterName(node->parametersNumber() - 1);
                }
		_out << ")";
		}else{
			_tabLevel = -1;
		}
		
        if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
            _out << " native '" << node->body()->nodeAt(0)->asNativeCallNode()->nativeName() << "';";
            _out << endl;
        } else {
            node->body()->visit(this);
            _out << endl;
        }

	}

	void visitCallNode(CallNode * node){
		_out << node->name() << " (";
		if(node->parametersNumber() > 0){
			for(uint32_t i = 0; i < node->parametersNumber() - 1; ++i){
				node->parameterAt(i)->visit(this);
				_out << ", ";
			}
			node->parameterAt(node->parametersNumber()-1)->visit(this);
		}
		_out << ")";
	}

	void visitPrintNode(PrintNode * node){
		_out << "print(";

		if(node->operands() > 0){
			for (uint32_t i = 0; i < node->operands() - 1; ++i) {
						node->operandAt(i)->visit(this);
							_out << ", ";
			}
			node->operandAt(node->operands()-1)->visit(this);
		}

            	_out << ");" << endl;
	}

	void printScopeVarDeclarations(Scope * scope){
		Scope::VarIterator it(scope);
		
		while(it.hasNext()){
			AstVar * var = it.next();
			_out << string(max(_tabLevel, 0), '\t');
			_out << typeToName(var->type()) << ' ' << var->name() << ';' << endl;
		}
	}

	private:
		int _tabLevel;
		ostream &  _out;
		string escapeString(string const & s){
			string res;
			for(unsigned int i = 0; i < s.size(); ++i){
				switch(s[i]){
					case '\n': res += "\\n";break;	
					case '\t': res += "\\t";break;
					case '\r': res += "\\r";break;
					case '\\': res += "\\\\";break;
					case '\'': res += "\\'";break;
					default: res += s[i];break;
				}
			}
			return res;
		}
    };

    class AstPrinter : public Translator {
      public:
        virtual Status* translate(const string& program, Code* *code)  {
              Parser parser;
              Status* status = parser.parseProgram(program);
              if (status && status->isError()) return status;
              AstPrinterVisitor printer(std::cout);
		printer.printScopeVarDeclarations(parser.top()->scope()->childScopeAt(0));
		Scope::FunctionIterator it(parser.top()->scope()->childScopeAt(0));
		while(it.hasNext()){
			it.next()->node()->visit(&printer);
		}
		parser.top()->node()->visit(&printer);
              return new Status("No executable code produced");
        }
    };

    Translator* Translator::create(const string& impl) {
       if (impl == "printer") {
         return new AstPrinter();
       } else {
          return NULL;
       }
    }
}
