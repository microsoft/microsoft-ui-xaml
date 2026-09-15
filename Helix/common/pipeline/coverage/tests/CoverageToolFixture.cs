// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Simulates the coverage console and vswhere with real process exits and controllable pipe stalls.

using System;
using System.IO;
using System.IO.Pipes;
using System.Threading;

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
        if (args[0] == "shutdown")
            File.WriteAllText(Path.Combine(root, "shutdown.pid"), System.Diagnostics.Process.GetCurrentProcess().Id.ToString());

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
            int validInputs = 0;
            for (int index = 1; index < Array.IndexOf(args, "--output"); index++)
            {
                // The real VS CLI skips corrupt inputs and still returns success.
                if (File.ReadAllText(args[index]) != "invalid")
                    validInputs++;
            }
            if (mode != "no-output-" + format)
                File.WriteAllText(Option(args, "--output"), mode == "empty-" + format ? "" :
                    format == "cobertura" ? "<coverage lines-valid=\"" + validInputs + "\" />" : "merged-" + format);
            return 0;
        }

        if (mode == "stall-shutdown" && args[0] == "collect")
        {
            using (var pipe = new NamedPipeServerStream("CodeCoverage.pipe." + Option(args, "--session-id")))
            {
                pipe.WaitForConnection();
                pipe.ReadByte();
                Thread.Sleep(Timeout.Infinite);
            }
        }

        if (mode == "stall-shutdown" && args[0] == "shutdown")
        {
            using (var pipe = new NamedPipeClientStream(".", "CodeCoverage.pipe." + args[1]))
            {
                pipe.Connect(10000);
                pipe.WriteByte(1);
                File.WriteAllText(Path.Combine(root, "shutdown-requested.txt"), "waiting for reply");
                pipe.ReadByte();
            }
        }

        if (args[0] == "shutdown")
        {
            Console.Write(mode == "verbose-shutdown" ? new string('o', 262144) : "shutdown completed");
            Console.Error.Write(mode == "verbose-shutdown" ? new string('e', 262144) :
                mode == "fail-shutdown" ? "fixture shutdown failure" : "");
            return mode == "fail-shutdown" ? 26 : 0;
        }

        if (args[0] == "test")
            return int.Parse(args[1]);

        return 27;
    }
}
