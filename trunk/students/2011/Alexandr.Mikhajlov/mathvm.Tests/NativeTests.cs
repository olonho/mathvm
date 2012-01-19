using NUnit.Framework;

namespace mathvm.Tests
{
	[TestFixture]
	class NativeTests : DefaultTests
	{
		public override string PrepareArgs(string testPath)
		{
			return testPath + " native silent";
		}
	}
}