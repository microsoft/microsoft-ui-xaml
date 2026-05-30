// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public class AssemblyItem
    {
        public AssemblyItem()
        {
        }

        public string ItemSpec { get; set; }
        public bool IsSystemReference { get; set; }
        public bool IsNuGetReference { get; set; }
        public bool IsStaticLibraryReference { get; set; }
    }
}
