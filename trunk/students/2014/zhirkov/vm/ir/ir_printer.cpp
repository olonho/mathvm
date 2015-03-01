#include <iostream>
#include "ir_printer.h"
#include "../util.h"
#include "../translator/ssa_utils.h"
#include <vector>

namespace mathvm {
    namespace IR {

        void IrPrinter::visit(Variable const *const expr) {
            _out << "[var " << expr->id  << "]";
            
        }

        void IrPrinter::visit(BinOp const *const expr) {
            expr->left->visit(this);
            _out << " ";
            switch (expr->type) {
#define BINOP_STR(name, str) case BinOp::BO_##name: _out << str; break;
                FOR_IR_BINOP(BINOP_STR)
#undef BINOP_STR
                case BinOp::BO_INVALID:
                    _out << "<INVALID OP>";
                    break;
            }
            _out << " ";
            expr->right->visit(this);
            
        }

        void IrPrinter::visit(UnOp const *const expr) {
            switch (expr->type) {
#define UNOP_STR(name, str) case UnOp::UO_##name: _out << str; break;
                FOR_IR_UNOP(UNOP_STR)
                case UnOp::UO_INVALID:
                    _out << "<INVALID OP>";
                    break;
            }
            expr->operand->visit(this);
            
        }

        void IrPrinter::visit(Return const *const expr) {
            _out << "return ";
            expr->atom->visit(this);
            _out << std::endl;
            
        }

        void IrPrinter::visit(Phi const *const expr) {
            expr->var->visit(this);
            _out << " = phi(";
            for (auto const v : expr->vars){
                v->visit(this);
                _out << " ";
            }
            _out << ")";
            
        }

        void IrPrinter::visit(Int const *const expr) {
            _out << expr->value;
            
        }

        void IrPrinter::visit(Double const *const expr) {
            _out << expr->value;
            
        }

        void IrPrinter::visit(Ptr const *const expr) {
            _out << "<ptr:" << expr->value;
            if (expr->isPooledString) _out << '@' ;
//            if (expr->isPooledString && currentFunction != NULL)
//                _out << "@\"" << escape(currentFunction->pool[expr->value]) << '\"';
            _out << ">";
            
        }

        void IrPrinter::visit(Block const *const expr) {
            _out << std::endl << "Block " << expr->name << std::endl
                    << "  predecessors: ";
            for(auto pred: expr->predecessors)
                _out << pred->name << " ";
            _out << std::endl;
            for (size_t i = 0; i < expr->contents.size(); i++) {
                _out << "   " << i << " : ";
                expr->contents[i]->visit(this);
                _out << std::endl;
            }

            if (!expr->isLastBlock())
                expr->getTransition()->visit(this);

            else _out << "<no jump>";
            _out << std::endl;
            
        }

        void IrPrinter::visit(JumpCond const *const expr) {
            _out << "Cond  ";
            expr->condition->visit(this);
            _out << " ";

            _out << std::endl;
            _out << "yes : " << (expr->yes ? expr->yes->name : "NULL") << std::endl;
            _out << "no  : " << (expr->no ? expr->no->name : "NULL") << std::endl;
            _out << std::endl;

            
        }

        void IrPrinter::visit(JumpAlways const *const expr) {
            _out << "Jump to " <<
                    (expr->destination ? expr->destination->name : "NULL") << std::endl;
            
        }

        void IrPrinter::visit(Assignment const *const expr) {
            expr->var->visit(this);
            _out << " = ";
            expr->value->visit(this);
            
        }

        void IrPrinter::visit(Function const *const expr) {
            currentFunction = expr;
            if (expr->isNative())
                _out << "Native ";
            _out << "Function id " << expr->id << " name '" << expr->name << "' returns " << varTypeStr(expr->returnType) << "\n";

            if (expr->parametersIds.size() > 0 ) {
                _out << "\n  parameters";
                for (auto v : expr->parametersIds)
                    _out << " " << v;
            }
            if (expr->refParameterIds.size() > 0) {
                _out << "\n  reference parameters: ";
                for (auto v: expr->refParameterIds)
                    _out << v << " ";

            }
            if (expr->memoryCells.size() > 0) {
                _out << "\n  memory cells for: ";
                for (auto v : expr->memoryCells) _out << v << " ";
                _out << std::endl;
            }
            if (!expr->isNative()) {
                _out << "\n entry point : ";
                if (expr->entry) _out << expr->entry->name;

                for (auto b : blocksOrder(expr->entry))
                    b->visit(this);
                _out << std::endl;
            }
            currentFunction = NULL;
            
        }

        void IrPrinter::visit(Call const *const expr) {
            _out << "call " << expr->funId << "(";
            for (auto p : expr->params)
            {
                _out << " " ;
                p->visit(this);
            }
            _out << " ) refs (";
            for (auto p : expr->refParams)
                _out << " " <<p;
            _out << " )";
            
        }

        void IrPrinter::visit(Print const *const expr) {
            _out << "print ";
            expr->atom->visit(this);
            
        }
        static void printVarMeta(SimpleIr::VarMeta const& meta, std::ostream& out) {
            out << meta.id << " : " << varTypeStr(meta.type) <<" -> ";
            if (meta.isSourceVar) out << "source id " << meta.originId;
            else out << "temp";
            out << std::endl;
        }
        void IrPrinter::print(SimpleIr const& ir) {
            _out<< "String pool:" << std::endl;
            int i = 0;
            for (auto s : ir.pool)
                _out<< i++ << " : " << mathvm::escape(s) << std::endl;
            _out << "Variables metadata" << std::endl;
            for (auto const meta :  ir.varMeta)
                printVarMeta(meta, _out);

            _out<< "Functions" << std::endl;
            for (auto f : ir.functions)
                if (!f->isNative())
                f->visit(this);

        }

        void IrPrinter::visit(WriteRef const *const expr) {
            _out << "writeref ";
            expr->atom->visit(this);
            _out << " to " << expr->refId;

            
        }

        void IrPrinter::visit(ReadRef const *const expr) {
            _out << "readref " << expr->refId;
            
        }
    }

}