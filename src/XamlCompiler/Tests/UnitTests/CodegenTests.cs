// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace UnitTests
{
    /// List of strings which cannot show up in codegen for the targeted min version
    internal static class ForbiddenCodegen
    {
        public static List<string> RS2 = new List<string> { };
        public static List<string> RS1 = RS2.Concat(new List<string> { "XamlMarkupHelper.UnloadObject", "XamlMarkupHelper::UnloadObject" }).ToList();
    }

    [TestClass]
    public class CodegenTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        private static void DiffFiles(string actualFile, string expectedFile, List<string> forbiddenLines)
        {
            string diffCommand = $"bcomp \"{actualFile}\" \"{expectedFile}\"";

            FileInfo actualFileInfo = new FileInfo(actualFile);
            FileInfo expectedFileInfo = new FileInfo(expectedFile);

            using (var actualReader = actualFileInfo.OpenText())
            {
                using (var expectedReader = expectedFileInfo.OpenText())
                {
                    string actualLine;
                    string expectedLine;
                    do
                    {
                        if (actualReader.EndOfStream || expectedReader.EndOfStream)
                        {
                            // If one of them is EOS, then both should be EOS.
                            Assert.AreEqual(expectedReader.EndOfStream, actualReader.EndOfStream,
                                $"File lengths differ: {diffCommand}");
                            break;
                        }

                        actualLine = actualReader.ReadLine();
                        expectedLine = expectedReader.ReadLine();

                        if (actualLine != expectedLine && !IsException(actualLine))
                        {
                            Assert.AreEqual(expectedLine, actualLine,
                                $"Files are different: {diffCommand}");
                        }

                        if (forbiddenLines != null && actualLine != null)
                        {
                            foreach (string forbiddenLine in forbiddenLines)
                            {
                                if (actualLine.Contains(forbiddenLine))
                                {
                                    Assert.Fail($"File {actualFile} contains version-forbidden string '{forbiddenLine}'. {diffCommand}");
                                }
                            }
                        }
                    }
                    while (actualLine != null);
                }
            }
        }

        [TestMethod]
        public void Codegen_DiffCodegenFiles_ValidatesLayeredMasters()
        {
            // Unit-test DiffCodegenFiles with synthetic output: the regression tests use real
            // codegen, while this fixture checks flavor selection and invalid file sets.
            string root = Path.Combine(Path.GetTempPath(), "XamlCompilerMasters-" + Guid.NewGuid().ToString("N"));
            const string target = "generated";
            try
            {
                foreach (string layer in new[] { "common", "chk", "fre" })
                {
                    string directory = Path.Combine(root, layer, target);
                    Directory.CreateDirectory(directory);
                    File.WriteAllText(Path.Combine(directory, layer + ".g.cs"), layer);
                }

                string actual = Path.Combine(root, "actual");
                Directory.CreateDirectory(actual);
                File.WriteAllText(Path.Combine(actual, "common.g.cs"), "common");
                File.WriteAllText(Path.Combine(actual, "chk.g.cs"), "chk");
                DiffCodegenFiles(actual, root, target, "chk", null);

                // Switch the generated flavor while keeping the same shared file.
                File.Delete(Path.Combine(actual, "chk.g.cs"));
                File.WriteAllText(Path.Combine(actual, "fre.g.cs"), "fre");
                DiffCodegenFiles(actual, root, target, "fre", null);
                File.Delete(Path.Combine(actual, "fre.g.cs"));
                File.WriteAllText(Path.Combine(actual, "chk.g.cs"), "chk");

                // Reject both an unexpected generated file and an orphaned master.
                string extra = Path.Combine(actual, "extra.g.cs");
                File.WriteAllText(extra, "extra");
                AssertCodegenDiffFails(() => DiffCodegenFiles(actual, root, target, "chk", null), "No master");
                File.Delete(extra);

                File.Delete(Path.Combine(actual, "chk.g.cs"));
                AssertCodegenDiffFails(() => DiffCodegenFiles(actual, root, target, "chk", null), "Stale master");

                // A flavor-specific file must not shadow a shared master.
                File.WriteAllText(Path.Combine(root, "chk", target, "common.g.cs"), "duplicate");
                AssertCodegenDiffFails(() => DiffCodegenFiles(actual, root, target, "chk", null), "both common and chk");
            }
            finally
            {
                if (Directory.Exists(root))
                {
                    Directory.Delete(root, true);
                }
            }
        }

        // Verify the expected mismatch, not an unrelated assertion failure.
        private static void AssertCodegenDiffFails(Action comparison, string expectedMessage)
        {
            bool failed = false;
            try
            {
                comparison();
            }
            catch (AssertFailedException exception)
            {
                StringAssert.Contains(exception.Message, expectedMessage);
                failed = true;
            }
            Assert.IsTrue(failed, $"Expected comparison to fail with '{expectedMessage}'.");
        }

        private static void DiffCodegenFiles(string codegenPath, string mastersRoot, string masterDir, string flavor, List<string> forbiddenLines)
        {
            // Each generated path must match exactly once in the union of common and the selected flavor;
            // extra masters must not survive a rename or deletion.
            var masters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (string layer in new[] { "common", flavor })
            {
                string directory = Path.Combine(mastersRoot, layer, masterDir);
                if (Directory.Exists(directory))
                {
                    foreach (string file in Directory.GetFiles(directory, "*.g.*", SearchOption.AllDirectories))
                    {
                        string relativePath = file.Substring(directory.Length + 1);
                        Assert.IsFalse(masters.ContainsKey(relativePath),
                            $"Master '{relativePath}' exists in both common and {flavor} for '{masterDir}'.");
                        masters.Add(relativePath, file);
                    }
                }
            }

            foreach (string codegenFile in Directory.GetFiles(codegenPath, "*.g.*", SearchOption.AllDirectories))
            {
                string relativePath = codegenFile.Substring(codegenPath.Length + 1);
                string masterFile;
                Assert.IsTrue(masters.TryGetValue(relativePath, out masterFile),
                    $"No master for '{relativePath}' in '{masterDir}'. Run copynewmasters.cmd for this flavor.");
                DiffFiles(codegenFile, masterFile, forbiddenLines);
                masters.Remove(relativePath);
            }

            Assert.AreEqual(0, masters.Count,
                $"Stale master(s) in '{masterDir}': {string.Join(", ", masters.Keys)}. Run copynewmasters.cmd for this flavor.");
        }

        private static void DiffCodegen(string targetDir, List<string> forbiddenLines = null)
        {
            /* targetDir is the master-path key in CodegenTargets.txt. The manifest maps it to the
             * independent $(GeneratedFilesDir) path used by this test and copynewmasters.cmd.
             */
            string masterDir = NormalizePath(targetDir);

            string codegenDir;
            Assert.IsTrue(CodegenTargets.Value.TryGetValue(masterDir, out codegenDir),
                $"'{masterDir}' is not listed in {CodegenTargetsFileName}. Add it there, so that this " +
                "test and copynewmasters.cmd agree on where its codegen is written.");

            string codegenPath = Path.Combine(CodegenRoot.Value, codegenDir);

            // Require codegen to actually be there. Without this, a project that failed to build
            // leaves an empty directory behind and the diff below passes over nothing at all,
            // turning a broken build into a green test. copynewmasters.cmd makes the same check
            // before it accepts a target.
            Assert.IsTrue(Directory.Exists(codegenPath) && Directory.EnumerateFiles(codegenPath, "*.g.*", SearchOption.AllDirectories).Any(),
                $"No codegen in '{codegenPath}'. Build the project behind '{masterDir}' before running this test.");

            DiffCodegenFiles(codegenPath, MastersRoot.Value, masterDir, GetBuildType(BuildOutput.Value.Flavor), forbiddenLines);
        }

        private const string CodegenTargetsFileName = "CodegenTargets.txt";

        private static readonly Lazy<Dictionary<string, string>> CodegenTargets =
            new Lazy<Dictionary<string, string>>(LoadCodegenTargets);

        private static readonly Lazy<BuildOutputLocation> BuildOutput =
            new Lazy<BuildOutputLocation>(() =>
            {
                BuildOutputLocation location = FindBuildOutput();
                Assert.IsNotNull(location,
                    $"Cannot locate BuildOutput above '{TestBinDir}'; the codegen tests need a built enlistment.");
                return location;
            });

        /// <summary>
        /// Where the regression projects' codegen is, for the flavor this test assembly belongs to.
        /// It is derived from the test assembly's own path rather than from the build environment,
        /// which is not set under every test runner. Codegen always lives under BuildOutput\obj, even
        /// when the test assembly itself was binplaced to BuildOutput\bin.
        /// </summary>
        private static readonly Lazy<string> CodegenRoot = new Lazy<string>(() =>
            Path.Combine(BuildOutput.Value.BuildOutputDir, "obj", BuildOutput.Value.Flavor));

        /// <summary>
        /// The masters in the enlistment are preferred, so that a copynewmasters.cmd run takes effect
        /// without rebuilding this project. The copy staged next to the test assembly is the fallback,
        /// and is what a test payload on another machine has.
        /// </summary>
        private static readonly Lazy<string> MastersRoot = new Lazy<string>(() =>
            Path.Combine(
                FindInEnlistmentOrNextToTests(@"src\XamlCompiler\TestMasters", "TestMasters", Directory.Exists),
                "RegressionProjects"));

        private static Dictionary<string, string> LoadCodegenTargets()
        {
            string targetsFile = FindInEnlistmentOrNextToTests(
                @"src\XamlCompiler\Tests\UnitTests\" + CodegenTargetsFileName, CodegenTargetsFileName, File.Exists);

            var targets = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (string rawLine in File.ReadAllLines(targetsFile))
            {
                string line = rawLine.Trim();
                if (line.Length == 0 || line.StartsWith("#"))
                {
                    continue;
                }

                int separator = line.IndexOf('|');
                Assert.AreNotEqual(-1, separator, $"Malformed line in '{targetsFile}': '{rawLine}'");
                targets[NormalizePath(line.Substring(0, separator))] = NormalizePath(line.Substring(separator + 1));
            }

            return targets;
        }

        private static string FindInEnlistmentOrNextToTests(string enlistmentRelativePath, string localName, Func<string, bool> exists)
        {
            BuildOutputLocation location = FindBuildOutput();
            if (location != null)
            {
                string enlisted = Path.Combine(location.EnlistmentRoot, enlistmentRelativePath);
                if (exists(enlisted))
                {
                    return enlisted;
                }
            }

            string staged = Path.Combine(TestBinDir, localName);
            Assert.IsTrue(exists(staged),
                $"Cannot find '{enlistmentRelativePath}' in the enlistment, nor '{localName}' next to the test assembly.");
            return staged;
        }

        private sealed class BuildOutputLocation
        {
            public string EnlistmentRoot { get; set; }
            public string BuildOutputDir { get; set; }
            public string Flavor { get; set; }
        }

        private static BuildOutputLocation FindBuildOutput()
        {
            var directory = new DirectoryInfo(TestBinDir);
            while (directory != null)
            {
                DirectoryInfo parent = directory.Parent;
                if (parent != null && parent.Parent != null &&
                    string.Equals(parent.Parent.Name, "BuildOutput", StringComparison.OrdinalIgnoreCase) &&
                    (string.Equals(parent.Name, "obj", StringComparison.OrdinalIgnoreCase) ||
                     string.Equals(parent.Name, "bin", StringComparison.OrdinalIgnoreCase)))
                {
                    return new BuildOutputLocation
                    {
                        EnlistmentRoot = parent.Parent.Parent.FullName,
                        BuildOutputDir = parent.Parent.FullName,
                        Flavor = directory.Name,
                    };
                }

                directory = parent;
            }

            return null;
        }

        private static string TestBinDir
        {
            get { return Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location); }
        }

        private static string NormalizePath(string path)
        {
            return path.Trim().Replace('/', '\\').Trim('\\');
        }

        private static string GetBuildType(string flavor)
        {
            string buildType = flavor.Length >= 3 ? flavor.Substring(flavor.Length - 3) : string.Empty;
            Assert.IsTrue(
                string.Equals(buildType, "chk", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(buildType, "fre", StringComparison.OrdinalIgnoreCase),
                $"Build output flavor '{flavor}' does not end in chk or fre.");
            return buildType.ToLowerInvariant();
        }

        private static bool IsException(string line)
        {
            // Lines that legitimately differ from their master, because they carry a checksum or a
            // tool version. tools\fixmasters\fixmasters.cs truncates the same set when a master is
            // taken, so the two must be kept in agreement.
            if (line.StartsWith("#pragma checksum \"") ||
                line.StartsWith("#ExternalChecksum(\"") ||
                line.StartsWith("// WARNING: Please don't edit this file"))
            {
                return true;
            }
            return false;
        }

        //
        // Basic Tests
        //

        [TestMethod]
        public void Codegen_BasicCppWinRT()
        {
            DiffCodegen(@"Basic\CppWinRT\Simple\generated");
        }

        [TestMethod]
        public void Codegen_BasicCS()
        {
            DiffCodegen(@"Basic\CSharp\Simple\generated");
        }

        [TestMethod]
        public void Codegen_EventHandlingCppWinRT()
        {
            DiffCodegen(@"Basic\CppWinRT\EventHandling_968976\generated");
        }

        [TestMethod]
        public void Codegen_NonStandardCppWinRT()
        {
            DiffCodegen(@"NonStandard\NonStandardCppWinRT\NonStandardCppWinRT\generated");
        }

        //
        // References Tests
        //

        [TestMethod]
        public void Codegen_References_CSExe()
        {
            DiffCodegen(@"Basic\References\CSharpExe\generated");
        }

        [TestMethod]
        public void Codegen_References_CSLib()
        {
            DiffCodegen(@"Basic\References\CSharpLib\generated");
        }

        [TestMethod]
        public void Codegen_References_CSWinRT()
        {
            DiffCodegen(@"Basic\References\CSharpWinrtComponent\generated");
        }

        [TestMethod]
        public void Codegen_References_CppWinRTExe()
        {
            DiffCodegen(@"Basic\References\CppWinRTExe\generated");
        }

        [TestMethod]
        public void Codegen_References_CppWinRTComponent()
        {
            DiffCodegen(@"Basic\References\CppWinRTComponent\generated");
        }

        //
        // BindTestbed Tests
        //

        [TestMethod]
        public void Codegen_BindtestbedCS()
        {
            DiffCodegen(@"Features\CompiledBinding\BindTestbedCS\generated");
        }

        [TestMethod]
        public void Codegen_BindtestbedCppWinRT()
        {
            DiffCodegen(@"Features\CompiledBinding\BindTestbedCppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_BindTestbedCppWinRTIncremental()
        {
            DiffCodegen(@"Features\CompiledBinding\BindTestbedCppWinRT\Incremental\generated");
        }

        [TestMethod]
        public void Codegen_BindPhasingTestbedCppWinRT()
        {
            DiffCodegen(@"Features\BindPhasingTestBedCppWinRT\BindPhasingTestBedCppWinRT\generated");
        }

        //
        // DeferLoadStrategy Tests
        //

        [TestMethod]
        public void Codegen_DeferLoadStrategyCS()
        {
            DiffCodegen(@"Features\DeferLoadStrategy\CSharp\generated");
        }

        [TestMethod]
        public void Codegen_DeferLoadStrategyCppWinRT()
        {
            DiffCodegen(@"Features\DeferLoadStrategy\CppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_MetadataTestbedCppWinRT()
        {
            DiffCodegen(@"Features\Metadata\MetadataTestbedCppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ConsumerCS()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs\generated");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ProviderCS()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ProviderCs\generated");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ProviderCppWinRT()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ProviderCppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ConsumerCppWinRT()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ConsumerCppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_MultipleViewsCppWinRT()
        {
            DiffCodegen(@"Features\MultiXamlFiles\MultipleViewsTestbedCppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_MultipleViewsCS()
        {
            DiffCodegen(@"Features\MultiXamlFiles\MultipleViewsTestbed\generated");
        }

        [TestMethod]
        public void Codegen_ConditionalControls()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalControls\generated");
        }

        [TestMethod]
        public void Codegen_ConditionalsModel()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalsModel\generated");
        }

        [TestMethod]
        public void Codegen_ConditionalsCS()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalsCS\generated");
        }

        [TestMethod]
        public void Codegen_ConditionalsCppWinRT()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalsCppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_MarkupExtensionsCppWinRT()
        {
            DiffCodegen(@"Features\MarkupExtensions\MarkupExtensionsCppWinRT\generated");
        }

        [TestMethod]
        public void Codegen_MarkupExtensionsCS()
        {
            DiffCodegen(@"Features\MarkupExtensions\MarkupExtensionsCS\generated");
        }
    }
}
