using System.IO;
using NUnit.Framework;

namespace mathvm.Tests
{
	public class AdditionalTests : BaseTest
	{
		public override string TestsDirectory
		{
			get { return Path.Combine(VmRoot, @"tests\additional"); }
		}

		[Test]
		public void FibClosure()
		{
			PerformTest("fib_closure");
		}

		[Test]
		public void Ackermann()
		{
			PerformTest("ackermann");
		}

		[Test]
		public void AckermannClosure()
		{
			PerformTest("ackermann_closure");
		}

		[Test]
		public void Casts()
		{
			PerformTest("casts");
		}

		[Test]
		public void Complex()
		{
			PerformTest("complex");
		}

		[Test]
		public void Complex2()
		{
			PerformTest("complex2");
		}

		[Test]
		public void Fib()
		{
			PerformTest("fib");
		}

		[Test]
		public void Function()
		{
			PerformTest("function");
		}

		[Test]
		public void FunctionCall()
		{
			PerformTest("function-call");
		}

		[Test]
		public void FunctionCast()
		{
			PerformTest("function-cast");
		}

		[Test]
		public void Vars()
		{
			PerformTest("vars");
		}
	}
}