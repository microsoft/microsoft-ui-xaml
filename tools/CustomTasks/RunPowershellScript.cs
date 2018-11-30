// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Management.Automation;
using System.IO;
using Microsoft.Build.Framework;
using System.Text.RegularExpressions;
using System.Management.Automation.Runspaces;

namespace CustomTasks
{
    public class RunPowershellScript : Microsoft.Build.Utilities.AppDomainIsolatedTask
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

        private Regex errorInFileRegex = new Regex(@"{ERROR:([^\|]+)\|([^\|]+)\|([^\|]+)\|([^\|]+)}", RegexOptions.Compiled | RegexOptions.IgnoreCase);

        public override bool Execute()
        {
            bool wasError = false;

            var sessionState = InitialSessionState.CreateDefault();
            sessionState.ExecutionPolicy = Microsoft.PowerShell.ExecutionPolicy.Bypass;

            using (PowerShell powerShellInstance = PowerShell.Create(sessionState))
            {
                string expression = Path ?? string.Empty;

                if (!string.IsNullOrEmpty(Parameters))
                {
                    expression += " " + Parameters.Replace("\"", "`\"");
                }

                Log.LogMessage("Executing \"" + expression + "\"...");
                powerShellInstance.AddScript("Invoke-Expression \"" + expression + "\"", true /* useLocalScope */);
                powerShellInstance.Invoke();

                foreach (ErrorRecord error in powerShellInstance.Streams.Error)
                {
                    Log.LogError(null, null, null, error.InvocationInfo.ScriptName, error.InvocationInfo.ScriptLineNumber, error.InvocationInfo.OffsetInLine, 0, 0, error.ToString());
                    wasError = true;
                }

                foreach (InformationRecord information in powerShellInstance.Streams.Information)
                {
                    string messageData = information.MessageData as string;

                    if (!string.IsNullOrWhiteSpace(messageData))
                    {
                        MatchCollection matches = errorInFileRegex.Matches(messageData);

                        if (matches.Count > 0)
                        {
                            foreach (Match match in errorInFileRegex.Matches(messageData))
                            {
                                if (match.Groups.Count >= 5)
                                {
                                    Log.LogError(null, null, null, match.Groups[2].Value, int.Parse(match.Groups[3].Value), int.Parse(match.Groups[4].Value), 0, 0, match.Groups[1].Value);
                                    wasError = true;
                                }
                            }
                        }
                        else
                        {
                            Log.LogMessage(messageData);
                        }
                    }
                }

                foreach (WarningRecord warning in powerShellInstance.Streams.Warning)
                {
                    Log.LogWarning(null, null, null, warning.InvocationInfo.ScriptName, warning.InvocationInfo.ScriptLineNumber, warning.InvocationInfo.OffsetInLine, 0, 0, warning.ToString());
                }
            }

            // If the script failed, then we should delete any files it wrote to, as they'll be in an unknown state.
            if (wasError && FilesWritten != null && FilesWritten.Length > 0)
            {
                foreach (string filePath in FilesWritten)
                {
                    if (File.Exists(filePath))
                    {
                        File.Delete(filePath);
                    }
                }
            }

            return !wasError;
        }
    }
}
