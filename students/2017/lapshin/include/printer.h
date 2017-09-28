#include "visitors.h"
#include <sstream>
#include <deque>

namespace mathvm {
	struct AstPrinterStyle {
		bool forceBlockBraces{false};
		bool forceExpressionBraces{false};
		bool splitFunctions{true};
		string scopeIndent{"    "};
		string funcBodyIndent{};
	};

	class AstPrinter: public AstVisitor {
	private:
		AstPrinterStyle style;

		bool wasStatement{false};
		deque<string> indents;
		stringstream ss;

		void printType(VarType type);
		void printVar(AstVar const *var);
		void indent();

		void visitExpression(AstNode *expr, bool braced = false);
		void visitBlock(BlockNode *node, bool unbraced = false);
	public:
		static AstPrinterStyle testStyle();
		static AstPrinterStyle prettyStyle();

		AstPrinter(AstPrinterStyle const &style);
		virtual ~AstPrinter() = default;

		string print(AstNode *root);

		#define VISITOR_FUNCTION(type, name) \
		virtual void visit##type(type *node);

		FOR_NODES(VISITOR_FUNCTION)
		#undef VISITOR_FUNCTION
	};
}
