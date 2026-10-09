// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
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
        public void XamlModuleNames_AreQualifiedAndCollisionResistant()
        {
            Assert.AreEqual(
                "OpenNet.Application_Xaml",
                CppWinRTProjectionDependency.GetXamlPrimaryModuleName("OpenNet"));
            Assert.AreEqual(
                "Application_Xaml",
                CppWinRTProjectionDependency.GetXamlPrimaryModuleName(String.Empty));
            Assert.AreEqual(
                "OpenNet.UI.Pages.MainPage",
                CppWinRTProjectionDependency.GetXamlPartitionName("OpenNet::UI::Pages::MainPage"));
            Assert.AreEqual(
                "OpenNet.Application_Xaml:OpenNet.UI.Pages.MainPage",
                CppWinRTProjectionDependency.GetXamlPartitionModuleName("OpenNet", "OpenNet::UI::Pages::MainPage"));

        }

        [TestMethod]
        public void ProjectContext_PropagatesNamedModuleMode()
        {
            var context = new CodeGeneratorProjectContext(new Version(KnownVersions.Latest));
            context.UseCppWinRTNamedModules = true;

            Assert.IsTrue(context.ProjectInfo.UseCppWinRTNamedModules);
        }

        [TestMethod]
        public void ProjectContext_StoresCompleteXamlPartitionSet()
        {
            var context = new CodeGeneratorProjectContext(new Version(KnownVersions.Latest));
            context.ProjectInfo.XamlClassNames = new[]
            {
                "OpenNet.App",
                "OpenNet.UI.Pages.MainPage",
            };

            CollectionAssert.AreEqual(
                new[] { "OpenNet.App", "OpenNet.UI.Pages.MainPage" },
                new System.Collections.Generic.List<string>(context.ProjectInfo.XamlClassNames));
        }


        [TestMethod]
        public void NoTypeInfoCodeGen_RemovesOptionalXamlTypeInfoPartition()
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
