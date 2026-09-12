// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
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

            List<string> pass1PublicSurface = ExtractXamlMetadataProviderPublicSurface(pass1[0].Contents);
            List<string> pass2PublicSurface = ExtractXamlMetadataProviderPublicSurface(pass2[0].Contents);
            Assert.IsNotNull(pass1PublicSurface, "Pass 1 did not generate XamlMetaDataProvider.");
            Assert.IsNotNull(pass2PublicSurface, "Pass 2 did not generate XamlMetaDataProvider.");
            CollectionAssert.AreEqual(
                pass2PublicSurface,
                pass1PublicSurface,
                "Pass 1 and pass 2 generated different XamlMetaDataProvider public APIs.");

            Assert.AreEqual(
                3,
                CountOccurrences(pass1[0].Contents, "throw new global::System.NotImplementedException();"));
            Assert.IsTrue(pass1[0].Contents.Contains("#pragma warning disable 3002, 3021"));
            Assert.IsFalse(pass1[0].Contents.Contains("internal partial class XamlTypeInfoProvider"));
            Assert.IsTrue(pass2[0].Contents.Contains("internal partial class XamlTypeInfoProvider"));
        }

        [TestMethod]
        public void CodeGenerator_CSharpTypeInfoPass1AndPass2OmitProviderWithoutLocalTypes()
        {
            CodeGeneratorProjectContext context = new CodeGeneratorProjectContext(
                new Version(KnownVersions.Latest),
                "CSharpTypeInfoNoLocalTypes");
            XamlSchemaCodeInfo schemaInfo = new XamlSchemaCodeInfo();

            List<FileNameAndContentPair> pass1 = _testHelper.GenerateTypeInfo(
                true,
                schemaInfo,
                context.ProjectInfo,
                new ClassName("CSharpTypeInfoNoLocalTypes.App"),
                CodeGenLanguage.CSharp);
            List<FileNameAndContentPair> pass2 = _testHelper.GenerateTypeInfo(
                false,
                schemaInfo,
                context.ProjectInfo,
                new ClassName("CSharpTypeInfoNoLocalTypes.App"),
                CodeGenLanguage.CSharp);

            Assert.AreEqual(1, pass1.Count);
            Assert.AreEqual("XamlTypeInfo.g.cs", pass1[0].FileName);
            Assert.AreEqual(1, pass2.Count);
            Assert.IsTrue(pass1[0].Contents.Contains("XamlMetaDataProvider API stub for managed pass 1."));
            Assert.IsTrue(pass1[0].Contents.Contains("// No local types."));
            Assert.IsNull(
                ExtractXamlMetadataProviderPublicSurface(pass1[0].Contents),
                "Pass 1 generated XamlMetaDataProvider without local types.");
            Assert.IsNull(
                ExtractXamlMetadataProviderPublicSurface(pass2[0].Contents),
                "Pass 2 generated XamlMetaDataProvider without local types.");
        }

        private static List<string> ExtractXamlMetadataProviderPublicSurface(string source)
        {
            SyntaxTree tree = CSharpSyntaxTree.ParseText(source);
            Diagnostic[] parseErrors = tree.GetDiagnostics()
                .Where(diagnostic => diagnostic.Severity == DiagnosticSeverity.Error)
                .ToArray();
            Assert.AreEqual(
                0,
                parseErrors.Length,
                "Generated C# did not parse:" + Environment.NewLine +
                    string.Join(Environment.NewLine, parseErrors.Select(error => error.ToString())));

            List<ClassDeclarationSyntax> providers = tree.GetRoot()
                .DescendantNodes()
                .OfType<ClassDeclarationSyntax>()
                .Where(type => type.Identifier.ValueText == "XamlMetaDataProvider")
                .ToList();
            Assert.IsTrue(
                providers.Count <= 1,
                "Generated C# contains more than one XamlMetaDataProvider declaration.");
            if (providers.Count == 0)
            {
                return null;
            }

            ClassDeclarationSyntax provider = providers[0];
            NamespaceDeclarationSyntax containingNamespace = provider.Ancestors()
                .OfType<NamespaceDeclarationSyntax>()
                .FirstOrDefault();
            Assert.IsNotNull(containingNamespace, "XamlMetaDataProvider is not inside a namespace.");

            var surface = new List<string>
            {
                "namespace " + NormalizeSyntax(containingNamespace.Name),
                "type " + NormalizeSyntax(
                    provider.WithMembers(default(SyntaxList<MemberDeclarationSyntax>))),
            };

            bool hasExplicitInstanceConstructor = provider.Members
                .OfType<ConstructorDeclarationSyntax>()
                .Any(constructor => !constructor.Modifiers.Any(
                    modifier => modifier.IsKind(SyntaxKind.StaticKeyword)));
            if (!hasExplicitInstanceConstructor)
            {
                surface.Add("member public " + provider.Identifier.Text + "()");
            }

            foreach (MemberDeclarationSyntax member in provider.Members.Where(IsPublicMember))
            {
                surface.Add("member " + GetPublicMemberDeclaration(member));
            }

            surface.Sort(StringComparer.Ordinal);
            return surface;
        }

        private static bool IsPublicMember(MemberDeclarationSyntax member)
        {
            return member.ChildTokens().Any(token => token.IsKind(SyntaxKind.PublicKeyword));
        }

        private static string GetPublicMemberDeclaration(MemberDeclarationSyntax member)
        {
            MethodDeclarationSyntax method = member as MethodDeclarationSyntax;
            if (method != null)
            {
                return NormalizeSyntax(method
                    .WithBody(null)
                    .WithExpressionBody(null)
                    .WithSemicolonToken(SyntaxFactory.Token(SyntaxKind.SemicolonToken)));
            }

            ConstructorDeclarationSyntax constructor = member as ConstructorDeclarationSyntax;
            if (constructor != null)
            {
                return NormalizeSyntax(constructor
                    .WithBody(null)
                    .WithExpressionBody(null)
                    .WithSemicolonToken(SyntaxFactory.Token(SyntaxKind.SemicolonToken)));
            }

            Assert.Fail(
                "Unsupported public XamlMetaDataProvider member. Update the API extractor for: " +
                    member.Kind());
            return null;
        }

        private static string NormalizeSyntax(SyntaxNode node)
        {
            return node.WithoutTrivia().NormalizeWhitespace().ToFullString();
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
