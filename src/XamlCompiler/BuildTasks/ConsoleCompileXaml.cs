// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Text.Json;
using Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop;

namespace Microsoft.UI.Xaml.Markup.Compiler.Executable
{
    class Options
    {
        // Path to existing JSON file containing compiler inputs (language, Xaml files, reference assemblies, etc.)
        public string JsonInputFile { get; set; }

        // Path for compiler's output (errors, generated files, etc.)
        public string JsonOutputFile { get; set; }
    }

    [Description("Generates code for a given set of XAML files.")]
    internal sealed class CompileXaml
    {
        private ConsoleLogger Log { get; } = new ConsoleLogger();

        // Reported as a diagnostic message of the compilation (see AssertionTraceListener)
        public void ReportAssertionFailure(string message)
        {
            Log.LogDiagnosticMessage(message);
        }

        public int Run(string[] args)
        {
            Options o = ParseCommandLineParameters(args);

            // If parameters are incorrect, print usage but don't consider it an error
            if (o == null)
            {
                PrintUsage();
                return 0;
            }

            // Verify the input json file exists - if not, it's a fatal error
            if (!System.IO.File.Exists(o.JsonInputFile))
            {
                Console.WriteLine($"Xaml compiler error: Input JSON file \"{o.JsonInputFile}\" doesn't exist!");
                return 1;
            }
            return ExecuteCompilation(o);
        }

        private Options ParseCommandLineParameters(string[] args)
        {
            if (args.Length != 2)
            {
                return null;
            }

            Options o = new Options();
            o.JsonInputFile = args[0];
            o.JsonOutputFile = args[1];
            return o;
        }

        private void PrintUsage()
        {
            Console.WriteLine("Usage: XamlCompiler.exe <input JSON file> <output JSON file>");
            Console.WriteLine("Example: XamlCompiler.exe input.json output.json");
        }

        private int ExecuteCompilation(Options parsedOptions)
        {
            bool result = false;
            CompileXamlInternal core = null;
            try
            {
                CompilerInputs ci = DeserializeCompilerInputs(parsedOptions.JsonInputFile);
                if (ci.ContractVersion > CompilerContract.Version)
                {
                    LogError($"The XAML compiler task (contract version {ci.ContractVersion}) is newer than the XAML compiler (contract version {CompilerContract.Version}): use the XamlCompiler.exe of the same Windows App SDK version as the task (see XamlCompilerNativeExePath).");
                }
                else if (ci.ForceSharedStateShutdown)
                {
                    // The in-process task released the state it kept from pass 1 to pass 2 when the build failed in between: each
                    // run of the compiler is its own process, so there is nothing to do.
                    result = true;
                }
                else
                {
                    core = CreateCore(ci);
                    core.SaveState = SavedStateManager.Load(ci.SavedStateFile);

                    result = core.DoExecute();
                    if (result)
                    {
                        core.SaveStateBeforeFinishing();
                    }
                }
            }
            catch (Exception e)
            {
                result = false;
                if (core != null)
                {
                    core.LogError_XamlInternalError(e, null);
                }
                else
                {
                    LogError($"{Properties.XamlCompilerResources.XamlInternlError}: {e.Message}");
                    Log.LogDiagnosticMessage(e.ToString());
                }
            }
            finally
            {
                // Always write results (including error log entries), so that the CompileXaml task can report proper diagnostics to MSBuild
                try
                {
                    SaveResults(core, parsedOptions.JsonOutputFile);
                }
                catch (Exception saveEx)
                {
                    // If we can't write the output file, fall back to writing errors
                    // to stderr so they're still visible in the build output.
                    Console.Error.WriteLine($"XamlCompiler: Failed to write output file: {saveEx.Message}");
                    foreach (var entry in Log.Entries)
                    {
                        if (entry.Type == MSBuildLogEntry.EntryType.Error)
                        {
                            Console.Error.WriteLine($"{entry.File}({entry.LineNumber},{entry.ColumnNumber}): error {entry.ErrorCode}: {entry.Message}");
                        }
                    }
                }
            }

            return result ? 0 : 1;
        }

        private void LogError(string message)
        {
            Log.LogError(null, "WMC9999", null, null, 0, 0, 0, 0, message);
        }

        private CompileXamlInternal CreateCore(CompilerInputs compilerInputs)
        {
            CompileXamlInternal core = new CompileXamlInternal();
            core.Log = Log;
            core.PopulateFromCompilerInputs(compilerInputs);
            core.IsDesignTimeBuild = compilerInputs.IsDesignTimeBuild || string.Equals(compilerInputs.CompileMode, "DesignTimeBuild", StringComparison.OrdinalIgnoreCase);

            List<FileOverlay> overlays = DeserializeFileOverlays(compilerInputs.FileOverlaysPath);
            core.TaskFileService = overlays != null
                ? new OverlayTaskFileService(core.LanguageSourceExtension, overlays)
                : new BuildTaskFileService(core.LanguageSourceExtension);
            return core;
        }

        private CompilerInputs DeserializeCompilerInputs(string inputJsonFile)
        {
            using (FileStream jsonFileStream = new FileStream(inputJsonFile, FileMode.Open, FileAccess.Read))
            {
                return JsonSerializer.Deserialize(jsonFileStream, CompilerJsonContext.Default.CompilerInputs);
            }
        }

        private List<FileOverlay> DeserializeFileOverlays(string overlaysJsonFile)
        {
            if (string.IsNullOrEmpty(overlaysJsonFile))
            {
                return null;
            }

            // The task creates the file with FileOptions.DeleteOnClose and keeps it open while the compiler runs
            using (FileStream jsonFileStream = new FileStream(overlaysJsonFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
            {
                List<FileOverlay> overlays = JsonSerializer.Deserialize(jsonFileStream, CompilerJsonContext.Default.ListFileOverlay) ?? new List<FileOverlay>();
                Log.LogDiagnosticMessage($"Using {overlays.Count} XAML file(s) open in the IDE.");
                return overlays;
            }
        }

        private void SaveResults(CompileXamlInternal core, string outputFile)
        {
            CompilerOutputs co = new CompilerOutputs();
            co.ContractVersion = CompilerContract.Version;
            co.GeneratedCodeFiles = core?.GeneratedCodeFiles ?? new List<string>();
            co.GeneratedXamlFiles = core?.GeneratedXamlFiles ?? new List<string>();
            co.GeneratedXamlPagesFiles = core?.GeneratedXamlPagesFiles ?? new List<string>();
            co.GeneratedXbfFiles = core?.GeneratedXbfFiles ?? new List<string>();
            co.MSBuildLogEntries = Log.Entries;

            AtomicFile.Write(outputFile, jsonFileStream => JsonSerializer.Serialize(jsonFileStream, co, CompilerJsonContext.Default.CompilerOutputs));
        }
    }

    // Reads the XAML files that are open in the IDE from the state the CompileXaml task got from the IDE host object (see
    // FileOverlay), instead of from the disk: the compiler sees the unsaved edits, like the in-process task used to.
    internal sealed class OverlayTaskFileService : BuildTaskFileService
    {
        private readonly Dictionary<string, FileOverlay> _overlays = new Dictionary<string, FileOverlay>(StringComparer.OrdinalIgnoreCase);

        public OverlayTaskFileService(string languageExtension, IEnumerable<FileOverlay> overlays)
            : base(languageExtension)
        {
            foreach (FileOverlay overlay in overlays)
            {
                if (!string.IsNullOrEmpty(overlay?.Path))
                {
                    _overlays[Path.GetFullPath(overlay.Path)] = overlay;
                }
            }
        }

        public override TextReader GetFileContents(string srcFile)
        {
            if (TryGetOverlay(srcFile, out FileOverlay overlay) && overlay.Contents != null)
            {
                return new StringReader(overlay.Contents);
            }
            return base.GetFileContents(srcFile);
        }

        public override DateTime GetLastChangeTime(string srcFile)
        {
            if (TryGetOverlay(srcFile, out FileOverlay overlay) && overlay.LastChangeFileTime != 0)
            {
                return DateTime.FromFileTime(overlay.LastChangeFileTime);
            }
            return base.GetLastChangeTime(srcFile);
        }

        private bool TryGetOverlay(string srcFile, out FileOverlay overlay)
        {
            overlay = null;
            return !string.IsNullOrEmpty(srcFile) && _overlays.TryGetValue(Path.GetFullPath(srcFile), out overlay);
        }
    }
}
