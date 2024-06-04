// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
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

        public static CompilerInputs FromMSBuildTaskInputs(ICompileXamlInputs i)
        {
            CompilerInputs r = new CompilerInputs();
            r.BuildConfiguration = i.BuildConfiguration;
            r.CIncludeDirectories = i.CIncludeDirectories;
            r.ClIncludeFiles = GetMSBuildItems(i.ClIncludeFiles);
            r.CodeGenerationControlFlags = i.CodeGenerationControlFlags;
            r.CompileMode = i.CompileMode;
            r.DisableXbfGeneration = i.DisableXbfGeneration;
            r.DisableXbfLineInfo = i.DisableXbfLineInfo;
            r.EnableXBindDiagnostics = i.EnableXBindDiagnostics;
            r.FeatureControlFlags = i.FeatureControlFlags;
            r.FingerprintIgnorePaths = i.FingerprintIgnorePaths;
            r.ForceSharedStateShutdown = i.ForceSharedStateShutdown;
            r.GenXbfPath = i.GenXbfPath;
            r.PrecompiledHeaderFile = i.PrecompiledHeaderFile;
            r.IgnoreSpecifiedTargetPlatformMinVersion = i.IgnoreSpecifiedTargetPlatformMinVersion;
            r.IsPass1 = i.IsPass1;
            r.Language = i.Language;
            r.LanguageSourceExtension = i.LanguageSourceExtension;
            r.LocalAssembly = GetMSBuildItems(i.LocalAssembly);
            r.OutputPath = i.OutputPath;
            r.OutputType = i.OutputType;
            r.PriIndexName = i.PriIndexName;
            r.ProjectName = i.ProjectName;
            r.ProjectPath = i.ProjectPath;
            r.ReferenceAssemblies = GetMSBuildItems(i.ReferenceAssemblies);
            r.ReferenceAssemblyPaths = GetMSBuildItems(i.ReferenceAssemblyPaths);
            r.RootNamespace = i.RootNamespace;
            r.RootsLog = i.RootsLog;
            r.SavedStateFile = i.SavedStateFile;
            r.SdkXamlPages = GetMSBuildItems(i.SdkXamlPages);
            r.SuppressWarnings = i.SuppressWarnings;
            r.TargetFileName = i.TargetFileName;
            r.TargetPlatformMinVersion = i.TargetPlatformMinVersion;
            r.VCInstallDir = i.VCInstallDir;
            r.VCInstallPath32 = i.VCInstallPath32;
            r.VCInstallPath64 = i.VCInstallPath64;
            r.WindowsSdkPath = i.WindowsSdkPath;
            r.XamlApplications = GetMSBuildItems(i.XamlApplications);
            r.XamlComponentResourceLocation = i.XamlComponentResourceLocation;
            r.XAMLFingerprint = i.XAMLFingerprint;
            r.UseVCMetaManaged = i.UseVCMetaManaged;
            r.XamlPages = GetMSBuildItems(i.XamlPages);
            r.XamlPlatform = i.XamlPlatform;
            r.XamlResourceMapName = i.XamlResourceMapName;
            return r;
        }

        private static MSBuildItem GetMSBuildItem(ITaskItem source)
        {
            if (!StringEq(source.GetMetadata("ExcludedFromBuild"), "true"))
            {
                return new MSBuildItem(source);
            }

            return null;
        }

        private static List<MSBuildItem> GetMSBuildItems(ITaskItem[] source)
        {
            List<MSBuildItem> target = null;
            if (source != null)
            {
                target = new List<MSBuildItem>();
                foreach (ITaskItem item in source)
                {
                    MSBuildItem buildItem = GetMSBuildItem(item);
                    if (buildItem != null)
                    {
                        target.Add(buildItem);
                    }
                }
            }
            return target;
        }

        private static bool StringEq(string a, string b)
        {
            return string.Equals(a, b, StringComparison.Ordinal);
        }
    }
}
