// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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
            CompilerInputs ci = DeserializeCompilerInputs(parsedOptions.JsonInputFile);
            CompileXamlInternal core = CreateCore(ci);
            try
            {
                core.SaveState = SavedStateManager.Load(ci.SavedStateFile);

                result = core.DoExecute();
                if (result)
                {
                    core.SaveStateBeforeFinishing();
                    SaveResults(core, parsedOptions.JsonOutputFile);
                }
            }
            catch (Exception e)
            {
                result = false;
                core.LogError_XamlInternalError(e, null);
            }
            return result ? 0 : 1;
        }

        private CompileXamlInternal CreateCore(CompilerInputs compilerInputs)
        {
            CompileXamlInternal core = new CompileXamlInternal();
            core.Log = Log;
            core.TaskFileService = new BuildTaskFileService(core.LanguageSourceExtension);
            core.PopulateFromCompilerInputs(compilerInputs);
            return core;
        }

        private CompilerInputs DeserializeCompilerInputs(string inputJsonFile)
        {
            using (FileStream jsonFileStream = new FileStream(inputJsonFile, FileMode.Open, FileAccess.Read))
            {
                return JsonSerializer.Deserialize<CompilerInputs>(jsonFileStream);
            }
        }

        private void SaveResults(CompileXamlInternal core, string outputFile)
        {
            CompilerOutputs co = new CompilerOutputs();
            co.GeneratedCodeFiles = core.GeneratedCodeFiles;
            co.GeneratedXamlFiles = core.GeneratedXamlFiles;
            co.GeneratedXamlPagesFiles = core.GeneratedXamlPagesFiles;
            co.GeneratedXbfFiles = core.GeneratedXbfFiles;
            co.MSBuildLogEntries = Log.Entries;

            using (FileStream jsonFileStream = new FileStream(outputFile, FileMode.Create, FileAccess.Write))
            {
                JsonSerializer.Serialize(jsonFileStream, co);
            }
        }
    }
}
