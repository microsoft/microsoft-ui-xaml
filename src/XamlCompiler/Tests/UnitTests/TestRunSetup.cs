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
    /// Replaces the default trace listener so ordinary Debug output is discarded while Debug.Assert
    /// failures remain visible and are written to UnitTests.assertions.log.
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
    /// Discards ordinary trace output and logs assertion failures without failing the test process.
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
