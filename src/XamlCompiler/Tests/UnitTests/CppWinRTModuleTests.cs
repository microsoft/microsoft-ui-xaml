// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class CppWinRTModuleTests
    {
        [TestMethod]
        public void ProjectionDependency_LowersNamespaceToHeaderAndModule()
        {
            const string projectionNamespace = "Microsoft.UI.Xaml.Controls";

            Assert.AreEqual(
                "winrt/Microsoft.UI.Xaml.Controls.h",
                CppWinRTProjectionDependency.GetHeaderFile(projectionNamespace));
            Assert.AreEqual(
                "winrt.Microsoft.UI.Xaml.Controls",
                CppWinRTProjectionDependency.GetModuleName(projectionNamespace));
        }

        [TestMethod]
        public void ProjectionDependency_RecursesThroughGenericArguments()
        {
            var namespaces = CppWinRTProjectionDependency.GetNamespaces(
                typeof(ProjectionDependencyFixtures.Outer.Container<
                    ProjectionDependencyFixtures.Middle.Envelope<
                        ProjectionDependencyFixtures.Inner.Payload>>));

            CollectionAssert.AreEquivalent(
                new[]
                {
                    "ProjectionDependencyFixtures.Outer",
                    "ProjectionDependencyFixtures.Middle",
                    "ProjectionDependencyFixtures.Inner",
                },
                namespaces);
        }

        [TestMethod]
        public void ProjectionDependency_UsesNamespaceFallbackForUnresolvedPass1Type()
        {
            var namespaces = CppWinRTProjectionDependency.GetNamespaces(null, "Simple");

            CollectionAssert.AreEqual(
                new[] { "Simple" },
                new System.Collections.Generic.List<string>(namespaces));

            var knownPrimitiveNamespaces = CppWinRTProjectionDependency.GetNamespaces(typeof(int), "Simple");
            Assert.AreEqual(0, new System.Collections.Generic.List<string>(knownPrimitiveNamespaces).Count);
        }

        [TestMethod]
        public void ProjectionDependency_DoesNotTreatProjectedPrimitiveAsNamespaceDependency()
        {
            var namespaces = CppWinRTProjectionDependency.GetNamespaces(
                typeof(ProjectionDependencyFixtures.Outer.Container<int>));

            CollectionAssert.AreEquivalent(
                new[] { "ProjectionDependencyFixtures.Outer" },
                namespaces);
        }

        [TestMethod]
        public void ClassModuleIdentity_PreservesReadableValidSegmentsAndEscapesKeywords()
        {
            Assert.AreEqual("OpenNet.Application_Xaml", CppWinRTProjectionDependency.GetXamlPrimaryModuleName("OpenNet"));
            Assert.AreEqual("Application_Xaml", CppWinRTProjectionDependency.GetXamlPrimaryModuleName(String.Empty));
            Assert.AreEqual("OpenNet.Application_Xaml.Views.MainPage", CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "Views::MainPage"));
            Assert.AreEqual("OpenNet.Application_Xaml.Views.MainPage", CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "OpenNet.Views.MainPage"));
            Assert.AreNotEqual(CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "Views.MainPage"), CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "Controls.MainPage"));
            Assert.AreNotEqual(CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "A_B.C"), CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "A.B_C"));
            StringAssert.StartsWith(CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "export.module"), "OpenNet.Application_Xaml.__XamlEscaped_");
            Assert.AreEqual("OpenNet.Application_Xaml.Support", CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "Support"));
            Assert.AreNotEqual(
                CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "Views.MainPage"),
                CppWinRTProjectionDependency.GetXamlClassModuleName("OpenNet", "views.MainPage"));
        }

        [TestMethod]
        public void ClassModuleIdentity_EscapesNamesReservedForIndependentModules()
        {
            const string root = "OpenNet";
            foreach (var className in new[] { "BindingInfo", "bindinginfo", "TypeInfo", "typeinfo" })
            {
                string classModule = CppWinRTProjectionDependency.GetXamlClassModuleName(root, className);
                Assert.IsFalse(String.Equals(root + ".Application_Xaml.BindingInfo", classModule, StringComparison.OrdinalIgnoreCase));
                Assert.IsFalse(String.Equals(root + ".Application_Xaml.TypeInfo", classModule, StringComparison.OrdinalIgnoreCase));
            }

            Assert.AreEqual(
                "OpenNet.Application_Xaml.Views.TypeInfo",
                CppWinRTProjectionDependency.GetXamlClassModuleName(root, "Views.TypeInfo"));
        }

        [TestMethod]
        public void ClassModuleIdentity_HandlesUnicodeNestedNamespacesAndDifferentProjectRoot()
        {
            var classes = new[] { "Views.MainPage", "Controls.MainPage", "A_B.C", "A.B_C", "export.module", "应用.视图.主页", "Other.Nested.Views.MainPage", "Other.App", "Other._0056iews.MainPage" };
            var identities = classes.Select(name => CppWinRTProjectionDependency.GetXamlClassModuleName("Project", name)).ToArray();
            Assert.AreEqual(classes.Length, identities.Distinct(StringComparer.Ordinal).Count());
            foreach (var identity in identities)
            {
                StringAssert.StartsWith(identity, "Project.Application_Xaml.");
                Assert.IsFalse(identity.Contains(".Class."));
            }
            Assert.AreEqual("应用.Application_Xaml", CppWinRTProjectionDependency.GetXamlPrimaryModuleName("应用"));
            Assert.AreEqual("Project.Application_Xaml.应用.视图.主页", CppWinRTProjectionDependency.GetXamlClassModuleName("Project", "应用.视图.主页"));
            Assert.AreEqual(CppWinRTProjectionDependency.GetXamlClassModuleName("Project", "应用.视图.主页"), CppWinRTProjectionDependency.GetXamlClassModuleName("Project", "应用::视图::主页"));
        }

        [TestMethod]
        public void RootModuleIdentity_UsesDeterministicFallbackForInvalidProjectNamespace()
        {
            foreach (var empty in new[] { null, "", " " })
            {
                Assert.AreEqual("Application_Xaml", CppWinRTProjectionDependency.GetXamlPrimaryModuleName(empty));
            }
            var invalid = new[] { "1Project", "Bad-Root", "A..B", "export.module", ".Root", "Root." };
            var identities = invalid.Select(CppWinRTProjectionDependency.GetXamlPrimaryModuleName).ToArray();
            Assert.AreEqual(invalid.Length, identities.Distinct(StringComparer.Ordinal).Count());
            for (int i = 0; i < invalid.Length; i++)
            {
                StringAssert.StartsWith(identities[i], "XamlProject.__XamlEscaped_");
                Assert.AreEqual(identities[i], CppWinRTProjectionDependency.GetXamlPrimaryModuleName(invalid[i]));
            }
        }

        [TestMethod]
        public void EarlyModuleGraph_DoesNotReadUninitializedProjectInfo()
        {
            CollectionAssert.AreEqual(new[] { false, false }, CppWinRTProjectionDependency.GetCodeGenerationDecisionsBeforeProjectInfo("Nothing"));
            CollectionAssert.AreEqual(new[] { true, false }, CppWinRTProjectionDependency.GetCodeGenerationDecisionsBeforeProjectInfo("NoPageCodeGen"));
            CollectionAssert.AreEqual(new[] { false, true }, CppWinRTProjectionDependency.GetCodeGenerationDecisionsBeforeProjectInfo("NoTypeInfoCodeGen"));
        }

        [TestMethod]
        public void DeclarationDependencies_DoNotTreatUnharvestedModelAsEmpty()
        {
            var definition = new PageDefinition(new XamlProjectInfo(), new XamlSchemaCodeInfo())
            {
                CodeInfo = new XamlClassCodeInfo("Test.NotHarvested", false)
            };
            try
            {
                var namespaces = definition.DeclarationCppWinRTProjectionNamespaces;
                Assert.Fail("Unharvested declaration dependencies must report an invalid lifecycle state.");
            }
            catch (System.Reflection.TargetInvocationException exception)
            {
                Assert.IsInstanceOfType(exception.InnerException, typeof(InvalidOperationException));
                StringAssert.Contains(exception.InnerException.ToString(), "must be harvested");
            }
        }

        [TestMethod]
        public void EmptyDeclarations_AreHarvestedBeforeDependencyCollection()
        {
            var helper = new TestHelper();
            var schema = helper.LoadSchema(SchemaMode.ManagedRuntime);
            foreach (var element in new[] { "Application", "Page", "UserControl", "ResourceDictionary" })
            {
                var context = new CodeGeneratorProjectContext(new Version(KnownVersions.Latest), "Test")
                {
                    RootNamespace = "Project", IsPass1 = true, IsApplication = element == "Application", BuildXamlModules = true
                };
                string xaml = "<" + element + " xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Class='DifferentRoot.Empty" + element + "' />";
                var files = helper.GenerateCodeBehind(context, new List<string> { xaml }, schema, CodeGenLanguage.CppWinRT);
                string module = files.Single(file => file.FileName.EndsWith(".ixx")).Contents;
                StringAssert.Contains(module, "export module " + CppWinRTProjectionDependency.GetXamlClassModuleName("Project", "DifferentRoot.Empty" + element) + ";");
                StringAssert.Contains(module, "export import winrt.Microsoft.UI.Xaml;");
                StringAssert.Contains(module, "export import winrt.Windows.Foundation;");
            }
        }

        [TestMethod]
        public void Aggregator_OnlyReExportsIndependentInterfacesFromTheFullClassSet()
        {
            string text = CppWinRTProjectionDependency.WriteAggregator("Test", new[] { "Test.SecondPage", "Test.App", "Test.MainPage", "Test.MainPage" });
            Assert.IsFalse(text.Contains("Application_Xaml.Support"));
            foreach (var name in new[] { "Test.App", "Test.MainPage", "Test.SecondPage" })
            {
                StringAssert.Contains(text, "export import " + CppWinRTProjectionDependency.GetXamlClassModuleName("Test", name) + ";");
            }
            Assert.IsFalse(text.Contains("#include"));
            Assert.IsFalse(text.Contains("namespace"));
            string removed = CppWinRTProjectionDependency.WriteAggregator("Test", new[] { "Test.App", "Test.MainPage" });
            Assert.IsFalse(removed.Contains("SecondPage"));
        }

        private static void AssertImplementationTextualHeaders(string source, params string[] expectedHeaders)
        {
            var actualHeaders = Regex.Matches(source, @"(?m)^#include <([^>]+)>\r?$")
                .Cast<Match>()
                .Select(match => match.Groups[1].Value)
                .Where(header => header != "windows.h" && header != "unknwn.h" && header != "winrt/base_macros.h")
                .ToArray();
            var orderedExpectedHeaders = expectedHeaders.OrderBy(header => header, StringComparer.Ordinal).ToArray();
            CollectionAssert.AreEqual(
                orderedExpectedHeaders,
                actualHeaders,
                "Expected textual STL headers [" + String.Join(", ", orderedExpectedHeaders) + "] but found [" + String.Join(", ", actualHeaders) + "].");

            int firstImport = source.IndexOf("import ", StringComparison.Ordinal);
            Assert.IsTrue(firstImport >= 0);
            foreach (string header in expectedHeaders)
            {
                int include = source.IndexOf("#include <" + header + ">", StringComparison.Ordinal);
                Assert.IsTrue(include >= 0 && include < firstImport, "STL header must precede module imports: " + header);
            }
            Assert.IsFalse(source.Contains("import std;"));
            Assert.IsFalse(Regex.IsMatch(source, @"(?m)^module(;|\s+[^;]+;)"));
        }

        private static List<FileNameAndContentPair> GeneratePage(bool buildModules, bool app = false, bool isPass1 = true, bool includeBinding = false)
        {
            var helper = new TestHelper();
            var context = new CodeGeneratorProjectContext(new Version(KnownVersions.Latest), "Test")
            {
                RootNamespace = "Test", IsPass1 = isPass1, IsApplication = app, BuildXamlModules = buildModules
            };
            string element = app ? "Application" : "Page";
            // Pass 2 resolves x:Bind against the x:Class type, so binding fixtures must
            // load a real user type instead of the declaration-only Test.MainPage.
            string className = includeBinding ? "LibManagedDll.BindPathParserClass" : "Test.MainPage";
            string xamlStart = "<" + element + " xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Class='" + className + "'";
            string xaml = includeBinding
                ? xamlStart + "><TextBlock Text='{x:Bind StringProperty}' /></" + element + ">"
                : xamlStart + " />";
            SchemaMode schemaMode = SchemaMode.ManagedRuntime;
            if (includeBinding)
            {
                schemaMode |= SchemaMode.LoadUserDll;
            }
            return helper.GenerateCodeBehind(context, new List<string> { xaml }, helper.LoadSchema(schemaMode), CodeGenLanguage.CppWinRT);
        }

        [TestMethod]
        public void ModuleBuild_MovesXamlDeclarationsOutOfTheLegacyHeaderContract()
        {
            var traditional = GeneratePage(false);
            var modules = GeneratePage(true);
            Assert.AreEqual(1, traditional.Count);
            Assert.AreEqual(2, modules.Count);

            string traditionalHeader = traditional.Single().Contents;
            StringAssert.Contains(traditionalHeader, "struct MainPageT");

            string sentinel = modules.Single(file => file.FileName.EndsWith(".h")).Contents;
            StringAssert.Contains(sentinel, "named-module sentinel");
            Assert.IsFalse(sentinel.Contains("struct MainPageT"));
            Assert.IsFalse(sentinel.Contains("#include <winrt/"));

            string module = modules.Single(file => file.FileName.EndsWith(".ixx")).Contents;
            StringAssert.Contains(module, "export module Test.Application_Xaml.MainPage;");
            StringAssert.Contains(module, "#define WINRT_IMPORT_MODULE");
            // TestHelper intentionally materializes DummyFile.xaml. The producer scaffold is
            // nevertheless named from x:Class (Test.MainPage), as cppwinrt emits MainPage.g.h.
            StringAssert.Contains(module, "#include \"MainPage.g.h\"");
            Assert.IsFalse(module.Contains("#include \"DummyFile.g.h\""));
            StringAssert.Contains(module, "#define XAML_IMPL_MODULE");
            StringAssert.Contains(module, "struct MainPageT");
            StringAssert.Contains(module, "export import winrt.Microsoft.UI.Xaml.Controls;");
            Assert.IsFalse(module.Contains("WINRT_XAML"));
            Assert.IsFalse(module.Contains("XAML_USE_MODULE"));
        }

        [TestMethod]
        public void App_ModuleOwnsTheGeneratedAppTemplate()
        {
            var files = GeneratePage(true, true);
            string sentinel = files.Single(file => file.FileName.EndsWith(".h")).Contents;
            Assert.IsFalse(sentinel.Contains("XamlAppMetadataProvider<D>::type"));

            string module = files.Single(file => file.FileName.EndsWith(".ixx")).Contents;
            StringAssert.Contains(module, "export import Test.Application_Xaml.TypeInfo;");
            StringAssert.Contains(module, "XamlAppMetadataProvider<D>::type");
            StringAssert.Contains(module, "winrt::make_self<XamlMetaDataProvider>()");
            Assert.IsFalse(module.Contains("AppT();"));
            Assert.IsFalse(module.Contains("~AppT();"));
        }

        [TestMethod]
        public void PageModule_ReExportsBindingInfoUsedByItsPublicTemplate()
        {
            var files = GeneratePage(true, app: false, isPass1: true, includeBinding: true);
            string module = files.Single(file => file.FileName.EndsWith(".ixx")).Contents;

            StringAssert.Contains(module, "export import Test.Application_Xaml.BindingInfo;");
        }

        [TestMethod]
        public void GeneratedImplementationUnits_UseOnlyTheirTextualStlDependencies()
        {
            var page = GeneratePage(true, isPass1: false).Single().Contents;
            AssertImplementationTextualHeaders(page, "cstdint", "memory", "type_traits", "utility");

        }

        private static void AssertGeneratedModuleFileLayout(string source, bool declarationModule)
        {
            StringAssert.StartsWith(source, "//------------------------------------------------------------------------------\r\n// <auto-generated>\r\n");
            Assert.AreEqual(1, Regex.Matches(source, "// <auto-generated>").Count, "The generated-file banner belongs at the top, once.");
            Assert.IsFalse(Regex.IsMatch(source, @"(?<!\r)\n|\r(?!\n)"), "Generated native module files must use CRLF consistently.");
            if (declarationModule)
            {
                Assert.IsFalse(source.Contains("#pragma once"), "Module interfaces are not textual include headers.");
                Assert.IsFalse(source.Contains("#ifndef XAML_IMPL_MODULE"), "Do not embed the inactive header-mode preamble.");
                int globalFragment = source.IndexOf("module;", StringComparison.Ordinal);
                int platformInclude = source.IndexOf("#include <unknwn.h>", StringComparison.Ordinal);
                int moduleDeclaration = source.IndexOf("export module ", StringComparison.Ordinal);
                int standardLibraryImport = source.IndexOf("import std;", StringComparison.Ordinal);
                int declarations = source.IndexOf("#define XAML_IMPL_MODULE", StringComparison.Ordinal);
                Assert.IsTrue(globalFragment >= 0 && platformInclude > globalFragment && moduleDeclaration > platformInclude);
                Assert.IsTrue(standardLibraryImport > moduleDeclaration && declarations > standardLibraryImport);
            }
        }

        [TestMethod]
        public void ModuleGeneratedFiles_UseOneLeadingBannerAndConsistentNativeLineEndings()
        {
            foreach (bool app in new[] { false, true })
            {
                foreach (bool pass1 in new[] { true, false })
                {
                    Console.WriteLine("Checking generated layout: application={0}, pass1={1}", app, pass1);
                    var files = GeneratePage(true, app: app, isPass1: pass1, includeBinding: !app);
                    foreach (var file in files)
                    {
                        AssertGeneratedModuleFileLayout(file.Contents, file.FileName.EndsWith(".ixx", StringComparison.Ordinal));
                    }
                    if (!app && !pass1)
                    {
                        StringAssert.Contains(files.Single().Contents, "Update_StringProperty", "Pass 2 must emit the real binding body before its layout is checked.");
                    }
                }
            }

            AssertGeneratedModuleFileLayout(CppWinRTProjectionDependency.WriteAggregator("Test", new[] { "Test.App", "Test.MainPage" }), false);
        }

        [TestMethod]
        public void TypeInfoModule_DoesNotEmbedTheTwoLegacyHeaderPreambles()
        {
            var helper = new TestHelper();
            var project = new XamlProjectInfo
            {
                RootNamespace = "Test", ProjectName = "Test", BuildXamlModules = true,
                TargetPlatformMinVersion = new Version(KnownVersions.Latest),
                ClassToHeaderFileMap = new Dictionary<string, string> { { "Test.MainPage", "MainPage.xaml.h" } }
            };
            project.SetEmptyAdditionalXamlTypeInfoIncludes();
            foreach (bool pass1 in new[] { true, false })
            {
                foreach (var file in helper.GenerateTypeInfo(pass1, new XamlSchemaCodeInfo(), project, new ClassName("Test.App"), CodeGenLanguage.CppWinRT))
                {
                    AssertGeneratedModuleFileLayout(file.Contents, file.FileName.EndsWith(".ixx", StringComparison.Ordinal));
                    if (file.FileName == "XamlTypeInfo.xaml.g.ixx")
                    {
                        StringAssert.Contains(file.Contents, "struct XamlTypeInfoProvider");
                        StringAssert.Contains(file.Contents, "struct XamlMetaDataProvider");
                        StringAssert.Contains(file.Contents, "#include \"XamlMetaDataProvider.g.h\"");
                        Assert.IsFalse(file.Contents.Contains("#include \"XamlTypeInfo.xaml.g.h\""));
                    }
                }
            }
        }

        [TestMethod]
        public void TypeInfoConsumer_SelectsHeadersOrModulesDuringGeneration()
        {
            var helper = new TestHelper();
            var project = new XamlProjectInfo
            {
                RootNamespace = "Test", ProjectName = "Test", TargetPlatformMinVersion = new Version(KnownVersions.Latest),
                ClassToHeaderFileMap = new Dictionary<string, string> { { "Test.MainPage", "MainPage.xaml.h" }, { "Test.SecondPage", "SecondPage.xaml.h" } }
            };
            project.SetEmptyAdditionalXamlTypeInfoIncludes();
            var schema = new XamlSchemaCodeInfo();
            var schemaContext = helper.LoadSchema(SchemaMode.ManagedRuntime);
            var providerType = new XamlType(
                new DirectUIXamlType(typeof(ProjectionDependencyFixtures.Providers.ExternalMetadataProvider), schemaContext).Instance);
            schema.SetOtherMetadataProviders(new[] { new TypeForCodeGen(providerType) });

            string headerText = helper.GenerateTypeInfo(false, schema, project, new ClassName("Test.App"), CodeGenLanguage.CppWinRT)
                .Single(file => file.FileName == "XamlTypeInfo.g.cpp").Contents;
            StringAssert.Contains(headerText, "#include <vector>");
            Assert.IsFalse(headerText.Contains("import winrt."));
            Assert.IsFalse(headerText.Contains("import Test.Application_Xaml;"));
            Assert.IsFalse(headerText.Contains("XAML_USE_MODULE"));

            project.BuildXamlModules = true;
            string moduleText = helper.GenerateTypeInfo(false, schema, project, new ClassName("Test.App"), CodeGenLanguage.CppWinRT)
                .Single(file => file.FileName == "XamlTypeInfo.g.cpp").Contents;
            int firstImport = moduleText.IndexOf("import ", StringComparison.Ordinal);
            int rootImport = moduleText.IndexOf("import Test.Application_Xaml;", StringComparison.Ordinal);
            Assert.IsTrue(firstImport >= 0);
            Assert.IsTrue(rootImport >= 0);
            AssertImplementationTextualHeaders(
                moduleText,
                "algorithm", "cstddef", "cstdint", "functional", "map", "memory",
                "mutex", "regex", "string", "type_traits", "utility", "vector");
            StringAssert.Contains(moduleText, "import Test.Application_Xaml.TypeInfo;");
            StringAssert.Contains(moduleText, "import Test.Application_Xaml.BindingInfo;");
            StringAssert.Contains(moduleText, "import winrt.ProjectionDependencyFixtures.Providers;");
            StringAssert.Contains(moduleText, "import winrt_base;");
            StringAssert.Contains(moduleText, "#define WINRT_IMPORT_MODULE");
            Assert.IsFalse(moduleText.Contains("Application_Xaml.Support"));
            Assert.IsFalse(moduleText.Contains("import std;"));
            Assert.IsFalse(moduleText.Contains("\nmodule;"));
            Assert.IsFalse(moduleText.Contains("module Test.Application_Xaml.TypeInfo;"));
            Assert.IsFalse(moduleText.Contains("XAML_USE_MODULE"));
            Assert.IsFalse(moduleText.Contains("export module"));
            foreach (string header in new[] { "MainPage.xaml.h", "SecondPage.xaml.h" })
            {
                int local = moduleText.IndexOf("#include \"" + header + "\"", StringComparison.Ordinal);
                Assert.IsTrue(local > firstImport);
            }
            Assert.AreNotEqual(headerText, moduleText);

            project.PrecompiledHeaderFile = "pch.h";
            foreach (bool pass1 in new[] { true, false })
            {
                foreach (var source in helper.GenerateTypeInfo(pass1, schema, project, new ClassName("Test.App"), CodeGenLanguage.CppWinRT).Where(file => file.FileName.EndsWith(".cpp", StringComparison.Ordinal)))
                {
                    Assert.IsFalse(source.Contents.Contains("pch.h"));
                }
            }
            project.BuildXamlModules = false;
            foreach (bool pass1 in new[] { true, false })
            {
                foreach (var source in helper.GenerateTypeInfo(pass1, schema, project, new ClassName("Test.App"), CodeGenLanguage.CppWinRT).Where(file => file.FileName.EndsWith(".cpp", StringComparison.Ordinal)))
                {
                    StringAssert.Contains(source.Contents, "#include \"pch.h\"");
                    Assert.IsFalse(source.Contents.Contains("XAML_USE_MODULE"));
                }
            }
        }

        [TestMethod]
        public void ProjectionDependency_UnwrapsJaggedGenericArrays()
        {
            CollectionAssert.AreEquivalent(new[] { "ProjectionDependencyFixtures.Outer", "ProjectionDependencyFixtures.Inner" }, CppWinRTProjectionDependency.GetNamespaces(typeof(ProjectionDependencyFixtures.Outer.Container<ProjectionDependencyFixtures.Inner.Payload[]>[][])));
        }

        [TestMethod]
        public void NoTypeInfoCodeGen_SuppressesTypeInfoSupportDeclarations()
        {
            var projectInfo = new XamlProjectInfo();
            Assert.IsTrue(projectInfo.ShouldGenerateTypeInfoCode);

            projectInfo.SetCodeGenFlags("NoTypeInfoCodeGen");

            Assert.IsFalse(projectInfo.ShouldGenerateTypeInfoCode);
        }

        [TestMethod]
        public void EmptyPage_StillRequiresGeneratedScaffoldingProjections()
        {
            const string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='TestApp.EmptyPage' />";

            var helper = new TestHelper();
            var schema = helper.LoadSchema(SchemaMode.ManagedRuntime);
            var domRoot = helper.LoadXamlDom(xaml, schema);
            var codeInfo = helper.HarvestClassCodeInfo(".", domRoot, true, false);
            var projectInfo = new XamlProjectInfo
            {
                ClassToHeaderFileMap = new System.Collections.Generic.Dictionary<string, string>(),
            };
            var definition = new PageDefinition(projectInfo, new XamlSchemaCodeInfo())
            {
                CodeInfo = codeInfo,
            };

            CollectionAssert.AreEquivalent(
                new[]
                {
                    "Windows.Foundation",
                    "Microsoft.UI.Xaml",
                    "Microsoft.UI.Xaml.Controls.Primitives",
                    "Microsoft.UI.Xaml.Markup",
                },
                definition.NeededCppWinRTProjectionNamespaces);
        }

        [TestMethod]
        public void PageProjectionDependency_ToleratesUnresolvedBindPathValueType()
        {
            const string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='TestApp.BindingPage'>
    <Grid>
        <Button x:Name='source' />
        <TextBlock Text='{x:Bind source.Tag}' />
    </Grid>
</Page>";

            var helper = new TestHelper();
            var schema = helper.LoadSchema(SchemaMode.ManagedRuntime);
            var domRoot = helper.LoadXamlDom(xaml, schema);
            var codeInfo = helper.HarvestClassCodeInfo(".", domRoot, true, false);
            var fileCodeInfo = helper.HarvestFileCodeInfo(".", true, codeInfo, domRoot);
            codeInfo.AddXamlFileInfo(fileCodeInfo);

            Assert.IsTrue(codeInfo.BindUniverses.Count > 0);
            codeInfo.BindUniverses[0].AddUnresolvedRootStepForTest("__unresolved");

            var projectInfo = new XamlProjectInfo
            {
                ClassToHeaderFileMap = new System.Collections.Generic.Dictionary<string, string>(),
            };
            var definition = new PageDefinition(projectInfo, new XamlSchemaCodeInfo())
            {
                CodeInfo = codeInfo,
            };

            CollectionAssert.Contains(
                definition.NeededCppWinRTProjectionNamespaces,
                "Microsoft.UI.Xaml");
        }
    }
}


namespace ProjectionDependencyFixtures.Outer
{
    internal sealed class Container<T>
    {
    }
}

namespace ProjectionDependencyFixtures.Middle
{
    internal sealed class Envelope<T>
    {
    }
}

namespace ProjectionDependencyFixtures.Inner
{
    internal sealed class Payload
    {
    }
}

namespace ProjectionDependencyFixtures.Providers
{
    internal sealed class ExternalMetadataProvider
    {
    }
}


