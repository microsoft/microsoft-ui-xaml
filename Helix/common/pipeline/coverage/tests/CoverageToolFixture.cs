// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.IO;

// A process, rather than a PowerShell mock, exercises native arguments and LASTEXITCODE.
internal static class CoverageToolFixture
{
    private static string Option(string[] args, string name)
    {
        int index = Array.IndexOf(args, name);
        return index >= 0 && index + 1 < args.Length ? args[index + 1] : null;
    }

    private static int Main(string[] args)
    {
        string root = Environment.GetEnvironmentVariable("WINUI_COVERAGE_TEST_ROOT");
        string mode = Environment.GetEnvironmentVariable("WINUI_COVERAGE_TEST_MODE");
        File.AppendAllText(Path.Combine(root, "calls.txt"), string.Join("\t", args) + Environment.NewLine);

        if (args[0] == "-latest")
        {
            Console.Write(Environment.GetEnvironmentVariable("WINUI_COVERAGE_TEST_DISCOVERY"));
            return 0;
        }

        if (args[0] == "instrument")
        {
            string binary = args[1];
            string symbols = Path.ChangeExtension(binary, ".pdb");
            if (!File.Exists(symbols))
                return 12;
            if (File.ReadAllText(symbols) != "symbols:" + Path.GetFileName(binary))
                return 13;
            if (mode == "fail-instrument" ||
                (mode == "fail-controls" && Path.GetFileName(binary) == "Microsoft.UI.Xaml.Controls.dll"))
                return 23;
            if (mode == "skip-controls" && Path.GetFileName(binary) == "Microsoft.UI.Xaml.Controls.dll")
                return 0;

            File.AppendAllText(binary, "|instrumented:" + Option(args, "--session-id"));
            if (mode != "no-runtime")
            {
                foreach (string architecture in new[] { "32", "64" })
                    File.WriteAllText(Path.Combine(Path.GetDirectoryName(binary), "static_covrun" + architecture + ".dll"),
                        "runtime-" + architecture);
            }
            return 0;
        }

        if (args[0] == "merge")
        {
            string format = Option(args, "--output-format");
            if (mode == "fail-" + format)
                return 24;
            for (int index = 1; index < Array.IndexOf(args, "--output"); index++)
            {
                if (File.ReadAllText(args[index]) == "invalid")
                    return 25;
            }
            if (mode != "no-output-" + format)
                File.WriteAllText(Option(args, "--output"), mode == "empty-" + format ? "" : "merged-" + format);
            return 0;
        }

        if (args[0] == "shutdown")
            return mode == "fail-shutdown" ? 26 : 0;

        if (args[0] == "test")
            return int.Parse(args[1]);

        return 27;
    }
}
