// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using Microsoft.Build.Evaluation;
    using Microsoft.Build.Framework;
    using Microsoft.Build.Utilities;
    using MSBuildInterop;

    /// <summary>
    /// Called to build the given set of xaml files
    /// </summary>
    public sealed class CompileXaml : Task, ICompileXamlInputs, ICompileXamlOutputs
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

        #region ICompileXamlOutputs backing fields
        private List<ITaskItem> _generatedCodeFiles = new List<ITaskItem>();
        private List<ITaskItem> _generatedXamlFiles = new List<ITaskItem>();
        private List<ITaskItem> _generatedXbfFiles = new List<ITaskItem>();
        private List<ITaskItem> _generatedXamlPagesFiles = new List<ITaskItem>();
        #endregion

        #region ICompileXamlOutputs implementation

        [Output]
        public ITaskItem[] GeneratedCodeFiles
        {
            get
            {
                return _generatedCodeFiles.ToArray();
            }
        }

        [Output]
        public ITaskItem[] GeneratedXamlFiles
        {
            get
            {
                return _generatedXamlFiles.ToArray();
            }
        }

        [Output]
        public ITaskItem[] GeneratedXbfFiles
        {
            get
            {
                return _generatedXbfFiles.ToArray();
            }
        }

        [Output]
        public ITaskItem[] GeneratedXamlPagesFiles
        {
            get
            {
                return _generatedXamlPagesFiles.ToArray();
            }
        }
        #endregion

        private bool _designTimeBuildMode;

        public override bool Execute()
        {
            this._designTimeBuildMode = this.CheckForDesignTimeBuildMode();
           CompileXamlInternal compileXamlInternal = new CompileXamlInternal();

            bool result = false;
            try
            {
                // <Target Name="_OnXamlPreCompileError" from the targets file
                if (this.ForceSharedStateShutdown)
                {
                    // While we aren't doing much when clearing our shared state, we still need to set the result to true to avoid MSBuild
                    // complaining about returning false from MSBuild without logging an error.
                    result = true;

                    // during a build, the first pass failed, we need to unload stuff
                    compileXamlInternal.UnloadReferences();
                }
                else
                {
                    this.PopulateWrapper(compileXamlInternal);
                    compileXamlInternal.SaveState = SavedStateManager.Load(this.SavedStateFile);

                    result = compileXamlInternal.DoExecute();
                    // Ensure we do not return an empty list for generated files.
                    this.ExtractWrapperResults(compileXamlInternal);

                    if (result)
                    {
                        compileXamlInternal.SaveStateBeforeFinishing();
                    }
                }
            }
            catch (Exception e)
            {
                result = false;
                compileXamlInternal.UnloadReferences();
                compileXamlInternal.LogError_XamlInternalError(e, null);
            }
            finally
            {
                // Unload everything on a pass2, or an intellisense build
                if (!this.IsPass1 || this._designTimeBuildMode || result == false)
                {
                    compileXamlInternal.UnloadReferences();
                }
            }
            return result;
        }

        /// <summary>
        /// There is a specific build task "DesignTimeBuild" which is called IN PROC for intellisense
        /// </summary>
        /// <returns>true if this is a design time build</returns>
        private bool CheckForDesignTimeBuildMode()
        {
            bool designTime = this.CompileMode.Equals("DesignTimeBuild", StringComparison.OrdinalIgnoreCase);

            if (designTime && this.HostObject == null)
            {
                Debug.WriteLine("The CompileXaml design time target was invoked without a HostObject.  This can cause unnecessary rebuilding and incorrect intellisense");
            }
            if (this.HostObject != null)
            {
#if !NETCOREAPP
                var hostObject = VsMSBuildFileManagerHostObjectWrapper.Acquire(this.HostObject);
                if (hostObject != null)
                {
                    bool isRealBuild = hostObject.IsRealBuildOperation();
                    if (designTime && isRealBuild)
                    {
                        Debug.WriteLine("The CompileXaml design time target was invoked with a HostObject that reports this is a real build.");
                    }
                    designTime |= !isRealBuild;  // if the choice is ambigious then take the design time build.
                }
#endif
            }
            return designTime;
        }

        private void PopulateWrapper(CompileXamlInternal wrapper)
        {
            wrapper.Log = new MSBuildLogger(this.Log);
            CompilerInputs ci = CompilerInputs.FromMSBuildTaskInputs(this);
            wrapper.PopulateFromCompilerInputs(ci);

            wrapper.IsDesignTimeBuild = _designTimeBuildMode;
#if !NETCOREAPP
            var taskFileManager = VsMSBuildFileManagerHostObjectWrapper.Acquire(this.HostObject);
            // In VB, we were getting a TaskFileManager on pass1, then NOT on pass2.
            // the ONLY time we should use the TaskFileManager is during designTimeMode builds.
            if (this.Language == ProgrammingLanguage.VB)
            {
                // Do not use the TaskFileManger in VB for non-designtimebiulds.
                if (this._designTimeBuildMode == false)
                {
                    taskFileManager = null;
                }
            }
            wrapper.TaskFileService = new VSMSBuildTaskFileService(taskFileManager, LanguageSourceExtension);
#else
            wrapper.TaskFileService = new BuildTaskFileService (LanguageSourceExtension);
#endif
        }

        private IList<IFileItem> GetFileItems(ITaskItem[] source)
        {
            IList<IFileItem> target = null;
            if (source != null)
            {
                target = new List<IFileItem>();
                foreach (ITaskItem item in source)
                {
                    if (!KS.Eq(item.GetMetadata("ExcludedFromBuild"), "true"))
                    {
                        target.Add(new MSBuildItem(item));
                    }
                }
            }
            return target;
        }

        private IList<IAssemblyItem> GetAssemblyItems(ITaskItem[] source)
        {
            IList<IAssemblyItem> target = null;
            if (source != null)
            {
                target = new List<IAssemblyItem>();
                foreach (ITaskItem item in source)
                {
                    if (!KS.Eq(item.GetMetadata("ExcludedFromBuild"), "true"))
                    {
                        target.Add(new MSBuildItem(item));
                    }
                }
            }
            return target;
        }

        private void ExtractWrapperResults(CompileXamlInternal wrapper)
        {
            foreach (string file in wrapper.GeneratedCodeFiles)
            {
                // File Paths need to be "escaped".  If they have URL escapes in them like %23
                // they will get processed, and we want to leave them alone
                TaskItem genFile = new TaskItem(ProjectCollection.Escape(file));
                this._generatedCodeFiles.Add(genFile);
            }

            foreach (var item in wrapper.GeneratedXamlFiles)
            {
                this._generatedXamlFiles.Add(new TaskItem(item));
            }

            foreach (var item in wrapper.GeneratedXbfFiles)
            {
                this._generatedXbfFiles.Add(new TaskItem(item));
            }

            foreach (string file in wrapper.GeneratedXamlPagesFiles)
            {
                TaskItem item = new TaskItem(ProjectCollection.Escape(file));
                this._generatedXamlPagesFiles.Add(item);
            }
        }
    }
}