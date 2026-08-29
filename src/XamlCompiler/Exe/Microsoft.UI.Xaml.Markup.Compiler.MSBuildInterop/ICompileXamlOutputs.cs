// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public interface ICompileXamlOutputs
    {
        [Output]
        ITaskItem[] GeneratedCodeFiles { get; }

        [Output]
        ITaskItem[] GeneratedXamlFiles { get; }

        [Output]
        ITaskItem[] GeneratedXbfFiles { get; }

        [Output]
        ITaskItem[] GeneratedXamlPagesFiles { get; }
    }
}
