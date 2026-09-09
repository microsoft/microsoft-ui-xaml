// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class CodeGeneratorTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
        public void CodeGenerator_NormalUsage()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyNamespace.MyClass'>
    <Grid>
        <Button x:Name='btn' Click='ClickHandler' />
        <Button x:Name='btn2' Click='ClickHandler' Loaded='LoadedHandler' />
        <Button x:Name='btn3' x:FieldModifier='public' />
    </Grid>
</Page>";

            List<string> xamlStrings = new List<string>();
            xamlStrings.Add(xaml);

            CodeGeneratorProjectContext context = new CodeGeneratorProjectContext(new Version(KnownVersions.Latest));
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);

            context.IsPass1 = true;

            for (CodeGenLanguage lang = CodeGenLanguage.CSharp; lang <= CodeGenLanguage.VisualBasic; lang++)
            {
                List<FileNameAndContentPair> pairs = _testHelper.GenerateCodeBehind(context, xamlStrings, schema, lang);
                Assert.AreEqual(pairs.Count, 1);
            }

            context.IsPass1 = false;

            for (CodeGenLanguage lang = CodeGenLanguage.CSharp; lang <= CodeGenLanguage.VisualBasic; lang++)
            {
                List<FileNameAndContentPair> pairs = _testHelper.GenerateCodeBehind(context, xamlStrings, schema, lang);
                Assert.AreEqual(pairs.Count, 1);
            }
        }

        [TestMethod]
        public void CodeGenerator_MultipleViewUsage()
        {
            string xaml1 = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyNamespace.MyClass'>
    <Grid>
        <Button x:Name='btn' Click='ClickHandler' />
        <Button x:Name='btn2' Click='ClickHandler' Loaded='LoadedHandler' />
        <Button x:Name='btn3' x:FieldModifier='public' />
    </Grid>
</Page>";
            string xaml2 = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyNamespace.MyClass'>
    <Grid>
        <Button x:Name='btn' Click='ClickHandler' />
        <CheckBox x:Name='btn2' Checked='CheckedHandler' Loaded='LoadedHandler' />
        <Button x:Name='btn4' x:FieldModifier='public' />
    </Grid>
</Page>";

            List<string> xamlStrings = new List<string>();
            xamlStrings.Add(xaml1);
            xamlStrings.Add(xaml2);

            CodeGeneratorProjectContext context = new CodeGeneratorProjectContext(new Version(KnownVersions.Latest));
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);

            context.IsPass1 = true;

            for (CodeGenLanguage lang = CodeGenLanguage.CSharp; lang <= CodeGenLanguage.VisualBasic; lang++)
            {
                List<FileNameAndContentPair> pairs = _testHelper.GenerateCodeBehind(context, xamlStrings, schema, lang);
                Assert.AreEqual(pairs.Count, 1);
                Assert.IsTrue(pairs[0].Contents.Contains("ButtonBase"));
            }

            context.IsPass1 = false;

            for (CodeGenLanguage lang = CodeGenLanguage.CSharp; lang <= CodeGenLanguage.VisualBasic; lang++)
            {
                List<FileNameAndContentPair> pairs = _testHelper.GenerateCodeBehind(context, xamlStrings, schema, lang);
                Assert.AreEqual(pairs.Count, 1);
                Assert.IsFalse(pairs[0].Contents.Contains("ButtonBase"));
            }
        }

        [TestMethod]
        public void CodeGenerator_CSharpTypeInfoPass1StubMatchesPass2PublicApi()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <dll:SimpleClass x:Key='key'/>
    </Page.Resources>
</Page>";

            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.LoadUserDll);
            TypeInfoCollector collector = _testHelper.CollectTypes(xaml, schema);
            CodeGeneratorProjectContext context = new CodeGeneratorProjectContext(
                new Version(KnownVersions.Latest),
                "CSharpTypeInfoPass1Stub");
            context.IsLibrary = true;
            context.RootNamespace = "StubRoot";
            context.ProjectInfo.SetCodeGenFlags("FullXamlMetadataProvider");

            List<FileNameAndContentPair> pass1 = _testHelper.GenerateTypeInfo(
                true,
                collector.SchemaInfo,
                context.ProjectInfo,
                new ClassName("StubRoot.App"),
                CodeGenLanguage.CSharp);
            List<FileNameAndContentPair> pass2 = _testHelper.GenerateTypeInfo(
                false,
                collector.SchemaInfo,
                context.ProjectInfo,
                new ClassName("StubRoot.App"),
                CodeGenLanguage.CSharp);

            Assert.AreEqual(1, pass1.Count);
            Assert.AreEqual("XamlTypeInfo.g.cs", pass1[0].FileName);
            Assert.AreEqual(1, pass2.Count);

            string[] sharedPublicApi =
            {
                "namespace StubRoot.CSharpTypeInfoPass1Stub_XamlTypeInfo",
                "[global::Microsoft.UI.Xaml.Markup.FullXamlMetadataProvider()]",
                "public sealed partial class XamlMetaDataProvider : global::Microsoft.UI.Xaml.Markup.IXamlMetadataProvider",
                "[global::Windows.Foundation.Metadata.DefaultOverload]",
                "public global::Microsoft.UI.Xaml.Markup.IXamlType GetXamlType(global::System.Type type)",
                "public global::Microsoft.UI.Xaml.Markup.IXamlType GetXamlType(string fullName)",
                "public global::Microsoft.UI.Xaml.Markup.XmlnsDefinition[] GetXmlnsDefinitions()",
            };

            foreach (string api in sharedPublicApi)
            {
                Assert.IsTrue(pass1[0].Contents.Contains(api), "Pass 1 is missing: " + api);
                Assert.IsTrue(pass2[0].Contents.Contains(api), "Pass 2 is missing: " + api);
            }

            Assert.AreEqual(
                3,
                CountOccurrences(pass1[0].Contents, "throw new global::System.NotImplementedException();"));
            Assert.IsTrue(pass1[0].Contents.Contains("#pragma warning disable 3002, 3021"));
            Assert.IsFalse(pass1[0].Contents.Contains("internal partial class XamlTypeInfoProvider"));
            Assert.IsTrue(pass2[0].Contents.Contains("internal partial class XamlTypeInfoProvider"));
        }

        [TestMethod]
        public void CodeGenerator_CSharpTypeInfoPass1PreservesNoLocalTypes()
        {
            CodeGeneratorProjectContext context = new CodeGeneratorProjectContext(
                new Version(KnownVersions.Latest),
                "CSharpTypeInfoNoLocalTypes");

            List<FileNameAndContentPair> pass1 = _testHelper.GenerateTypeInfo(
                true,
                new XamlSchemaCodeInfo(),
                context.ProjectInfo,
                new ClassName("CSharpTypeInfoNoLocalTypes.App"),
                CodeGenLanguage.CSharp);

            Assert.AreEqual(1, pass1.Count);
            Assert.AreEqual("XamlTypeInfo.g.cs", pass1[0].FileName);
            Assert.IsTrue(pass1[0].Contents.Contains("XamlMetaDataProvider API stub for managed pass 1."));
            Assert.IsTrue(pass1[0].Contents.Contains("// No local types."));
            Assert.IsFalse(pass1[0].Contents.Contains("public sealed partial class XamlMetaDataProvider"));
        }

        private static int CountOccurrences(string text, string value)
        {
            int count = 0;
            int index = 0;
            while ((index = text.IndexOf(value, index, StringComparison.Ordinal)) >= 0)
            {
                count++;
                index += value.Length;
            }
            return count;
        }
    }
}
