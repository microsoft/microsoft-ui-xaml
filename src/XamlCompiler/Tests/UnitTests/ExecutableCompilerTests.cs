// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace UnitTests
{
    [TestClass]
    public class ExecutableCompilerTests
    {
        private const string SuccessfulOutput =
            "{\"GeneratedCodeFiles\":[],\"GeneratedXamlFiles\":[],\"GeneratedXbfFiles\":[]," +
            "\"GeneratedXamlPagesFiles\":[],\"MSBuildLogEntries\":[]}";

        [TestMethod]
        public void ExecutableCompiler_RawDiagnosticsDoNotBecomeBuildErrors()
        {
            VerifyInvocations(
                "echo line 1:26 token recognition error at: '?' 1>&2\r\n" +
                "echo tool.cs(1,1): warning RAW0001: incidental console output\r\n" +
                ">\"%~2\" echo " + SuccessfulOutput + "\r\nexit /b 0",
                true, null);
        }

        [TestMethod]
        public void ExecutableCompiler_StructuredErrorsStillFailTheBuild()
        {
            string output = SuccessfulOutput.Replace("\"MSBuildLogEntries\":[]",
                "\"MSBuildLogEntries\":[{\"Type\":2,\"ErrorCode\":\"WMC1110\"," +
                "\"File\":\"Page.xaml\",\"LineNumber\":3,\"ColumnNumber\":5," +
                "\"Message\":\"Invalid binding path\"}]");
            VerifyInvocations(">\"%~2\" echo " + output + "\r\nexit /b 1", false, "WMC1110");
        }

        [TestMethod]
        public void ExecutableCompiler_NonzeroExitCannotReportSuccess()
        {
            VerifyInvocations(">\"%~2\" echo " + SuccessfulOutput + "\r\nexit /b 7", false, "exited with code 7");
        }

        [TestMethod]
        public void ExecutableCompiler_MissingOutputCannotReusePreviousResults()
        {
            VerifyInvocations("exit /b 0", false, "was not created");
        }

        private static void VerifyInvocations(string compilerBody, bool expectedSuccess, string expectedError)
        {
            string payload = Path.GetDirectoryName(typeof(ExecutableCompilerTests).Assembly.Location);
            string targetsPath = Path.Combine(payload, "Microsoft.UI.Xaml.Markup.Compiler.interop.targets");
            string ioAssembly = Path.Combine(payload, "Microsoft.UI.Xaml.Markup.Compiler.IO.dll");
            Assert.IsTrue(File.Exists(targetsPath), "The compiler targets must be staged with the test payload.");
            Assert.IsTrue(File.Exists(ioAssembly), "Build the compiler's executable/IO projects before running these tests.");

            var targets = XDocument.Load(targetsPath);
            XNamespace ns = targets.Root.Name.Namespace;
            var invocations = targets.Root.Elements(ns + "Target").Where(target =>
                target.Elements(ns + "Exec").Any(exec =>
                    ((string)exec.Attribute("Command") ?? "").Contains("$(_XamlCompilerCommand)"))).ToList();
            Assert.IsTrue(invocations.Count > 0, "No executable compiler invocations found in the production targets.");

            string msbuild = Environment.GetEnvironmentVariable("MSBUILD_EXE_PATH");
            if (string.IsNullOrEmpty(msbuild) || !File.Exists(msbuild) ||
                !msbuild.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
            {
                msbuild = (Environment.GetEnvironmentVariable("PATH") ?? "").Split(Path.PathSeparator)
                    .Select(directory => Path.Combine(directory.Trim('"'), "MSBuild.exe")).FirstOrDefault(File.Exists);
            }
            Assert.IsFalse(string.IsNullOrEmpty(msbuild), "Run these tests in the initialized WinUI build environment.");

            foreach (var invocation in invocations)
            {
                string root = Path.Combine(Path.GetTempPath(), "XamlCompiler executable " + Guid.NewGuid().ToString("N"));
                Directory.CreateDirectory(root);
                try
                {
                    string compiler = Path.Combine(root, "fake compiler.cmd");
                    string input = Path.Combine(root, "input.json");
                    string output = Path.Combine(root, "output.json");
                    string errors = Path.Combine(root, "errors.log");
                    string warnings = Path.Combine(root, "warnings.log");
                    File.WriteAllText(compiler, "@echo off\r\n" + compilerBody + "\r\n");
                    File.WriteAllText(input, "{}");
                    File.WriteAllText(output, SuccessfulOutput);

                    // Exercise the production invocation and JSON replay, without requiring a XAML app
                    // or its native dependencies to test the executable's diagnostic protocol.
                    var steps = invocation.Elements().Where(element =>
                        element.Name == ns + "Exec" || element.Name == ns + "OutputDeserializer" ||
                        (element.Name == ns + "Delete" &&
                         (string)element.Attribute("Files") == "$(XamlCompilerExeOutputJson)"));
                    var project = new XElement(ns + "Project",
                        new XElement(ns + "UsingTask",
                            new XAttribute("TaskName", "Microsoft.UI.Xaml.Markup.Compiler.IO.OutputDeserializer"),
                            new XAttribute("AssemblyFile", ioAssembly)),
                        new XElement(ns + "PropertyGroup",
                            new XElement(ns + "UseXamlCompilerExecutable", "true"),
                            new XElement(ns + "ManagedAssembly", "true"),
                            new XElement(ns + "_XamlCompilerCommand", "\"" + compiler + "\""),
                            new XElement(ns + "XamlCompilerExeInputJson", input),
                            new XElement(ns + "XamlCompilerExeOutputJson", output)),
                        new XElement(ns + "Target", new XAttribute("Name", "Test"),
                            steps.Select(element => new XElement(element))));
                    string projectPath = Path.Combine(root, "test.proj");
                    project.Save(projectPath);

                    using (var process = Process.Start(new ProcessStartInfo
                    {
                        FileName = msbuild,
                        Arguments = "\"" + projectPath + "\" /t:Test /nologo /nr:false /v:minimal " +
                            "\"/flp:LogFile=" + errors + ";ErrorsOnly\" " +
                            "\"/flp1:LogFile=" + warnings + ";WarningsOnly\"",
                        WorkingDirectory = root,
                        UseShellExecute = false,
                        CreateNoWindow = true,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true
                    }))
                    {
                        Task<string> stdout = process.StandardOutput.ReadToEndAsync();
                        Task<string> stderr = process.StandardError.ReadToEndAsync();
                        if (!process.WaitForExit(60000))
                        {
                            process.Kill();
                            process.WaitForExit();
                            Assert.Fail("MSBuild timed out testing " + (string)invocation.Attribute("Name"));
                        }
                        Task.WaitAll(stdout, stderr);
                        string context = (string)invocation.Attribute("Name") + Environment.NewLine +
                            stdout.Result + stderr.Result;
                        string errorLog = File.ReadAllText(errors);
                        if (expectedSuccess)
                        {
                            Assert.AreEqual(0, process.ExitCode, context);
                            Assert.IsTrue(string.IsNullOrWhiteSpace(errorLog), context);
                            Assert.IsTrue(string.IsNullOrWhiteSpace(File.ReadAllText(warnings)), context);
                        }
                        else
                        {
                            Assert.AreNotEqual(0, process.ExitCode, context);
                            StringAssert.Contains(errorLog, expectedError, context);
                        }
                    }
                }
                finally
                {
                    Directory.Delete(root, true);
                }
            }
        }
    }
}
