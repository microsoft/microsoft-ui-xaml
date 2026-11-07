// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.Executable
{
    /// <summary>
    /// Replaces the default trace listener of the compiler process, which terminates the process when a Debug.Assert fails (.NET
    /// has no assert dialog): in the Debug builds of the compiler, a failed assertion is reported as a diagnostic message of the
    /// compilation instead (and to an attached debugger), which then goes on and writes its results, like the in-process compiler
    /// task used to (it disabled the assert dialogs). The compiler has no Trace.Assert, so this only matters for Debug builds.
    /// </summary>
    internal sealed class AssertionTraceListener : TraceListener
    {
        private readonly Action<string> _report;
        private readonly object _gate = new object();

        private AssertionTraceListener(Action<string> report)
        {
            _report = report;
        }

        public static void Install(Action<string> report)
        {
            Trace.Listeners.Clear();
            Trace.Listeners.Add(new AssertionTraceListener(report));
        }

        // Debug.Write/WriteLine only go to an attached debugger, like with the default listener
        public override void Write(string message)
        {
            if (Debugger.IsLogging())
            {
                Debugger.Log(0, null, message);
            }
        }

        public override void WriteLine(string message)
        {
            Write(message + Environment.NewLine);
        }

        public override void Fail(string message)
        {
            Fail(message, null);
        }

        public override void Fail(string message, string detailMessage)
        {
            StringBuilder text = new StringBuilder("XAML compiler assertion failed: ").Append(message);
            if (!string.IsNullOrEmpty(detailMessage))
            {
                text.Append(' ').Append(detailMessage);
            }
            text.AppendLine().Append(new StackTrace(1, false));

            WriteLine(text.ToString());
            lock (_gate)
            {
                _report(text.ToString());
            }
        }
    }
}
