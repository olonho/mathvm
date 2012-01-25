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

	[TestFixture]
	public class NativeAdditionalTests : AdditionalTests
	{
		public override string PrepareArgs(string testPath)
		{
			return testPath + " native silent";
		}
	}
}