using System;
using System.Threading;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;


namespace MUXControls.Test
{
    internal class DebugHelper
    {
        public static void Debug(TestContext context)
        {
            if (context.Properties.Contains("WaitForDebugger") || context.Properties.Contains("WaitForAppDebugger"))
            {
                var processId = global::Windows.System.Diagnostics.ProcessDiagnosticInfo.GetForCurrentProcess().ProcessId;
                while (!System.Diagnostics.Debugger.IsAttached)
                {
                    Log.Comment(string.Format("Waiting for a debugger to attach (processId = {0})...", processId));
                    Thread.Sleep(1000);
                }
            }
        }
    }
}
