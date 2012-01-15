using System;
using System.Diagnostics;
using System.IO;
using NUnit.Framework;

namespace mathvm.Tests
{
	public abstract class BaseTest
	{
		public virtual string ExecutablePath { get { return @"..\..\..\x64\Debug\mathvm.exe"; } }
		public virtual string TestsDirectory { get { return Path.Combine(VmRoot, "tests"); } }
		public virtual string VmRoot { get { return @"..\..\..\..\..\..\"; } }

		public void PerformTest(string testName)
		{
			var test = Path.ChangeExtension(testName, "mvm");
			test = Path.Combine(TestsDirectory, test);
			var info = new ProcessStartInfo(ExecutablePath) {Arguments = PrepareArgs(test), UseShellExecute = false, RedirectStandardError = true, RedirectStandardOutput = true};

			var process = Process.Start(info);

			var errors = process.StandardError;
			if (!errors.EndOfStream) Assert.Fail(errors.ReadToEnd());

			var result = process.StandardOutput;
			var expect = new StreamReader(GetExpectedFilePath(test));
			CompareRun(result, expect);
		}

		private void CompareRun(StreamReader result, StreamReader expected)
		{
			while (true)
			{
				var line1 = result.ReadLine();
				var line2 = expected.ReadLine();
				if (line1 == null && line2 == null) return;
				if (line1 == null || line2 == null) Assert.Fail("Result lines number mismatch");

				if (line1 == "Press any key to continue . . . ") line1 = result.ReadLine();

				Console.WriteLine("R: {0}", line1);
				Console.WriteLine("E: {0}\n", line2);

				Assert.AreEqual(line2, line1);
			}
		}

		public virtual string PrepareArgs (string testPath)
		{
			return testPath;
		}

		public string GetExpectedFilePath(string testPath)
		{
			return Path.ChangeExtension(testPath, "expect");
		}
	 
	}
}