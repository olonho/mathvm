using System.IO;
using NUnit.Framework;

namespace mathvm.Tests
{
	public class AdditionalTests : BaseTest
	{
		public override string TestsDirectory
		{
			get { return Path.Combine(VmRoot, @"additional\tests"); }
		}

		[Test]
		public void FibClosure()
		{
			PerformTest("fib_closure");
		}
	}
}