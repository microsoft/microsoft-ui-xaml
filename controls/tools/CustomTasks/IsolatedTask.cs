using Microsoft.Build.Framework;
using System;
using System.IO;

namespace CustomTasks
{
    // Due to a bug in Visual Studio, if we try to unload multiple app domains in the same process,
    // we'll hit a deadlock.  In order to avoid that, we'll have all our isolated tasks derive from
    // this class, which runs in a separate process that we then manually kill after all tasks
    // running in it have completed.
    public abstract class IsolatedTask : CustomTask
    {
        [Output]
        public int MSBuildProcessId
        {
            get;
            set;
        }

        private static bool processIsInUse = false;

        private class ProcessUseMonitor : IDisposable
        {
            private readonly IsolatedTask parentTask;

            public ProcessUseMonitor(IsolatedTask parentTask)
            {
                this.parentTask = parentTask;
                parentTask.LogMessage($"Isolated task ({parentTask.GetType().Name}) has begun using MSBuild.exe with process ID {parentTask.MSBuildProcessId}.");
                IsolatedTask.processIsInUse = true;
            }

            public void Dispose()
            {
                parentTask.LogMessage($"Isolated task ({parentTask.GetType().Name}) is done using MSBuild.exe with process ID {parentTask.MSBuildProcessId}.");
                IsolatedTask.processIsInUse = false;
            }
        }

        public abstract bool ExecuteCore();

        public override bool Execute()
        {
            if (processIsInUse)
            {
                throw new InvalidOperationException("Isolated tasks must be run using TaskFactory=\"TaskHostFactory\" followed by KillMSBuild, in order to ensure no two tasks are run in the same process.");
            }

            MSBuildProcessId = System.Diagnostics.Process.GetCurrentProcess().Id;

            // We'll create a temp file to flag this process as needing cleanup,
            // in case the build is canceled and we aren't able to do so.
            string tempFilePath = Path.Combine(Environment.GetEnvironmentVariable("TEMP"), $"{MSBuildProcessId}.msbproc");
            if (!File.Exists(tempFilePath))
            {
                File.WriteAllText(tempFilePath, string.Empty);
            }

            using (new ProcessUseMonitor(this))
            {
                return ExecuteCore();
            }
        }
    }
}
