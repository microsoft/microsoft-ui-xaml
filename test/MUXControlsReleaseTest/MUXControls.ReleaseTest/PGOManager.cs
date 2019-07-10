using Common;
using System;
using System.Diagnostics;
using System.Text;

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
            Log.Comment("Running pgosweep for test:" + pgcFileName);
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
                        Log.Comment(process.StandardOutput.ReadToEnd());
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Comment("Failed trying to pgosweep. " + ex.ToString());
                throw;
            }
#endif
        }
    }
}
