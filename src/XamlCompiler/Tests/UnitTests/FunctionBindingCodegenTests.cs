// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    /// <summary>
    /// Codegen tests for x:Bind function bindings. They assert on the generated Update_ and
    /// Invoke_ methods, which are emitted the same way for every binding mode, so the bindings
    /// below do not need to opt into change notification to cover the one way scenarios.
    /// </summary>
    [TestClass]
    public class FunctionBindingCodegenTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void TestInitialize()
        {
            _testHelper = new TestHelper();
        }

        private string GenerateBindings(string xaml, CodeGenLanguage language)
        {
            string page = String.Format(
                @"<Page x:Class='LibManagedDll.BindPathParserClass'
                xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>{0}</Page>",
                xaml);

            var context = new CodeGeneratorProjectContext(new Version(KnownVersions.Latest), "FunctionBindingCodegen");
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime | SchemaMode.LoadUserDll);
            List<FileNameAndContentPair> pairs = _testHelper.GenerateCodeBehind(
                context, new List<string> { page }, schema, language);

            Assert.AreEqual(1, pairs.Count);
            return pairs[0].Contents;
        }

        private static void AssertGenerated(string code, string pattern)
        {
            // Path steps are named after the path they walk plus a hash of the function arguments,
            // so match the shape of the generated code rather than the exact code names.
            Assert.IsTrue(Regex.IsMatch(code, pattern),
                String.Format("Generated code does not match '{0}':{1}{2}", pattern, Environment.NewLine, code));
        }

        [TestMethod]
        public void FunctionBinding_UpdatesWhenArgumentIsNull()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind FormatTitle(StringProperty)}'/>", CodeGenLanguage.CSharp);

            // The function binding depends on StringProperty. A null value is a legitimate
            // argument rather than an unresolved path, so the update must not be skipped for it.
            AssertGenerated(code,
                @"private void Update_StringProperty\(global::System\.String obj, int phase\)\s*" +
                @"\{\s*this\.Update_M_FormatTitle_\d+\(phase\);\s*\}");
        }

        [TestMethod]
        public void FunctionBinding_UpdatesWhenArgumentIsNull_VisualBasic()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind FormatTitle(StringProperty)}'/>", CodeGenLanguage.VisualBasic);

            AssertGenerated(code,
                @"Private Sub Update_StringProperty\(obj As Global\.System\.String, phase As Integer\)\s*" +
                @"Me\.Update_M_FormatTitle_\d+\(phase\)\s*End Sub");
        }

        [TestMethod]
        public void FunctionBinding_KeepsNullCheckOnPathSteps()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind InnerClass.StringFunction()}'/>", CodeGenLanguage.CSharp);

            // Walking on to a child step still has to be skipped when the step itself is null.
            AssertGenerated(code,
                @"private void Update_\(global::LibManagedDll\.BindPathParserClass obj, int phase\)\s*" +
                @"\{\s*if \(obj != null\)");
        }

        [TestMethod]
        public void FunctionBinding_RetrievesInstanceWhenArgumentIsOutsideItsPath()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind InnerClass.Format(StringProperty)}'/>", CodeGenLanguage.CSharp);

            // StringProperty can schedule the binding while InnerClass is null, so the instance
            // has to be retrieved safely and the call made on it rather than on the path.
            AssertGenerated(code,
                @"private void Invoke_InnerClass_M_Format_\d+\(int phase\)\s*\{\s*" +
                @"global::System\.String p0;\s*" +
                @"if \(!TryGet_StringProperty\(out p0\)\) \{ return; \}\s*" +
                @"global::LibManagedDll\.AnotherClassForPathing instance;\s*" +
                @"if \(!TryGet_InnerClass\(out instance\) \|\| instance == null\) \{ return; \}\s*" +
                @"global::System\.String result = instance\.Format\(p0\);");
        }

        [TestMethod]
        public void FunctionBinding_DoesNotRetrieveInstanceWhenArgumentIsUnderIt()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind FormatTitle(StringProperty)}'/>", CodeGenLanguage.CSharp);

            // StringProperty sits under the data root the function is invoked on, so it can only
            // schedule the binding while that root is non null and no retrieval is needed.
            AssertGenerated(code,
                @"private void Invoke_M_FormatTitle_\d+\(int phase\)\s*\{\s*" +
                @"global::System\.String p0;\s*" +
                @"if \(!TryGet_StringProperty\(out p0\)\) \{ return; \}\s*" +
                @"global::System\.String result = this\.dataRoot\.FormatTitle\(p0\);");
        }
    }
}
