#pragma once

#include <vector>
#include <memory>
#include <deque>
#include <set>
#include "../util.h"

namespace mathvm {
    namespace IR {

#define IR_COMMON_FUNCTIONS(ir) \
virtual IrElement* visit(IrVisitor *const v) const { v->visit(this); }\
virtual bool is##ir() const { return true; } ;\
virtual ir const* as##ir() const { return this; } ;\
virtual IrType getType() const { return IT_##ir; }

#define FOR_IR(DO) \
        DO(BinOp)\
        DO(UnOp)\
        DO(Variable)\
        DO(Return)\
        DO(Phi)\
        DO(Int)\
        DO(Double)\
        DO(Ptr)\
        DO(Block)\
        DO(Assignment)\
        DO(Call)\
        DO(Print)\
        DO(FunctionRecord)\
        DO(JumpAlways)\
        DO(JumpCond)

#define DECLARE_IR(ir) struct ir;
        FOR_IR(DECLARE_IR)
#undef DECLARE_IR

        struct IrVisitor;

        class IrElement {
        public:
            enum IrType {

#define DECLARE_IR_TYPE(ir) IT_##ir,
                FOR_IR(DECLARE_IR_TYPE)
                IT_INVALID
#undef DECLARE_IR_TYPE
            };

            virtual IrElement *visit(IrVisitor *const visitor) const = 0;

#define HELPER(ir) virtual bool is##ir() const { return false; } ; virtual ir const* as##ir() const { return NULL; } ;

            FOR_IR(HELPER)
#undef HELPER
        };

        struct IrVisitor {
#define VISITORABSTR(ir) virtual IrElement *visit(const ir *const expr)  = 0;

            FOR_IR(VISITORABSTR)

#undef VISITORABSTR

#define VISITOR(ir) virtual IrElement *visit(const ir * const expr) ;

        };


        struct Expression : IrElement {
        };
        struct Atom : Expression {
        };

        enum VarType {
            VT_Undefined,
            VT_Bot,
            VT_Int,
            VT_Double,
            VT_Ptr
        };

        struct Variable : Atom {
            Variable(uint64_t id) : id(id) {
            }

            const uint64_t id;

            IR_COMMON_FUNCTIONS(Variable)
        };

        struct Statement : IrElement {
        };

        struct Jump : IrElement {
            virtual ~Jump() {
            }
        };


        struct JumpAlways : Jump {
            std::shared_ptr<Block> const destination;

            JumpAlways(Block *dest) : destination(dest) {
            }

            IR_COMMON_FUNCTIONS(JumpAlways)


        };

        struct JumpCond : Jump {
            JumpCond(Block *const yes, Block *const no, Atom const *const condition)
                    : yes(yes), no(no), condition(condition) {
            }

            Block *const yes;
            Block *const no;
            const std::shared_ptr<const Atom> condition;


            IR_COMMON_FUNCTIONS(JumpCond)
        };

        struct Assignment : Statement {
            const std::shared_ptr<const Variable> var;
            const std::shared_ptr<const Expression> value;

            Assignment(Variable const * const var, Expression const *const expr) : var(var), value(expr) {
            }
            Assignment(uint64_t id, Expression const *const expr) : var(new Variable(id)), value(expr) {
            }

            IR_COMMON_FUNCTIONS(Assignment)

        };

        struct Return : Statement {
            Return(const Atom *const atom) : atom(atom) {
            }

            const std::shared_ptr<Atom const> atom;

            IR_COMMON_FUNCTIONS(Return)

        };

        struct Phi : Statement {
            std::shared_ptr<const Variable> var;

            Phi(uint64_t id) : var(new Variable(id)) {
            }

            Phi(Variable const *id) : var(id) {
            }

            std::set<std::shared_ptr<Variable const> > vars;

            IR_COMMON_FUNCTIONS(Phi)

        };

        struct Call : Expression {
            Call(uint16_t id, std::vector<Atom const *> const &params) : funId(id), params(params) {

            }

            const std::vector<Atom const *> params;
            const uint16_t funId;

            IR_COMMON_FUNCTIONS(Call)
        };

        struct BinOp : Expression {

            const std::shared_ptr<Expression const> left;
            const std::shared_ptr<Expression const> right;
#define FOR_IR_BINOP(DO) \
DO(ADD, "+")\
DO(SUB, "-")\
DO(MUL, "*")\
DO(DIV, "/")\
DO(MOD, "%")\
DO(LT, "<")\
DO(LE, "<=")\
DO(EQ, "==")\
DO(NEQ, "!=")\
DO(OR, "|")\
DO(AND, "&")\
DO(LOR, "||")\
DO(LAND, "&&")\
DO(XOR, "^")

#define BINOP_ENUM(name, _) BO_##name,
            enum Type {
                FOR_IR_BINOP(BINOP_ENUM)
                BO_INVALID
            };
            const Type type;

            BinOp(Expression const *left, Expression const *right, Type type) : left(left), right(right), type(type) {
            }

            IR_COMMON_FUNCTIONS(BinOp)
        };

        struct UnOp : Expression {
            const std::shared_ptr<Expression const> operand;
#define FOR_IR_UNOP(DO)\
DO(CAST_I2D, "<i2d>")\
DO(CAST_D2I,"<d2i>")\
DO(CAST_P2I,"<p2i>")\
DO(CAST_I2P, "<i2p>")\
DO(NEG, "-")\
DO(NOT, "!")
#define UNOP_NAME(o, _) UO_##o,

            enum Type {
                FOR_IR_UNOP(UNOP_NAME)
                UO_INVALID
            };
#undef UNOP_NAME

            const Type type;

            UnOp(const Expression *const operand, Type type) : operand(operand), type(type) {
            }

            IR_COMMON_FUNCTIONS(UnOp)

        };

        struct Int : Atom {
            const int64_t value;

            Int(int64_t value) : value(value) {
            }

            IR_COMMON_FUNCTIONS(Int)
        };

        struct Double : Atom {
            const double value;

            Double(double value) : value(value) {
            }

            IR_COMMON_FUNCTIONS(Double)
        };

        struct Ptr : Atom {
            const uint64_t value;
            const bool isPooledString;

            Ptr(uint64_t value, bool isPooledString) : value(value), isPooledString(isPooledString) {
            }
//            Ptr(Ptr const& ptr) : value(ptr.value), isPooledString(ptr.isPooledString) {
//            }

            IR_COMMON_FUNCTIONS(Ptr)
        };


        struct Block : IrElement {
        private:
            Jump *_transition;
        public:
            const std::string name;

            Jump *getTransition() const {
                return _transition;
            }

            void setTransition(Jump *jmp) {
                _transition = jmp;
            }


            std::vector<const Block *> predecessors;

            Block(std::string const &name) : name(name), _transition(NULL) {
            }


            void link(JumpCond *cond) {
                _transition = cond;
                cond->yes->predecessors.push_back(this);
                cond->no->predecessors.push_back(this);
            }

            void link(Block *next) {
                _transition = new JumpAlways(next);
                next->predecessors.push_back(this);
            }

            std::deque<Statement const *> contents;


            virtual ~Block() {
                for (auto s : contents)
                    delete s;
            }

            IR_COMMON_FUNCTIONS(Block)
        };


        class FunctionRecord : public IrElement {

        public:
            FunctionRecord(uint16_t id, VarType returnType)
                    : id(id), entry(new Block(mathvm::toString(id))), returnType(returnType) {
            }

            FunctionRecord(uint16_t id, VarType returnType, Block *startBlock)
                    : id(id), entry(startBlock), returnType(returnType) {
            }

            const uint16_t id;
            std::shared_ptr<Block> entry;
            std::vector<uint64_t> parametersIds;
            //should we add variables here?



            VarType returnType;

            IR_COMMON_FUNCTIONS(FunctionRecord)


        };

        struct Print : Statement {
            Print(Atom const *const atom) : atom(atom) {
            }

            const std::shared_ptr<const Atom> atom;

            IR_COMMON_FUNCTIONS(Print)
        };

        struct SimpleIr {
            struct VarMeta {
                const uint64_t id;
                const bool isSourceVar;
                const uint64_t originId;
                VarType type;

                VarMeta(uint64_t id, uint64_t from, VarType type)
                        : isSourceVar(true),originId(from), id(id), type(type) {
                }

                VarMeta(uint64_t id) : isSourceVar(false), originId(0), id(id), type(VT_Undefined) {
                }
            };

            typedef std::vector<std::string> StringPool;

            StringPool pool;
            std::vector<FunctionRecord *> functions;
            std::vector<VarMeta> varMeta;

        };
        typedef SimpleIr SimpleSsaIr;
    }
}