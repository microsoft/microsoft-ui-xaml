// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.IO;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading;

namespace FakeXamlCompiler
{
    /// <summary>
    /// Stands in for XamlCompiler.exe in the CompileXaml task tests: "FakeXamlCompiler input.json output.json", like the real
    /// compiler. It always writes what it saw (arguments, working directory, DOTNET_gcServer, the inputs and the IDE overlays file)
    /// to output.json.probe.json, then does what FAKE_XAMLCOMPILER_SCENARIO says.
    /// </summary>
    public static class Program
    {
        public static int Main(string[] args)
        {
            string inputPath = args[0];
            string outputPath = args[1];
            string scenario = Environment.GetEnvironmentVariable("FAKE_XAMLCOMPILER_SCENARIO") ?? "ok";

            JsonNode input = JsonNode.Parse(File.ReadAllText(inputPath));
            string overlaysPath = (string)input["FileOverlaysPath"];
            JsonObject probe = new JsonObject
            {
                ["Arguments"] = new JsonArray(args[0], args[1]),
                ["WorkingDirectory"] = Environment.CurrentDirectory,
                ["DOTNET_gcServer"] = Environment.GetEnvironmentVariable("DOTNET_gcServer"),
                ["Input"] = input,
            };
            if (!string.IsNullOrEmpty(overlaysPath))
            {
                // Like the real compiler: the task created the file with FileOptions.DeleteOnClose
                using (FileStream stream = new FileStream(overlaysPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                using (StreamReader reader = new StreamReader(stream))
                {
                    probe["Overlays"] = JsonNode.Parse(reader.ReadToEnd());
                }
            }
            File.WriteAllText(outputPath + ".probe.json", probe.ToJsonString());

            switch (scenario)
            {
                case "ok":
                    WriteOutput(outputPath, 1, new JsonArray());
                    return 0;

                case "oldcontract":
                    WriteOutput(outputPath, null, new JsonArray());
                    return 0;

                case "rawdiagnostics":
                    Console.Error.WriteLine("line 1:26 token recognition error at: '?'");
                    Console.WriteLine("tool.cs(1,1): warning RAW0001: incidental console output");
                    WriteOutput(outputPath, 1, new JsonArray());
                    return 0;

                case "diagnostics":
                    WriteOutput(outputPath, 1, new JsonArray(
                        LogEntry(0, null, null, 0, 0, "A message from the compiler"),
                        LogEntry(1, "WMC1500", "Views\\Page.xaml", 12, 34, "A warning from the compiler"),
                        LogEntry(2, "WMC0001", "Views\\Page.xaml", 5, 6, "An error from the compiler")));
                    return 1;

                case "silentfailure":
                    WriteOutput(outputPath, 1, new JsonArray());
                    return 3;

                case "nooutput":
                    return 0;

                case "invalidoutput":
                    File.WriteAllText(outputPath, "{ \"GeneratedCodeFiles\": [");
                    return 0;

                case "crash":
                    Console.Error.WriteLine("Unhandled exception. System.InvalidOperationException: The fake compiler crashed");
                    Console.Error.WriteLine("   at FakeXamlCompiler.Program.Main(String[] args)");
                    return unchecked((int)0xE0434352);

                case "hang":
                    File.WriteAllText(outputPath + ".started", string.Empty);
                    Thread.Sleep(Timeout.Infinite);
                    return 0;

                default:
                    Console.Error.WriteLine("Unknown scenario '" + scenario + "'");
                    return 2;
            }
        }

        private static JsonObject LogEntry(int type, string code, string file, int line, int column, string message)
        {
            return new JsonObject
            {
                ["Type"] = type,
                ["SubCategory"] = null,
                ["ErrorCode"] = code,
                ["HelpKeyword"] = null,
                ["File"] = file,
                ["LineNumber"] = line,
                ["ColumnNumber"] = column,
                ["EndLineNumber"] = line,
                ["EndColumnNumber"] = column + 1,
                ["Message"] = message,
            };
        }

        private static void WriteOutput(string outputPath, int? contractVersion, JsonArray logEntries)
        {
            JsonObject output = new JsonObject
            {
                ["GeneratedCodeFiles"] = new JsonArray("obj\\MainPage.g.cs", "obj\\Page%23.g.cs"),
                ["GeneratedXamlFiles"] = new JsonArray("obj\\MainPage.xaml"),
                ["GeneratedXbfFiles"] = new JsonArray("obj\\MainPage.xbf"),
                ["GeneratedXamlPagesFiles"] = new JsonArray("obj\\XamlTypeInfo.g.cpp"),
                ["MSBuildLogEntries"] = logEntries,
            };
            if (contractVersion.HasValue)
            {
                output["ContractVersion"] = contractVersion.Value;
            }
            File.WriteAllText(outputPath, output.ToJsonString());
        }
    }
}
