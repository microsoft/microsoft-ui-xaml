// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public interface IItemBase
    {
        string ItemSpec { get; }
    }

    public interface IAssemblyItem : IItemBase
    {
        bool IsSystemReference { get; }
        bool IsNuGetReference { get; }
        bool IsStaticLibraryReference { get; }
    }

    public interface IFileItem : IItemBase
    {
        string DependentUpon { get; }
        string FullPath { get; }
        string MSBuild_Link { get; }
        string MSBuild_TargetPath { get; }
        string MSBuild_XamlResourceMapName { get; }
        string MSBuild_XamlComponentResourceLocation { get; }
    }
}
