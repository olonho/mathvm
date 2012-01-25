using NUnit.Framework;

namespace mathvm.Tests
{
	[TestFixture]
	public class DefaultTests : BaseTest
	{
		[Test]
		public void DivTest()
		{
			PerformTest("div");
		}

		[Test]
		public void AddTest()
		{
			PerformTest("add");
		}

		[Test]
		public void ForTest()
		{
			PerformTest("for");
		}

		[Test]
		public void FunctionTest()
		{
			PerformTest("function");
		}

		[Test]
		public void IfTest()
		{
			PerformTest("if");
		}

		[Test]
		public void LiteralTest()
		{
			PerformTest("literal");
		}

		[Test]
		public void MulTest()
		{
			PerformTest("mul");
		}

		[Test]
		public void ExprTest()
		{
			PerformTest("expr");
		}

		[Test]
		public void SubTest()
		{
			PerformTest("sub");
		}
		[Test]
		public void WhileTest()
		{
			PerformTest("while");
		}
	}
}