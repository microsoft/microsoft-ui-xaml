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

        private static void DiffFiles(string codegenFile, string masterFile, List<string> forbiddenLines)
        {
            string diffCommand = $"bcomp \"{codegenFile}\" \"{masterFile}\"";

            FileInfo codegenFileInfo = new FileInfo(codegenFile);
            FileInfo masterFileInfo = new FileInfo(masterFile);

            using (var codegenReader = codegenFileInfo.OpenText())
            {
                using (var masterReader = masterFileInfo.OpenText())
                {
                    string codegenLine;
                    string masterLine;
                    do
                    {
                        if (codegenReader.EndOfStream || masterReader.EndOfStream)
                        {
                            // If one of them is EOS, then both should be EOS.
                            Assert.AreEqual(codegenReader.EndOfStream, masterReader.EndOfStream,
                                $"File lengths differ: {diffCommand}");
                            break;
                        }

                        codegenLine = codegenReader.ReadLine();
                        masterLine = masterReader.ReadLine();

                        if (codegenLine != masterLine && !IsException(codegenLine))
                        {
                            Assert.AreEqual(codegenLine, masterLine, 
                                $"Files are different: {diffCommand}");
                        }

                        if (forbiddenLines != null && codegenLine != null)
                        {
                            foreach (string forbiddenLine in forbiddenLines)
                            {
                                if (codegenLine.Contains(forbiddenLine))
                                {
                                    Assert.Fail($"File {codegenFile} contains version-forbidden string '{forbiddenLine}'. {diffCommand}");
                                }
                            }
                        }
                    }
                    while (codegenLine != null);
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
            /* This method is passed in a subdirectory to diff all codegenned files against (including its own children directories).
             * The path is relative to XAMLCompiler\Tests.  E.g. to diff BindTestbedCS's Debug x86 build, targetDir would be
             * "RegressionProjects\Features\CompiledBinding\BindTestbedCS\obj\x86\Debug".  This method would then check the generated files there against
             * the corresponding masters in XAMLCompiler\TestMasters\RegressionProjects\Features\CompiledBinding\BindTestbedCS\obj\x86\Debug.
             */

            //Get the directory where the solution is located (XAMLCompiler).  Tests are run from XAMLCompiler\Tests\UnitTests\UnitTestingBin,
            //so we need to get the great-grandparent of the current directory to get there.
            var solutionDir = FindSolutionDir(new DirectoryInfo(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location)));
            Assert.AreEqual(solutionDir.Name, "XAMLCompiler", true);

            string codegenDir = Path.Combine(solutionDir.FullName, "Tests", targetDir);
            string mastersDir = Path.Combine(solutionDir.FullName, "TestMasters", targetDir);

            DiffCodegenDirs(codegenDir, mastersDir, forbiddenLines);
        }

        private static DirectoryInfo FindSolutionDir(DirectoryInfo currentDirectory)
        {
            if (currentDirectory == null)
            {
                throw new ArgumentNullException("Cannot find the XamlCompiler sollution directory");
            }
            if (currentDirectory.GetFiles("XamlCompiler.sln").Length > 0)
            {
                return currentDirectory;
            }
            return FindSolutionDir(currentDirectory.Parent);
        }

        private static bool IsException(string line)
        {
            // This line contains a path which is ok to be different.
            if (line.StartsWith("#pragma checksum \"") ||
                line.StartsWith("#ExternalChecksum(\""))
            {
                return true;
            }
            return false;
        }

        //
        // Basic Tests
        //

        [TestMethod]
        [Ignore]
        public void Codegen_BasicCppWinRT()
        {
            DiffCodegen(@"RegressionProjects\Basic\CppWinRT\Simple\Generated Files");
        }

        [Ignore]
        [TestMethod]
        public void Codegen_BasicCS()
        {
            DiffCodegen(@"RegressionProjects\Basic\CSharp\Simple\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BasicCX()
        {
            DiffCodegen(@"RegressionProjects\Basic\VC\Simple\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_EventHandlingCX()
        {
            DiffCodegen(@"RegressionProjects\Basic\VC\EventHandling_968976\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BasicVB()
        {
            DiffCodegen(@"RegressionProjects\Basic\VisualBasic\Simple\obj\x86\Debug");
        }

        //
        // References Tests
        //

        [TestMethod]
        [Ignore]
        // Behaviors SDK no longer ships with VS 2017
        public void Codegen_VC_Old_Behaviors_SDK()
        {
            DiffCodegen(@"RegressionProjects\Basic\OldPlatformsReferences\VC_Old_Behaviors_SDK\VC_Exe\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_CSExe()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\CSharpExe\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_CSLib()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\CSharpLib\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_CSWinRT()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\CSharpWinrtComponent\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_VBExe()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\VBExe\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_VBLib()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\VBLib\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_VBWinRT()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\VBWinrtComponent\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_CXExe()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\VCExe\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_References_CXWinRT()
        {
            DiffCodegen(@"RegressionProjects\Basic\References\VCWinrtComponent\Generated Files");
        }

        //
        // BindTestbed Tests
        //

        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedCS()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedCS\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedVB()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedVB\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedCX()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedCX\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedCXIncremental()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedCX\Incremental\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedCppWinRT()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindTestbedCppWinRTIncremental()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedCppWinRT\Incremental\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindPhasingTestbedCX()
        {
            DiffCodegen(@"RegressionProjects\Features\BindPhasingTestBedCpp\BindPhasingTestBedCpp\Generated Files");
        }

        //
        // BindTestbed backcompat RS1 tests
        //
        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedCS_RS1()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCS\obj\x86\Debug", ForbiddenCodegen.RS1);
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedVB_RS1()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedVB\obj\x86\Debug", ForbiddenCodegen.RS1);
        }

        [TestMethod]
        [Ignore]
        public void Codegen_BindtestbedCX_RS1()
        {
            DiffCodegen(@"RegressionProjects\Features\CompiledBinding\BindTestbedBackcompat\RS1\BindTestbedCX\Generated Files", ForbiddenCodegen.RS1);
        }

        //
        // DeferLoadStrategy Tests
        //

        [TestMethod]
        [Ignore]
        public void Codegen_DeferLoadStrategyCS()
        {
            DiffCodegen(@"RegressionProjects\Features\DeferLoadStrategy\CSharp\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_DeferLoadStrategyVB()
        {
            DiffCodegen(@"RegressionProjects\Features\DeferLoadStrategy\VisualBasic\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_DeferLoadStrategyCX()
        {
            DiffCodegen(@"RegressionProjects\Features\DeferLoadStrategy\VC\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ReduceProviderLoading_ConsumerCS()
        {
            DiffCodegen(@"RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ReduceProviderLoading_ConsumerVB()
        {
            DiffCodegen(@"RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCpp\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ReduceProviderLoading_ConsumerCX()
        {
            DiffCodegen(@"RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ConsumerCs\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ReduceProviderLoading_ProviderCS()
        {
            DiffCodegen(@"RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCs\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ReduceProviderLoading_ProviderVB()
        {
            DiffCodegen(@"RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCpp\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ReduceProviderLoading_ProviderCX()
        {
            DiffCodegen(@"RegressionProjects\Features\ReduceProviderLoading\ConsumerProvider\ProviderCs\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_MultipleViewsCX()
        {
            DiffCodegen(@"RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedCPP\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_MultipleViewsCS()
        {
            DiffCodegen(@"RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbed\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_MultipleViewsVB()
        {
            DiffCodegen(@"RegressionProjects\Features\MultiXamlFiles\MultipleViewsTestbedVB\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ConditionalControls()
        {
            DiffCodegen(@"RegressionProjects\Features\Conditionals\ConditionalControls\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ConditionalsModel()
        {
            DiffCodegen(@"RegressionProjects\Features\Conditionals\ConditionalsModel\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ConditionalsCS()
        {
            DiffCodegen(@"RegressionProjects\Features\Conditionals\ConditionalsCS\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ConditionalsVB()
        {
            DiffCodegen(@"RegressionProjects\Features\Conditionals\ConditionalsVB\obj\x86\Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_ConditionalsCX()
        {
            DiffCodegen(@"RegressionProjects\Features\Conditionals\ConditionalsCX\Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_MarkupExtensionsCX()
        {
            DiffCodegen(@"RegressionProjects/Features/MarkupExtensions/MarkupExtensionsCX/Generated Files");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_MarkupExtensionsCS()
        {
            DiffCodegen(@"RegressionProjects/Features/MarkupExtensions/MarkupExtensionsCS/obj/x86/Debug");
        }

        [TestMethod]
        [Ignore]
        public void Codegen_MarkupExtensionsVB()
        {
            DiffCodegen(@"RegressionProjects/Features/MarkupExtensions/MarkupExtensionsVB/obj/x86/Debug");
        }
    }
}
