// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

namespace Microsoft.UI.Xaml.Markup.Compiler.IO
{
    /// <summary>
    /// Computes the environment variable that selects the GC mode of the native XAML compiler, which is launched as a
    /// child process (so this never changes the environment of MSBuild itself). The native compiler links both GC modes,
    /// and picks one at startup from the DOTNET_gcServer environment variable. Its built-in default is Server GC, so
    /// the workstation GC default (which measured as fast as Server GC on real apps, with far fewer threads and less
    /// memory) is selected explicitly here.
    /// </summary>
    public class GetXamlCompilerGCMode : Task
    {
        private const string GCServerVariableName = "DOTNET_gcServer";

        /// <summary>
        /// Gets or sets whether the compiler uses Server GC: 'true', 'false', 'auto' to select it from the machine
        /// (see <see cref="XamlCompilerGCPolicy"/>), or empty for the default, which is workstation GC.
        /// </summary>
        public string EnableServerGC { get; set; }

        /// <summary>Gets the 'NAME=value' environment variable to set for the compiler process, or empty to leave its environment as is.</summary>
        [Output]
        public string EnvironmentVariable { get; set; } = string.Empty;

        public override bool Execute()
        {
            bool enabled;
            if (string.IsNullOrEmpty(EnableServerGC))
            {
                if (!string.IsNullOrEmpty(Environment.GetEnvironmentVariable(GCServerVariableName)))
                {
                    // The user already chose the GC mode in the environment, which the compiler process inherits
                    Log.LogMessage(MessageImportance.Low, "XAML compiler {0}={1} (inherited).", GCServerVariableName, Environment.GetEnvironmentVariable(GCServerVariableName));

                    return true;
                }

                enabled = false;
            }
            else if (string.Equals(EnableServerGC, "auto", StringComparison.OrdinalIgnoreCase))
            {
                enabled = XamlCompilerGCPolicy.ShouldUseServerGC();
            }
            else if (!bool.TryParse(EnableServerGC, out enabled))
            {
                Log.LogError("Invalid 'XamlCompilerEnableServerGC' value '{0}'. It must be empty (workstation GC), 'true', 'false', or 'auto'.", EnableServerGC);

                return false;
            }

            EnvironmentVariable = GCServerVariableName + (enabled ? "=1" : "=0");

            Log.LogMessage(MessageImportance.Low, "XAML compiler {0}.", EnvironmentVariable);

            return true;
        }
    }
}
