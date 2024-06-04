// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal interface IItemBase
    {
        string ItemSpec { get; }
    }

    internal interface IAssemblyItem : IItemBase
    {
        bool IsSystemReference { get; set; }
        bool IsNuGetReference { get; set; }
        bool IsStaticLibraryReference { get; set; }
    }

    internal interface IFileItem : IItemBase
    {
        string DependentUpon { get; set; }
        string FullPath { get; }
        string MSBuild_Link { get; }
        string MSBuild_TargetPath { get; }
        string MSBuild_XamlResourceMapName { get; }
        string MSBuild_XamlComponentResourceLocation { get; }
    }
}
