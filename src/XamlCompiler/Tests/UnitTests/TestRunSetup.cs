// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;

namespace UnitTests
{
    /// <summary>
    /// Assembly-wide trace configuration.
    ///
    /// The tests load a <c>chk</c> build of the compiler, so <see cref="Debug"/> output is compiled
    /// in. Two sites in <c>TypeResolver</c> (<c>AddClrAssemblies</c> and <c>AddWinmdAssembly</c>)
    /// emit one formatted <c>Debug.WriteLine</c> per duplicate type name, and a managed schema
    /// carries ~6,360 duplicates because every C#/WinRT projection is loaded alongside the winmd it
    /// projects - <c>Microsoft.WinUI</c> vs <c>Microsoft.UI.Xaml.winmd</c>,
    /// <c>Microsoft.Windows.SDK.NET</c> vs <c>Windows.Foundation.UniversalApiContract.winmd</c>,
    /// <c>Microsoft.InteractiveExperiences.Projection</c> vs <c>Microsoft.UI.winmd</c>. Loading both
    /// is deliberate; see the ordering comment in <c>TestHelper.GetRuntimeAssemblyPaths</c>.
    ///
    /// A full run builds 550 such universes, so ~1.75 million messages are produced. While
    /// <c>App.config</c> set <c>&lt;assert logfilename="..."/&gt;</c>, every one of them was written
    /// to disk by <c>DefaultTraceListener</c>, which opens, seeks, writes, flushes and closes the
    /// file on each call. Measured cost: 15.8 of 36.2 minutes, and a log that had grown to 6.77 GB
    /// because it is appended to and never truncated.
    ///
    /// The log existed so that assertion failures stay visible when the modal assert dialog is
    /// disabled, and that is preserved here: <see cref="AssertOnlyTraceListener"/> drops
    /// Write/WriteLine but still records <c>Fail</c>. Measured over a full run, the old log
    /// contained 0 assertions and 100% duplicate-type messages, so nothing diagnostic is lost.
    /// </summary>
    [TestClass]
    public static class TestRunSetup
    {
        internal const string AssertionLogFileName = "UnitTests.assertions.log";

        [AssemblyInitialize]
        public static void AssemblyInitialize(TestContext context)
        {
            // Removes DefaultTraceListener, which is what routes Debug.Write/WriteLine to the log
            // file and to OutputDebugString.
            Trace.Listeners.Clear();
            Trace.Listeners.Add(new AssertOnlyTraceListener());
        }

        [AssemblyCleanup]
        public static void AssemblyCleanup()
        {
            // TestHelper caches one type universe per SchemaMode for the life of the process.
            // Dispose them and drop the compiler statics that reference them, so the run does not
            // end holding every loaded assembly's native metadata open.
            TestHelper.ReleaseCachedUniverses();
        }
    }

    /// <summary>
    /// Records assertion failures and discards ordinary trace output. Assertions keep the behaviour
    /// the old <c>&lt;assert logfilename="..."/&gt;</c> configuration had: they are logged and
    /// execution continues, so a test is not turned into a failure by an assert that did not
    /// previously fail one.
    /// </summary>
    internal sealed class AssertOnlyTraceListener : TraceListener
    {
        private static readonly object s_gate = new object();
        private readonly string _logPath;

        public AssertOnlyTraceListener()
        {
            string dir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            _logPath = Path.Combine(dir, TestRunSetup.AssertionLogFileName);
        }

        // Ordinary Debug/Trace output is discarded; see the class comment on TestRunSetup.
        public override void Write(string message) { }

        public override void WriteLine(string message) { }

        public override void Fail(string message)
        {
            Fail(message, null);
        }

        public override void Fail(string message, string detailMessage)
        {
            var sb = new StringBuilder();
            sb.AppendLine("ASSERTION FAILED: " + message);
            if (!String.IsNullOrEmpty(detailMessage))
            {
                sb.AppendLine(detailMessage);
            }
            sb.AppendLine(new StackTrace(true).ToString());
            string text = sb.ToString();

            // Surfaced in the test's output as well as the log, so a failure is visible without
            // having to go looking for the file.
            Console.Error.Write(text);

            try
            {
                lock (s_gate)
                {
                    File.AppendAllText(_logPath, text);
                }
            }
            catch (IOException)
            {
                // The console copy above is the one that matters; never let logging fail a run.
            }
        }
    }
}
