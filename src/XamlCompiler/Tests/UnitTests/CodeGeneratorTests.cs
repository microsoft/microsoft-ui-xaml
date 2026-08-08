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
    }
}
