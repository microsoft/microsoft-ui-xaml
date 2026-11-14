// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop
{
    // Represents a serializable version of all necessary data for compiler inputs obtained from MSBuild.  Because CompilerInputs needs to be serializable, all of its property types must be concrete classes, not interfaces.
    // The actual compiler may process this into different forms, e.g. an array of filenames instead of a semi-colon separated list of filenames
    public sealed class CompilerInputs
    {
        public CompilerInputs()
        {

        }

        // The version of this contract the producer (the CompileXaml task) implements: see CompilerContract
        public int ContractVersion { get; set; }

        // Whether this is a design-time (IntelliSense) build. Set by the task from CompileMode and from the IDE host object, if
        // any; the compiler also treats CompileMode=DesignTimeBuild as a design-time build, for the inputs of older tasks.
        public bool IsDesignTimeBuild { get; set; }

        // Optional path to a JSON list of FileOverlay: the contents and/or last change times of the XAML files open in the IDE
        // (design-time builds only). The task deletes it after the run.
        public string FileOverlaysPath { get; set; }

        public string ProjectPath { get; set; }

        public string Language { get; set; }

        public string LanguageSourceExtension { get; set; }

        public string OutputPath { get; set; }

        public List<MSBuildItem> ReferenceAssemblies { get; set; }

        public string TargetPlatformMinVersion { get; set; }

        public List<MSBuildItem> ReferenceAssemblyPaths { get; set; }

        public string BuildConfiguration { get; set; }

        public bool ForceSharedStateShutdown { get; set; }

        public bool DisableXbfGeneration { get; set; }

        public bool DisableXbfLineInfo { get; set; }

        public bool EnableXBindDiagnostics { get; set; }

        public List<MSBuildItem> ClIncludeFiles { get; set; }

        public string CIncludeDirectories { get; set; }

        public List<MSBuildItem> XamlApplications { get; set; }

        public List<MSBuildItem> XamlPages { get; set; }

        public List<MSBuildItem> LocalAssembly { get; set; }

        public List<MSBuildItem> SdkXamlPages { get; set; }

        public string ProjectName { get; set; }

        public bool IsPass1 { get; set; }

        public string RootNamespace { get; set; }

        public string OutputType { get; set; }

        public string PriIndexName { get; set; }

        public string CodeGenerationControlFlags { get; set; }

        public string FeatureControlFlags { get; set; }

        public bool XAMLFingerprint { get; set; }

        public bool UseVCMetaManaged { get; set; } = true;

        public string[] FingerprintIgnorePaths { get; set; }

        // VCInstallDir gives us the directory where vcmeta.dll is located.
        // Deprecated, as we hardcode the paths of vcmeta.dll based off VCInstallDir,
        // which has been inaccurate.  However as we rely on VS to supply us with the
        // newer VCInstallPath32 and VCInstallPath64, we still need to support it
        // when running with older versions of Visual Studio.
        public string VCInstallDir { get; set; }

        // The exact paths to the 32-bit and 64-bit vcmeta.dll binaries.
        // If these are supplied, they take priority over the deprecated
        // VCInstallDir value.
        public string VCInstallPath32 { get; set; }
        public string VCInstallPath64 { get; set; }

        public string WindowsSdkPath { get; set; }

        public string CompileMode { get; set; }

        public string SavedStateFile { get; set; }

        public string RootsLog { get; set; }

        public string SuppressWarnings { get; set; }

        public string GenXbfPath { get; set; }
        public string PrecompiledHeaderFile { get; set; }
        public string XamlResourceMapName { get; set; }
        public string XamlComponentResourceLocation { get; set; }
        public string XamlPlatform { get; set; }
        public string TargetFileName { get; set; }

        // Controls whether or not the TargetPlatformMinVersion specified in the project should
        // be ignored. For now, TargetPlatformMinVersion is used for:
        // 1. validation of feature usage (platform API, x:Bind functionality,
        // conditional XAML, etc.)
        // 2. XBF format version
        public bool IgnoreSpecifiedTargetPlatformMinVersion { get; set; }

        public string EnabledXamlOptionalChanges { get; set; }
        public string DisabledXamlOptionalChanges { get; set; }
    }
}
