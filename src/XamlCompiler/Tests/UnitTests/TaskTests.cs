// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using Microsoft.UI.Xaml.Markup.Compiler.Tasks;
using Microsoft.VisualStudio.ProjectSystem.Interop;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTests
{
    /// <summary>
    /// Tests of the CompileXaml MSBuild task (Microsoft.UI.Xaml.Markup.Compiler.Tasks.dll), which runs the XAML compiler process:
    /// here a stand-in (FakeXamlCompiler) that records what it got and plays a scenario (see FakeXamlCompiler\Program.cs).
    /// </summary>
    [TestClass]
    public class TaskTests
    {
        private const string ScenarioVariable = "FAKE_XAMLCOMPILER_SCENARIO";

        private string _directory;
        private string _projectPath;
        private string _outputPath;

        private static string FakeCompilerPath => Path.Combine(Path.GetDirectoryName(typeof(TaskTests).Assembly.Location), "FakeXamlCompiler", "FakeXamlCompiler.exe");

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            // The stand-in compiler is a framework-dependent app: let it find the runtime the tests run on
            string runtimeDirectory = RuntimeEnvironment.GetRuntimeDirectory();
            Environment.SetEnvironmentVariable("DOTNET_ROOT", Path.GetFullPath(Path.Combine(runtimeDirectory, "..", "..", "..")));
        }

        [TestInitialize]
        public void Initialize()
        {
            _directory = Path.Combine(Path.GetTempPath(), "XamlCompilerTaskTests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_directory);
            _projectPath = Path.Combine(_directory, "App.csproj");
            File.WriteAllText(_projectPath, "<Project />");
            _outputPath = Path.Combine(_directory, "obj") + Path.DirectorySeparatorChar;
        }

        [TestCleanup]
        public void Cleanup()
        {
            Environment.SetEnvironmentVariable("DOTNET_gcServer", null);
            try
            {
                Directory.Delete(_directory, recursive: true);
            }
            catch (IOException)
            {
            }
        }

        private CompileXaml CreateTask(FakeBuildEngine engine, string scenario, string compileMode = "RealBuildPass1")
        {
            string page = Path.Combine(_directory, "MainPage.xaml");
            File.WriteAllText(page, "<Page />");
            return new CompileXaml
            {
                BuildEngine = engine,
                XamlCompilerPath = FakeCompilerPath,
                EnvironmentVariables = new[] { ScenarioVariable + "=" + scenario },
                ProjectPath = _projectPath,
                Language = "C#",
                LanguageSourceExtension = ".cs",
                OutputPath = _outputPath,
                ReferenceAssemblies = Array.Empty<ITaskItem>(),
                ReferenceAssemblyPaths = Array.Empty<ITaskItem>(),
                XamlPages = new ITaskItem[] { new TaskItem(page) },
                CompileMode = compileMode,
                IsPass1 = true,
            };
        }

        private string InputJsonPath(string compileMode) => Path.Combine(_outputPath, "XamlCompiler." + compileMode + ".input.json");

        private string OutputJsonPath(string compileMode) => Path.Combine(_outputPath, "XamlCompiler." + compileMode + ".output.json");

        private JsonElement ReadProbe(string compileMode)
        {
            using (JsonDocument document = JsonDocument.Parse(File.ReadAllText(OutputJsonPath(compileMode) + ".probe.json")))
            {
                return document.RootElement.Clone();
            }
        }

        [TestMethod]
        public void Task_RunsTheCompilerAndSetsTheOutputs()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");

            Assert.IsTrue(task.Execute(), engine.Summary);

            Assert.AreEqual(0, engine.Errors.Count, engine.Summary);
            Assert.AreEqual(0, engine.Warnings.Count, engine.Summary);
            CollectionAssert.AreEqual(new[] { @"obj\MainPage.g.cs", @"obj\Page%23.g.cs" }, task.GeneratedCodeFiles.Select(i => i.ItemSpec).ToArray(),
                "File names must be escaped, so that MSBuild doesn't unescape '%23'");
            CollectionAssert.AreEqual(new[] { @"obj\MainPage.xaml" }, task.GeneratedXamlFiles.Select(i => i.ItemSpec).ToArray());
            CollectionAssert.AreEqual(new[] { @"obj\MainPage.xbf" }, task.GeneratedXbfFiles.Select(i => i.ItemSpec).ToArray());
            CollectionAssert.AreEqual(new[] { @"obj\XamlTypeInfo.g.cpp" }, task.GeneratedXamlPagesFiles.Select(i => i.ItemSpec).ToArray());

            JsonElement probe = ReadProbe("RealBuildPass1");
            Assert.AreEqual(_directory.TrimEnd('\\'), probe.GetProperty("WorkingDirectory").GetString().TrimEnd('\\'), "The compiler runs in the project directory");
            Assert.AreEqual(InputJsonPath("RealBuildPass1"), probe.GetProperty("Arguments")[0].GetString());
            Assert.AreEqual(OutputJsonPath("RealBuildPass1"), probe.GetProperty("Arguments")[1].GetString());
            JsonElement input = probe.GetProperty("Input");
            Assert.AreEqual(1, input.GetProperty("ContractVersion").GetInt32());
            Assert.IsFalse(input.GetProperty("IsDesignTimeBuild").GetBoolean());
            Assert.AreEqual(JsonValueKind.Null, input.GetProperty("FileOverlaysPath").ValueKind);
            Assert.AreEqual("RealBuildPass1", input.GetProperty("CompileMode").GetString());
            Assert.IsTrue(input.GetProperty("IsPass1").GetBoolean());
        }

        [TestMethod]
        public void Task_RawDiagnosticsDoNotBecomeBuildErrors()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "rawdiagnostics");

            Assert.IsTrue(task.Execute(), engine.Summary);
            Assert.AreEqual(0, engine.Errors.Count, engine.Summary);
            Assert.AreEqual(0, engine.Warnings.Count, engine.Summary);
            Assert.IsTrue(engine.Messages.Any(m => m.Message.Contains("token recognition error")), engine.Summary);
            Assert.IsTrue(engine.Messages.Any(m => m.Message.Contains("RAW0001")), engine.Summary);
        }

        [TestMethod]
        public void Task_InputMapsTheItemMetadata()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.ReferenceAssemblies = new ITaskItem[]
            {
                Item(@"C:\refs\Framework.dll", "ResolvedFrom", "{TargetFrameworkDirectory}"),
                Item(@"C:\refs\System.dll", "IsSystemReference", "True"),
                Item(@"C:\refs\Package.dll", "NuGetPackageId", "Some.Package"),
                Item(@"C:\refs\Static.winmd", "ProjectType", "StaticLibrary"),
            };
            task.XamlPages = new ITaskItem[]
            {
                Item(@"C:\app\Views\MainPage.xaml",
                    "Link", @"Linked\MainPage.xaml",
                    "TargetPath", @"Views\Target.xaml",
                    "DependentUpon", "MainPage.xaml.cs",
                    "XamlResourceMapName", "MapName",
                    "XamlComponentResourceLocation", "Nested"),
                Item(@"C:\app\Views\Excluded.xaml", "ExcludedFromBuild", "true"),
            };

            Assert.IsTrue(task.Execute(), engine.Summary);

            JsonElement input = ReadProbe("RealBuildPass1").GetProperty("Input");
            AssertJsonEquals(@"[
                { ""DependentUpon"": """", ""FullPath"": ""C:\\refs\\Framework.dll"", ""ItemSpec"": ""C:\\refs\\Framework.dll"", ""IsSystemReference"": true, ""IsNuGetReference"": false, ""IsStaticLibraryReference"": false, ""MSBuild_Link"": """", ""MSBuild_TargetPath"": """", ""MSBuild_XamlResourceMapName"": """", ""MSBuild_XamlComponentResourceLocation"": """" },
                { ""DependentUpon"": """", ""FullPath"": ""C:\\refs\\System.dll"", ""ItemSpec"": ""C:\\refs\\System.dll"", ""IsSystemReference"": true, ""IsNuGetReference"": false, ""IsStaticLibraryReference"": false, ""MSBuild_Link"": """", ""MSBuild_TargetPath"": """", ""MSBuild_XamlResourceMapName"": """", ""MSBuild_XamlComponentResourceLocation"": """" },
                { ""DependentUpon"": """", ""FullPath"": ""C:\\refs\\Package.dll"", ""ItemSpec"": ""C:\\refs\\Package.dll"", ""IsSystemReference"": false, ""IsNuGetReference"": true, ""IsStaticLibraryReference"": false, ""MSBuild_Link"": """", ""MSBuild_TargetPath"": """", ""MSBuild_XamlResourceMapName"": """", ""MSBuild_XamlComponentResourceLocation"": """" },
                { ""DependentUpon"": """", ""FullPath"": ""C:\\refs\\Static.winmd"", ""ItemSpec"": ""C:\\refs\\Static.winmd"", ""IsSystemReference"": false, ""IsNuGetReference"": false, ""IsStaticLibraryReference"": true, ""MSBuild_Link"": """", ""MSBuild_TargetPath"": """", ""MSBuild_XamlResourceMapName"": """", ""MSBuild_XamlComponentResourceLocation"": """" }
            ]", input.GetProperty("ReferenceAssemblies"));
            AssertJsonEquals(@"[
                { ""DependentUpon"": ""MainPage.xaml.cs"", ""FullPath"": ""C:\\app\\Views\\MainPage.xaml"", ""ItemSpec"": ""C:\\app\\Views\\MainPage.xaml"", ""IsSystemReference"": false, ""IsNuGetReference"": false, ""IsStaticLibraryReference"": false, ""MSBuild_Link"": ""Linked\\MainPage.xaml"", ""MSBuild_TargetPath"": ""Views\\Target.xaml"", ""MSBuild_XamlResourceMapName"": ""MapName"", ""MSBuild_XamlComponentResourceLocation"": ""Nested"" }
            ]", input.GetProperty("XamlPages"));
            Assert.AreEqual(JsonValueKind.Null, input.GetProperty("LocalAssembly").ValueKind, "Unset item parameters stay null");
        }

        [TestMethod]
        public void Task_LogsTheCompilerDiagnosticsWithTheirLocation()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "diagnostics");

            Assert.IsFalse(task.Execute());

            BuildErrorEventArgs error = engine.Errors.Single();
            Assert.AreEqual("WMC0001", error.Code);
            Assert.AreEqual(@"Views\Page.xaml", error.File);
            Assert.AreEqual(5, error.LineNumber);
            Assert.AreEqual(6, error.ColumnNumber);
            Assert.AreEqual(5, error.EndLineNumber);
            Assert.AreEqual(7, error.EndColumnNumber);
            Assert.AreEqual("An error from the compiler", error.Message);
            BuildWarningEventArgs warning = engine.Warnings.Single();
            Assert.AreEqual("WMC1500", warning.Code);
            Assert.AreEqual(@"Views\Page.xaml", warning.File);
            Assert.AreEqual(12, warning.LineNumber);
            Assert.AreEqual(34, warning.ColumnNumber);
            Assert.AreEqual("A warning from the compiler", warning.Message);
            Assert.IsTrue(engine.Messages.Any(m => m.Message == "A message from the compiler"), engine.Summary);
            Assert.IsFalse(engine.Errors.Any(e => e.Code == "MSB6006"), "The exit code of the compiler must not be reported on top of its own errors");
        }

        [TestMethod]
        public void Task_ExitCodeZeroWithoutOutputIsAnError()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "nooutput");

            Assert.IsFalse(task.Execute());

            StringAssert.Contains(engine.Errors.Single().Message, "exited with code 0 without producing a valid output file");
            StringAssert.Contains(engine.Errors.Single().Message, "the file doesn't exist");
        }

        [TestMethod]
        public void Task_FailureWithoutAnErrorIsAnError()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "silentfailure");

            Assert.IsFalse(task.Execute());

            StringAssert.Contains(engine.Errors.Single().Message, "exited with code 3 without reporting an error");
        }

        [TestMethod]
        public void Task_DefaultsToTheCompilerNextToTheTaskAssembly()
        {
            // Build logic that calls CompileXaml directly, without XamlCompilerPath (e.g. controls\dev\dll\Microsoft.UI.Xaml.Common.targets)
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.XamlCompilerPath = null;

            Assert.IsTrue(task.Execute(), engine.Summary);

            Assert.IsTrue(File.Exists(OutputJsonPath("RealBuildPass1") + ".probe.json"), "The compiler next to the task assembly must have run");
            string tools = Path.GetDirectoryName(typeof(CompileXaml).Assembly.Location);
            Assert.AreEqual(Path.Combine(tools, "win-x64", "XamlCompiler.exe"), CompileXaml.GetDefaultCompilerPath(Architecture.X64));
            // There is no win-arm64 compiler next to the tasks: the x64 one runs on ARM64 too
            Assert.AreEqual(Path.Combine(tools, "win-x64", "XamlCompiler.exe"), CompileXaml.GetDefaultCompilerPath(Architecture.Arm64));
        }

        [TestMethod]
        public void Task_MissingCompilerIsAnError()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.XamlCompilerPath = Path.Combine(_directory, "missing", "XamlCompiler.exe");

            Assert.IsFalse(task.Execute());

            StringAssert.Contains(engine.Errors.Single().Message, "The XAML compiler was not found at '" + task.XamlCompilerPath + "'");
            Assert.IsFalse(File.Exists(InputJsonPath("RealBuildPass1")), "The compiler must not run");
        }

        [TestMethod]
        public void Task_InvalidOutputIsAnError()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "invalidoutput");

            Assert.IsFalse(task.Execute());

            StringAssert.Contains(engine.Errors.Single().Message, "without producing a valid output file");
            Assert.AreEqual(0, task.GeneratedCodeFiles.Length);
        }

        [TestMethod]
        public void Task_CompilerCrashIsAnErrorWithItsStandardError()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "crash");

            Assert.IsFalse(task.Execute());

            BuildErrorEventArgs error = engine.Errors.Single();
            StringAssert.Contains(error.Message, "exited with code -532462766 without producing a valid output file");
            Assert.IsTrue(engine.Messages.Any(m => m.Importance == MessageImportance.High && m.Message.Contains("System.InvalidOperationException: The fake compiler crashed")), engine.Summary);
            Assert.IsTrue(engine.Messages.Any(m => m.Message.Contains("at FakeXamlCompiler.Program.Main")), engine.Summary);
        }

        [TestMethod]
        public void Task_CompilerOlderThanTheTaskIsAWarningInDesignTimeBuilds()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "oldcontract", "DesignTimeBuild");

            Assert.IsTrue(task.Execute(), engine.Summary);
            StringAssert.Contains(engine.Warnings.Single().Message, "is older than the CompileXaml task");

            engine = new FakeBuildEngine();
            task = CreateTask(engine, "oldcontract");
            Assert.IsTrue(task.Execute(), engine.Summary);
            Assert.AreEqual(0, engine.Warnings.Count, engine.Summary);
        }

        [TestMethod]
        [Timeout(60000)]
        public void Task_CancelKillsTheCompiler()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "hang");
            task.YieldDuringToolExecution = true;

            System.Threading.Tasks.Task<bool> execute = System.Threading.Tasks.Task.Run(() => task.Execute());
            string started = OutputJsonPath("RealBuildPass1") + ".started";
            while (!File.Exists(started))
            {
                Assert.IsFalse(execute.IsCompleted, "The task completed before the compiler started: " + engine.Summary);
                Thread.Sleep(50);
            }
            task.Cancel();

            Assert.IsTrue(execute.Wait(TimeSpan.FromSeconds(30)), "The task didn't stop after being canceled");
            Assert.IsFalse(execute.Result);
            Assert.AreEqual(0, engine.Errors.Count, "A canceled build doesn't report the missing output as an error: " + engine.Summary);
            Assert.AreEqual(1, engine.YieldCount);
            Assert.AreEqual(1, engine.ReacquireCount);
        }

        [TestMethod]
        [DataRow(null, null, "0", DisplayName = "Workstation GC by default")]
        [DataRow(null, "1", "1", DisplayName = "Inherited DOTNET_gcServer is respected")]
        [DataRow("", "0", "0", DisplayName = "Inherited DOTNET_gcServer=0 is respected")]
        [DataRow("true", null, "1", DisplayName = "Forced Server GC")]
        [DataRow("True", "0", "1", DisplayName = "Forced Server GC overrides the environment")]
        [DataRow("false", "1", "0", DisplayName = "Forced workstation GC overrides the environment")]
        public void Task_SelectsTheGCModeOfTheCompiler(string enableServerGC, string inherited, string expected)
        {
            Environment.SetEnvironmentVariable("DOTNET_gcServer", inherited);
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.EnableServerGC = enableServerGC;

            Assert.IsTrue(task.Execute(), engine.Summary);

            Assert.AreEqual(expected, ReadProbe("RealBuildPass1").GetProperty("DOTNET_gcServer").GetString());
        }

        [TestMethod]
        public void Task_AutoGCModeFollowsThePolicy()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.EnableServerGC = "auto";

            Assert.IsTrue(task.Execute(), engine.Summary);

            Assert.AreEqual(XamlCompilerGCPolicy.ShouldUseServerGC() ? "1" : "0", ReadProbe("RealBuildPass1").GetProperty("DOTNET_gcServer").GetString());
        }

        [TestMethod]
        public void Task_GCModeFromTheTaskEnvironmentVariables()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.EnvironmentVariables = task.EnvironmentVariables.Concat(new[] { "DOTNET_gcServer=1" }).ToArray();

            Assert.IsTrue(task.Execute(), engine.Summary);

            Assert.AreEqual("1", ReadProbe("RealBuildPass1").GetProperty("DOTNET_gcServer").GetString());
        }

        [TestMethod]
        public void Task_InvalidGCModeIsAnError()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.EnableServerGC = "sometimes";

            Assert.IsFalse(task.Execute());

            StringAssert.Contains(engine.Errors.Single().Message, "Invalid 'XamlCompilerEnableServerGC' value 'sometimes'");
            Assert.IsFalse(File.Exists(InputJsonPath("RealBuildPass1")), "The compiler must not run");
        }

        [TestMethod]
        public void Task_DesignTimeBuildsUseTheirOwnFiles()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok", "DesignTimeBuild");

            Assert.IsTrue(task.Execute(), engine.Summary);

            Assert.IsTrue(File.Exists(InputJsonPath("DesignTimeBuild")));
            Assert.IsTrue(File.Exists(OutputJsonPath("DesignTimeBuild")));
            JsonElement input = ReadProbe("DesignTimeBuild").GetProperty("Input");
            Assert.IsTrue(input.GetProperty("IsDesignTimeBuild").GetBoolean());
            Assert.AreEqual(JsonValueKind.Null, input.GetProperty("FileOverlaysPath").ValueKind, "No IDE host object, no overlays");
        }

        [TestMethod]
        public void Task_SendsTheUnsavedFilesOfTheLegacyHostObject()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            // A design-time build with a compile mode of a real build: the host object says what it is
            CompileXaml task = CreateTask(engine, "ok");
            string edited = Path.Combine(_directory, "Edited.xaml");
            string open = Path.Combine(_directory, "Open.xaml");
            string closed = Path.Combine(_directory, "Closed.xaml");
            foreach (string file in new[] { edited, open, closed })
            {
                File.WriteAllText(file, "<Page />");
            }
            task.XamlPages = new ITaskItem[] { new TaskItem(edited), new TaskItem(open), new TaskItem(closed), Item(Path.Combine(_directory, "Excluded.xaml"), "ExcludedFromBuild", "true") };
            LegacyHost host = new LegacyHost(isRealBuild: false);
            host.Documents[edited] = ("<Page x:Name=\"Unsaved\" />", 1234567890L);
            host.Documents[open] = ("<Page />", 1234567891L);
            task.HostObject = host;

            Assert.IsTrue(task.Execute(), engine.Summary);

            JsonElement probe = ReadProbe("DesignTimeBuild");
            JsonElement input = probe.GetProperty("Input");
            Assert.IsTrue(input.GetProperty("IsDesignTimeBuild").GetBoolean());
            string overlaysPath = input.GetProperty("FileOverlaysPath").GetString();
            Assert.IsFalse(File.Exists(overlaysPath), "The unsaved text of the IDE must not stay on the disk");
            AssertJsonEquals($@"[
                {{ ""Path"": {JsonSerializer.Serialize(edited)}, ""Contents"": ""<Page x:Name=\""Unsaved\"" />"", ""LastChangeFileTime"": 1234567890 }},
                {{ ""Path"": {JsonSerializer.Serialize(open)}, ""Contents"": null, ""LastChangeFileTime"": 1234567891 }}
            ]", probe.GetProperty("Overlays"));
            CollectionAssert.DoesNotContain(host.RequestedFiles, Path.Combine(_directory, "Excluded.xaml"));
        }

        [TestMethod]
        public void Task_IgnoresTheLegacyHostObjectInRealBuilds()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            LegacyHost host = new LegacyHost(isRealBuild: true);
            host.Documents[task.XamlPages[0].ItemSpec] = ("<Page x:Name=\"Unsaved\" />", 1234567890L);
            task.HostObject = host;

            Assert.IsTrue(task.Execute(), engine.Summary);

            JsonElement input = ReadProbe("RealBuildPass1").GetProperty("Input");
            Assert.IsFalse(input.GetProperty("IsDesignTimeBuild").GetBoolean());
            Assert.AreEqual(JsonValueKind.Null, input.GetProperty("FileOverlaysPath").ValueKind);
            Assert.AreEqual(0, host.RequestedFiles.Count);
        }

        [TestMethod]
        public void Task_SendsTheUnsavedFilesOfTheFreeThreadedHostObject()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            FreeThreadedHost host = new FreeThreadedHost();
            string page = task.XamlPages[0].ItemSpec;
            DateTime changeTime = new DateTime(2026, 1, 2, 3, 4, 5, DateTimeKind.Utc);
            host.Documents[page] = ("<Page x:Name=\"Unsaved\" />", changeTime);
            task.HostObject = host;

            Assert.IsTrue(task.Execute(), engine.Summary);

            // Only design-time builds get the free-threaded host object
            JsonElement probe = ReadProbe("DesignTimeBuild");
            Assert.IsTrue(probe.GetProperty("Input").GetProperty("IsDesignTimeBuild").GetBoolean());
            AssertJsonEquals($@"[
                {{ ""Path"": {JsonSerializer.Serialize(page)}, ""Contents"": ""<Page x:Name=\""Unsaved\"" />"", ""LastChangeFileTime"": {changeTime.ToFileTimeUtc()} }}
            ]", probe.GetProperty("Overlays"));
        }

        [TestMethod]
        public void Task_BuildsFromTheDiskWhenTheHostObjectFails()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok", "DesignTimeBuild");
            LegacyHost host = new LegacyHost(isRealBuild: false) { Fail = true };
            task.HostObject = host;

            Assert.IsTrue(task.Execute(), engine.Summary);

            JsonElement input = ReadProbe("DesignTimeBuild").GetProperty("Input");
            Assert.IsTrue(input.GetProperty("IsDesignTimeBuild").GetBoolean());
            Assert.AreEqual(JsonValueKind.Null, input.GetProperty("FileOverlaysPath").ValueKind);
        }

        [TestMethod]
        public void Task_IgnoresUnknownHostObjects()
        {
            FakeBuildEngine engine = new FakeBuildEngine();
            CompileXaml task = CreateTask(engine, "ok");
            task.HostObject = new UnknownHost();

            Assert.IsTrue(task.Execute(), engine.Summary);

            Assert.IsFalse(ReadProbe("RealBuildPass1").GetProperty("Input").GetProperty("IsDesignTimeBuild").GetBoolean());
        }

        private static ITaskItem Item(string itemSpec, params string[] metadata)
        {
            TaskItem item = new TaskItem(itemSpec);
            for (int i = 0; i < metadata.Length; i += 2)
            {
                item.SetMetadata(metadata[i], metadata[i + 1]);
            }
            return item;
        }

        // Compares JSON values, ignoring the order of the properties of objects
        internal static void AssertJsonEquals(string expected, JsonElement actual)
        {
            using (JsonDocument expectedDocument = JsonDocument.Parse(expected))
            {
                Assert.AreEqual(Canonicalize(expectedDocument.RootElement), Canonicalize(actual));
            }
        }

        private static string Canonicalize(JsonElement element)
        {
            switch (element.ValueKind)
            {
                case JsonValueKind.Object:
                    return "{" + string.Join(",", element.EnumerateObject().OrderBy(p => p.Name, StringComparer.Ordinal).Select(p => JsonSerializer.Serialize(p.Name) + ":" + Canonicalize(p.Value))) + "}";
                case JsonValueKind.Array:
                    return "[" + string.Join(",", element.EnumerateArray().Select(Canonicalize)) + "]";
                case JsonValueKind.String:
                    return JsonSerializer.Serialize(element.GetString());
                default:
                    return element.GetRawText();
            }
        }

        // Visual Studio's file manager host object of design-time builds (STA, legacy project systems and out-of-process CPS)
        private sealed class LegacyHost : IVsMSBuildTaskFileManager, ITaskHost
        {
            private readonly bool _isRealBuild;

            public LegacyHost(bool isRealBuild)
            {
                _isRealBuild = isRealBuild;
            }

            public Dictionary<string, (string Contents, long ChangeTime)> Documents { get; } = new Dictionary<string, (string, long)>(StringComparer.OrdinalIgnoreCase);

            public List<string> RequestedFiles { get; } = new List<string>();

            public bool Fail { get; set; }

            public string GetFileContents(string wszFilename)
            {
                RequestedFiles.Add(wszFilename);
                ThrowIfFailing();
                return Documents.TryGetValue(wszFilename, out var document) ? document.Contents : File.ReadAllText(wszFilename);
            }

            public object GetFileDocData(string wszFilename) => throw new NotSupportedException();

            public long GetFileLastChangeTime(string wszFilename)
            {
                RequestedFiles.Add(wszFilename);
                ThrowIfFailing();
                return Documents.TryGetValue(wszFilename, out var document) ? document.ChangeTime : File.GetLastWriteTimeUtc(wszFilename).ToFileTimeUtc();
            }

            public void PutGeneratedFileContents(string wszFilename, string strFileContents) => throw new NotSupportedException();

            public bool IsRealBuildOperation() => _isRealBuild;

            public void Delete(string wszFilename) => throw new NotSupportedException();

            public bool Exists(string wszFilename, bool fOnlyCheckOnDisk) => throw new NotSupportedException();

            private void ThrowIfFailing()
            {
                if (Fail)
                {
                    throw new COMException("The IDE went away", unchecked((int)0x800706BA));
                }
            }
        }

        // The free-threaded file manager host object of in-process CPS design-time builds
        private sealed class FreeThreadedHost : IVsMSBuildTaskFreeThreadedFileManager, ITaskHost
        {
            public Dictionary<string, (string Contents, DateTime ChangeTime)> Documents { get; } = new Dictionary<string, (string, DateTime)>(StringComparer.OrdinalIgnoreCase);

            public TextReader GetFileContents(string fileName)
            {
                return new StringReader(Documents.TryGetValue(fileName, out var document) ? document.Contents : File.ReadAllText(fileName));
            }

            public DateTime GetFileLastChangeTime(string fileName)
            {
                return Documents.TryGetValue(fileName, out var document) ? document.ChangeTime : File.GetLastWriteTimeUtc(fileName);
            }

            public void PutGeneratedFileContents(string fileName, string contents) => throw new NotSupportedException();

            public void Delete(string fileName) => throw new NotSupportedException();

            public bool Exists(string fileName, bool onlyCheckOnDisk) => throw new NotSupportedException();
        }

        private sealed class UnknownHost : ITaskHost
        {
        }

        private sealed class FakeBuildEngine : IBuildEngine3
        {
            public List<BuildErrorEventArgs> Errors { get; } = new List<BuildErrorEventArgs>();
            public List<BuildWarningEventArgs> Warnings { get; } = new List<BuildWarningEventArgs>();
            public List<BuildMessageEventArgs> Messages { get; } = new List<BuildMessageEventArgs>();
            public int YieldCount { get; private set; }
            public int ReacquireCount { get; private set; }

            public string Summary
            {
                get
                {
                    lock (this)
                    {
                        return string.Join(Environment.NewLine, Errors.Select(e => "error " + e.Code + ": " + e.Message)
                            .Concat(Warnings.Select(w => "warning " + w.Code + ": " + w.Message))
                            .Concat(Messages.Select(m => "message: " + m.Message)));
                    }
                }
            }

            public bool ContinueOnError => false;
            public int LineNumberOfTaskNode => 0;
            public int ColumnNumberOfTaskNode => 0;
            public string ProjectFileOfTaskNode => "App.csproj";
            public bool IsRunningMultipleNodes => false;

            public void LogErrorEvent(BuildErrorEventArgs e) { lock (this) { Errors.Add(e); } }
            public void LogWarningEvent(BuildWarningEventArgs e) { lock (this) { Warnings.Add(e); } }
            public void LogMessageEvent(BuildMessageEventArgs e) { lock (this) { Messages.Add(e); } }
            public void LogCustomEvent(CustomBuildEventArgs e) { }

            public void Yield() => YieldCount++;
            public void Reacquire() => ReacquireCount++;

            public bool BuildProjectFile(string projectFileName, string[] targetNames, IDictionary globalProperties, IDictionary targetOutputs) => throw new NotSupportedException();
            public bool BuildProjectFile(string projectFileName, string[] targetNames, IDictionary globalProperties, IDictionary targetOutputs, string toolsVersion) => throw new NotSupportedException();
            public bool BuildProjectFilesInParallel(string[] projectFileNames, string[] targetNames, IDictionary[] globalProperties, IDictionary[] targetOutputsPerProject, string[] toolsVersion, bool useResultsCache, bool unloadProjectsOnCompletion) => throw new NotSupportedException();
            public BuildEngineResult BuildProjectFilesInParallel(string[] projectFileNames, string[] targetNames, IDictionary[] globalProperties, IList<string>[] removeGlobalProperties, string[] toolsVersion, bool returnTargetOutputs) => throw new NotSupportedException();
        }
    }
}
