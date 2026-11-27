// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    // An MSBuild item (XAML file, reference assembly, ...) with the metadata the compiler uses: see TaskItems.ToMSBuildItem in the
    // CompileXaml task (Microsoft.UI.Xaml.Markup.Compiler.Tasks) for how they're computed
    public sealed class MSBuildItem : IFileItem, IAssemblyItem
    {
        public string DependentUpon { get; set; }

        public string FullPath { get; set; }

        public string ItemSpec { get; set; }

        public bool IsSystemReference { get; set; }

        public bool IsNuGetReference { get; set; }

        public bool IsStaticLibraryReference { get; set; }

        public string MSBuild_Link { get; set; }

        public string MSBuild_TargetPath { get; set; }

        public string MSBuild_XamlResourceMapName { get; set; }

        public string MSBuild_XamlComponentResourceLocation { get; set; }
    }
}
