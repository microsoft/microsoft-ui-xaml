using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace MUXControls.ReleaseTest
{
    public static class PGOManager
    {
        public static void PGOSweepIfInstrumented(string pgcFileName, string instumentedAssemblyName = "Microsoft.ui.xaml.dll")
        {
#if PGO_INSTRUMENT            
            LogOutput("Running pgosweep for test:" + pgcFileName);
            try
            {
                var startInfo = new ProcessStartInfo() {
                    FileName = "pgosweep.exe",
                    Arguments = "microsoft.ui.xaml.dll " + pgcFileName + ".pgc",
                    UseShellExecute = false,
                    RedirectStandardOutput = true
                };
                using (var process = Process.Start(startInfo))
                {
                    var output = new StringBuilder();
                    while (!process.HasExited)
                    {
                        LogOutput(process.StandardOutput.ReadToEnd());
                    }
                }
            }
            catch (Exception ex)
            {
                LogOutput("Failed trying to pgosweep. " + ex.ToString());
                throw;
            }
#endif
        }

        public static void LogOutput(string message)
        {
#if USING_TAEF
            Log.Comment(message);
#else
            Console.WriteLine(message);
#endif
        }
    }
}
