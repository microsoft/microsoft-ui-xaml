// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public sealed class CompilerOutputs
    {
        // The version of this contract the compiler implements: see CompilerContract
        public int ContractVersion { get; set; }

        public IList<string> GeneratedCodeFiles { get; set; }

        public IList<string> GeneratedXamlFiles { get; set; }

        public IList<string> GeneratedXbfFiles { get; set; }

        public IList<string> GeneratedXamlPagesFiles { get; set; }

        public IList<MSBuildLogEntry> MSBuildLogEntries { get; set; }
    }
}
