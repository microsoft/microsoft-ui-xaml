// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    /// <summary>
    /// Compiles the XAML files of a project, by running the XAML compiler (XamlCompiler.exe, a Native AOT executable).
    /// </summary>
    /// <remarks>
    /// <para>
    /// The inputs are written to a JSON file the compiler reads, and the compiler writes its results (generated files and
    /// diagnostics) to another JSON file, which the task reads back to set its outputs and log the diagnostics (see CompilerContract).
    /// </para>
    /// <para>
    /// The task must be named CompileXaml and run in the MSBuild node (no task host): Visual Studio gives its file manager host
    /// object (see <see cref="IdeHostFileManager"/>) to the tasks with this name in design-time builds, so that the compiler sees the
    /// unsaved edits of the XAML files open in the editor.
    /// </para>
    /// </remarks>
    public sealed class CompileXaml : ToolTask
    {
        private const string DesignTimeBuildCompileMode = "DesignTimeBuild";
        private const string GCServerVariableName = "DOTNET_gcServer";

        private bool _isDesignTimeBuild;
        private string _inputJsonPath;
        private string _outputJsonPath;
        private string _overlaysJsonPath;
        private FileStream _overlaysStream;
        private List<FileOverlay> _overlays;
        private bool _toolRan;
        private bool _outputReplayed;
        private string _compilerPath;
        private string _outputError;
        private volatile bool _canceled;

        #region Compiler inputs

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

        /// <summary>Obsolete: the compiler keeps no state from a build pass to the next one, so there is nothing to release.</summary>
        public bool ForceSharedStateShutdown { get; set; }

        public bool DisableXbfGeneration { get; set; }

        public bool DisableXbfLineInfo { get; set; }

        public bool EnableXBindDiagnostics { get; set; }

        public ITaskItem[] ClIncludeFiles { get; set; }

        public string CIncludeDirectories { get; set; }

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

        // VCInstallDir gives the directory where vcmeta.dll is located. Deprecated in favor of VCInstallPath32/VCInstallPath64,
        // the exact paths of the 32-bit and 64-bit vcmeta.dll binaries, which take priority.
        public string VCInstallDir { get; set; }

        public string VCInstallPath32 { get; set; }

        public string VCInstallPath64 { get; set; }

        public string WindowsSdkPath { get; set; }

        /// <summary>RealBuildPass1, RealBuildPass2 or DesignTimeBuild.</summary>
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

        // Controls whether or not the TargetPlatformMinVersion specified in the project should be ignored. TargetPlatformMinVersion
        // is used for the validation of feature usage (platform API, x:Bind functionality, conditional XAML, etc.) and the XBF version.
        public bool IgnoreSpecifiedTargetPlatformMinVersion { get; set; }

        public string EnabledXamlOptionalChanges { get; set; }

        public string DisabledXamlOptionalChanges { get; set; }

        #endregion

        #region Compiler process

        /// <summary>
        /// The path of the XAML compiler (XamlCompiler.exe) to run. Defaults to the one next to the task assembly in the package
        /// (tools\win-&lt;architecture of Windows&gt;\XamlCompiler.exe, or tools\win-x64\XamlCompiler.exe on ARM64 without the
        /// ARM64 one), like the default XamlCompilerNativeExePath of the targets, for the build logic that calls the task directly.
        /// </summary>
        public string XamlCompilerPath { get; set; }

        /// <summary>
        /// Empty for workstation GC (unless a DOTNET_gcServer variable is inherited), 'true'/'false' to force Server GC on or off,
        /// or 'auto' for Server GC on machines with enough cores and memory (see <see cref="XamlCompilerGCPolicy"/>).
        /// </summary>
        public string EnableServerGC { get; set; }

        /// <summary>The input file of the compiler. Defaults to XamlCompiler.&lt;CompileMode&gt;.input.json in <see cref="OutputPath"/>.</summary>
        public string InputJsonFile { get; set; }

        /// <summary>The output file of the compiler. Defaults to XamlCompiler.&lt;CompileMode&gt;.output.json in <see cref="OutputPath"/>.</summary>
        public string OutputJsonFile { get; set; }

        #endregion

        #region Outputs

        [Output]
        public ITaskItem[] GeneratedCodeFiles { get; private set; } = Array.Empty<ITaskItem>();

        [Output]
        public ITaskItem[] GeneratedXamlFiles { get; private set; } = Array.Empty<ITaskItem>();

        [Output]
        public ITaskItem[] GeneratedXbfFiles { get; private set; } = Array.Empty<ITaskItem>();

        [Output]
        public ITaskItem[] GeneratedXamlPagesFiles { get; private set; } = Array.Empty<ITaskItem>();

        #endregion

        protected override string ToolName => "XamlCompiler.exe";

        // The stack trace of a compiler crash, if any, is the most useful thing to see
        protected override MessageImportance StandardErrorLoggingImportance => MessageImportance.High;

        private string ProjectDirectory => Path.GetDirectoryName(Path.GetFullPath(ProjectPath));

        public override bool Execute()
        {
            _isDesignTimeBuild = string.Equals(CompileMode, DesignTimeBuildCompileMode, StringComparison.OrdinalIgnoreCase);
            ReadIdeHostObject();

            // Design-time builds have their own files, so they never mix with those of a command-line build running at the same time
            string fileNamePrefix = "XamlCompiler." + (_isDesignTimeBuild ? DesignTimeBuildCompileMode : string.IsNullOrEmpty(CompileMode) ? "Compile" : CompileMode);
            string outputFolder = Path.Combine(ProjectDirectory, OutputPath ?? string.Empty);
            _inputJsonPath = Path.GetFullPath(string.IsNullOrEmpty(InputJsonFile) ? Path.Combine(outputFolder, fileNamePrefix + ".input.json") : Path.Combine(ProjectDirectory, InputJsonFile));
            _outputJsonPath = Path.GetFullPath(string.IsNullOrEmpty(OutputJsonFile) ? Path.Combine(outputFolder, fileNamePrefix + ".output.json") : Path.Combine(ProjectDirectory, OutputJsonFile));
            _overlaysJsonPath = Path.Combine(Path.GetDirectoryName(_inputJsonPath), fileNamePrefix + ".overlays.json");

            bool result;
            try
            {
                // Possibly left behind by a build that was killed
                TryDeleteFile(_overlaysJsonPath);

                result = base.Execute();
            }
            finally
            {
                // The unsaved text of the documents open in the IDE must not stay on the disk
                _overlaysStream?.Dispose();
                _overlaysStream = null;
                TryDeleteFile(_overlaysJsonPath);
            }

            // ToolTask only reports a failure for a nonzero exit code: a compiler that ran but didn't produce its results also failed
            if (_toolRan && !_outputReplayed && !_canceled)
            {
                Log.LogError("The XAML compiler ('{0}') exited with code {1} without producing a valid output file ('{2}'){3}. See the messages above for details.",
                    _compilerPath, ExitCode, _outputJsonPath, _outputError != null ? ": " + _outputError : string.Empty);
            }
            // The compiler failed without saying why: the task must not fail without an error (MSB4181)
            else if (_outputReplayed && ExitCode != 0 && !_canceled && !Log.HasLoggedErrors)
            {
                Log.LogError("The XAML compiler ('{0}') exited with code {1} without reporting an error. See the messages above for details.", _compilerPath, ExitCode);
            }

            return result && !_canceled && !Log.HasLoggedErrors;
        }

        public override void Cancel()
        {
            _canceled = true;
            base.Cancel();
        }

        protected override bool ValidateParameters()
        {
            if (!string.IsNullOrEmpty(EnableServerGC) &&
                !string.Equals(EnableServerGC, "auto", StringComparison.OrdinalIgnoreCase) &&
                !bool.TryParse(EnableServerGC, out _))
            {
                Log.LogError("Invalid 'XamlCompilerEnableServerGC' value '{0}'. It must be empty (workstation GC), 'true', 'false', or 'auto'.", EnableServerGC);
                return false;
            }

            Architecture osArchitecture = RuntimeInformation.OSArchitecture;
            if (osArchitecture == Architecture.X86 || osArchitecture == Architecture.Arm)
            {
                Log.LogError("The XAML compiler requires a 64-bit version of Windows.");
                return false;
            }

            _compilerPath = string.IsNullOrEmpty(XamlCompilerPath) ? GetDefaultCompilerPath(osArchitecture) : XamlCompilerPath;
            if (!File.Exists(_compilerPath))
            {
                Log.LogError("The XAML compiler was not found at '{0}'.{1}", _compilerPath,
                    string.IsNullOrEmpty(XamlCompilerPath) ? " Set the XamlCompilerPath parameter of the CompileXaml task (e.g. to $(XamlCompilerNativeExePath))." : string.Empty);
                return false;
            }

            return base.ValidateParameters();
        }

        // The compiler of the package the task assembly is in (see XamlCompilerNativeExePath in the targets)
        internal static string GetDefaultCompilerPath(Architecture osArchitecture)
        {
            string toolsDirectory = Path.GetDirectoryName(typeof(CompileXaml).Assembly.Location);
            string x64Path = Path.Combine(toolsDirectory, "win-x64", "XamlCompiler.exe");
            if (osArchitecture == Architecture.Arm64)
            {
                // A package built from a single flavor only has the x64 compiler, which also runs on ARM64 (emulated)
                string arm64Path = Path.Combine(toolsDirectory, "win-arm64", "XamlCompiler.exe");
                return File.Exists(arm64Path) || !File.Exists(x64Path) ? arm64Path : x64Path;
            }
            return x64Path;
        }

        protected override string GenerateFullPathToTool()
        {
            return _compilerPath;
        }

        // The inputs have paths relative to the project directory, which is the current directory of the MSBuild node
        protected override string GetWorkingDirectory()
        {
            return ProjectDirectory;
        }

        protected override string GenerateCommandLineCommands()
        {
            CommandLineBuilder builder = new CommandLineBuilder();
            builder.AppendFileNameIfNotNull(_inputJsonPath);
            builder.AppendFileNameIfNotNull(_outputJsonPath);
            return builder.ToString();
        }

        protected override ProcessStartInfo GetProcessStartInfo(string pathToTool, string commandLineCommands, string responseFileSwitch)
        {
            ProcessStartInfo startInfo = base.GetProcessStartInfo(pathToTool, commandLineCommands, responseFileSwitch);

            // The compiler links both GC modes, and picks one from the DOTNET_gcServer variable of its process
            string value;
            if (string.IsNullOrEmpty(EnableServerGC))
            {
                // On .NET, the indexer throws for a missing variable (on .NET Framework it returns null)
                value = startInfo.EnvironmentVariables.ContainsKey(GCServerVariableName) ? startInfo.EnvironmentVariables[GCServerVariableName] : null;
                if (string.IsNullOrEmpty(value))
                {
                    value = "0";
                }
                else
                {
                    Log.LogMessage(MessageImportance.Low, "Using the inherited {0}={1} for the XAML compiler.", GCServerVariableName, value);
                }
            }
            else if (string.Equals(EnableServerGC, "auto", StringComparison.OrdinalIgnoreCase))
            {
                value = XamlCompilerGCPolicy.ShouldUseServerGC() ? "1" : "0";
            }
            else
            {
                value = bool.Parse(EnableServerGC) ? "1" : "0";
            }

            startInfo.EnvironmentVariables[GCServerVariableName] = value;
            Log.LogMessage(MessageImportance.Low, "XAML compiler {0}={1}.", GCServerVariableName, value);
            return startInfo;
        }

        protected override int ExecuteTool(string pathToTool, string responseFileCommands, string commandLineCommands)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_inputJsonPath));
            Directory.CreateDirectory(Path.GetDirectoryName(_outputJsonPath));

            CompilerInputs inputs = CreateCompilerInputs();
            if (_overlays != null && _overlays.Count > 0)
            {
                // Deleted when closed, also if MSBuild dies: it only exists while the compiler runs, which opens it with FileShare.Delete
                _overlaysStream = new FileStream(_overlaysJsonPath, FileMode.Create, FileAccess.ReadWrite, FileShare.Read | FileShare.Delete, 4096, FileOptions.DeleteOnClose);
                CompilerJson.Write(_overlaysStream, _overlays);
                _overlaysStream.Flush(flushToDisk: true);
                inputs.FileOverlaysPath = _overlaysJsonPath;
            }
            CompilerJson.Write(_inputJsonPath, inputs);

            // Never read the results of a previous run
            TryDeleteFile(_outputJsonPath);

            int exitCode = base.ExecuteTool(pathToTool, responseFileCommands, commandLineCommands);
            _toolRan = true;

            if (!_canceled)
            {
                ReplayOutput();
            }

            return exitCode;
        }

        // ToolTask logs MSB6006 ("exited with code N") when the compiler fails: its own diagnostics were already logged instead
        protected override bool HandleTaskExecutionErrors()
        {
            Log.LogMessage(MessageImportance.Low, "The XAML compiler exited with code {0}.", ExitCode);
            return false;
        }

        private CompilerInputs CreateCompilerInputs()
        {
            return new CompilerInputs
            {
                ContractVersion = CompilerContract.Version,
                IsDesignTimeBuild = _isDesignTimeBuild,
                BuildConfiguration = BuildConfiguration,
                CIncludeDirectories = CIncludeDirectories,
                ClIncludeFiles = TaskItems.ToMSBuildItems(ClIncludeFiles),
                CodeGenerationControlFlags = CodeGenerationControlFlags,
                CompileMode = CompileMode,
                DisableXbfGeneration = DisableXbfGeneration,
                DisableXbfLineInfo = DisableXbfLineInfo,
                EnableXBindDiagnostics = EnableXBindDiagnostics,
                FeatureControlFlags = FeatureControlFlags,
                FingerprintIgnorePaths = FingerprintIgnorePaths,
                ForceSharedStateShutdown = ForceSharedStateShutdown,
                GenXbfPath = GenXbfPath,
                PrecompiledHeaderFile = PrecompiledHeaderFile,
                IgnoreSpecifiedTargetPlatformMinVersion = IgnoreSpecifiedTargetPlatformMinVersion,
                IsPass1 = IsPass1,
                Language = Language,
                LanguageSourceExtension = LanguageSourceExtension,
                LocalAssembly = TaskItems.ToMSBuildItems(LocalAssembly),
                OutputPath = OutputPath,
                OutputType = OutputType,
                PriIndexName = PriIndexName,
                ProjectName = ProjectName,
                ProjectPath = ProjectPath,
                ReferenceAssemblies = TaskItems.ToMSBuildItems(ReferenceAssemblies),
                ReferenceAssemblyPaths = TaskItems.ToMSBuildItems(ReferenceAssemblyPaths),
                RootNamespace = RootNamespace,
                RootsLog = RootsLog,
                SavedStateFile = SavedStateFile,
                SdkXamlPages = TaskItems.ToMSBuildItems(SdkXamlPages),
                SuppressWarnings = SuppressWarnings,
                TargetFileName = TargetFileName,
                TargetPlatformMinVersion = TargetPlatformMinVersion,
                VCInstallDir = VCInstallDir,
                VCInstallPath32 = VCInstallPath32,
                VCInstallPath64 = VCInstallPath64,
                WindowsSdkPath = WindowsSdkPath,
                XamlApplications = TaskItems.ToMSBuildItems(XamlApplications),
                XamlComponentResourceLocation = XamlComponentResourceLocation,
                XAMLFingerprint = XAMLFingerprint,
                UseVCMetaManaged = UseVCMetaManaged,
                XamlPages = TaskItems.ToMSBuildItems(XamlPages),
                XamlPlatform = XamlPlatform,
                XamlResourceMapName = XamlResourceMapName,
                EnabledXamlOptionalChanges = EnabledXamlOptionalChanges,
                DisabledXamlOptionalChanges = DisabledXamlOptionalChanges,
            };
        }

        /// <summary>
        /// In design-time builds in Visual Studio, reads the XAML files that are open in the editor from the IDE host object, if any,
        /// so that the compiler sees their unsaved edits: on the MSBuild thread, before the compiler runs (and the task yields).
        /// </summary>
        private void ReadIdeHostObject()
        {
            IdeHostFileManager fileManager = HostObject != null ? IdeHostFileManager.Acquire(HostObject) : null;
            if (fileManager == null)
            {
                return;
            }

            Stopwatch stopwatch = Stopwatch.StartNew();
            int fileCount = 0;
            try
            {
                // The free-threaded host object is only given to design-time builds
                _isDesignTimeBuild |= !fileManager.IsRealBuildOperation();
                if (!_isDesignTimeBuild)
                {
                    return;
                }

                List<FileOverlay> overlays = new List<FileOverlay>();
                foreach (ITaskItem[] items in new[] { XamlApplications, XamlPages, SdkXamlPages })
                {
                    if (items == null)
                    {
                        continue;
                    }

                    foreach (ITaskItem item in items)
                    {
                        if (TaskItems.IsExcludedFromBuild(item))
                        {
                            continue;
                        }

                        fileCount++;
                        FileOverlay overlay = GetFileOverlay(fileManager, item.GetMetadata("FullPath"));
                        if (overlay != null)
                        {
                            overlays.Add(overlay);
                        }
                    }
                }

                _overlays = overlays;
            }
            catch (Exception e) when (e is COMException || e is InvalidCastException || e is InvalidComObjectException || e is IOException || e is UnauthorizedAccessException)
            {
                // E.g. the IDE went away: build from the files on the disk
                _overlays = null;
                Log.LogMessage(MessageImportance.Low, "The XAML files open in the IDE can't be read through its host object ({0}): {1}", fileManager.Kind, e.Message);
            }

            Log.LogMessage(MessageImportance.Low, "Read {0} XAML file(s) through the IDE host object ({1}) in {2} ms: {3} open in the IDE.",
                fileCount, fileManager.Kind, stopwatch.ElapsedMilliseconds, _overlays?.Count ?? 0);
        }

        // The state of a file in the IDE: null if it isn't open (same last change time as on the disk). Its text is only included if
        // it differs from the file on the disk (unsaved edits), but its last change time always is, like the in-process task used to.
        private FileOverlay GetFileOverlay(IdeHostFileManager fileManager, string path)
        {
            long changeTime = fileManager.GetFileLastChangeTime(path);
            bool exists = File.Exists(path);
            if (exists && changeTime == File.GetLastWriteTimeUtc(path).ToFileTimeUtc())
            {
                return null;
            }

            // The host object returns the text without its BOM, as File.ReadAllText does
            string contents = fileManager.GetFileContents(path);
            if (exists && string.Equals(contents, File.ReadAllText(path), StringComparison.Ordinal))
            {
                contents = null;
            }

            Log.LogMessage(MessageImportance.Low, "'{0}' is open in the IDE{1}.", path, contents != null ? ", with unsaved changes" : string.Empty);
            return new FileOverlay { Path = path, Contents = contents, LastChangeFileTime = changeTime };
        }

        private void ReplayOutput()
        {
            CompilerOutputs outputs;
            try
            {
                if (!File.Exists(_outputJsonPath))
                {
                    _outputError = "the file doesn't exist";
                    return;
                }
                outputs = CompilerJson.Read<CompilerOutputs>(_outputJsonPath);
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is System.Runtime.Serialization.SerializationException || e is InvalidCastException)
            {
                _outputError = e.Message;
                return;
            }

            _outputReplayed = true;

            if (outputs.ContractVersion < CompilerContract.Version)
            {
                string message = $"The XAML compiler '{_compilerPath}' is older than the CompileXaml task (contract version {outputs.ContractVersion} instead of {CompilerContract.Version}): " +
                    "use the XamlCompiler.exe of the same Windows App SDK version (see XamlCompilerNativeExePath).";
                if (_isDesignTimeBuild)
                {
                    // An older compiler doesn't know about design-time builds, and would change the outputs of the last real build
                    Log.LogWarning(message);
                }
                else
                {
                    Log.LogMessage(MessageImportance.Low, message);
                }
            }

            GeneratedCodeFiles = ToTaskItems(outputs.GeneratedCodeFiles, escape: true);
            GeneratedXamlFiles = ToTaskItems(outputs.GeneratedXamlFiles, escape: false);
            GeneratedXbfFiles = ToTaskItems(outputs.GeneratedXbfFiles, escape: false);
            GeneratedXamlPagesFiles = ToTaskItems(outputs.GeneratedXamlPagesFiles, escape: true);

            if (outputs.MSBuildLogEntries == null)
            {
                return;
            }

            foreach (MSBuildLogEntry entry in outputs.MSBuildLogEntries)
            {
                switch (entry.Type)
                {
                    case MSBuildLogEntry.EntryType.Message:
                        Log.LogMessage(MessageImportance.Low, "{0}", entry.Message);
                        break;
                    case MSBuildLogEntry.EntryType.Warning:
                        Log.LogWarning(entry.SubCategory, entry.ErrorCode, entry.HelpKeyword, entry.File, entry.LineNumber,
                            entry.ColumnNumber, entry.EndLineNumber, entry.EndColumnNumber, entry.Message);
                        break;
                    case MSBuildLogEntry.EntryType.Error:
                        Log.LogError(entry.SubCategory, entry.ErrorCode, entry.HelpKeyword, entry.File, entry.LineNumber,
                            entry.ColumnNumber, entry.EndLineNumber, entry.EndColumnNumber, entry.Message);
                        break;
                    default:
                        Log.LogError("Unhandled XAML compiler log entry type '{0}': {1}", entry.Type, entry.Message);
                        break;
                }
            }
        }

        private static ITaskItem[] ToTaskItems(IList<string> files, bool escape)
        {
            if (files == null || files.Count == 0)
            {
                return Array.Empty<ITaskItem>();
            }

            // File names need to be escaped: names with URL escapes like %23 would get unescaped otherwise
            ITaskItem[] items = new ITaskItem[files.Count];
            for (int i = 0; i < items.Length; i++)
            {
                items[i] = new TaskItem(escape ? TaskItems.Escape(files[i]) : files[i]);
            }
            return items;
        }

        private void TryDeleteFile(string path)
        {
            try
            {
                if (File.Exists(path))
                {
                    File.Delete(path);
                }
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                Log.LogMessage(MessageImportance.Low, "Could not delete '{0}': {1}", path, e.Message);
            }
        }
    }
}
