// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using MSBuildInterop;

    [Flags]
    internal enum CodeGenCtrlFlags
    {
        Nothing = 0,
        NoPageCodeGen = 0x01,
        NoTypeInfoCodeGen = 0x02,
        IncrementalTypeInfoCodeGen = 0x04,
        DoNotGenerateOtherProviders = 0x08,
        FullXamlMetadataProvider = 0x10,
        DoNotGenerateCppWinRTStaticAsserts = 0x20,
    }

    [Flags]
    internal enum FeatureCtrlFlags
    {
        Nothing = 0,
        EnableTypeInfoReflection = 0x01,
        EnableXBindDiagnostics = 0x02,
        EnableDefaultValidationContextGeneration = 0x04,
        EnableWin32Codegen = 0x08,
        UsingCSWinRT = 0x10
    }

    internal class XamlProjectInfo
    {
        public IEnumerable<IFileItem> AdditionalXamlTypeInfoIncludes { get; set; }
        public String RootNamespace { get; set; }
        public String ProjectName { get; set; }
        public Boolean IsLibrary { get; set; }
        public Boolean IsCLSCompliant { get; set; }
        public CodeGenCtrlFlags CodeGenFlags { get; set; }

        public bool ShouldGenerateDisableXBind { get; set; }

        // Properties only relevent for C++
        public Dictionary<String, String> ClassToHeaderFileMap { get; set; }

        public string GenXbf32Path { get; set; }
        public string GenXbf64Path { get; set; }
        public string GenXbfArm64Path { get; set; }

        // Properties relevant to building in razzle
        public bool VSDesignerDontLoadAsDll { get; set; }

        public bool EnableTypeInfoReflection { get; set; }
        public bool EnableDefaultValidationContextGeneration { get; set; }
        public bool HasCodeGenFlag(CodeGenCtrlFlags flag)
        {
            return (CodeGenFlags & flag) == flag;
        }

        public bool GenerateIncrementalTypeInfo
        {
            get { return HasCodeGenFlag(CodeGenCtrlFlags.IncrementalTypeInfoCodeGen); }
        }

        public bool GenerateProviderCode
        {
            get { return !HasCodeGenFlag(CodeGenCtrlFlags.DoNotGenerateOtherProviders); }
        }

        public bool GenerateCppWinRTStaticAsserts
        {
            get { return !HasCodeGenFlag(CodeGenCtrlFlags.DoNotGenerateCppWinRTStaticAsserts); }
        }

        /// <summary>
        /// We generate calls to other providers if we're an app (old case),
        /// or if we're building with FullXamlMetadataProvider.
        /// This is used by CX only, where we have missbehaving clients.
        /// C# and VB have always generated "other providers"
        /// and C++\WinRT will follow their lead.
        /// </summary>
        public bool GenerateOtherProvidersForCX
        {
            get { return !IsLibrary || GenerateFullXamlMetadataProvider; }
        }

        public bool GenerateFullXamlMetadataProvider
        {
            get { return HasCodeGenFlag(CodeGenCtrlFlags.FullXamlMetadataProvider); }
        }

        public bool IsInputValidationEnabled { get; set; }
        public Version TargetPlatformMinVersion { get; set; }

        public String XamlTypeInfoNamespace
        {
            get
            {
                string rootNamespace = string.IsNullOrWhiteSpace(RootNamespace) ? "XamlDefaultRootNamespace" : RootNamespace;
                string projectName = string.IsNullOrWhiteSpace(ProjectName) ? "XamlDefaultProjectName" : ProjectName;

                // This should never change, as we have a contract with Visual Studio.
                return $"{rootNamespace}.{projectName}_XamlTypeInfo";
            }
        }

        public String XamlTypeInfoReflectionNamespace
        {
            get
            {
                return "Microsoft.UI.Xaml.Markup";
            }
        }

        public bool IsWin32App { get; set; }

        public bool UsingCSWinRT { get; set; }
        
        public string PrecompiledHeaderFile { get; set; }
    }
}
