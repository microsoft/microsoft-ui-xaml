// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    public interface ICompileXamlInputs
    {
        [Required]
        string ProjectPath { get; set; }

        [Required]
        string Language { get; set; }

        [Required]
        string LanguageSourceExtension { get; set; }

        [Required]
        string OutputPath { get; set; }

        [Required]
        ITaskItem[] ReferenceAssemblies { get; set; }

        [Required]
        string TargetPlatformMinVersion { get; set; }

        [Required]
        ITaskItem[] ReferenceAssemblyPaths { get; set; }

        string BuildConfiguration { get; set; }

        bool ForceSharedStateShutdown { get; set; }

        bool DisableXbfGeneration { get; set; }

        bool DisableXbfLineInfo { get; set; }

        bool EnableXBindDiagnostics { get; set; }

        ITaskItem[] ClIncludeFiles { get; set; }

        String CIncludeDirectories { get; set; }

        ITaskItem[] XamlApplications { get; set; }

        ITaskItem[] XamlPages { get; set; }

        ITaskItem[] LocalAssembly { get; set; }

        ITaskItem[] SdkXamlPages { get; set; }

        string ProjectName { get; set; }

        bool IsPass1 { get; set; }

        string RootNamespace { get; set; }

        string OutputType { get; set; }

        string PriIndexName { get; set; }

        string CodeGenerationControlFlags { get; set; }

        string FeatureControlFlags { get; set; }

        bool XAMLFingerprint { get; set; }

        bool UseVCMetaManaged { get; set; }

        string[] FingerprintIgnorePaths { get; set; }

        // VCInstallDir gives us the directory where vcmeta.dll is located.
        // Deprecated, as we hardcode the paths of vcmeta.dll based off VCInstallDir,
        // which has been inaccurate.  However as we rely on VS to supply us with the
        // newer VCInstallPath32 and VCInstallPath64, we still need to support it
        // when running with older versions of Visual Studio.
        string VCInstallDir { get; set; }

        // The exact paths to the 32-bit and 64-bit vcmeta.dll binaries.
        // If these are supplied, they take priority over the deprecated
        // VCInstallDir value.
        string VCInstallPath32 { get; set; }
        string VCInstallPath64 { get; set; }

        string WindowsSdkPath { get; set; }

        string CompileMode { get; set; }

        string SavedStateFile { get; set; }

        string RootsLog { get; set; }

        string SuppressWarnings { get; set; }

        string GenXbfPath { get; set; }
        string PrecompiledHeaderFile { get; set; }
        string XamlResourceMapName { get; set; }
        string XamlComponentResourceLocation { get; set; }
        string XamlPlatform { get; set; }
        string TargetFileName { get; set; }

        // Controls whether or not the TargetPlatformMinVersion specified in the project should
        // be ignored. For now, TargetPlatformMinVersion is used for:
        // 1. validation of feature usage (platform API, x:Bind functionality,
        // conditional XAML, etc.)
        // 2. XBF format version
        bool IgnoreSpecifiedTargetPlatformMinVersion { get; set; }
    }
}
