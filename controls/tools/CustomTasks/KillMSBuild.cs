using Microsoft.Build.Framework;
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace CustomTasks
{
    public class KillMSBuild : CustomTask
    {
        [Required]
        public int[] MSBuildProcessIds
        {
            get;
            set;
        }

        public override bool Execute()
        {
            bool success = true;

            foreach (int msBuildProcessId in MSBuildProcessIds.Distinct())
            {
                bool failedToKillProcess = false;
                Process msBuildProcess = Process.GetProcessesByName("MSBuild").Where(p => p.Id == msBuildProcessId && !p.HasExited).SingleOrDefault();

                if (msBuildProcess != null)
                {
                    LogMessage($"Killing MSBuild with process ID {msBuildProcessId}...");
                    msBuildProcess.Kill();

                    bool processKilled = false;

                    for (int attempts = 0; attempts < 10; attempts++)
                    {
                        processKilled = Process.GetProcessesByName("MSBuild").Where(p => p.Id == msBuildProcessId && !p.HasExited).Count() == 0;

                        if (processKilled)
                        {
                            break;
                        }
                        else
                        {
                            LogMessage($"Waiting for process ID {msBuildProcessId} to be killed...");
                            System.Threading.Thread.Sleep(300);
                        }
                    }

                    if (processKilled)
                    {
                        LogMessage($"Successfully killed process ID {msBuildProcessId}.");
                    }
                    else
                    {
                        LogError($"Failed to kill process ID {msBuildProcessId}!");
                        failedToKillProcess = true;
                    }
                }
                else
                {
                    LogWarning($"MSBuild with process ID {msBuildProcessId} not found!");
                }

                if (!failedToKillProcess)
                {
                    // If we didn't fail to kill the process (i.e., either we killed it or it didn't exist),
                    // then we can also clean up the temp file left over to clean this up if the build had been canceled.
                    string tempFilePath = Path.Combine(Environment.GetEnvironmentVariable("TEMP"), $"{msBuildProcessId}.msbproc");
                    if (File.Exists(tempFilePath))
                    {
                        File.Delete(tempFilePath);
                    }
                }
                else
                {
                    success = false;
                }
            }

            return success;
        }
    }
}
