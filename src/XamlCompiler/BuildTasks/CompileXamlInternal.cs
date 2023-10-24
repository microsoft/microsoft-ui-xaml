// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using Tracing;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Text;
    using System.Xaml;
    using System.Xml;
    using System.Xml.Linq;
    using System.Reflection.Adds;
    using CodeGen;
    using DirectUI;
    using Lmr;
    using Properties;
    using RootLog;
    using Utilities;
    using XamlDom;
    using XBF;
    using MSBuildInterop;
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Called to build the .designer files for the given set of xaml files
    /// </summary>
    internal class CompileXamlInternal
    {
        static readonly char[] _separator = new char[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar };

        private List<IXbfFileNameInfo> _newlyGeneratedXamlFiles = new List<IXbfFileNameInfo>();
        private List<Assembly> _loadedAssemblies = null;
        private List<Assembly> _loadedSystemAssemblies;
        private List<Assembly> _loadedNonSystemAssemblies;
        private List<string> _systemExtraReferenceItems;

        private SourceFileManager _SourceFileManager;

        private string _projectFolderFullpath;
        private List<string> _includeFolderList;

        private Assembly _localAssembly;
        private string _localAssemblyName;
        private DirectUISchemaContext _schemaContext;
        private IXbfMetadataProvider _xamlMetadataProvider;

        private TypeInfoCollector _typeInfoCollector;
        private XamlProjectInfo _projectInfo;

        private static XamlTypeUniverse s_typeUniverse;
        private static TypeResolver s_typeResolver;

        private Dictionary<String, XamlClassCodeInfo> _classCodeInfos = new Dictionary<string, XamlClassCodeInfo>();

        private List<String> _suppressedWarnings = new List<string>();

        private string _xamlPlatformString = null;

        private XamlCodeGenerator _codeGenerator;
        internal IList<string> _generatedCodeFiles = new List<string>();
        internal IList<string> _generatedXamlFiles = new List<string>();
        internal IList<string> _generatedXbfFiles = new List<string>();
        internal IList<string> _generatedXamlPagesFiles = new List<string>();

        #region Input Properties
        public bool DisableXbfGeneration { get; set; }
        public bool Fingerprint { get; set; }

        public bool UseVCMetaManaged { get; set; } = true;

        public string[] FingerprintIgnorePaths { get; set; }

        public string VCInstallDir { get; set; }

        public string VCInstallPath32 { get; set; }
        public string VCInstallPath64 { get; set; }

        public string WindowsSdkPath { get; set; }
        public string[] SuppressWarnings { get; set; }

        public string ProjectPath { get; set; }
        public string RootsLog { get; set; }
        public string TargetPlatformMinVersion { get; set; }

        public Language Language { get; set; }

        public string LanguageSourceExtension { get; set; }

        public string OutputPath { get; set; }

        public IList<IAssemblyItem> ReferenceAssemblies { get; set; }

        public String[] ReferenceAssemblyPaths { get; set; }

        public string[] CIncludeDirectories { get; set; }

        public IList<IFileItem> ClIncludeFiles { get; set; }

        public IList<IFileItem> XamlApplications { get; set; }

        public IList<IFileItem> XamlPages { get; set; }

        public IList<IFileItem> SdkXamlPages { get; set; }

        public IAssemblyItem LocalAssembly { get; set; }

        public string ProjectName { get; set; }

        public bool IsPass1 { get; set; }

        public bool IsDesignTimeBuild { get; set; }

        public string RootNamespace { get; set; }

        public string OutputType { get; set; }

        public string PriIndexName { get; set; }

        public CodeGenCtrlFlags CodeGenerationControlFlags { get; set; }
        public FeatureCtrlFlags FeatureControlFlags { get; set; }
        public ILog Log { get; set; }
        public uint XbfGenerationFlags { get; set; }

        public string XamlResourceMapName { get; set; }
        public string XamlComponentResourceLocation { get; set; }

        public string GenXbfPath { get; set; }

        public string PrecompiledHeaderFile { get; set; }

        // For compatability, if we aren't explicitly given a target platform from the targets file, assume it's UWP only
        public string XamlPlatformString
        {
            get
            {
                if (string.IsNullOrWhiteSpace(_xamlPlatformString))
                {
                    return "UWP";
                }

                return _xamlPlatformString;
            }
            set
            {
                _xamlPlatformString = value;
            }
        }

        public Platform XamlPlatform
        {
            get
            {
                Platform plat;
                if (Enum.TryParse<Platform>(XamlPlatformString, out plat))
                {
                    return plat;
                }
                else
                {
                    throw new ArgumentOutOfRangeException(ResourceUtilities.FormatString(
                        XamlCompilerResources.XamlCompiler_PlatformUnsupported, XamlPlatformString));
                }
            }
        }

        public bool EnableXBindDiagnostics { get; private set; }
        public bool EnableTypeInfoReflection { get; private set; }
        public bool EnableDefaultValidationContextGeneration { get; private set; }
        public bool EnableWin32Codegen { get; private set; }
        public bool UsingCSWinRT { get; private set; }

        // Controls whether or not usage of features (platform API, x:Bind functionality,
        // conditional XAML, etc.) should be validated against TargetPlatformMinVersion
        public bool IgnoreSpecifiedTargetPlatformMinVersion { get; set; }

        #endregion

        #region Output Properties

        public IList<string> GeneratedCodeFiles
        {
            get
            {
                return _generatedCodeFiles;
            }
        }

        public IList<string> GeneratedXamlFiles
        {
            get
            {
                return _generatedXamlFiles;
            }
        }

        public IList<string> GeneratedXbfFiles
        {
            get
            {
                return _generatedXbfFiles;
            }
        }

        public IList<string> GeneratedXamlPagesFiles
        {
            get
            {
                return _generatedXamlPagesFiles;
            }
        }

        #endregion

        #region Internal Properties

        internal SavedStateManager SaveState { get; set; }

        internal string ProjectFolderFullpath
        {
            get
            {
                if (_projectFolderFullpath == null)
                {
                    _projectFolderFullpath = Path.GetDirectoryName(ProjectPath);
                }
                return _projectFolderFullpath;
            }
        }

        // The IncludeFolderList is all the folders on the Include Path.
        internal List<string> IncludeFolderList
        {
            get
            {
                if (_includeFolderList == null)
                {
                    string projectFolder = Path.GetDirectoryName(ProjectPath);
                    _includeFolderList = XamlHelper.EnsureFullpaths(this.CIncludeDirectories, ProjectFolderFullpath);
                }
                return _includeFolderList;
            }
        }
        internal string GeneratedExtension
        {
            get
            {
                return IsPass1 ? Language.Pass1Extension : Language.Pass2Extension;
            }
        }

        internal string OutputFolderFullpath
        {
            get
            {
                // If output dir is not rooted we assume it is relative to the project
                if (!Path.IsPathRooted(OutputPath))
                {
                    return Path.Combine(Path.GetDirectoryName(ProjectPath), OutputPath);
                }
                else
                {
                    return OutputPath;
                }
            }
        }

        #endregion

        #region Populating inputs

        // Populates inputs common to both the executable and MSBuild task compiler
        public void PopulateFromCompilerInputs(CompilerInputs i)
        {
            FeatureControlFlags = TryParseFeatureFlags(i.FeatureControlFlags);

            ClIncludeFiles = GetFileItems(i.ClIncludeFiles);
            CIncludeDirectories = i.CIncludeDirectories != null ? i.CIncludeDirectories.Split(';') : null;
            CodeGenerationControlFlags = TryParseCodeGenFlags(i.CodeGenerationControlFlags);
            DisableXbfGeneration = i.DisableXbfGeneration;
            Fingerprint = i.XAMLFingerprint;
            UseVCMetaManaged = i.UseVCMetaManaged;
            FingerprintIgnorePaths = i.FingerprintIgnorePaths;
            IsPass1 = i.IsPass1;
            GenXbfPath = i.GenXbfPath;
            PrecompiledHeaderFile = i.PrecompiledHeaderFile;
            Language = Compiler.Language.Parse(i.Language);
            LanguageSourceExtension = i.LanguageSourceExtension;
            LocalAssembly = i.LocalAssembly?[0] != null ? i.LocalAssembly[0] : null;
            ProjectName = i.ProjectName;

            OutputPath = i.OutputPath;
            OutputType = i.OutputType;
            PriIndexName = i.PriIndexName;
            ProjectPath = i.ProjectPath;
            ReferenceAssemblies = GetAssemblyItems(i.ReferenceAssemblies);
            ReferenceAssemblyPaths = GetStringsFromItems(i.ReferenceAssemblyPaths);

            // .NET Core (and possibly C++?) projects don't use ReferenceAssemblyPaths anymore,
            // so if this is null, then use the ReferenceAssemblies
            if (ReferenceAssemblies == null)
            {
                ReferenceAssemblyPaths = GetStringsFromItems(i.ReferenceAssemblies);
            }
            RootNamespace = i.RootNamespace;
            RootsLog = i.RootsLog;
            TargetPlatformMinVersion = i.TargetPlatformMinVersion;
            VCInstallDir = i.VCInstallDir;
            VCInstallPath32 = i.VCInstallPath32;
            VCInstallPath64 = i.VCInstallPath64;
            SuppressWarnings = i.SuppressWarnings != null ? i.SuppressWarnings.Split(';') : null;
            WindowsSdkPath = i.WindowsSdkPath;
            XbfGenerationFlags = i.DisableXbfLineInfo ? (uint)1 : 0;
            XamlResourceMapName = i.XamlResourceMapName;
            XamlComponentResourceLocation = i.XamlComponentResourceLocation;
            XamlPlatformString = i.XamlPlatform ?? "UWP";
            EnableXBindDiagnostics = FeatureControlFlags.HasFlag(FeatureCtrlFlags.EnableXBindDiagnostics);
            EnableTypeInfoReflection = FeatureControlFlags.HasFlag(FeatureCtrlFlags.EnableTypeInfoReflection);
            EnableDefaultValidationContextGeneration = FeatureControlFlags.HasFlag(FeatureCtrlFlags.EnableDefaultValidationContextGeneration);
            EnableWin32Codegen = FeatureControlFlags.HasFlag(FeatureCtrlFlags.EnableWin32Codegen);
            UsingCSWinRT = FeatureControlFlags.HasFlag(FeatureCtrlFlags.UsingCSWinRT);
            IgnoreSpecifiedTargetPlatformMinVersion = IgnoreSpecifiedTargetPlatformMinVersion;

            XamlApplications = GetFileItems(i.XamlApplications);
            XamlPages = GetFileItems(i.XamlPages);
            SdkXamlPages = GetFileItems(i.SdkXamlPages);
        }

        private IList<IFileItem> GetFileItems(List<MSBuildItem> list)
        {
            List<IFileItem> fileList = new List<IFileItem>();
            if (list != null && list.Count > 0)
            {
                foreach (MSBuildItem item in list)
                {
                    fileList.Add(item);
                }
            }

            return fileList;
        }

        private IList<IAssemblyItem> GetAssemblyItems(List<MSBuildItem> list)
        {
            List<IAssemblyItem> assemblyList = new List<IAssemblyItem>();
            foreach (MSBuildItem item in list)
            {
                assemblyList.Add(item);
            }

            return assemblyList;
        }

        internal static String[] GetStringsFromItems(List<MSBuildItem> items)
        {
            if (items == null)
            {
                return null;
            }
            String[] strings = new String[items.Count];
            for (int i = 0; i < items.Count; i++)
            {
                strings[i] = items[i].ItemSpec;
            }
            return strings;
        }

        private CodeGenCtrlFlags TryParseCodeGenFlags(string flags)
        {
            var parsedFlags = CodeGenCtrlFlags.Nothing;
            if (!String.IsNullOrWhiteSpace(flags))
            {
                foreach (String flag in flags.Split(';'))
                {
                    CodeGenCtrlFlags f;
                    if (!Enum.TryParse<CodeGenCtrlFlags>(flag, out f))
                    {
                        LogError_BadCodeGenFlags(flag);
                    }
                    parsedFlags |= f;
                }
            }
            return parsedFlags;
        }

        private FeatureCtrlFlags TryParseFeatureFlags(string flags)
        {
            var parsedFlags = FeatureCtrlFlags.Nothing;
            if (!String.IsNullOrWhiteSpace(flags))
            {
                foreach (String flag in flags.Split(';'))
                {
                    FeatureCtrlFlags f;
                    if (!Enum.TryParse<FeatureCtrlFlags>(flag, out f))
                    {
                        LogError_BadCodeGenFlags(flag);
                    }
                    parsedFlags |= f;
                }
            }
            return parsedFlags;
        }
        #endregion

        void CleanUpSavedState()
        {
            // The SaveState contains a list of files that were compiled last time.
            // It is possible that the project has changes and files have gone away.
            // We need to check and cull for the removed files.
            List<string> removeFiles = new List<string>();
            foreach (String oldXamlFile in SaveState.XamlPerFileInfo.Keys)
            {
                bool foundIt = false;
                foreach (TaskItemFilename taskItem in SourceFileManager.ProjectXamlTaskItems)
                {
                    if (oldXamlFile == taskItem.XamlGivenPath)
                    {
                        foundIt = true;
                        break;
                    }
                }
                if (!foundIt)
                {
                    removeFiles.Add(oldXamlFile);
                }
            }
            foreach (string badFile in removeFiles)
            {
                SaveState.XamlPerFileInfo.Remove(badFile);
            }
        }

        #region FingerPrinting
        internal bool DidAssembliesChange()
        {
            if (this.Fingerprint == false)
            {
                return true;
            }
            FingerPrinter fingerPrinter = new FingerPrinter(LocalAssembly, ReferenceAssemblies, this.FingerprintIgnorePaths, this.VCInstallDir, this.VCInstallPath32, this.VCInstallPath64, this.UseVCMetaManaged);
            if (String.IsNullOrEmpty(SaveState.LocalAssemblyName))
            {
                SaveState.LocalAssemblyName = fingerPrinter.LocalAssemblyPath;
            }

            // Check them all even if one is different.  This keeps the cache up to date.
            bool listDiffers = fingerPrinter.HasAssemblyFileListChanged(SaveState.ReferenceAssemblyList);
            bool localDifferent = fingerPrinter.HasLocalAssemblyHashChanged(SaveState.ReferenceAssemblyGuids);
            bool refDifferent = fingerPrinter.HaveReferenceAssembliesHashesChanged(SaveState.ReferenceAssemblyGuids);

            if (listDiffers)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "List of Assemblies Changed");
            }
            if (localDifferent)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "Local Assembly FingerPrint Changed");
                Guid guid;
                if (SaveState.ReferenceAssemblyGuids.TryGetValue(SaveState.LocalAssemblyName, out guid))
                {
                    PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, String.Format("{0} - {1}", guid.ToString(), SaveState.LocalAssemblyName));
                }
            }
            if (refDifferent)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_FingerprintCheck, "Reference Assemblies Fingerprint Changed");
            }

            return listDiffers | localDifferent | refDifferent;
        }

        /// <summary>
        /// This is just a XAML timestamp check - both passes.
        /// If any of the generated files are out of date, we need to regen them
        /// </summary>
        /// <returns>TRUE if we need to perform any additional work</returns>
        public bool DidXAMLFilesChange()
        {
            if (CodeGenerationControlFlags.HasFlag(CodeGenCtrlFlags.NoTypeInfoCodeGen))
            {
                return false;
            }

            bool workFound = false;

            // Check the date/time for each file
            foreach (TaskItemFilename tif in SourceFileManager.ProjectXamlTaskItems)
            {
                if (tif.OutOfDate())
                {
                    workFound = true;
                    break;
                }
            }

            return workFound;
        }

        private bool DidFeatureControlFlagsChange()
        {
            string featureCtrlFlags = FeatureControlFlags.ToString();
            if (string.Compare(SaveState.XamlFeatureControlFlags, featureCtrlFlags, StringComparison.OrdinalIgnoreCase) != 0)
            {
                SaveState.XamlFeatureControlFlags = featureCtrlFlags;
                return true;
            }

            return false;
        }

        //
        // If you did a clean 'underneath' a previously built project
        // out caches would match, the XAML files would match, but we'd not build
        // check for XamlTypeInfo having content.
        //
        private bool IsXAMLTypeInfoNeeded()
        {
            bool workFound = false;

            // Get the 2nd pass extension for the xamltypeinfo.
            string extension = Language.Pass2Extension;
            if (extension == ".g.hpp")
            {
                extension = ".g.cpp";
            }

            // If the xamltypeinfo pass2 isn't there
            string xamlTypeInfoFile = Path.Combine(OutputFolderFullpath, KnownStrings.XamlTypeInfo + extension);
            FileInfo fileInfo = new FileInfo(xamlTypeInfoFile);
            if (fileInfo.Exists == false)
            {
                workFound = true;
            }
            else
            {
                // managed will always be zero length (from pass1)
                if (Language.IsNative)
                {
                    if (fileInfo.Length == 0)
                    {
                        workFound = true;
                    }
                }
            }

            return workFound;
        }
        #endregion

        #region Skipped File Processing
        private void ReportExistingGeneratedXamlFiles(TaskItemFilename tif)
        {
            _generatedXamlFiles.Add(tif.XamlOutputFilename);
            if (!this.DisableXbfGeneration)
            {
                AddGeneratedXbfFile(tif.XbfOutputFilename);
            }
        }

        internal void ReportExistingGeneratedCodeFile(string targetFolder, string codeFileName)
        {
            string srcOutputFileName1 = Path.Combine(targetFolder, codeFileName + Language.Pass1Extension);
            if (Language.IsNative)
            {
                _generatedCodeFiles.Add(srcOutputFileName1);
                if (!IsPass1 && this.CodeGenerationControlFlags.HasFlag(CodeGenCtrlFlags.IncrementalTypeInfoCodeGen))
                {
                    string pageCodeFile = Path.Combine(targetFolder, codeFileName + Language.Pass2Extension);
                    _generatedXamlPagesFiles.Add(pageCodeFile);
                }
            }
            else if (IsPass1)  // only output code file in Pass1 of Managed
            {
                _generatedCodeFiles.Add(srcOutputFileName1);

                // this is the 2nd pass .g.*
                string srcOutputFileName2 = Path.Combine(targetFolder, codeFileName + Language.Pass2Extension);
                CreateFileIfNecessary(srcOutputFileName2);

                // Let intellisense know it should parse this file
                _generatedCodeFiles.Add(srcOutputFileName2);
            }
        }

        internal bool ShortcutBackupRestoreGeneratedPass2Files_WhenNothingExternalHasChanged()
        {
            if (Language.IsNative)
            {
                return true;
            }

            // Here we are processing the generated Pass2 files *.g.[cs/vb]
            // 1) The design type build (pass1) needs to report all generated file (including pass2 generated files)
            // 2) We don't want the preBuild to compile the pass2 code from the previous build
            //    because things may have changed and it may fail to compile.
            // SO we Zero out the Pass2 file in Pass1.
            // BUT we also want to preserve it for pass2 so that simple
            // incremental builds don't see a NEW *.g.[cs/vb] if they don't have to.
            // We backup a copy of the contents for later use in Pass2.
            // (the timestamp is saved on the Zero length file) 
            if (IsPass1)
            {
                // Don't delete the Pass2 files in DesignTime build.
                if (!IsDesignTimeBuild)
                {
                    foreach (var generatedPass2CodeFile in SourceFileManager.CodeGenFiles.Select(cgf => Path.Combine(cgf.TargetFolderFullPath, cgf.BaseFileName + Language.Pass2Extension)))
                    {
                        FileHelpers.BackupIfExistsAndTruncateToNull(generatedPass2CodeFile);
                    }
                }

                return true;
            }
            else    // Pass2: restore the back up.
            {
                // This is only called for Pass2 when nothing else (assemblies, XAML files) has changed.
                // if we can't restore a saved backup file then return false.
                bool ret = true;

                foreach (var generatedPass2CodeFile in SourceFileManager.CodeGenFiles.Select(cgf => Path.Combine(cgf.TargetFolderFullPath, cgf.BaseFileName + Language.Pass2Extension)))
                {
                    ret &= FileHelpers.RestoreBackupFile(generatedPass2CodeFile);
                }

                // After restoring the backups, we need to refresh the state of the generated files
                foreach (TaskItemFilename tif in SourceFileManager.ProjectXamlTaskItems)
                {
                    tif.Refresh(SaveState);
                }

                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_RestoredGeneratedPass2CodeFileBackup);
                return ret;
            }
        }

        internal bool ShortcutBackupRestoreXamlTypeInfoFile_WhenNothingExternalHasChanged()
        {
            if (Language.IsNative)
            {
                return true;
            }

            // Here we are processing the XamlTypeInfo.g.[cs/vb]
            // 1) The design type build (pass1) needs to report all generated file (including pass2 generated files)
            // 2) We don't want the preBuild to compile the pass2 code from the previous build
            //    because things may have changed and it may fail to compile.
            // SO we Zero out the Pass2 file in Pass1.
            // BUT we also want to preserve it for pass2 so that simple
            // incremental builds don't see a NEW xamlTypeInfo.g.[cs/vb] if they don't have to.
            // We backup a copy of the contents for later use in Pass2. [GenerateTypeInfo()]
            // (the timestamp saved on the Zero length file)
            string xamlTypeInfoFile = Path.Combine(OutputFolderFullpath, KnownStrings.XamlTypeInfo + Language.Pass2Extension);
            Debug.Assert(_generatedCodeFiles.Contains(xamlTypeInfoFile) == false);

            if (IsPass1)
            {
                // Don't delete the Pass2 files in DesignTime build.
                if (!IsDesignTimeBuild)
                {
                    FileHelpers.BackupIfExistsAndTruncateToNull(xamlTypeInfoFile);
                }
                _generatedCodeFiles.Add(xamlTypeInfoFile);
                return true;
            }
            else    // Pass2: restore the back up.
            {
                // This is only called for Pass2 when nothing else (assemblies, XAML files) has changed.
                // if we can't restore a saved XamlTypeInfo backup file then return false.
                bool ret = FileHelpers.RestoreBackupFile(xamlTypeInfoFile);
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_RestoredTypeInfoBackup);
                return ret;
            }
        }

        internal void UpdateGeneratedFilesLists()
        {
            foreach (ClassCodeGenFile codeGenFile in SourceFileManager.CodeGenFiles)
            {
                ReportExistingGeneratedCodeFile(codeGenFile.TargetFolderFullPath, codeGenFile.BaseFileName);
            }

            foreach (TaskItemFilename tif in SourceFileManager.ProjectXamlTaskItems)
            {
                ReportExistingGeneratedXamlFiles(tif);
            }

            if (SourceFileManager.SdkXamlTaskItems != null && SourceFileManager.SdkXamlTaskItems.Count > 0 && !this.DisableXbfGeneration)
            {
                foreach (TaskItemFilename sdkTif in SourceFileManager.SdkXamlTaskItems)
                {
                    AddGeneratedXbfFile(sdkTif.XbfOutputFilename);
                }
            }

            // C++ has a few extra files that it adds that aren't tracked by the above, but which we still need to preserve.
            // Otherwise, these files will be deleted by the incremental clean step.
            if (Language.IsNative)
            {
                List<string> extraFilePaths = new List<string>();

                if (IsPass1)
                {
                    extraFilePaths.Add(Path.Combine(SourceFileManager.OutputFolderFullpath, "XamlMetaDataProvider.h"));
                    extraFilePaths.Add(Path.Combine(SourceFileManager.OutputFolderFullpath, "XamlLibMetadataProvider.g.cpp"));
                    extraFilePaths.Add(Path.Combine(SourceFileManager.OutputFolderFullpath, "XamlTypeInfo.Impl.g.cpp"));
                }
                else
                {
                    extraFilePaths.Add(Path.Combine(SourceFileManager.OutputFolderFullpath, "XamlTypeInfo.g.cpp"));
                }

                foreach (string extraFilePath in extraFilePaths)
                {
                    if (File.Exists(extraFilePath))
                    {
                        _generatedCodeFiles.Add(extraFilePath);
                    }
                }
            }
        }
        #endregion

        //
        // Debug only.
        // All timestamps should be correct.
        //
        public bool VerifyWorkDone()
        {
            if (this.IsPass1 == true)
                return true;

            // Get the latest timestamp of the generated file
            foreach (TaskItemFilename tif in SourceFileManager.ProjectXamlTaskItems)
            {
                tif.Refresh(SaveState);
            }

            // We should not need to do any work
            return DidXAMLFilesChange() == false;
        }

        private SourceFileManager SourceFileManager
        {
            get
            {
                if (_SourceFileManager == null)
                {
                    _SourceFileManager = new SourceFileManager(this);
                }
                return _SourceFileManager;
            }
        }

        public void SaveStateBeforeFinishing()
        {
            this.SourceFileManager.SaveState();
        }

        public bool DoExecute()
        {
            bool result = true;
            bool shouldVerifyWorkDone = false;

            PerformanceUtility.Initialize(Log);

            PerformanceUtility.FireCodeMarker(
                this.IsPass1 ? CodeMarkerEvent.perfXC_StartPass1 : CodeMarkerEvent.perfXC_StartPass2,
                this.ProjectName);

            _generatedCodeFiles.Clear();
            _generatedXamlFiles.Clear();
            _generatedXbfFiles.Clear();
            _generatedXamlPagesFiles.Clear();

            if (s_typeUniverse != null && !String.Equals(s_typeUniverse.ProjectPath, this.ProjectPath))
            {
                // we have been reused for a different project after doing a Pass1 for the first project.
                // Ensure we clear the type reslution subsystem.
                this.UnloadReferences();
            }

            if (!BuildWarningSuppressionList())
            {
                return false;
            }

            // if there are no XAML files then issue a warning and exit (successfully), because we have nothing to do.
            if ((XamlApplications == null || !XamlApplications.Any()) && (XamlPages == null || XamlPages.Count == 0))
            {
                LogWarning(new XamlValidationWarningNoXaml());
                return true;        // exit the compiler but not as a failure, just "done"
            }

            if (!CheckTaskArgumentsValid())
            {
                return false;
            }

            if (this.CodeGenerationControlFlags != CodeGenCtrlFlags.Nothing)
            {
                LogWarning(new XamlValidationWarningUsingCodeGenFlags(this.CodeGenerationControlFlags));
            }

            if (this.Language.IsExperimental)
            {
                LogWarning(new XamlValidationWarningPreview(ErrorCode.WMC1502, Language.Name));
            }

            CleanUpSavedState();

            bool areGeneratedFilesListsUpdated = false;

            // Checking this always keeps us up-to-date, e.g. the first build after a solution load
            // prepares us for the 2nd build.
            bool didAssembliesChange = DidAssembliesChange();
            bool xamlTypeInfoNeeded = IsXAMLTypeInfoNeeded();
            // If the feature ctrl flags change, then skip checking every xaml file - we'll assume that since the user
            // edited the project file that something needs to be done.
            bool didFeatureCtrlFlagsChange = DidFeatureControlFlagsChange();

            // During Pass 2, we can skip most type info collection if type info reflection is enabled since we don't need our type tables.
            bool skipPass2TypeInfo = EnableTypeInfoReflection;

            if ((xamlTypeInfoNeeded == false) && (didAssembliesChange == false) && (didFeatureCtrlFlagsChange == false))
            {
                bool haveGeneratedPass2CodeFiles = ShortcutBackupRestoreGeneratedPass2Files_WhenNothingExternalHasChanged();
                bool xamlFilesChanged = DidXAMLFilesChange();

                if ((xamlFilesChanged == false))
                {
                    // TODO: does this fix the file generation issue
                    UpdateGeneratedFilesLists();
                    areGeneratedFilesListsUpdated = true;

                    bool haveXamlTypeInfo = ShortcutBackupRestoreXamlTypeInfoFile_WhenNothingExternalHasChanged();

                    if (IsPass1 || (haveGeneratedPass2CodeFiles && haveXamlTypeInfo))
                    {
                        return true;
                    }
                }
            }

            if (s_typeUniverse == null)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_CreatingTypeUniverse);

                s_typeUniverse = new XamlTypeUniverse(Language.IsManaged);
                s_typeUniverse.ProjectPath = this.ProjectPath;
                s_typeUniverse.ReferenceAssemblyPaths = this.ReferenceAssemblyPaths;

                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_CreatedTypeUniverse);
            }

            SetupLmrAssemblyResolveHandler();

            try
            {
                _schemaContext = LoadSchemaContext();
                if (_schemaContext == null)
                {
                    return false;  // stop now  (see finally block)
                }

                SourceFileManager.PropagateOutOfDateStatus(_schemaContext);

                _xamlMetadataProvider = new XbfMetadataProvider(_schemaContext);

                _projectInfo = GetProjectInfo();
                _typeInfoCollector = new TypeInfoCollector(_schemaContext, XamlPlatform);
                Type ixt = GetIXamlType(_loadedAssemblies);
                if (ixt == null)
                {
                    LogError_CannotResolveWinUIMetadata();
                    return false;
                }

                foreach (TaskItemFilename tif in SourceFileManager.ProjectXamlTaskItems)
                {
                    // We currently can't ever skip a file in pass2 (unless we skip all of them)
                    // In Pass2 if one changes we read them all.
                    // If assemblies or feature control flags have changed, we need to regenerate all files regardless of whether
                    // the file itself is different. Some of the feature ctrl flags will cause different code to be generated
                    // on a per page basis (i.e. EnableXBindDiagnostics), while others will only affect app.xaml (i.e. EnableWin32CodeGen).
                    // But we'll be conservative and just assume that all files need to regenerate if the flags have changed
                    bool forceRegenerate = didAssembliesChange || didFeatureCtrlFlagsChange;
                    if (IsPass1 && !tif.OutOfDate() && !forceRegenerate)
                    {
                        // If the file is up to date then report the existing "on disk" generated
                        // files so that packaging will pick them up.
                        // Report only the generated XAML or XBF because the code files are per "class"
                        // not per XAML source file.
                        // All generated file for pass1 and pass2 files are reported in pass1.
                        ReportExistingGeneratedXamlFiles(tif);
                    }
                    else
                    {
                        // Otherwise compile the file
                        // Pass1 and Pass2
                        PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageStart, tif.XamlGivenPath);
                        XamlDomObject xamlDomRoot = null;
                        bool failed = false;
                        if (!IsPass1)
                        {
                            // Even if we want to skip type info collecting in pass 2, we always need to collect
                            // the App's type info to figure out the application name.
                            // When we aren't skipping type info, we want to collect everything.
                            if (!skipPass2TypeInfo || tif.IsApplication)
                            {
                                failed |= !ProcessXamlFile_XamlTypeInfo(tif, ref xamlDomRoot);
                            }
                        }
                        if (tif.OutOfDate() || forceRegenerate)
                        {
                            failed |= !ProcessXamlFile_PerPageInfo(tif, ref xamlDomRoot);
                        }
                        PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageDone, tif.XamlGivenPath);

                        if (failed)
                        {
                            result = false;
                        }
                        else
                        {
                            if (IsPass1)
                            {
                                SaveState.SetXamlFileTimeAtLastCompile(tif.XamlGivenPath, tif.XamlLastChangeTime.Ticks);
                                SaveState.SetClassFullName(tif.XamlGivenPath, tif.ClassFullName);
                            }
                        }
                    }
                }

                // All the XAML files have been processed (and all XAML file errors reported).
                // If there was a problem we can fail out now.
                if (!result)
                {
                    return false;
                }

                // Only search for Metadata providers and Bindable types in Pass2.
                if (!IsPass1 && !ShouldSuppressTypeInfoCodeGen())
                {
                    _typeInfoCollector.AddMetadataAndBindableTypes(_loadedNonSystemAssemblies, _localAssembly);
                    if (!ReportSchemaErrors(String.Empty))
                    {
                        return false;
                    }

                    // Skip type info validation if we skipped collecting type information.
                    if (!skipPass2TypeInfo)
                    {
                        // Validate that the type info we collected is consistent
                        if (!ValidateXamlTypeInfo())
                        {
                            return false;
                        }
                    }
                }

                // Create Code Generator
                _codeGenerator = new XamlCodeGenerator(Language, IsPass1, _projectInfo, _typeInfoCollector.SchemaInfo);

                // We have collected all the information.
                // Start writing out the generated files  code and edited XAML.
                // XBF generation is later (from the edited XAML).
                foreach (TaskItemFilename tif in SourceFileManager.ClasslessXamlFiles)
                {
                    if (!GenerateClasslessXamlOutputFile(tif.SourceXamlFullPath, tif.XamlGivenPath, tif.TargetFolder, tif.FileNameNoExtension, tif.XamlOutputFilename))
                        return false;
                }

                foreach (XamlClassCodeInfo classCodeInfo in _classCodeInfos.Values)
                {
                    if (!GeneratePageOutputFiles(classCodeInfo))
                        return false;
                }

                if (EnableTypeInfoReflection && !IsPass1)
                {
                    // TODO: re-enable this if we do decide to modify the appxmanifest for EnableTypeInfoReflection
                    // GenerateRuntimeTypeInfoManifestData(_typeInfoCollector.SchemaInfo, IsOutputTypeLibrary, _localAssembly, TargetFileName);

                    _typeInfoCollector.AddAllConstructibleTypesFromLocalAssembly(_localAssembly);
                }

                // Generating Binding Info needs to happen after GeneratePageOutputFiles because
                // Binding Steps are parsed there.
                GenerateBindingInfo();

                // TODO: does this fix the file generation issue
                if (!areGeneratedFilesListsUpdated)
                {
                    UpdateGeneratedFilesLists();
                    areGeneratedFilesListsUpdated = true;
                }

                if (!IsPass1 && !String.IsNullOrWhiteSpace(this.RootsLog))
                {
                    if (!WriteRootsFile(_typeInfoCollector.RootLog, this.RootsLog))
                        return false;
                }

                // C++ has Pass1 typeinfo file.  Managed code just returns OK nothing in Pass1.
                if (!GenerateTypeInfo())
                {
                    return false;
                }

                try
                {
                    if (!IsPass1 && !this.DisableXbfGeneration)
                    {
                        DetermineGenXbfPath(_projectInfo);

                        if (!GenerateXbfFiles(_newlyGeneratedXamlFiles))
                        {
                            return false;
                        }
                        if (!GenerateSdkXbfFiles())
                        {
                            return false;
                        }
                    }
                }
                catch (TypeLoadException)
                {
                    LogError("CompileXaml", ErrorCode.WMC9998, null, GeneratedXamlFiles[0], 0, 0, 0, 0, XamlCompilerResources.XbfGeneration_MissingXbfApi);
                    return false;
                }

                // changes the finally.
                shouldVerifyWorkDone = true;
            }
            catch (UnresolvedAssemblyException e)
            {
                // TODO: this logging was originally handled under universe_OnResolveEvent, but was moved out here
                // to accomodate unresolved forwarded types.  If we ever fix up type resolution to be deferred until runtime,
                // the logging should move back to universe_OnResolveEvent, and we should remove this catch.
                // Note this exception can be thrown for unresolved type-forwarded types, but
                // also for other reasons.
                LogError_CannotResolveAssembly(e.Message);
            }
            catch (Exception e)
            {
                result = false;

                // We make every effort to ensure that the user never sees this error message.
                // However if this situation does arises, emit the exception text.
                LogError_XamlInternalError(e, null);
            }
            finally
            {
                if (!areGeneratedFilesListsUpdated)
                {
                    // Ensure we do not return an empty list for generated files even if we did fail.
                    // Bug 1101648:Error in one XAML file causes Error list to show tons of random build errors in other XAML files
                    this.UpdateGeneratedFilesLists();
                }

                RemoveLmrAssemblyResolveHandler();
                if (shouldVerifyWorkDone == true)
                {
                    bool isDone = VerifyWorkDone();
                    Debug.Assert(isDone, "VerifyWorkDone checked failed");
                }
                Core.InstanceCacheManager.ClearCache();

                if (this.IsPass1)
                {
                    PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_EndPass1, this.ProjectName);
                }
                else
                {
                    PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_EndPass2, this.ProjectName);
                }
                PerformanceUtility.Shutdown();
            }
            return result;
        }

        private void DetermineGenXbfPath(XamlProjectInfo projectInfo)
        {
            // Construct path to GenXbf.dll from the task's GenXbfPath property.
            // If GenXbf doesn't exist XBF generation will throw the correct error later
            // on so we don't need to handle it explicitly here.
            if (!String.IsNullOrEmpty(this.GenXbfPath))
            {
                projectInfo.GenXbf32Path = Path.Combine(this.GenXbfPath, @"x86\genxbf.dll");
                projectInfo.GenXbf64Path = Path.Combine(this.GenXbfPath, @"x64\genxbf.dll");
                projectInfo.GenXbfArm64Path = Path.Combine(this.GenXbfPath, @"arm64\genxbf.dll");
            }
        }

        // Returns False if the compiler should exit.
        // TODO: consider moving this to SourceFileManager.
        private bool CheckTaskArgumentsValid()
        {
            if (XamlApplications != null && XamlApplications.Any())
            {
                if (XamlApplications.Count() > 1)
                {
                    LogError_MoreThanOneApplicationXaml();
                    return false;
                }
            }

            bool errorFound = false;

            // Check that the apparent name of every XAML file is unique
            for (int i = 0; i < SourceFileManager.ProjectXamlTaskItems.Count; i++)
            {
                TaskItemFilename file1 = SourceFileManager.ProjectXamlTaskItems[i];
                for (int k = i + 1; k < SourceFileManager.ProjectXamlTaskItems.Count; k++)
                {
                    TaskItemFilename file2 = SourceFileManager.ProjectXamlTaskItems[k];
                    if (KS.EqIgnoreCase(file1.ApparentRelativePath, file2.ApparentRelativePath))
                    {
                        LogError_XamlFilesWithSameApparentPath(file1.XamlGivenPath, file2.XamlGivenPath, file1.ApparentRelativePath);
                        errorFound = true;
                    }
                }
            }

            return !errorFound;
        }

        #region Assembly Loading
        private List<Assembly> LoadAssemblyItems(List<string> referenceAssemblies, bool isSystemAssembly)
        {
            List<Assembly> assemblies = new List<Assembly>();

            foreach (var item in referenceAssemblies)
            {
                Assembly asm = LoadAssemblyItem(item, isSystemAssembly);
                if (asm != null)
                {
                    assemblies.Add(asm);
                }
            }
            Assembly mscorlib;
            if (DoesMscorlibNeedToBeLoaded(out mscorlib))
            {
                assemblies.Add(mscorlib);
            }

            return assemblies;
        }

        private Assembly LoadAssemblyItem(string item, bool isSystemAssembly)
        {
            Assembly asm = null;

            // if we get File Not found on a system assembly
            // let it throw through.
            if (isSystemAssembly)
            {
                asm = LoadAssembly(item);
            }
            else
            {
                // file not found on P2P references is allowed.
                // The design time build can run before the DLL's are present
                try
                {
                    asm = LoadAssembly(item);
                }
                catch (FileNotFoundException)
                {
                    asm = null;
                }
            }
            return asm;
        }

        // Tries to load the given assembly.  If it doesn't exist, logs an error.
        public Assembly LoadAssembly(string reference)
        {
            Assembly asm = TryLoadAssembly(reference);
            if (asm == null)
            {
                LogError_CannotResolveAssembly(reference);
            }
            return asm;
        }

        // Tries to load the given assembly.  If the assembly doesn't exist, returns null instead.
        // Does not log any errors.
        public static Assembly TryLoadAssembly(string reference)
        {
            string fullPath = Path.GetFullPath(reference);
            if (!File.Exists(fullPath))
            {
                return null;
            }
            else
            {
                Assembly asm = s_typeUniverse.LoadAssemblyFromFile(fullPath);
                return asm;
            }
        }

        internal Assembly LoadAssemblyFromReferencePath(string fileName)
        {
            string namePath;
            foreach (string dirPath in ReferenceAssemblyPaths)
            {
                namePath = Path.Combine(dirPath, fileName);
                if (File.Exists(namePath))
                {
                    Assembly asm = LoadAssembly(namePath);
                    return asm;
                }
            }
            return null;
        }

        internal Assembly LoadAssemblyFromSystemExtraReferences(string fileName)
        {
            foreach (var item in this._systemExtraReferenceItems)
            {
                if (Path.GetFileName(item) == fileName)
                {
                    return LoadAssembly(item);
                }
            }

            return null;
        }

        internal bool DoesMscorlibNeedToBeLoaded(out Assembly mscorlib)
        {
            mscorlib = null;
            if (!s_typeUniverse.IsSystemAssemblyLoaded)
            {
                mscorlib = s_typeUniverse.GetSystemAssembly();
            }
            return mscorlib != null;
        }

        protected void SetupLmrAssemblyResolveHandler()
        {
            s_typeUniverse.OnResolveEvent += new EventHandler<System.Reflection.Adds.ResolveAssemblyNameEventArgs>(universe_OnResolveEvent);
        }

        protected void RemoveLmrAssemblyResolveHandler()
        {
            s_typeUniverse.OnResolveEvent -= new EventHandler<System.Reflection.Adds.ResolveAssemblyNameEventArgs>(universe_OnResolveEvent);
        }

        private void universe_OnResolveEvent(object sender, System.Reflection.Adds.ResolveAssemblyNameEventArgs e)
        {
            string fileName;
            if (e.Name.ContentType == AssemblyContentType.WindowsRuntime)
            {
                fileName = e.Name.Name + ".winmd";
            }
            else
            {
                fileName = e.Name.Name + ".dll";
            }
            e.Target = LoadAssemblyFromReferencePath(fileName);

            // We can no longer depend on ReferencePath for resolving .Net reference assemblies after .NuGet changes
            // instead, we leave the bulk of the assemblies in systemExtraReferenceItems as Win81 was doing and attempt
            // to resolve the assembly from there. Unlike Win81, the above LoadAssemblyFromReferencePath used to work for
            // assemblies like system.runtime.dll since the framework had a known location on disk.
            if (e.Target == null)
            {
                e.Target = LoadAssemblyFromSystemExtraReferences(fileName);
            }
        }

        /// <summary>
        /// We have to free up any assemblies we may have kept open
        /// </summary>
        public void UnloadReferences()
        {
            if (s_typeUniverse != null)
            {
                // We'll enable this in the future if needed.
                // PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_ReleasingTypeUniverse);
                s_typeResolver = null;
                s_typeUniverse.Dispose();
                s_typeUniverse = null;

                DirectUI.ReflectionHelper.Release();
            }
        }

        private DirectUISchemaContext LoadSchemaContext()
        {
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_CreatingSchemaContext);
            List<string> systemReferenceItems;
            List<string> nonSystemReferenceItems;
            List<string> systemExtraReferenceItems;
            SortReferenceAssemblies(out systemReferenceItems, out nonSystemReferenceItems, out systemExtraReferenceItems);

            _loadedSystemAssemblies = LoadAssemblyItems(systemReferenceItems, true);
            _loadedNonSystemAssemblies = LoadAssemblyItems(nonSystemReferenceItems, false);
            // DO NOT LOAD systemExtraReferenceItems
            _systemExtraReferenceItems = systemExtraReferenceItems;

            if (LocalAssembly != null)
            {
                _localAssemblyName = LocalAssembly.ItemSpec;
                _localAssembly = LoadAssembly(_localAssemblyName);
                _loadedNonSystemAssemblies.Add(_localAssembly);
            }

            _loadedAssemblies = new List<Assembly>(_loadedSystemAssemblies);
            _loadedAssemblies.AddRange(_loadedNonSystemAssemblies);

            ISet<string> staticLibraryAssemblyNames = new HashSet<string>();
            foreach (var a in ReferenceAssemblies.Where(a => a.IsStaticLibraryReference))
            {
                staticLibraryAssemblyNames.Add(a.ItemSpec);
            }

            // The Type Resolver is cleared when the Type Universe is cleared.
            // So use the existing one when it exists.
            if (s_typeResolver == null)
            {
                s_typeResolver = new TypeResolver(s_typeUniverse);

                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_InitializeTypeNameMapStart);
                s_typeResolver.InitializeTypeNameMap();
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_InitializeTypeNameMapEnd);
            }

            if (_localAssembly != null)
            {
                s_typeResolver.AddLocalAssemblyToTypeNameMap(_localAssembly);
            }
            else if (!IsPass1)
            {
                // We should almost always have a local assembly in Pass 2 - if we don't, raise a warning
                LogWarning(new XamlLocalAssemblyNotFound());
            }

            // Only load up all the assemblies if we're not in pass1
            var schemaContext = new DirectUISchemaContext(
                _loadedAssemblies,
                IsPass1 ? null : _systemExtraReferenceItems,
                _localAssembly,
                staticLibraryAssemblyNames,
                this.WindowsSdkPath,
                Language.IsStringNullable);

            schemaContext.TypeResolver = s_typeResolver;
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_CreatedSchemaContext);
            return schemaContext;
        }
        #endregion

        #region WriteFilesToDisk
        protected void WriteOutputFilesToDisk(List<FileNameAndContentPair> generatedFiles, string targetFolder, bool updateOnlyIfContentNew)
        {
            if (generatedFiles == null || generatedFiles.Count == 0)
            {
                return;
            }
            Directory.CreateDirectory(targetFolder);

            foreach (FileNameAndContentPair codeFile in generatedFiles)
            {
                string outputFilename = Path.Combine(targetFolder, codeFile.FileName);

                if (updateOnlyIfContentNew)
                {
                    WriteOnlyIfContentsAreNew(outputFilename, codeFile.Contents);
                }
                else
                {
                    TaskFileService.WriteFile(codeFile.Contents, outputFilename);
                }
            }
        }

        private bool WriteOnlyIfContentsAreNew(string outputFilename, string fileContents)
        {
            bool fileContentsAreTheSame = false;
            if (File.Exists(outputFilename))
            {
                string oldContents = File.ReadAllText(outputFilename);
                fileContentsAreTheSame = AreGeneratedCodeStringsTheSame(oldContents, fileContents);
            }
            if (!fileContentsAreTheSame)
            {
                TaskFileService.WriteFile(fileContents, outputFilename);
                return true;
            }
            return false;
        }

        protected void WriteXamlTypeInfoFilesToDisk(List<FileNameAndContentPair> codeFiles)
        {
            if (codeFiles != null)
            {
                if (codeFiles.Count != 1 || Language.IsNative)
                {
                    WriteOutputFilesToDisk(codeFiles, OutputFolderFullpath, true);
                }
                else
                {
                    // XamlTypeInfo.g.[cs/vb] is a special case.
                    // the XamlTypeInfo.g.cs, was zeroed out in pass1, but a backup was saved
                    //
                    FileNameAndContentPair codeFile0 = codeFiles[0];
                    bool fileContentsAreTheSame = false;

                    // If you find a backup file for XamlTypeInfo.g.[cs/vb].backup
                    // then if the contents of the new version are the same as the backup
                    // move the timestamp back to the same time as the backup.
                    string outputFilename = Path.Combine(OutputFolderFullpath, codeFile0.FileName);
                    string backupFilename = outputFilename + KnownStrings.BackupSuffix;
                    if (File.Exists(backupFilename))
                    {
                        string oldContents = File.ReadAllText(backupFilename);
                        fileContentsAreTheSame = AreGeneratedCodeStringsTheSame(oldContents, codeFile0.Contents);
                    }
                    if (fileContentsAreTheSame)
                    {
                        FileHelpers.RestoreBackupFile(outputFilename);
                    }
                    else
                    {
                        TaskFileService.WriteFile(codeFile0.Contents, outputFilename);
                    }
                }
            }
        }

        private bool WriteRootsFile(Roots roots, String filename)
        {
            XamlObjectReaderSettings objRdrSettings = new XamlObjectReaderSettings();
            objRdrSettings.LocalAssembly = Assembly.GetExecutingAssembly();
            XamlObjectReader objReader = new XamlObjectReader(roots, objRdrSettings);

            MemoryStream memoryStream = new MemoryStream();
            XmlWriterSettings xmlWriterSettings = new XmlWriterSettings();
            xmlWriterSettings.Encoding = new UTF8Encoding(false);
            xmlWriterSettings.ConformanceLevel = ConformanceLevel.Document;
            xmlWriterSettings.Indent = true;

            XmlWriter xmlWriter = XmlWriter.Create(memoryStream, xmlWriterSettings);
            XamlXmlWriter xamlXmlWriter = new XamlXmlWriter(xmlWriter, objReader.SchemaContext);

            XamlServices.Transform(objReader, xamlXmlWriter);

            xmlWriter.Flush();
            xmlWriter.Close();
            String xamlSaveString = Encoding.UTF8.GetString(memoryStream.ToArray());

            String localXmlns = "xmlns=\"clr-namespace:Microsoft.UI.Xaml.Markup.Compiler.RootLog;assembly=Microsoft.UI.Xaml.Markup.Compiler\"";
            String looseXmlString = xamlSaveString.Replace(localXmlns, String.Empty);

            string pathName = Path.Combine(OutputFolderFullpath, filename);
            WriteOnlyIfContentsAreNew(pathName, looseXmlString);
            return true;
        }

        // Compare two strings, but allow extra whitespace at the end.
        private bool AreGeneratedCodeStringsTheSame(string oldString, string newString)
        {
            string longer = oldString;
            string shorter = newString;

            if (oldString.Length != newString.Length)
            {
                string extra;
                if (oldString.Length < newString.Length)
                {
                    longer = newString;
                    shorter = oldString;
                }
                extra = longer.Substring(shorter.Length);
                longer = longer.Substring(0, shorter.Length);

                if (!String.IsNullOrWhiteSpace(extra))
                {
                    return false;
                }
            }

            // Strings should be the same length now.
            bool isTheSame = longer.Equals(shorter, StringComparison.Ordinal);
            return isTheSame;
        }
        #endregion

        bool GenerateTypeInfo()
        {
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_GenerateTypeInfoStart);
            bool hasCodeGen = true;

            List<FileNameAndContentPair> codeFiles;
            if (ShouldSuppressTypeInfoCodeGen())
            {
                codeFiles = null;
                hasCodeGen = false;
            }
            else
            {
                codeFiles = _codeGenerator.GenerateTypeInfo(_typeInfoCollector.AppXamlInfo);
            }
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_GenerateTypeInfoEnd);

            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_WriteTypeinfoFilesToDiskStart);
            WriteXamlTypeInfoFilesToDisk(codeFiles);
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_WriteTypeinfoFilesToDiskEnd);

            // Tell MSBUILD that we have generated files which need to be built
            if (hasCodeGen)
            {
                if (Language.IsNative)
                {
                    if (codeFiles != null && codeFiles.Count > 0)
                    {
                        foreach (FileNameAndContentPair file in codeFiles)
                        {
                            string outputFileName = Path.Combine(OutputFolderFullpath, file.FileName);

                            // Check the Source Extension so that in C++, only the .cpp and not the .h
                            // are added to the Compile line.
                            if (outputFileName.EndsWith(LanguageSourceExtension))
                            {
                                _generatedCodeFiles.Add(outputFileName);
                            }
                        }
                    }
                }
                else
                {
                    //   The VS language service (debuggers etc.) need to know the names of all the
                    // generated code files, but they only look at Pass1 outputs.  So we return all the
                    // generated files in pass1, even though the pass2 files are not written yet.
                    //   The files need to exist or the language service might get confused.
                    //   We leave existing (previous build) pass2 files, otherwise we create empty files.
                    if (IsPass1)
                    {
                        string xamlTypeInfo = Path.Combine(OutputFolderFullpath, "XamlTypeInfo" + Language.Pass2Extension);
                        // Should not clear the file on design time builds. However, BackupIfExistsAndTruncateToNull should be allowed to
                        // create an empty file if it doesn't exist.
                        if (!this.IsDesignTimeBuild || !File.Exists(xamlTypeInfo))
                        {
                            FileHelpers.BackupIfExistsAndTruncateToNull(xamlTypeInfo);
                        }
                        _generatedCodeFiles.Add(xamlTypeInfo);
                    }
                }
            }
            return true;
        }

        /*
        // TODO: re-enable this if we do decide to modify the appxmanifest for EnableTypeInfoReflection
        private bool IsTypeActivatable(Type type)
        {
            //We don't care about any of the constructor arguments, as long as the attribute is present it's activatable
            var activatableAttributeEnum = Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.GetCustomAttributeData(type, false, DUI.ActivatableAttribute);
            return activatableAttributeEnum != null && activatableAttributeEnum.Any();
        }

        private string GetThreadingModelString(Type type)
        {
            var activatableAttributeEnum = Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ReflectionHelper.GetCustomAttributeData(type, false, DUI.ThreadingAttribute);
            foreach (var attr in activatableAttributeEnum)
            {
                // This is the only attribute type we know about, where there's a constructor with the threading model
                if (attr.ConstructorArguments.Count == 1)
                {
                    // This is a Windows.Foundation.Metadata.ThreadingModel - unfortuanately we don't actually have the enum type,
                    // so we have to hardcode its values
                    int threadingModel = (int)(attr.ConstructorArguments[0].Value);

                    switch (threadingModel)
                    {
                        // Windows.Foundation.Metadata.ThreadingModel.InvalidThreading - can't actually activate this class, return null
                        case 0:
                            return null;

                        // Windows.Foundation.Metadata.ThreadingModel.STA
                        case 1:
                            return "STA";

                        // Windows.Foundation.Metadata.ThreadingModel.MTA
                        case 2:
                            return "MTA";

                        // Windows.Foundation.Metadata.ThreadingModel.Both
                        case 3:
                            return "both";

                        default:
                            return null;
                    }
                }
            }

            return null;
        }

        //<Extensions>
        //  <Extension Category = "windows.activatableClass.inProcessServer">
        //    <InProcessServer>
        //      <Path>CXTestApp.exe</Path>
        //      <ActivatableClass ActivatableClassId="CXTestApp.MainPage" ThreadingModel="both" />
        //    </InProcessServer>
        //  </Extension>
        //</Extensions>
        private void GenerateRuntimeTypeInfoManifestData(XamlSchemaCodeInfo schemaInfo, bool isLibrary, Assembly localAssembly, string targetFileName)
        {
            // For reflection-based type info at runtime, we need to modify the app manifest with all the types of the local assembly
            // if it's CX or CppWinRT, since otherwise the types won't be activatable via reflection.  This method generates the XML we need to inject
            // into the manifest.  MSBuild picks up this generated XML later and does the actual injection.
            string outputFileName = Path.Combine(OutputFolderFullpath, "RuntimeTypeInfo.xml");

            XmlWriterSettings settings = new XmlWriterSettings();

            using (XmlWriter writer = XmlWriter.Create(outputFileName, settings))
            {
                if (!Language.IsManaged)
                {
                    {
                        writer.WriteStartElement("Extension");
                        writer.WriteAttributeString("Category", "windows.activatableClass.inProcessServer");
                        writer.WriteStartElement("InProcessServer");

                        string localAsmName = localAssembly.GetName().Name;

                        writer.WriteStartElement("Path");
                        writer.WriteString(targetFileName);
                        writer.WriteEndElement();

                        foreach (var asmTypeInfo in localAssembly.GetTypes())
                        {
                            // Verify all types defined in the assmebly follow the WinRT naming rules - if a type doesn't, we can't
                            // reflect on the local assembly at all
                            if (!asmTypeInfo.FullName.StartsWith(localAsmName))
                            {
                                LogWarning(new XamlTypeInfoReflectionTypeNamingConventionViolation(asmTypeInfo.FullName, localAsmName));
                            }

                            // Check if the type is activatable
                            if (IsTypeActivatable(asmTypeInfo))
                            {
                                string threadingModel = GetThreadingModelString(asmTypeInfo);
                                if (threadingModel != null)
                                {
                                    writer.WriteStartElement("ActivatableClass");
                                    writer.WriteAttributeString("ActivatableClassId", asmTypeInfo.FullName);
                                    writer.WriteAttributeString("ThreadingModel", threadingModel);
                                    writer.WriteEndElement();
                                }
                            }
                        }

                        writer.WriteEndElement();
                        writer.WriteEndElement();
                    }
                }
                writer.Flush();
                writer.Close();
            }
        }
        */

        void GenerateBindingInfo()
        {
            // Binding Infos are only needed for C++
            if (Language.IsNative)
            {
                List<FileNameAndContentPair> codeFiles;
                Dictionary<string, XamlType> observableVectorTypes = new Dictionary<string, XamlType>();
                Dictionary<string, XamlType> observableMapTypes = new Dictionary<string, XamlType>();
                Dictionary<string, XamlMember> bindingSetters = new Dictionary<string, XamlMember>();
                bool eventBindingUsed = false;

                SaveState.ProcessBindingInfo();

                foreach (SaveStatePerXamlFile state in SaveState.XamlPerFileInfo.Values)
                {
                    if (!IsPass1)
                    {
                        // We only load observableVectorTypes, observableMapTypes and bindingSetters
                        // in Pass2 because they may refer to local types that won't be known in Pass1

                        foreach (SaveStateXamlType type in state.BindingObservableVectorTypes.Values)
                        {
                            if (!observableVectorTypes.ContainsKey(type.FullName))
                            {
                                Type underlyingType = _schemaContext.TypeResolver.GetTypeByFullName(type.FullName);
                                Debug.Assert(underlyingType != null, String.Format("Can't resolve typename '{0}'", type.FullName));

                                if (underlyingType == null)
                                {
                                    LogError(new XamlSchemaError_UnknownTypeError(type.FullName));
                                    continue;
                                }

                                XamlType xamlType = _schemaContext.GetXamlType(underlyingType);
                                Debug.Assert(xamlType != null, String.Format("Can't find type '{0}'", type.FullName));
                                observableVectorTypes.Add(type.FullName, xamlType);
                            }
                        }

                        foreach (SaveStateXamlType type in state.BindingObservableMapTypes.Values)
                        {
                            if (!observableMapTypes.ContainsKey(type.FullName))
                            {
                                Type underlyingType = _schemaContext.TypeResolver.GetTypeByFullName(type.FullName);
                                Debug.Assert(underlyingType != null, String.Format("Can't resolve typename '{0}'", type.FullName));

                                if (underlyingType == null)
                                {
                                    LogError(new XamlSchemaError_UnknownTypeError(type.FullName));
                                    continue;
                                }

                                XamlType xamlType = _schemaContext.GetXamlType(underlyingType);
                                Debug.Assert(xamlType != null, String.Format("Can't find type '{0}'", type.FullName));
                                observableMapTypes.Add(type.FullName, xamlType);
                            }
                        }

                        foreach (SaveStateXamlMember member in state.BindingSetters.Values)
                        {
                            if (!bindingSetters.ContainsKey(member.ToString()))
                            {
                                Type underlyingType = _schemaContext.TypeResolver.GetTypeByFullName(member.DeclaringTypeFullName);
                                Debug.Assert(underlyingType != null, String.Format("Can't resolve typename '{0}'", member.DeclaringTypeFullName));

                                if (underlyingType == null)
                                {
                                    LogError(new XamlSchemaError_UnknownTypeError(member.DeclaringTypeFullName));
                                    continue;
                                }

                                XamlType declaringType = _schemaContext.GetXamlType(underlyingType);
                                Debug.Assert(declaringType != null, String.Format("Can't find type '{0}'", member.DeclaringTypeFullName));
                                XamlMember xamlMember = declaringType.GetMember(member.Name);
                                if (xamlMember == null)
                                {
                                    xamlMember = declaringType.GetAttachableMember(member.Name);
                                }
                                Debug.Assert(xamlMember != null, String.Format("Can't resolve member '{0}'", member.Name));
                                bindingSetters.Add(member.ToString(), xamlMember);
                            }
                        }
                    }
                    eventBindingUsed |= state.HasBoundEventAssignments;
                }
                codeFiles = _codeGenerator.GenerateBindingInfo(observableVectorTypes, observableMapTypes, bindingSetters, eventBindingUsed);
                WriteOutputFilesToDisk(codeFiles, OutputFolderFullpath, true);
            }
        }

        private Type GetIXamlType(List<Assembly> loadedAssemblies)
        {
            return GetType(KnownTypes.IXamlType, loadedAssemblies);
        }

        private Type GetType(string typeName, List<Assembly> loadedAssemblies)
        {
            Type type;
            foreach (Assembly asm in loadedAssemblies)
            {
                type = asm.GetType(typeName);
                if (type != null)
                {
                    return type;
                }
            }
            return null;
        }

        private bool TypeExists(string typeName, List<Assembly> loadedAssemblies)
        {
            return GetType(typeName, loadedAssemblies) != null;
        }

        private bool ReportSchemaErrors(string xamlFile)
        {
            if (_schemaContext.SchemaErrors.Count > 0)
            {
                foreach (var error in _schemaContext.SchemaErrors)
                {
                    LogError("XamlCompiler", error.Code, null, xamlFile, error.LineNumber, error.LineOffset, 0, 0, error.Message);
                }
            }
            if (_schemaContext.SchemaWarnings.Count > 0)
            {
                foreach (var warning in _schemaContext.SchemaWarnings)
                {
                    LogWarning(warning, xamlFile);
                }
            }

            _schemaContext.SchemaWarnings.Clear();

            bool ret = _schemaContext.SchemaErrors.Count == 0;
            _schemaContext.SchemaErrors.Clear();
            return ret;
        }

        private bool ReportXbfErrors(XbfGenerator xbfGenerator)
        {
            if (xbfGenerator.XbfErrors.Count > 0)
            {
                foreach (var error in xbfGenerator.XbfErrors)
                {
                    LogError("XamlCompiler", error.Code, null, error.FileName, error.LineNumber, error.LineOffset, 0, 0, error.Message);
                }
            }
            if (xbfGenerator.XbfWarnings.Count > 0)
            {
                foreach (var warning in xbfGenerator.XbfWarnings)
                {
                    LogWarning(warning);
                }
            }

            xbfGenerator.XbfWarnings.Clear();

            bool ret = xbfGenerator.XbfErrors.Count == 0;
            xbfGenerator.XbfErrors.Clear();
            return ret;
        }

        private XamlProjectInfo GetProjectInfo()
        {
            XamlProjectInfo projectInfo = new XamlProjectInfo();

            projectInfo.CodeGenFlags = this.CodeGenerationControlFlags;
            projectInfo.ProjectName = FileHelpers.GetSafeName(ProjectName) ?? String.Empty;
            projectInfo.RootNamespace = RootNamespace;
            projectInfo.IsLibrary = IsOutputTypeLibrary || IsOutputTypeWinMd;
            projectInfo.IsCLSCompliant = _localAssembly != null && _localAssembly.IsClsCompliant();
            projectInfo.ShouldGenerateDisableXBind = this.EnableXBindDiagnostics;
            projectInfo.EnableTypeInfoReflection = EnableTypeInfoReflection;
            projectInfo.EnableDefaultValidationContextGeneration = EnableDefaultValidationContextGeneration;
            // only C++ pass 2.  Otherwise maps are empty;
            projectInfo.ClassToHeaderFileMap = GetClassToHeaderFileMap();
            projectInfo.AdditionalXamlTypeInfoIncludes = GetAdditionalXamlTypeInfoIncludes();

            if (this.IgnoreSpecifiedTargetPlatformMinVersion)
            {
                projectInfo.TargetPlatformMinVersion = ReleaseDefinition.MaxSupportedVersion;
            }
            else if (!String.IsNullOrEmpty(this.TargetPlatformMinVersion))
            {
                projectInfo.TargetPlatformMinVersion = new Version(this.TargetPlatformMinVersion);
            }

            projectInfo.IsInputValidationEnabled = TypeExists(KnownTypes.IInputValidationControl, _loadedAssemblies);
            projectInfo.IsWin32App = EnableWin32Codegen;
            projectInfo.UsingCSWinRT = UsingCSWinRT;
            projectInfo.PrecompiledHeaderFile = PrecompiledHeaderFile;
            return projectInfo;
        }

        private bool ProcessXamlFile_XamlTypeInfo(TaskItemFilename tif, ref XamlDomObject xamlDomRoot)
        {
            if (!LoadAndValidateXamlDom(tif.SourceXamlFullPath, tif.ApparentRelativePath, out xamlDomRoot))
            {
                return false;
            }

            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageTypeCollectStart);
            _typeInfoCollector.Collect(xamlDomRoot);
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageTypeCollectEnd);

            if (!ReportSchemaErrors(tif.XamlGivenPath))
            {
                return false;
            }

            // Check the root for x:Class
            // 1) The type needs to be present to support Jupiter runtime checks in LoadComponent()
            // 2) Page classes need to be in the XamlTypeInfo to support direct Navigation to a type name.
            // 3) In C++ applications we need to construct a map of x:Class names to *.xaml.h files.
            string classFullName = DomHelper.GetStringValueOfProperty(xamlDomRoot, XamlLanguage.Class);
            if (!String.IsNullOrWhiteSpace(classFullName))
            {
                if (tif.IsApplication)
                {
                    // store the Application class name which is required later for TypeInfoCodeGen
                    _typeInfoCollector.AppXamlInfo = new ClassName(classFullName);
                }
                else
                {
                    var xamlClassTypeName = XamlSchemaCodeInfo.GetXamlTypeNameFromFullName(classFullName);
                    XamlType classType = _schemaContext.GetXamlType(xamlClassTypeName);
                    if (classType != null)
                    {
                        // If the class is "hard deprecated" it is a build error to use it.
                        // So keep the type out of the generated code.
                        DirectUIXamlType duiClassType = (DirectUIXamlType)classType;
                        if (!duiClassType.IsHardDeprecated)
                        {
                            _typeInfoCollector.SchemaInfo.AddTypeAndProperties(classType);
                        }
                        _typeInfoCollector.AddTypeToRootLog((DirectUIXamlType)classType);
                    }
                    else
                    {
                        // xClass types come from local XAML which we expect to be compiled into the local assembly.
                        AssemblyName localAssemblyName = _localAssembly.GetName();
                        bool isWinMd = (localAssemblyName.ContentType == AssemblyContentType.WindowsRuntime);
                        string localBinaryName = localAssemblyName.Name;
                        if (isWinMd && !classFullName.StartsWith(localBinaryName))
                        {
                            LogError_ClassDoesntMatchWinmdName(classFullName, localBinaryName + ".winmd", tif.XamlGivenPath);
                            return false;
                        }
                        LogError_ClassIsNotFoundInAssembly(classFullName, localBinaryName, tif.XamlGivenPath);
                        return false;
                    }
                }
            }

            return true;
        }

        private bool ProcessXamlFile_PerPageInfo(TaskItemFilename tif, ref XamlDomObject xamlDomRoot)
        {
            string projectFolder = Path.GetDirectoryName(ProjectPath);
            var harvester = new XamlHarvester(projectFolder, IsPass1, this.XamlPlatform);

            if (xamlDomRoot == null)
            {
                if (!LoadAndValidateXamlDom(tif.SourceXamlFullPath, tif.ApparentRelativePath, out xamlDomRoot))
                {
                    return false;
                }
            }

            // Don't generate a field for an x:Name on the root element in Native code.
            // COM style reference counting is vulnerable to refernce loops and a page
            // with a field pointing at itself is a memory leak.
            // Managed Garbage Collection does not have this problem.
            if (Language.IsNative)
            {
                harvester.SkipNameFieldsForRootElements = true;
            }
            string classFullName = XamlHarvester.GetClassFullName(xamlDomRoot);
            if (!String.IsNullOrWhiteSpace(classFullName))
            {
                // Compute or fetch the existing ClassCodeInfo
                XamlClassCodeInfo classCodeInfo = null;
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageHarvestStart, tif.XamlGivenPath);
                if (!_classCodeInfos.TryGetValue(classFullName, out classCodeInfo))
                {
                    classCodeInfo = harvester.HarvestClassInfo(classFullName, xamlDomRoot, tif.IsApplication);
                    classCodeInfo.RootNamespace = RootNamespace;
                    classCodeInfo.TargetFolder = tif.TargetFolder;
                    classCodeInfo.PriIndexName = PriIndexName;
                    if (!String.IsNullOrWhiteSpace(XamlResourceMapName) ||
                        !String.IsNullOrWhiteSpace(tif.XamlResourceMapName))
                    {
                        classCodeInfo.XamlResourceMapName = String.IsNullOrEmpty(tif.XamlResourceMapName) ? XamlResourceMapName : tif.XamlResourceMapName;
                    }
                    if (!String.IsNullOrWhiteSpace(XamlComponentResourceLocation) ||
                        !String.IsNullOrWhiteSpace(tif.XamlComponentResourceLocation))
                    {
                        classCodeInfo.XamlComponentResourceLocation = GetComponentResourceLocation(tif.XamlComponentResourceLocation);
                    }
                    _classCodeInfos.Add(classFullName, classCodeInfo);
                }
                else
                {
                    classCodeInfo.TargetFolder = FileHelpers.ComputeBaseFolder(classCodeInfo.TargetFolder, tif.TargetFolder);
                }

                // Harvest the Name Fields, Event etc, from the XamlFile.
                XamlFileCodeInfo fileCodeInfo = harvester.HarvestXamlFileInfo(classCodeInfo, xamlDomRoot);
                if (fileCodeInfo != null)
                {
                    // set various file path related fields of the XamlFileCodeInfo.
                    fileCodeInfo.ApparentRelativePath = tif.ApparentRelativePath;
                    fileCodeInfo.FullPathToXamlFile = tif.SourceXamlFullPath;
                    fileCodeInfo.SourceXamlGivenPath = tif.XamlGivenPath;
                    fileCodeInfo.RelativePathFromGeneratedCodeToXamlFile = tif.RelativePathFromGeneratedCodeToXamlFile;
                    fileCodeInfo.XamlOutputFilename = tif.XamlOutputFilename;

                    classCodeInfo.AddXamlFileInfo(fileCodeInfo);
                    if (Language.IsNative)
                    {
                        SaveState.AddBindingInfo(fileCodeInfo);
                    }
                    if (fileCodeInfo.XPropertyInfo?.xProperties?.FirstOrDefault() != null)
                    {
                        LogWarning(new XamlValidationWarningExperimental(ErrorCode.WMC1503, "x:Property"));
                    }
                }
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageHarvestEnd, tif.XamlGivenPath);

                // The Harvester should be protected from most problems by the Validator ensuring the XAML is valid,
                // But the Type System can still find issues.  Report them here and fail.
                if (!ReportSchemaErrors(tif.XamlGivenPath))
                {
                    return false;
                }

                if (tif.IsApplication)
                {
                    if (classCodeInfo.HasEventAssignments)
                    {
                        LogError_EventsInAppXaml(tif.XamlGivenPath);
                        return false;
                    }
                }
            }
            return true;  // No errors
        }

        private bool GenerateClasslessXamlOutputFile(string sourceXamlFullPath, string givenXamlPath, string targetFolder, string codeFileName, string xamlOutputFileName)
        {
            List<FileNameAndContentPair> generatedCodeFiles = null;
            List<FileNameAndContentPair> generatedXamlFiles = null;
            string codeOutputPath = Path.Combine(targetFolder, codeFileName + GeneratedExtension);
            string xbfOutputPath = Path.Combine(targetFolder, codeFileName + KnownStrings.XbfExtension);

            // Non-Class files need to generated non-zero length files
            // Zero length files are considered not "up to date".
            // MultiXaml TODO these should go away now that we have saved data.
            generatedCodeFiles = new List<FileNameAndContentPair>();
            generatedCodeFiles.Add(new FileNameAndContentPair(Path.GetFileName(codeOutputPath), GeneratedExtension.EndsWith("cpp") ? " #include \"pch.h\"" : " "));

            if (!IsPass1)
            {
                // Don't edit the XAML.  Prepare a copy to the Output directory
                // with the other edited XAML files.
                generatedXamlFiles = new List<FileNameAndContentPair>();
                generatedXamlFiles.Add(new FileNameAndContentPair(Path.GetFileName(givenXamlPath), File.ReadAllText(givenXamlPath)));

                _newlyGeneratedXamlFiles.Add(new XbfFileNameInfo(sourceXamlFullPath, givenXamlPath, xamlOutputFileName, xbfOutputPath));
            }

            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_WriteFilesToDiskStart);
            WriteOutputFilesToDisk(generatedCodeFiles, targetFolder, true);
            WriteOutputFilesToDisk(generatedXamlFiles, targetFolder, true);
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_WriteFilesToDiskEnd);
            return true;
        }

        // Prune any extraneous BindUniverses here.  We can't do this check in XamlHarvester as we don't know whether a BindUniverse
        // would be needed for an out-of-scope named element bind until the x:Bind has actually been evaluated after
        // the bind universes have been parsed and their x:Binds evaluated
        private void PruneBindUniverses(XamlClassCodeInfo classCodeInfo)
        {
            List<BindUniverse> prunedList = new List<BindUniverse>();
            foreach (BindUniverse bindUniverse in classCodeInfo.BindUniverses)
            {
                bool needsPrune = true;

                // If the bind universe has any elements that use x:Bind or is needed for an out-of-scope binding, we can't prune it.
                if (bindUniverse.HasBindAssignments || bindUniverse.HasBoundEventAssignments || bindUniverse.NeededForOuterScopeElement)
                {
                    needsPrune = false;
                }

                if (needsPrune)
                {
                    prunedList.Add(bindUniverse);
                }
            }

            foreach (BindUniverse prunedUniverse in prunedList)
            {
                classCodeInfo.BindUniverses.Remove(prunedUniverse);
                foreach (BindUniverse regularUniverse in classCodeInfo.BindUniverses)
                {
                    if (regularUniverse.Children.Contains(prunedUniverse))
                    {
                        regularUniverse.Children.Remove(prunedUniverse);
                    }

                    if (regularUniverse.Parent == prunedUniverse)
                    {
                        regularUniverse.Parent = null;
                    }
                }
            }
        }

        private bool GeneratePageOutputFiles(XamlClassCodeInfo classCodeInfo)
        {
            List<FileNameAndContentPair> generatedCodeFiles = null;
            List<FileNameAndContentPair> generatedXamlFiles = null;
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageCodeGenStart, classCodeInfo.BaseFileName);

            if (!IsPass1)
            {
                if (classCodeInfo.HasBindAssignments || classCodeInfo.HasBoundEventAssignments)
                {
                    var allIssues = new List<XamlCompileErrorBase>();
                    foreach (BindUniverse bindUniverse in classCodeInfo.BindUniverses)
                    {
                        var issues = bindUniverse.Parse(classCodeInfo, _projectInfo.TargetPlatformMinVersion);
                        allIssues.AddRange(issues);
                    }

                    PruneBindUniverses(classCodeInfo);

                    foreach (var issue in allIssues.OfType<XamlCompileError>())
                    {
                        var error = issue as XamlCompileError;
                        LogError("XamlCompiler", error.Code, null, error.FileName, error.LineNumber, error.LineOffset, 0, 0, error.Message);
                    }
                    foreach (var issue in allIssues.OfType<XamlCompileWarning>())
                    {
                        var warning = issue as XamlCompileWarning;
                        LogWarning(warning);
                    }

                    // Must return false here if we have errors
                    if (allIssues.OfType<XamlCompileError>().Any())
                    {
                        return false;
                    }
                }
            }

            // Create the Generated code strings
            IEnumerable<FileNameAndChecksumPair> xamlFilesChecksumPairs;
            generatedCodeFiles = _codeGenerator.GenerateCodeBehind(classCodeInfo, out xamlFilesChecksumPairs);
            if (!classCodeInfo.IsApplication && ShouldSuppressPageCodeGen())
            {
                generatedCodeFiles = null;
            }
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageCodeGenEnd, classCodeInfo.BaseFileName);

            // We need to delete the existing Pass2 code files in Pass1 in order to get a clean intermediate compile
            // of the local assembly.   But we don't need to do this if Pass1 is just a Design Time Build from VS.
            // There was an issue with C++ where if the VS buffer was dirty, Design Type Build would run immediatly after
            // normal build and can delete the pass2 (.hpp) file, if we don't check what *kind* of Pass1 we are running.
            if (IsPass1 && !IsDesignTimeBuild)
            {
                Debug.Assert(!String.IsNullOrEmpty(classCodeInfo.BaseFileName));
                string srcOutputFileName2 = Path.Combine(classCodeInfo.TargetFolder, classCodeInfo.BaseFileName + Language.Pass2Extension);
                if (File.Exists(srcOutputFileName2))
                {
                    //There was a build before this that generated a Pass2 file.  Back up the previously generated Pass2 file so we can restore it during our Pass2.
                    //If the file's contents haven't changed between the two builds, our Pass2 won't write to disk and cause the file to be recompiled due to the newer timestamp
                    FileHelpers.BackupFile(srcOutputFileName2);
                    File.Delete(srcOutputFileName2);
                }
            }

            // Now edit the XAML files.
            if (!IsPass1)
            {
                foreach (XamlFileCodeInfo fileCodeInfo in classCodeInfo.PerXamlFileInfo)
                {
                    if (!GenerateEditedXamlFile(ref generatedXamlFiles, classCodeInfo, fileCodeInfo))
                    {
                        return false;
                    }
                    TaskItemFilename tif = SourceFileManager.FindTaskItemByFullPath(fileCodeInfo.FullPathToXamlFile);
                    Debug.Assert(tif != null);
                    string hashString = xamlFilesChecksumPairs.Where(x => x.FileName == fileCodeInfo.FullPathToXamlFile).FirstOrDefault().Checksum;
                    _newlyGeneratedXamlFiles.Add(new XbfFileNameInfo(tif.SourceXamlFullPath, tif.XamlGivenPath, tif.XamlOutputFilename, tif.XbfOutputFilename, hashString));
                }
            }

            //During Pass2, if there was a Pass2 code file from a previous build, restore it before writing to disk.  That way if this build's Pass2 file
            //is the same as the one previously on disk, we won't write to disk and cause a recompile due to the newer timestamp.
            if (!IsPass1)
            {
                string srcOutputFileName2 = Path.Combine(classCodeInfo.TargetFolder, classCodeInfo.BaseFileName + Language.Pass2Extension);

                //This is safe even if there was no file originally backed up (this is the first time building) - it will just return false, which we don't care about
                //since the WriteOutputFilesToDisk call below will either create a new file, overwrite the file if there were changes,
                //or do nothing if the backed up file and newly genned file have the same content
                FileHelpers.RestoreBackupFile(srcOutputFileName2);
            }

            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_WriteFilesToDiskStart);
            WriteOutputFilesToDisk(generatedCodeFiles, classCodeInfo.TargetFolder, true);
            WriteOutputFilesToDisk(generatedXamlFiles, classCodeInfo.TargetFolder, true);
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_WriteFilesToDiskEnd);
            return true;
        }

        private void CreateDirectoryIfNecessary(DirectoryInfo directoryInfo)
        {
            if (!directoryInfo.Exists)
            {
                CreateDirectoryIfNecessary(directoryInfo.Parent);
                directoryInfo.Create();
            }
        }

        private void CreateFileIfNecessary(string filename)
        {
            FileInfo fileInfo = new FileInfo(filename);
            CreateDirectoryIfNecessary(fileInfo.Directory);
            if (!fileInfo.Exists)
            {
                using (FileStream f = fileInfo.Create()) { /* create empty file */ }
            }
        }

        private bool GenerateEditedXamlFile(ref List<FileNameAndContentPair> generatedSources, XamlClassCodeInfo classCodeInfo, XamlFileCodeInfo fileCodeInfo)
        {
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageEditStart, fileCodeInfo.SourceXamlGivenPath);
            XamlConnectionIdRewriter connectionIdRewriter = new XamlConnectionIdRewriter();
            string newXamlContents = connectionIdRewriter.Edit(fileCodeInfo.FullPathToXamlFile, classCodeInfo, fileCodeInfo);

            if (connectionIdRewriter.Errors.Count > 0)
            {
                foreach (var error in connectionIdRewriter.Errors)
                {
                    LogError("XamlCompiler", error.Code, null, fileCodeInfo.SourceXamlGivenPath, error.LineNumber, error.LineOffset, 0, 0, error.Message);
                }
                return false;
            }

            Debug.Assert(newXamlContents != null);

            string filePath = FileHelpers.GetRelativePath(classCodeInfo.TargetFolder, fileCodeInfo.XamlOutputFilename);
            if (generatedSources == null)
            {
                generatedSources = new List<FileNameAndContentPair>();
            }
            generatedSources.Add(new FileNameAndContentPair(filePath, newXamlContents));

            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageEditEnd, fileCodeInfo.SourceXamlGivenPath);
            return true;
        }

        #region XBF processing
        private bool GenerateXbfFiles(IEnumerable<IXbfFileNameInfo> xamlList)
        {
            if (xamlList.Any())
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_GenerateXBFStart, xamlList.Count().ToString());
                XbfGenerator xbfGenerator = new XbfGenerator(_projectInfo, _xamlMetadataProvider);
                xbfGenerator.SetXamlInputFiles(xamlList);
                bool allOk = xbfGenerator.GenerateXbfFiles(XbfGenerationFlags);
                if (allOk)
                {
                    AddToGeneratedXbfFiles(xamlList);
                }
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_GenerateXBFEnd);
                bool noErrors = ReportXbfErrors(xbfGenerator);
                Debug.Assert(allOk == noErrors);
                return allOk;
            }
            return true;
        }

        private bool GenerateSdkXbfFiles()
        {
            bool allOk = true;
            if (SourceFileManager.SdkXamlTaskItems != null && SourceFileManager.SdkXamlTaskItems.Count > 0)
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_GenerateSdkXBFStart, SourceFileManager.SdkXamlTaskItems.Count.ToString());

                var xbfGenerator = new XbfGenerator(_projectInfo, _xamlMetadataProvider);
                if (SourceFileManager.SdkNon80XamlTaskItems != null && SourceFileManager.SdkNon80XamlTaskItems.Count > 0)
                {
                    xbfGenerator.SetXamlInputFilesFromTaskItems(SourceFileManager.SdkNon80XamlTaskItems, isSdk: true);
                    allOk = xbfGenerator.GenerateXbfFiles(this.XbfGenerationFlags);
                }
                if (allOk && SourceFileManager.Sdk80XamlTaskItems != null && SourceFileManager.Sdk80XamlTaskItems.Count > 0)
                {
                    xbfGenerator.SetXamlInputFilesFromTaskItems(SourceFileManager.Sdk80XamlTaskItems, isSdk: true);
                    allOk = xbfGenerator.GenerateXbfFiles(this.XbfGenerationFlags, v80Compat: true);
                }
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_GenerateSdkXBFEnd);
                bool noErrors = ReportXbfErrors(xbfGenerator);
                Debug.Assert(allOk == noErrors);
            }
            return allOk;
        }

        private void AddGeneratedXbfFile(string filename)
        {
            if (_generatedXbfFiles.Where(x => x == filename).FirstOrDefault() == null)
            {
                _generatedXbfFiles.Add(filename);
            }
        }

        private void AddToGeneratedXbfFiles(IEnumerable<IXbfFileNameInfo> xamlXbfInfoList)
        {
            if (xamlXbfInfoList != null)
            {
                foreach (IXbfFileNameInfo xbf in xamlXbfInfoList)
                {
                    AddGeneratedXbfFile(xbf.OutputXbfName);
                }
            }
        }

        #endregion

        #region XProperty processing
        private bool ProcessXProperty(XamlDomObject domRoot, XamlDomItem domItem, string xamlApparentRelativeName, string[] xamlLines)
        {
            XamlDomObject domPropertyObject = domItem as XamlDomObject;
            xProperty storedProp = new xProperty();
            storedProp.OriginalXProperty = domPropertyObject;
            storedProp.CodegenComment = String.Format("{0}, line {1}", xamlApparentRelativeName, domPropertyObject.StartLineNumber);
            foreach (XamlDomMember domPropertyMember in domPropertyObject.MemberNodes)
            {
                switch (domPropertyMember.Member.Name)
                {
                    case "Name":
                        storedProp.Name = ((domPropertyMember.Item as XamlDomValue).Value) as string;
                        break;
                    case "ChangedHandler":
                        storedProp.ChangedHandler = ((domPropertyMember.Item as XamlDomValue).Value) as string;
                        break;
                    case "IsReadOnly":
                        storedProp.IsReadOnly = Boolean.Parse(((domPropertyMember.Item as XamlDomValue).Value) as string);
                        break;
                    case "Type":
                        string strType = ((domPropertyMember.Item as XamlDomValue).Value) as string;
                        string fullTypeName = null;
                        XamlType propXamlType = domRoot.ResolveXmlName(strType);
                        if (propXamlType != null)
                        {
                            string propShortName = XamlSchemaCodeInfo.GetFullGenericNestedName(propXamlType.UnderlyingType, ProgrammingLanguage.IdlWinRT, false);

                            fullTypeName = propXamlType.UnderlyingType.Namespace + "." + propXamlType.Name;
                            storedProp.PropertyType = propXamlType;
                        }
                        else
                        {
                            // Local types in Pass 1 don't have an underlying type yet
                            Debug.Assert(IsPass1);
                            System.Xaml.Schema.XamlTypeName typeName = domRoot.ResolveXmlNameToTypeName(strType);
                            fullTypeName = typeName.Namespace.StripUsingPrefix() + "." + typeName.Name;
                        }
                        storedProp.FullTypeName = fullTypeName;
                        break;
                    case "DefaultValue":
                    case "_UnknownContent":
                        // If the default value is a markup object, we need to get a string representation of the markup, modified
                        // with the namespace declaration from the page's root dom in case the object uses those namespaces
                        XamlDomObject domValueObj = domPropertyMember.Item as XamlDomObject;
                        if (domValueObj != null)
                        {
                            // Collect type info from the tree.  If there are custom types, we need to make sure
                            // we generate type info for them since the parser needs to instantiate them
                            if (!IsPass1)
                            {
                                _typeInfoCollector.Collect(domValueObj);
                            }

                            // Get the fixed source info of the object
                            FixedSourceInfo valSrcInfo = XamlSourceInfoFixer.GetFixedSourceInfo(new StrippableObject(domValueObj), xamlLines);
                            string openingTag = XamlSourceInfoFixer.ReadMarkup(valSrcInfo.StartOpeningTag, valSrcInfo.StartClosingTag, xamlLines);

                            //Build the namespace declarations to be injected into the opening tag.  Prepend a space to avoid conflicts with any preceding
                            // property assignments
                            StringBuilder sb = new StringBuilder();
                            sb.Append(" ");
                            foreach (NamespaceDeclaration nameSpace in domRoot.GetNamespacePrefixes())
                            {
                                sb.Append("xmlns");

                                // If there's a prefix, we need to append a colon and a prefix to the assignment.  Otherwise it's
                                // just the xmlns assignment
                                if (!String.IsNullOrEmpty(nameSpace.Prefix))
                                {
                                    sb.Append(":");
                                    sb.Append(nameSpace.Prefix);
                                }

                                //Append the namespace assignment with a trailing space
                                sb.Append("=\"");
                                sb.Append(nameSpace.Namespace);
                                sb.Append("\" ");
                            }

                            string namespacesInjection = sb.ToString();

                            // Now inject the namespaces at the end of the opening tag.  For self-closing tags we need to inject 2 spaces before the closing
                            // tag insead of 1 to preserve the terminating slash.  We also want to lop off the closing tag from the string
                            // so we don't get a duplicate closing tag when appending the rest of the default value declaration

                            int openingTagEndIndex = openingTag.LastIndexOf('>') - 1;
                            string endingString = "";
                            if (valSrcInfo.SelfClosing)
                            {
                                Debug.Assert(openingTag[openingTagEndIndex] == '/');
                                openingTagEndIndex--;
                                endingString = "/";
                            }

                            string injectedOpeningTag = openingTag.Substring(0, openingTagEndIndex + 1) + namespacesInjection + endingString;

                            // Get the rest of the default value
                            string restOfDefaultValue = XamlSourceInfoFixer.ReadMarkup(valSrcInfo.StartClosingTag, valSrcInfo.EndClosingTag, xamlLines);

                            // Put it all together
                            string modifiedDefaultValueMarkup = injectedOpeningTag + restOfDefaultValue;

                            // Convert the markup string into a literal (e.g. if there's a quote in the current markup string, the literalized string will have a
                            // backslash to escape it so the compiler won't complain after spitting it out in codegen).
                            using (var writer = new StringWriter())
                            {
                                string providerLang = null;
                                switch (Language.Name)
                                {
                                    case ProgrammingLanguage.CSharp:
                                        providerLang = "cs";
                                        break;
                                    case ProgrammingLanguage.VB:
                                        providerLang = "vb";
                                        break;
                                    default:
                                        // The cpp codedom provider was deprecated by VS in version dev15.
                                        LogError(new ErrorXPropertyUsageNotSupported(domPropertyObject, Language));
                                        return false;
                                }
                                using (var provider = System.CodeDom.Compiler.CodeDomProvider.CreateProvider(providerLang))
                                {
                                    provider.GenerateCodeFromExpression(new System.CodeDom.CodePrimitiveExpression(modifiedDefaultValueMarkup), writer, null);
                                    storedProp.DefaultValueMarkup = writer.ToString();
                                }
                            }
                        }
                        else
                        {
                            storedProp.DefaultValueString = ((domPropertyMember.Item as XamlDomValue).Value) as string;
                        }
                        break;
                }
            }
            domRoot.XPropertyInfo.xProperties.Add(storedProp);
            return true;
        }

        // Strip the x:Properties node out of the loaded dom, and process its properties
        private bool PreprocessxProperties(XamlDomObject domRoot, string xamlFileName, string xamlApparentRelativeName)
        {
            // Need a content property to check for x:Properties
            XamlMember contentProperty = domRoot.Type.ContentProperty;
            if (contentProperty == null)
            {
                return true;
            }

            XamlDomMember xPropertiesDomMember = null;
            foreach (XamlDomMember rootDomMember in domRoot.MemberNodes)
            {
                if (rootDomMember.Member.Equals(contentProperty))
                {
                    XamlDomObject xProperties = rootDomMember?.Item as XamlDomObject;

                    //TODO: add check for namespace for Properties
                    if (xProperties != null && xProperties.Type.Equals(_schemaContext.DirectUIXamlLanguage.Properties))
                    {
                        xPropertiesDomMember = rootDomMember;
                        domRoot.XPropertyInfo = new xPropertyInfo();
                        domRoot.XPropertyInfo.xProperties = new List<xProperty>();
                        domRoot.XPropertyInfo.xPropertiesNode = xProperties;
                        domRoot.XPropertyInfo.xPropertiesRoot = domRoot;

                        string[] xamlLines = null;
                        try
                        {
                            xamlLines = File.ReadAllLines(xamlFileName);
                        }
                        catch (Exception ex)
                        {
                            var error = new XamlRewriterErrorFileOpenFailure(xamlFileName, ex.Message);
                            LogError("XamlCompiler", error.Code, null, xamlFileName, error.LineNumber, error.LineOffset, 0, 0, error.Message);
                            return false;
                        }

                        foreach (XamlDomMember domMember in xProperties.MemberNodes)
                        {
                            //Process the collection of x:Properties
                            if (domMember.Member.Name.Equals("_Items"))
                            {
                                foreach (XamlDomItem domItem in domMember.Items)
                                {
                                    if (!ProcessXProperty(domRoot, domItem, xamlApparentRelativeName, xamlLines))
                                    {
                                        return false;
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }

            //Remove the x:Properties member as a content member, since it isn't really part of the content property
            if (xPropertiesDomMember != null)
            {
                domRoot.MemberNodes.Remove(xPropertiesDomMember);
            }
            return true;
        }
        #endregion

        // Load the Xaml into a XamlDom and Validate before doing any analysis.
        // The Harvester and TypeCollector expect the XAML to be (mostly) correct.
        private bool LoadAndValidateXamlDom(string xamlFileName, string xamlApparentRelativeName, out XamlDomObject xamlDomRoot)
        {
            xamlDomRoot = LoadXamlDom(xamlFileName);
            if (xamlDomRoot == null)
            {
                return false;
            }

            if (!PreprocessxProperties(xamlDomRoot, xamlFileName, xamlApparentRelativeName))
            {
                return false;
            }

            return ValidateXaml(xamlDomRoot, xamlFileName);
        }

        private XamlDomObject LoadXamlDom(string xamlPath)
        {
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageLoadStart, xamlPath);
            XamlDomObject xamlDomRoot = null;

            try
            {
                // We need to read the "Contents" from the Task Service.  So that
                // we can read from IDE buffers.
                using (var fileReader = TaskFileService.GetFileContents(xamlPath))
                {
                    using (XmlReader xmlReader = XmlReader.Create(fileReader))
                    {
                        XamlXmlReaderSettings settings = new XamlXmlReaderSettings();
                        settings.LocalAssembly = _localAssembly;
                        settings.AllowProtectedMembersOnRoot = true;
                        settings.ProvideLineInfo = true;
                        settings.IgnoreUidsOnPropertyElements = true;

                        var xamlReader = new XamlXmlReader(xmlReader, _schemaContext, settings);
                        var domWriter = new XamlDomWriter(_schemaContext, xamlPath);
                        XamlServices.Transform(xamlReader, domWriter, true);

                        xamlDomRoot = domWriter.RootNode as XamlDomObject;
                    }
                }
            }
            catch (XmlException e)
            {
                LogError_XamlXMLParsingError(e, xamlPath);
            }
            catch (XamlParseException e)
            {
                LogError_XamlXMLParsingError(e, xamlPath);
            }
            catch (Exception e)
            {
                LogError_XamlInternalError(e, xamlPath);
            }
            finally
            {
                PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageLoadEnd, xamlPath);
            }
            return xamlDomRoot;
        }

        private bool ValidateXaml(XamlDomObject domRoot, string xamlRelativePath)
        {
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageValidateStart, xamlRelativePath);

            var validator = new XamlDomValidator();

            validator.IsPass1 = IsPass1;
            validator.TargetPlatformMinVersion = _projectInfo.TargetPlatformMinVersion;
            validator.XamlPlatform = this.XamlPlatform;
            validator.Validate(domRoot);

            bool schemaErrors = !ReportSchemaErrors(xamlRelativePath);

            if (validator.HasErrors)
            {
                foreach (var error in validator.Errors)
                {
                    LogError("XamlCompiler", error.Code, null, xamlRelativePath, error.LineNumber, error.LineOffset, 0, 0, error.Message);
                }
            }
            if (validator.HasWarnings)
            {
                foreach (var warning in validator.Warnings)
                {
                    LogWarning(warning, xamlRelativePath);
                }
            }
            PerformanceUtility.FireCodeMarker(CodeMarkerEvent.perfXC_PageValidateEnd, xamlRelativePath);
            if (validator.HasErrors || schemaErrors)
            {
                return false;
            }
            return true;
        }

        private bool ValidateXamlTypeInfo()
        {
            var validator = new XamlTypeInfoValidator(_schemaContext);

            validator.Validate(_typeInfoCollector.SchemaInfo.TypeTableFromAllAssemblies);

            foreach (var error in validator.Errors)
            {
                LogError(error);
            }
            foreach (var warning in validator.Warnings)
            {
                LogWarning(warning);
            }
            return !validator.Errors.Any();
        }

        private bool IsOutputTypeLibrary
        {
            get { return KS.EqIgnoreCase(OutputType, KnownStrings.Library); }
        }

        private bool IsOutputTypeWinMd
        {
            get { return KS.EqIgnoreCase(OutputType, KnownStrings.WinMdObj); }
        }

        private string GetComponentResourceLocation(string xamlComponentResourceLocation)
        {
            string componentResourceLocation = String.IsNullOrEmpty(xamlComponentResourceLocation) ? XamlComponentResourceLocation : xamlComponentResourceLocation;

            if (!String.IsNullOrEmpty(componentResourceLocation))
            {
                if (componentResourceLocation.ToLower(CultureInfo.InvariantCulture) == "application")
                {
                    componentResourceLocation = "Application";
                }
                else if (componentResourceLocation.ToLower(CultureInfo.InvariantCulture) == "nested")
                {
                    componentResourceLocation = "Nested";
                }
                else
                {
                    // non localized - internal windows build only code
                    LogError_XamlInternalError(new Exception("ComponentResourceLocation '" + componentResourceLocation + "' unrecognized."), null);
                }
            }

            return componentResourceLocation;
        }

        //
        // We are passed in a list of ReferenceAssemblies
        // Return 3 lists,
        //  system assemblies which hold most commonly used types
        //  non system assemblies
        //  rarely uses system assemblies
        //
        //
        private void SortReferenceAssemblies(out List<string> systemItems, out List<string> nonSystemItems, out List<string> systemExtraItems)
        {
            List<string> systemList = new List<string>();
            List<string> nonSystemList = new List<string>();
            List<string> systemExtraList = new List<string>();
            foreach (var item in ReferenceAssemblies)
            {
                string asmName = Path.GetFileName(item.ItemSpec).ToLower(CultureInfo.InvariantCulture);

                bool loadByDefault = false;

                // It's unclear whether these winmds are necessary.  Initially, only the merged Microsoft.UI.winmd
                // had a case.  With the adoption of IXP transport and internal packages, namespaces are now broken
                // to depth 3, so the same metadata requires 7 entries.
                switch (asmName)
                {
                    case "microsoft.winui.dll":
                    case "microsoft.ui.text.winmd":
                    case "microsoft.ui.winmd":
                    case "microsoft.ui.xaml.winmd":
                    case "mscorlib.dll":
                    case "windows.winmd":
                    case "microsoft.csharp.metadata_dll":
                    case "microsoft.visualbasic.metadata_dll":
                    case "system.runtime.windowsruntime.ui.xaml.metadata_dll":
                    case "system.runtime.windowsruntime.metadata_dll":
                    case "microsoft.csharp.dll":
                    case "microsoft.visualbasic.dll":
                    case "system.runtime.windowsruntime.ui.xaml.dll":
                    case "system.runtime.windowsruntime.dll":
                        loadByDefault = true;
                        break;
                }

                bool isMsCorLib = asmName.Equals("mscorlib.dll", StringComparison.OrdinalIgnoreCase);

                // Only consider nuget based assemblies as system assemblies in design time builds, to push these including .Net
                // which ships as a Nuget onto systemExtraList, which is omitted from the initial lookup assemblies we look at
                // in DirectUISchemaContext.GetXamlTypeFromUsing() as a saving perf tactic from dev11.
                // For real builds, we will not do that, and treat libraries referenced as nugets as nonSystem assemblies, such that if they
                // expose TypeMetadatProviders they get parsed and get code to hook them generated in XamlTypeInfo.
                if (item.IsSystemReference|| isMsCorLib || (this.IsDesignTimeBuild && item.IsNuGetReference))
                {
                    if (loadByDefault)
                        systemList.Add(item.ItemSpec);
                    else
                        systemExtraList.Add(item.ItemSpec);
                }
                else
                {
                    nonSystemList.Add(item.ItemSpec);
                }
            }
            systemItems = systemList;
            nonSystemItems = nonSystemList;
            systemExtraItems = systemExtraList;
        }

        private IEnumerable<IFileItem> GetAdditionalXamlTypeInfoIncludes()
        {
            return ClIncludeFiles?
                .Where(h => !string.IsNullOrEmpty(h.DependentUpon))
                .Where(h => {
                    // Include all headers that are dependent on IDL files (for C++\WinRT)
                    var fi = new FileInfo(h.DependentUpon);
                    return string.Compare(fi.Extension, ".idl", false) == 0;
                });
        }

        private Dictionary<String, String> GetClassToHeaderFileMap()
        {
            Dictionary<String, String> className2headerMap = new Dictionary<string, String>();

            if (IsPass1)
            {
                return className2headerMap;  // empty
            }

            if (Language.IsManaged || ClIncludeFiles == null || ClIncludeFiles.Count == 0)
            {
                return className2headerMap;  // empty
            }

            // ClInclude is the '.h' files.  (the '.cpp' files are in ClCompile)
            foreach (var item in ClIncludeFiles)
            {
                // Look for a dependent upon XAML file so we can link the XAML class to the C++ header file.
                string dependentFileName = item.DependentUpon;
                if (String.IsNullOrWhiteSpace(dependentFileName))
                {
                    continue;
                }

                string dependentFileDir = Path.GetDirectoryName(dependentFileName);
                dependentFileName = Path.GetFileName(dependentFileName);

                string filepath = item.ItemSpec;
                string filename = Path.GetFileName(filepath);
                string filedir = Path.GetDirectoryName(filepath);
                string dependentFilePath = Path.Combine(String.IsNullOrEmpty(dependentFileDir) ? filedir : dependentFileDir, dependentFileName);
                string classFullName = null;

                if (!Path.IsPathRooted(dependentFilePath))
                {
                    dependentFilePath = Path.Combine(this.ProjectFolderFullpath, dependentFilePath);
                }

                try
                {
                    if (dependentFilePath.EndsWith(KnownStrings.XamlExtension, StringComparison.OrdinalIgnoreCase))
                    {
                        if (File.Exists(dependentFilePath))
                        {
                            // We need to get the File Contents from the Task Service.  So that
                            // we can read from (possibly dirty) IDE buffers.
                            using (var fileReader = TaskFileService.GetFileContents(dependentFilePath))
                            {
                                // No need to load the full DOM.  The x:Class is very near the top so read the x:Class as simply as possible.
                                classFullName = XamlNodeStreamHelper.ReadXClassFromXamlFileStream(fileReader, _schemaContext);
                            }
                            if (classFullName != null)
                            {
                                // Compute and store the path to the XAML control's header file.
                                // This takes the project's AdditionalIncludeDirectories into account when computing the path
                                // to use in the generated XamlTypeInfo's #include statements.
                                string rootedFilePath = Path.GetFullPath(filepath);
                                string rootedProjectFolder = Path.GetDirectoryName(Path.GetFullPath(ProjectPath));

                                string pathToIncludeFile = GetDefaultXamlLinkMetadata(rootedFilePath, rootedFilePath,
                                                                                                    ProjectFolderFullpath, IncludeFolderList);
                                className2headerMap.Add(classFullName, pathToIncludeFile);
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    LogError_XamlInternalError(e, dependentFilePath);
                }
            }
            return className2headerMap;
        }

        private bool BuildWarningSuppressionList()
        {
            if (this.SuppressWarnings != null)
            {
                foreach (var warnString in this.SuppressWarnings)
                {
                    int warnNumber;
                    if (!Int32.TryParse(warnString, out warnNumber))
                    {
                        LogError_BadSuppressWarningsString(warnString);
                        return false;
                    }
                    _suppressedWarnings.Add(warnNumber.AsErrorCode());
                }
            }
            return true;
        }


        // C++ does not use <Link> data so this will compute "implied" link data based on the IncludePath
        // Managed Items should use <Link> metadata in the PROj file and should not use this.
        // If nothing usefull can be found, the "relative path" is the bare filename.
        // returns null if the fileItemSpec (relative file path specified on the item) is in the project tree and useable as is.
        static internal string GetDefaultXamlLinkMetadata(string fileFullpath, string fileItemSpec, string projectFolderFullpath, IEnumerable<string> includeFullpathList)
        {
            string relativePath = null;
            string tail;
            if (!IsFilePathInOrUnderFolderPath(fileFullpath, projectFolderFullpath, out tail))
            {
                relativePath = GetBestSubPath(fileFullpath, includeFullpathList);
                if (relativePath == null)
                {
                    relativePath = Path.GetFileName(fileFullpath);
                }
            }
            else
            {
                // If the XAML page is in the project but the path is
                // not a canonical relative path; i.e. either rooted or with "..".
                // Then use the XAML Default Link metadata to correct that.
                if (Path.IsPathRooted(fileItemSpec) || PathContainsComponent(fileItemSpec, ".."))
                {
                    relativePath = fileFullpath.Substring(projectFolderFullpath.Length + 1);
                }
            }
            return relativePath;
        }


        static internal bool IsFilePathInOrUnderFolderPath(string file, string folder, out string relativePath)
        {
            relativePath = null;
            if (String.IsNullOrWhiteSpace(file) || String.IsNullOrWhiteSpace(folder))
            {
                return false;
            }
            if (file.Length <= folder.Length)
            {
                return false;
            }
            if (!folder.EndsWith("\\"))
            {
                folder += '\\';
            }
            if (file.StartsWith(folder, StringComparison.OrdinalIgnoreCase))
            {
                relativePath = file.Substring(folder.Length);
                return true;
            }
            return false;
        }

        // Find the shortest "downward" relative path for fullpath that is "below"
        // a folder on the folderlist.
        // returns NULL if none match.
        static internal string GetBestSubPath(string fullpath, IEnumerable<string> folderList)
        {
            string result = null;

            foreach (string folder in folderList)
            {
                string tail = null;
                if (IsFilePathInOrUnderFolderPath(fullpath, folder, out tail))
                {
                    if (result == null || tail.Length < result.Length)
                    {
                        result = tail;
                    }
                }
            }
            return result;
        }

        internal static bool PathContainsComponent(string path, string component)
        {
            string[] nodes = path.Split(_separator);

            foreach (string node in nodes)
            {
                if (String.Equals(node, component, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }
            return false;
        }

        private bool ShouldSuppressTypeInfoCodeGen()
        {
            return _projectInfo.HasCodeGenFlag(CodeGenCtrlFlags.NoTypeInfoCodeGen);
        }

        private bool ShouldSuppressPageCodeGen()
        {
            return _projectInfo.HasCodeGenFlag(CodeGenCtrlFlags.NoPageCodeGen);
        }

        internal BuildTaskFileService TaskFileService { get; set; }

        #region ErrorMessages
        private void LogWarning(XamlCompileWarning warning, string xamlFile)
        {
            if (!_suppressedWarnings.Contains(warning.Code.AsErrorCode()))
            {
                // razzle does not like subcategories being present (arg!)
                // as it breaks its warning-to-error transform filters in build.exe.
                string subcategory = "XamlCompiler";
                Log.LogWarning(subcategory, warning.Code.AsErrorCode(), null, xamlFile, warning.LineNumber, warning.LineOffset, 0, 0, warning.Message);
            }
        }

        private void LogWarning(XamlCompileWarning warning)
        {
            LogWarning(warning, warning.FileName);
        }

        private void LogError(XamlCompileError error)
        {
            Log.LogError("XamlCompiler", error.Code.AsErrorCode(), null, error.FileName, error.LineNumber, error.LineOffset, 0, 0, error.Message);
        }

        public void LogError(string subcategory, ErrorCode code, string helpKeyword, string file,
                             int lineNumber, int columnNumber, int endLineNumber, int endColumnNumber,
                             string message)
        {
            if (Log != null)
            {
                Log.LogError(subcategory, code.AsErrorCode(), helpKeyword, file,
                             lineNumber, columnNumber, endLineNumber, endColumnNumber,
                             message);
            }
        }

        private void LogError_ClassIsNotFoundInAssembly(string className, string localAssemblyName, string xamlFile)
        {
            string msg = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_xClassTypeIsNotFound, className, localAssemblyName);
            LogError("XamlCompiler", ErrorCode.WMC1002, null, xamlFile, 0, 0, 0, 0, msg);
        }

        public void LogError_BadCodeGenFlags(string flag)
        {
            string msg = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_CodeGenString_Bad, flag);
            LogError("XamlCompiler", ErrorCode.WMC1003, null, null, 0, 0, 0, 0, msg);
        }

        private void LogError_EventsInAppXaml(string fileName)
        {
            LogError("XamlCompiler", ErrorCode.WMC1005, null, fileName, 0, 0, 0, 0, XamlCompilerResources.XamlCompiler_NoEventsInAppXaml);
        }

        private void LogError_CannotResolveAssembly(string name)
        {
            LogError("XamlCompiler", ErrorCode.WMC1006, null, ProjectPath, 0, 0, 0, 0, string.Format(XamlCompilerResources.XamlCompiler_CantResolveAssembly, name));
        }

        private void LogError_CannotResolveWinUIMetadata()
        {
            LogError("XamlCompiler", ErrorCode.WMC1007, null, ProjectPath, 0, 0, 0, 0, XamlCompilerResources.XamlCompiler_CantResolveWinUIMetadata);
        }

        private void LogError_ClassDoesntMatchWinmdName(string className, string winmdName, string xamlFile)
        {
            string msg = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_xClassTypeDoesntMatchWinmd, className, winmdName);
            LogError("XamlCompiler", ErrorCode.WMC1009, null, xamlFile, 0, 0, 0, 0, msg);
        }

        private void LogError_XamlFileMustEndInDotXaml(string xamlFile)
        {
            LogError("XamlCompiler", ErrorCode.WMC1010, null, xamlFile, 0, 0, 0, 0, XamlCompilerResources.XamlCompiler_XamlFileMustEndInDotXaml);
        }

        private void LogError_BadSuppressWarningsString(string warn)
        {
            string msg = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_BadValueInSupressWarningsList, warn);
            LogError("XamlCompiler", ErrorCode.WMC1011, null, ProjectPath, 0, 0, 0, 0, msg);
        }

        private void LogError_MoreThanOneApplicationXaml()
        {
            LogError("XamlCompiler", ErrorCode.WMC1012, null, ProjectPath, 0, 0, 0, 0, XamlCompilerResources.XamlCompiler_MoreThanOneApplicationXaml);
        }

        private void LogError_XamlFilesWithSameApparentPath(string xamlFile1, string xamlFile2, string commonApparentPath)
        {
            string msg = ResourceUtilities.FormatString(XamlCompilerResources.XamlCompiler_XamlFilesHaveTheSameName, xamlFile1, xamlFile2, commonApparentPath);
            LogError("XamlCompiler", ErrorCode.WMC1013, null, ProjectPath, 0, 0, 0, 0, msg);
        }

        private void LogError_XamlXMLParsingError(Exception e, string file)
        {
            LogUnhandledException(XamlCompilerResources.XamlXmlParsingError, ErrorCode.WMC9997, e, file);
        }

        internal void LogError_XamlInternalError(Exception e, string file)
        {
            LogUnhandledException(XamlCompilerResources.XamlInternlError, ErrorCode.WMC9999, e, file);
        }

        internal void LogUnhandledException(string subcategory, ErrorCode code, Exception e, string file)
        {
            XamlException xamlException = e as XamlException;
            int line = xamlException != null ? xamlException.LineNumber : 0;
            int column = xamlException != null ? xamlException.LinePosition : 0;
            string message = e.Message;

            LogError(subcategory, code, null, file, line, column, 0, 0, message);
            Log.LogDiagnosticMessage(e.StackTrace);
        }
        #endregion
    }
}
