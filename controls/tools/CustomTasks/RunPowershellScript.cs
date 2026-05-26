// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
using System.IO;
using System.Linq;
using System.Management.Automation;
using System.Management.Automation.Runspaces;
using System.Text.RegularExpressions;

namespace CustomTasks
{
    public class RunPowershellScript : IsolatedTask
    {
        [Required]
        public string Path
        {
            get;
            set;
        }

        public string Parameters
        {
            get;
            set;
        }

        public string[] FilesWritten
        {
            get;
            set;
        }

        [Output]
        public string[] Messages
        {
            get;
            set;
        }

        [Output]
        public string[] Warnings
        {
            get;
            set;
        }

        [Output]
        public string[] Errors
        {
            get;
            set;
        }

        [Output]
        public string ExitCode
        {
            get;
            set;
        }

        private readonly Regex errorInFileRegex = new Regex(@"{ERROR:([^\|]+)\|([^\|]+)\|([^\|]+)\|([^\|]+)}", RegexOptions.Compiled | RegexOptions.IgnoreCase);

        public override bool ExecuteCore()
        {
            var sessionState = InitialSessionState.CreateDefault();
            sessionState.ExecutionPolicy = Microsoft.PowerShell.ExecutionPolicy.Bypass;

            using (PowerShell powerShellInstance = PowerShell.Create(sessionState))
            {
                string expression = Path ?? string.Empty;

                if (!string.IsNullOrEmpty(Parameters))
                {
                    expression += " " + Parameters.Replace("\"", "`\"");
                }

                LogMessage("Executing \"" + expression + "\"...");
                powerShellInstance.AddScript("Invoke-Expression \"" + expression + "\"", true /* useLocalScope */);
                powerShellInstance.Streams.Information.DataAdded += OnInformationDataAdded;
                powerShellInstance.Streams.Warning.DataAdded += OnWarningDataAdded;
                powerShellInstance.Streams.Error.DataAdded += OnErrorDataAdded;

                powerShellInstance.Invoke();

                Messages = powerShellInstance.Streams.Information.Select(record => record.MessageData.ToString()).ToArray();
                Warnings = powerShellInstance.Streams.Warning.Select(record => record.ToString()).ToArray();
                Errors = powerShellInstance.Streams.Error.Select(record => record.ToString()).ToArray();

                var lastExitCode = powerShellInstance.Runspace.SessionStateProxy.GetVariable("LASTEXITCODE");

                if (lastExitCode != null)
                {
                    ExitCode = lastExitCode.ToString();
                }
                else
                {
                    // Absent an exit code to the contrary, we'll assume an exit code of 0 (success).
                    ExitCode = "0";
                }
            }

            // If the script failed, then we should delete any files it wrote to, as they'll be in an unknown state.
            if (HasLoggedErrors && FilesWritten?.Length > 0)
            {
                foreach (string filePath in FilesWritten)
                {
                    if (File.Exists(filePath))
                    {
                        File.Delete(filePath);
                    }
                }
            }

            return !HasLoggedErrors;
        }

        private T GetNewRecord<T>(object sender, DataAddedEventArgs e)
        {
            return ((PSDataCollection<T>)sender)[e.Index];
        }

        private void OnInformationDataAdded(object sender, DataAddedEventArgs e)
        {
            var information = GetNewRecord<InformationRecord>(sender, e);
            var messageData = information.MessageData.ToString();
            var foregroundColor = (information.MessageData as HostInformationMessage)?.ForegroundColor;
            var backgroundColor = (information.MessageData as HostInformationMessage)?.BackgroundColor;

            if (!string.IsNullOrWhiteSpace(messageData))
            {
                MatchCollection matches = errorInFileRegex.Matches(messageData);

                if (matches.Count > 0)
                {
                    foreach (Match match in matches)
                    {
                        if (match.Groups.Count >= 5)
                        {
                            LogError(null, null, null, match.Groups[2].Value, int.Parse(match.Groups[3].Value), int.Parse(match.Groups[4].Value), match.Groups[1].Value);
                        }
                    }
                }
                else
                {
                    LogMessage(messageData, foregroundColor, backgroundColor);
                }
            }
        }

        private void OnWarningDataAdded(object sender, DataAddedEventArgs e)
        {
            var warning = GetNewRecord<WarningRecord>(sender, e);
            LogWarning(null, null, null, warning.InvocationInfo.ScriptName, warning.InvocationInfo.ScriptLineNumber, warning.InvocationInfo.OffsetInLine, warning.ToString());
        }

        private void OnErrorDataAdded(object sender, DataAddedEventArgs e)
        {
            var error = GetNewRecord<ErrorRecord>(sender, e);
            LogError(null, null, null, error.InvocationInfo.ScriptName, error.InvocationInfo.ScriptLineNumber, error.InvocationInfo.OffsetInLine, error.ToString());
        }
    }
}
