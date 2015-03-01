#pragma once
#include <set>
#include "ir.h"

namespace mathvm {
    namespace IR {

        template<typename T, typename Ctx>
        struct IrAnalyzer {

        IrAnalyzer(std::ostream& debug, char const* name) : _debug(debug), _name(name) {}
        protected :
            mutable Ctx _status;
            virtual T defaultAnswer() const = 0;
            std::ostream& _debug;
            const char* const _name;
            mutable std::set<Block const*> visited;

        public:
            Ctx const& status() const { return _status; }
            T    visitElement(IrElement const * element) const {
#define MAC(type) if (element->is##type()) return visit(element->as##type());
                FOR_IR(MAC)
                return defaultAnswer();
            }

            virtual T visit(const BinOp *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const UnOp *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Variable *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Return *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Phi *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Int *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Double *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Ptr *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Block *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Assignment *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Call *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Print *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const Function *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const JumpAlways *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const JumpCond *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const WriteRef *const expr) const {
                return defaultAnswer();
            }

            virtual T visit(const ReadRef *const expr) const {
                return defaultAnswer();
            }

            T visitExpression(const Expression* const expr) const {
                T result;
                IrElement::IrType type = expr->getType();
                switch (type) {
                    case IrElement::IT_BinOp: result = visit(expr->asBinOp());break;
                    case IrElement::IT_UnOp:result = visit(expr->asUnOp());break;
                    case IrElement::IT_Variable:result = visit(expr->asVariable());break;
                    case IrElement::IT_Int:result = visit(expr->asInt());break;
                    case IrElement::IT_Double:result = visit(expr->asDouble());break;
                    case IrElement::IT_Ptr:result = visit(expr->asPtr());break;
                    case IrElement::IT_Call:result = visit(expr->asCall());break;
                    case IrElement::IT_ReadRef:result = visit(expr->asReadRef()); break;
                    default:
                    _debug << _name << ": visitExpression called on IrElement, which is not an expression";
                        return defaultAnswer();
                }
                return result;
            }
        };
    }
}