// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;
using System;
using System.IO;
using System.Text.Json;

namespace Microsoft.UI.Xaml.Markup.Compiler.IO
{
    // An MSBuild task which takes the MSBuild arguments meant for the executable Xaml compiler and serializes them into a JSON file for the executable compiler to use.
    public class InputSerializer : Task, ICompileXamlInputs
    {
        #region ICompileXamlInputs implementation

        [Required]
        public string ProjectPath { get; set; }

        [Required]
        public string Language { get; set; }

        [Required]
        public string LanguageSourceExtension { get; set; }

        [Required]
        public string OutputPath { get; set; }

        [Required]
        public ITaskItem[] ReferenceAssemblies { get; set; }

        [Required]
        public string TargetPlatformMinVersion { get; set; }

        [Required]
        public ITaskItem[] ReferenceAssemblyPaths { get; set; }

        public string BuildConfiguration { get; set; }

        public bool ForceSharedStateShutdown { get; set; }

        public bool DisableXbfGeneration { get; set; }

        public bool DisableXbfLineInfo { get; set; }

        public bool EnableXBindDiagnostics { get; set; }

        public ITaskItem[] ClIncludeFiles { get; set; }

        public String CIncludeDirectories { get; set; }

        public ITaskItem[] XamlApplications { get; set; }

        public ITaskItem[] XamlPages { get; set; }

        public ITaskItem[] LocalAssembly { get; set; }

        public ITaskItem[] SdkXamlPages { get; set; }

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
        #endregion

        #region Task Input Properties
        public string JsonFilePath { get; set; }
        #endregion

        public override bool Execute()
        {
            try
            {
                CompilerInputs inputs = CompilerInputs.FromMSBuildTaskInputs(this);

                using (FileStream jsonFileStream = new FileStream(JsonFilePath, FileMode.Create, FileAccess.Write))
                {
                    JsonSerializer.Serialize(jsonFileStream, inputs);
                }
                return true;
            }
            catch (Exception e)
            {
                Log.LogError(e.Message);
                Log.LogMessage(MessageImportance.Low, e.StackTrace);
                return false;
            }
        }
    }
}
