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

        /// <summary>
        /// Helper method for diffing codegenned files against their masters. This method 
        /// actually does the diffing of the passed in directories, and will recurisvely 
        /// call itself against other directories present in codegenDir.
        /// </summary>
        private static void DiffCodegenDirs(string codegenDir, string mastersDir, List<string> forbiddenLines)
        {
            string[] codegenFiles = Directory.GetFiles(codegenDir, "*.g.*");
            Array.Sort(codegenFiles);

            /* Normal case, where the codegen directory has files and should also have a corresponding master directory.*/
            if (codegenFiles.Length > 0)
            {
                Assert.IsTrue(Directory.Exists(mastersDir), 
                    $"Masters directory '{mastersDir}' does not exist for non-empty codegen directory '{codegenDir}'. " +
                    "Use copynewmasters.cmd to generate masters for it.");

                string[] masterFiles = Directory.GetFiles(mastersDir, "*.g.*");
                Array.Sort(masterFiles);
                Assert.AreEqual(codegenFiles.Length, masterFiles.Length, 
                    $"Differing number of generated files in '{mastersDir}' vs '{codegenDir}'.");

                for (int i = 0; i < codegenFiles.Length; i++)
                {
                    string codegenFile = codegenFiles[i];
                    string masterFile = masterFiles[i];

                    string codegenLocalFile = Path.GetFileName(codegenFile);
                    string masterLocalFile = Path.GetFileName(masterFile);
                    Assert.AreEqual(codegenLocalFile, masterLocalFile, 
                        $"File name mismatch for '{codegenLocalFile}' and '{masterLocalFile}'." +
                        "If you have deleted or renamed a file recently, make sure you've also deleted " +
                        "its master file and reran copynewmasters.cmd.");

                    DiffFiles(codegenFile, masterFile, forbiddenLines);
                }
            }

            /* Also search the code-genned folders subdirectories.  We need to pull out
             * the local directory name from the full path given by GetDirectories to construct
             * the masters' directory name.
             */
            string[] codegenDirs = Directory.GetDirectories(codegenDir);
            foreach (var dir in Directory.GetDirectories(codegenDir))
            {
                //codegenDir doesn't include the slash preceding the filename, so we have to add one to its file length
                //to remove it properly
                string dirLocalName = new DirectoryInfo(dir).Name;
                DiffCodegenDirs(dir, Path.Combine(mastersDir, dirLocalName), forbiddenLines);
            }
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
            string mastersPath = Path.Combine(MastersRoot.Value, masterDir);

            // Require codegen to actually be there. Without this, a project that failed to build
            // leaves an empty directory behind and the diff below passes over nothing at all,
            // turning a broken build into a green test. copynewmasters.cmd makes the same check
            // before it accepts a target.
            Assert.IsTrue(Directory.Exists(codegenPath) && Directory.EnumerateFiles(codegenPath, "*.g.*", SearchOption.AllDirectories).Any(),
                $"No codegen in '{codegenPath}'. Build the project behind '{masterDir}' before running this test.");

            DiffCodegenDirs(codegenPath, mastersPath, forbiddenLines);
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
                "RegressionProjects",
                GetBuildType(BuildOutput.Value.Flavor)));

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
            DiffCodegen(@"Basic\CppWinRT\Simple\Generated Files");
        }

        [TestMethod]
        public void Codegen_BasicCS()
        {
            DiffCodegen(@"Basic\CSharp\Simple\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_EventHandlingCppWinRT()
        {
            DiffCodegen(@"Basic\CppWinRT\EventHandling_968976\Generated Files");
        }

        [TestMethod]
        public void Codegen_NonStandardCppWinRT()
        {
            DiffCodegen(@"NonStandard\NonStandardCppWinRT\NonStandardCppWinRT\Generated Files");
        }

        //
        // References Tests
        //

        [TestMethod]
        public void Codegen_References_CSExe()
        {
            DiffCodegen(@"Basic\References\CSharpExe\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_References_CSLib()
        {
            DiffCodegen(@"Basic\References\CSharpLib\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_References_CSWinRT()
        {
            DiffCodegen(@"Basic\References\CSharpWinrtComponent\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_References_CppWinRTExe()
        {
            DiffCodegen(@"Basic\References\CppWinRTExe\Generated Files");
        }

        [TestMethod]
        public void Codegen_References_CppWinRTComponent()
        {
            DiffCodegen(@"Basic\References\CppWinRTComponent\Generated Files");
        }

        //
        // BindTestbed Tests
        //

        [TestMethod]
        public void Codegen_BindtestbedCS()
        {
            DiffCodegen(@"Features\CompiledBinding\BindTestbedCS\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_BindtestbedCppWinRT()
        {
            DiffCodegen(@"Features\CompiledBinding\BindTestbedCppWinRT\Generated Files");
        }

        [TestMethod]
        public void Codegen_BindTestbedCppWinRTIncremental()
        {
            DiffCodegen(@"Features\CompiledBinding\BindTestbedCppWinRT\Incremental\Generated Files");
        }

        [TestMethod]
        public void Codegen_BindPhasingTestbedCppWinRT()
        {
            DiffCodegen(@"Features\BindPhasingTestBedCppWinRT\BindPhasingTestBedCppWinRT\Generated Files");
        }

        //
        // DeferLoadStrategy Tests
        //

        [TestMethod]
        public void Codegen_DeferLoadStrategyCS()
        {
            DiffCodegen(@"Features\DeferLoadStrategy\CSharp\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_DeferLoadStrategyCppWinRT()
        {
            DiffCodegen(@"Features\DeferLoadStrategy\CppWinRT\Generated Files");
        }

        [TestMethod]
        public void Codegen_MetadataTestbedCppWinRT()
        {
            DiffCodegen(@"Features\Metadata\MetadataTestbedCppWinRT\Generated Files");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ConsumerCS()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ProviderCS()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ProviderCs\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ProviderCppWinRT()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ProviderCppWinRT\Generated Files");
        }

        [TestMethod]
        public void Codegen_ReduceProviderLoading_ConsumerCppWinRT()
        {
            DiffCodegen(@"Features\ReduceProviderLoading\ConsumerProvider\ConsumerCppWinRT\Generated Files");
        }

        [TestMethod]
        public void Codegen_MultipleViewsCppWinRT()
        {
            DiffCodegen(@"Features\MultiXamlFiles\MultipleViewsTestbedCppWinRT\Generated Files");
        }

        [TestMethod]
        public void Codegen_MultipleViewsCS()
        {
            DiffCodegen(@"Features\MultiXamlFiles\MultipleViewsTestbed\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_ConditionalControls()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalControls\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_ConditionalsModel()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalsModel\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_ConditionalsCS()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalsCS\obj\x86\Debug");
        }

        [TestMethod]
        public void Codegen_ConditionalsCppWinRT()
        {
            DiffCodegen(@"Features\Conditionals\ConditionalsCppWinRT\Generated Files");
        }

        [TestMethod]
        public void Codegen_MarkupExtensionsCppWinRT()
        {
            DiffCodegen(@"Features/MarkupExtensions/MarkupExtensionsCppWinRT/Generated Files");
        }

        [TestMethod]
        public void Codegen_MarkupExtensionsCS()
        {
            DiffCodegen(@"Features/MarkupExtensions/MarkupExtensionsCS/obj/x86/Debug");
        }
    }
}
