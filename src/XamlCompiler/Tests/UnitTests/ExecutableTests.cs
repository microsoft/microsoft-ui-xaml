// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.Json;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    /// <summary>
    /// Tests of the compiler executable's driver (XamlCompiler.exe: ConsoleCompileXaml.cs) and of the pieces it uses to talk to
    /// the CompileXaml task: the JSON contract, the IDE file overlays of design-time builds and the atomic file writes.
    /// </summary>
    [TestClass]
    public class ExecutableTests
    {
        private const string DriverTypeName = "Microsoft.UI.Xaml.Markup.Compiler.Executable.CompileXaml";
        private const string OverlayServiceTypeName = "Microsoft.UI.Xaml.Markup.Compiler.Executable.OverlayTaskFileService";
        private const string ContractNamespace = "Microsoft.UI.Xaml.Markup.Compiler.MSBuildInterop.";

        private string _directory;

        private static Assembly Compiler => Assembly.LoadFrom(ProxyHelper.XamlCompilerPath);

        [TestInitialize]
        public void Initialize()
        {
            _directory = Path.Combine(Path.GetTempPath(), "XamlCompilerExecutableTests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_directory);
        }

        [TestCleanup]
        public void Cleanup()
        {
            try
            {
                Directory.Delete(_directory, recursive: true);
            }
            catch (IOException)
            {
            }
        }

        // A type of the JSON contract, as compiled into the compiler executable
        private static Type GetContractType(string name)
        {
            return Compiler.GetType(ContractNamespace + name, true);
        }

        private static int CurrentContractVersion => (int)GetContractType("CompilerContract").GetField("Version").GetValue(null);

        private int RunCompiler(string inputJson, out JsonElement output)
        {
            string input = Path.Combine(_directory, "input.json");
            string outputPath = Path.Combine(_directory, "output.json");
            File.WriteAllText(input, inputJson);
            object driver = Activator.CreateInstance(Compiler.GetType(DriverTypeName, true));
            int exitCode = (int)driver.GetType().GetMethod("Run").Invoke(driver, new object[] { new[] { input, outputPath } });
            Assert.IsTrue(File.Exists(outputPath), "The compiler must always write its output file");
            using (JsonDocument document = JsonDocument.Parse(File.ReadAllText(outputPath)))
            {
                output = document.RootElement.Clone();
            }
            Assert.AreEqual(0, Directory.GetFiles(_directory, "*.tmp").Length, "No temporary file must be left behind");
            return exitCode;
        }

        private static JsonElement[] GetErrors(JsonElement output)
        {
            // MSBuildLogEntry.EntryType.Error
            return output.GetProperty("MSBuildLogEntries").EnumerateArray().Where(e => e.GetProperty("Type").GetInt32() == 2).ToArray();
        }

        [TestMethod]
        public void Executable_ForceSharedStateShutdownIsANoOp()
        {
            int exitCode = RunCompiler($"{{\"ContractVersion\":{CurrentContractVersion},\"ForceSharedStateShutdown\":true,\"CompileMode\":\"OnErrorShutdown\"}}", out JsonElement output);
            Assert.AreEqual(0, exitCode);
            Assert.AreEqual(CurrentContractVersion, output.GetProperty("ContractVersion").GetInt32());
            Assert.AreEqual(0, GetErrors(output).Length);
            Assert.AreEqual(0, output.GetProperty("GeneratedCodeFiles").GetArrayLength());
        }

        [TestMethod]
        public void Executable_NewerContractVersionIsReportedInTheOutput()
        {
            int exitCode = RunCompiler($"{{\"ContractVersion\":{CurrentContractVersion + 1}}}", out JsonElement output);
            Assert.AreEqual(1, exitCode);
            JsonElement[] errors = GetErrors(output);
            Assert.AreEqual(1, errors.Length);
            Assert.AreEqual("WMC9999", errors[0].GetProperty("ErrorCode").GetString());
            StringAssert.Contains(errors[0].GetProperty("Message").GetString(), "contract version");
        }

        [TestMethod]
        public void Executable_InvalidInputIsReportedInTheOutput()
        {
            int exitCode = RunCompiler("this isn't JSON", out JsonElement output);
            Assert.AreEqual(1, exitCode);
            JsonElement[] errors = GetErrors(output);
            Assert.AreEqual(1, errors.Length);
            Assert.AreEqual("WMC9999", errors[0].GetProperty("ErrorCode").GetString());
        }

        [TestMethod]
        public void Executable_DesignTimeBuildModeComesFromTheInputs()
        {
            Type inputsType = GetContractType("CompilerInputs");
            MethodInfo createCore = Compiler.GetType(DriverTypeName, true).GetMethod("CreateCore", BindingFlags.NonPublic | BindingFlags.Instance);
            object driver = Activator.CreateInstance(Compiler.GetType(DriverTypeName, true));

            bool IsDesignTimeBuild(string compileMode, bool isDesignTimeBuild)
            {
                object inputs = Activator.CreateInstance(inputsType);
                inputsType.GetProperty("Language").SetValue(inputs, "C#");
                inputsType.GetProperty("LanguageSourceExtension").SetValue(inputs, ".cs");
                inputsType.GetProperty("CompileMode").SetValue(inputs, compileMode);
                inputsType.GetProperty("IsDesignTimeBuild").SetValue(inputs, isDesignTimeBuild);
                foreach (string list in new[] { "ReferenceAssemblies", "ReferenceAssemblyPaths" })
                {
                    PropertyInfo property = inputsType.GetProperty(list);
                    property.SetValue(inputs, Activator.CreateInstance(property.PropertyType));
                }
                object core = createCore.Invoke(driver, new[] { inputs });
                return (bool)core.GetType().GetProperty("IsDesignTimeBuild").GetValue(core);
            }

            Assert.IsTrue(IsDesignTimeBuild("DesignTimeBuild", false), "CompileMode=DesignTimeBuild (inputs of older tasks)");
            Assert.IsTrue(IsDesignTimeBuild("RealBuildPass1", true), "IsDesignTimeBuild (e.g. from the IDE host object)");
            Assert.IsFalse(IsDesignTimeBuild("RealBuildPass1", false));
            Assert.IsFalse(IsDesignTimeBuild("RealBuildPass2", false));
        }

        [TestMethod]
        public void DesignTimeBuild_LoadsNuGetReferencesLikeRealBuilds()
        {
            // Design-time builds used to treat NuGet references as (lazily loaded) system references, so the types they define
            // (e.g. Windows.UI.Color, from the Windows SDK projection) were unknown and the design-time builds failed on them
            Type core = Compiler.GetType("Microsoft.UI.Xaml.Markup.Compiler.CompileXamlInternal", true);
            Type itemType = GetContractType("MSBuildItem");
            Type assemblyItemType = GetContractType("IAssemblyItem");
            object Item(string path, bool isSystem, bool isNuGet)
            {
                object item = Activator.CreateInstance(itemType);
                itemType.GetProperty("ItemSpec").SetValue(item, path);
                itemType.GetProperty("IsSystemReference").SetValue(item, isSystem);
                itemType.GetProperty("IsNuGetReference").SetValue(item, isNuGet);
                return item;
            }

            foreach (bool isDesignTimeBuild in new[] { false, true })
            {
                object compiler = Activator.CreateInstance(core);
                core.GetProperty("IsDesignTimeBuild").SetValue(compiler, isDesignTimeBuild);
                IList references = (IList)Activator.CreateInstance(typeof(System.Collections.Generic.List<>).MakeGenericType(assemblyItemType));
                references.Add(Item(@"C:\refs\Microsoft.Windows.SDK.NET.dll", false, true));
                references.Add(Item(@"C:\refs\Windows.winmd", true, false));
                references.Add(Item(@"C:\refs\System.Xml.dll", true, false));
                core.GetProperty("ReferenceAssemblies").SetValue(compiler, references);

                object[] arguments = new object[3];
                core.GetMethod("SortReferenceAssemblies", BindingFlags.NonPublic | BindingFlags.Instance).Invoke(compiler, arguments);
                CollectionAssert.AreEqual(new[] { @"C:\refs\Windows.winmd" }, (ICollection)arguments[0], $"system, design-time: {isDesignTimeBuild}");
                CollectionAssert.AreEqual(new[] { @"C:\refs\Microsoft.Windows.SDK.NET.dll" }, (ICollection)arguments[1], $"non-system, design-time: {isDesignTimeBuild}");
                CollectionAssert.AreEqual(new[] { @"C:\refs\System.Xml.dll" }, (ICollection)arguments[2], $"system extra, design-time: {isDesignTimeBuild}");
            }
        }

        [TestMethod]
        public void Executable_FileOverlaysReplaceTheFilesOpenInTheIde()
        {
            string dirty = Path.Combine(_directory, "Dirty.xaml");
            string open = Path.Combine(_directory, "Open.xaml");
            string closed = Path.Combine(_directory, "Closed.xaml");
            foreach (string file in new[] { dirty, open, closed })
            {
                File.WriteAllText(file, "<Page />");
            }
            long editTime = new DateTime(2030, 1, 2, 3, 4, 5, DateTimeKind.Utc).ToFileTimeUtc();

            Type overlayType = GetContractType("FileOverlay");
            IList overlays = (IList)Activator.CreateInstance(typeof(System.Collections.Generic.List<>).MakeGenericType(overlayType));
            object Overlay(string path, string contents, long time)
            {
                object overlay = Activator.CreateInstance(overlayType);
                overlayType.GetProperty("Path").SetValue(overlay, path);
                overlayType.GetProperty("Contents").SetValue(overlay, contents);
                overlayType.GetProperty("LastChangeFileTime").SetValue(overlay, time);
                return overlay;
            }
            // Paths are compared in full, case-insensitively, as the compiler and the IDE spell them differently
            overlays.Add(Overlay(Path.Combine(_directory, ".", "DIRTY.xaml"), "<Page><Grid /></Page>", editTime));
            overlays.Add(Overlay(open, null, editTime));

            object service = Activator.CreateInstance(Compiler.GetType(OverlayServiceTypeName, true), ".cs", overlays);
            string Read(string file)
            {
                using (TextReader reader = (TextReader)service.GetType().GetMethod("GetFileContents").Invoke(service, new object[] { file }))
                {
                    return reader.ReadToEnd();
                }
            }
            DateTime Time(string file) => (DateTime)service.GetType().GetMethod("GetLastChangeTime").Invoke(service, new object[] { file });

            Assert.AreEqual("<Page><Grid /></Page>", Read(dirty));
            Assert.AreEqual(DateTime.FromFileTime(editTime), Time(dirty));
            Assert.AreEqual("<Page />", Read(open), "Null contents mean the file is unchanged");
            Assert.AreEqual(DateTime.FromFileTime(editTime), Time(open));
            Assert.AreEqual("<Page />", Read(closed));
            Assert.AreEqual(File.GetLastWriteTime(closed), Time(closed));
        }

        [TestMethod]
        public void Executable_FailedWritesKeepThePreviousFile()
        {
            Type atomicFile = Compiler.GetType("Microsoft.UI.Xaml.Markup.Compiler.AtomicFile", true);
            MethodInfo write = atomicFile.GetMethod("Write");
            string file = Path.Combine(_directory, "Generated.g.cs");
            File.WriteAllText(file, "previous");

            Action<Stream> failingWriter = stream =>
            {
                stream.Write(new byte[] { 1, 2, 3 }, 0, 3);
                throw new IOException("Simulated failure");
            };
            TargetInvocationException e = Assert.ThrowsExactly<TargetInvocationException>(() => write.Invoke(null, new object[] { file, failingWriter }));
            Assert.IsInstanceOfType(e.InnerException, typeof(IOException));
            Assert.AreEqual("previous", File.ReadAllText(file));

            Action<Stream> writer = stream =>
            {
                byte[] bytes = System.Text.Encoding.UTF8.GetBytes("new");
                stream.Write(bytes, 0, bytes.Length);
            };
            write.Invoke(null, new object[] { file, writer });
            Assert.AreEqual("new", File.ReadAllText(file));
            Assert.AreEqual(1, Directory.GetFiles(_directory).Length, "No temporary file must be left behind");
        }

        [TestMethod]
        public void Executable_WritesWaitForADestinationLockedForAMoment()
        {
            Type atomicFile = Compiler.GetType("Microsoft.UI.Xaml.Markup.Compiler.AtomicFile", true);
            MethodInfo writeAllBytes = atomicFile.GetMethod("WriteAllBytes");
            string file = Path.Combine(_directory, "Generated.xbf");
            File.WriteAllText(file, "previous");

            // E.g. an antivirus scanner that opened the previous file
            FileStream locker = new FileStream(file, FileMode.Open, FileAccess.Read, FileShare.None);
            System.Threading.Tasks.Task release = System.Threading.Tasks.Task.Delay(150).ContinueWith(_ => locker.Dispose());
            writeAllBytes.Invoke(null, new object[] { file, new byte[] { 1, 2, 3 } });
            release.Wait();

            CollectionAssert.AreEqual(new byte[] { 1, 2, 3 }, File.ReadAllBytes(file));
            Assert.AreEqual(1, Directory.GetFiles(_directory).Length, "No temporary file must be left behind");
        }

        [TestMethod]
        public void Executable_FailedAssertionsAreReportedInsteadOfTerminatingTheCompiler()
        {
            // Installed by Program.Main: a failed Debug.Assert would terminate the Debug builds of the compiler otherwise
            Type listenerType = Compiler.GetType("Microsoft.UI.Xaml.Markup.Compiler.Executable.AssertionTraceListener", true);
            List<string> reports = new List<string>();
            TraceListener listener = (TraceListener)Activator.CreateInstance(listenerType, BindingFlags.NonPublic | BindingFlags.Instance, null, new object[] { new Action<string>(reports.Add) }, null);

            listener.Fail("Validator should have checked this scenario", "detail");

            Assert.AreEqual(1, reports.Count);
            StringAssert.StartsWith(reports[0], "XAML compiler assertion failed: Validator should have checked this scenario detail");
            StringAssert.Contains(reports[0], nameof(Executable_FailedAssertionsAreReportedInsteadOfTerminatingTheCompiler));

            // And they become diagnostic messages of the compilation
            object driver = Activator.CreateInstance(Compiler.GetType(DriverTypeName, true));
            driver.GetType().GetMethod("ReportAssertionFailure").Invoke(driver, new object[] { "XAML compiler assertion failed: test" });
            string input = Path.Combine(_directory, "input.json");
            string output = Path.Combine(_directory, "output.json");
            File.WriteAllText(input, "{ \"ContractVersion\": 1, \"ForceSharedStateShutdown\": true }");
            Assert.AreEqual(0, (int)driver.GetType().GetMethod("Run").Invoke(driver, new object[] { new[] { input, output } }));
            using (JsonDocument document = JsonDocument.Parse(File.ReadAllText(output)))
            {
                JsonElement entry = document.RootElement.GetProperty("MSBuildLogEntries").EnumerateArray().Single();
                Assert.AreEqual(0, entry.GetProperty("Type").GetInt32(), "A message, not a warning or an error");
                Assert.AreEqual("XAML compiler assertion failed: test", entry.GetProperty("Message").GetString());
            }
        }
    }
}
