// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using System;
using System.Collections;
using System.IO;
using System.Linq;
using System.Reflection;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class XamlOptimizationTests
    {
        private TestHelper helper;
        private DirectUISchemaContext schema;

        [TestInitialize]
        public void Initialize()
        {
            helper = new TestHelper();
            schema = helper.LoadSchema(SchemaMode.ManagedRuntime);
        }

        private static string Page(string body, string namespaces = "")
        {
            return "<Page x:Class='OptimizationTests.Page' " +
                "xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
                "xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' " + namespaces +
                "><Grid>" + body + "</Grid></Page>";
        }

        private static string Border(string content, string attributes = "")
        {
            return "<Border><Border.Padding><Thickness" + attributes + ">" +
                content + "</Thickness></Border.Padding></Border>";
        }

        private object Analyze(string xaml)
        {
            var root = helper.LoadXamlDom(xaml, schema);
            var pipeline = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.Optimization.XamlOptimizationPipeline");
            return pipeline.GetMethod("Analyze", BindingFlags.Public | BindingFlags.Static).Invoke(
                null, new object[] { root.Instance, xaml.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None) });
        }

        private static object[] Items(object result, string property)
        {
            return ((IEnumerable)result.GetType().GetProperty(property).GetValue(result, null)).Cast<object>().ToArray();
        }

        private string Rewrite(string xaml, bool enabled, out XamlConnectionIdRewriter editor)
        {
            var root = helper.LoadXamlDom(xaml, schema);
            var validator = helper.ValidateXamlDom(root, false);
            Assert.AreEqual(0, validator.Errors.Count, string.Join("; ", validator.Errors.Select(e => e.Message)));
            var classInfo = helper.HarvestClassCodeInfo(".", root, false, false);
            var fileInfo = helper.HarvestFileCodeInfo(".", false, classInfo, root);
            editor = new XamlConnectionIdRewriter();
            if (enabled)
            {
                editor.EnableOptimizations(root);
            }
            var output = editor.Parse(xaml, classInfo, fileInfo);
            Assert.AreEqual(0, editor.Errors.Count);
            return output;
        }

        [TestMethod]
        public void LiteralFormsPreserveTextAndSourcePositions()
        {
            foreach (var literal in new[] { "0", "2.5", "1,2", "1,2,3,4", " .5, +2e0, 3., -0 " })
            {
                foreach (var newline in new[] { "\n", "\r\n" })
                {
                    var xaml = Page(Border(newline + literal + newline));
                    var baseline = Rewrite(xaml, false, out var off);
                    var actual = Rewrite(xaml, true, out var on);
                    var expected = baseline.Replace("<Thickness>", new string(' ', "<Thickness>".Length))
                        .Replace("</Thickness>", new string(' ', "</Thickness>".Length));
                    Assert.AreEqual(expected, actual, literal);
                    CollectionAssert.AreEqual(new[] { "Lowered" }, on.OptimizationReasons);
                    Assert.AreEqual(0, off.OptimizationReasons.Length);
                }
            }
        }

        [TestMethod]
        public void ResolvedNamespaceAliasesAndMultilineOpeningTagsAreSupported()
        {
            var xaml = Page(
                "<p:Border><p:Border.Padding><v:Thickness\n >1,2,3,4</v:Thickness></p:Border.Padding></p:Border>",
                "xmlns:p='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
                "xmlns:v='using:Microsoft.UI.Xaml'");
            var baseline = Rewrite(xaml, false, out _);
            var actual = Rewrite(xaml, true, out var editor);
            Assert.AreEqual(baseline.Length, actual.Length);
            CollectionAssert.AreEqual(new[] { "Lowered" }, editor.OptimizationReasons);
            Assert.IsFalse(actual.Contains("v:Thickness"));
            Assert.IsTrue(actual.Contains("<p:Border.Padding>"));
        }

        [TestMethod]
        public void NativeSchemaResolvesTheSameBuiltInPattern()
        {
            schema = helper.LoadSchema(SchemaMode.NativeRuntime);
            var source = Page(Border("1,2,3,4"));
            var baseline = Rewrite(source, false, out _);
            var actual = Rewrite(source, true, out var editor);
            Assert.AreEqual(baseline.Replace("<Thickness>", new string(' ', 11))
                .Replace("</Thickness>", new string(' ', 12)), actual);
            CollectionAssert.AreEqual(new[] { "Lowered" }, editor.OptimizationReasons);
        }

        [TestMethod]
        public void BuiltInThicknessMayRequireProjectionCodegen()
        {
            var root = helper.LoadXamlDom(
                "<Thickness xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>1,2</Thickness>", schema);
            var type = root.Instance.GetType().GetProperty("Type").GetValue(root.Instance, null);
            var codegen = type.GetType().GetField("isCodeGenType", BindingFlags.Instance | BindingFlags.NonPublic);
            var previous = codegen.GetValue(type);
            try
            {
                // Model SDK projections without a WinUIContract attribute on Thickness.
                codegen.SetValue(type, true);
                var source = Page(Border("1,2"));
                var baseline = Rewrite(source, false, out _);
                var actual = Rewrite(source, true, out var editor);
                Assert.AreEqual(baseline.Replace("<Thickness>", new string(' ', 11))
                    .Replace("</Thickness>", new string(' ', 12)), actual);
                CollectionAssert.AreEqual(new[] { "Lowered" }, editor.OptimizationReasons);
            }
            finally
            {
                codegen.SetValue(type, previous);
            }
        }

        [TestMethod]
        public void AmbiguousLegacySourceSpansRemainUnchanged()
        {
            foreach (var body in new[] {
                "<Button/>" + Border("1,2"),
                Border("1,2").Replace("</Thickness>", "</Thickness\n >") })
            {
                var baseline = Rewrite(Page(body), false, out _);
                var actual = Rewrite(Page(body), true, out var editor);
                Assert.AreEqual(baseline, actual);
                CollectionAssert.AreEqual(new[] { "UnsupportedSourceShape" }, editor.OptimizationReasons);
            }
        }

        [TestMethod]
        public void OptimizationPrecedesAdjacentConnectionIdAndEventEdits()
        {
            var xaml = Page(Border("1,2,3,4") + "<Button x:Name='button' Click='OnClick'/>");
            var baseline = Rewrite(xaml, false, out _);
            var actual = Rewrite(xaml, true, out var editor);
            Assert.IsTrue(baseline.Contains("x:ConnectionId="));
            Assert.AreEqual(baseline.Replace("<Thickness>", new string(' ', 11))
                .Replace("</Thickness>", new string(' ', 12)), actual);
            CollectionAssert.AreEqual(new[] { "Lowered" }, editor.OptimizationReasons);
        }

        [TestMethod]
        public void UnsupportedNumbersRemainUnchanged()
        {
            foreach (var literal in new[] { "-1", "NaN", "Infinity", "Auto", "1e999", "1,2,3", "1;2", "", " " })
            {
                var result = Analyze(Page(Border(literal)));
                Assert.AreEqual(0, Items(result, "Edits").Length, literal);
                Assert.AreEqual(1, Items(result, "Decisions").Length, literal);
            }
        }

        [TestMethod]
        public void UnmodeledXmlContentRemainsUnchanged()
        {
            foreach (var literal in new[] { "1<!--comment-->,2", "<![CDATA[1,2]]>", "&#49;,2", "<?probe value?>1,2" })
            {
                var result = Analyze(Page(Border(literal)));
                Assert.AreEqual(0, Items(result, "Edits").Length, literal);
                Assert.AreEqual(1, Items(result, "Decisions").Length, literal);
            }
        }

        [TestMethod]
        public void DecoratedThicknessIsNotLowered()
        {
            foreach (var attribute in new[] {
                " x:Name='padding'", " x:Uid='padding'", " xml:space='preserve'",
                " xmlns:v='using:Microsoft.UI.Xaml'", " x:Key='padding'" })
            {
                Assert.AreEqual(0, Items(Analyze(Page(Border("1,2", attribute))), "Edits").Length, attribute);
            }
        }

        [TestMethod]
        public void OtherPropertiesAndUnknownTypesAreNotLowered()
        {
            foreach (var body in new[] {
                "<Border><Border.Margin><Thickness>1,2</Thickness></Border.Margin></Border>",
                "<StackPanel><StackPanel.Padding><Thickness>1,2</Thickness></StackPanel.Padding></StackPanel>",
                "<Border><Border.Padding><local:Thickness>1,2</local:Thickness></Border.Padding></Border>",
                "<local:Border><local:Border.Padding><Thickness>1,2</Thickness></local:Border.Padding></local:Border>" })
            {
                Assert.AreEqual(0, Items(Analyze(Page(body, "xmlns:local='using:Unmodeled'")), "Edits").Length, body);
            }
        }

        [TestMethod]
        public void IdentityDynamicAndDeferredScopesAreNotLowered()
        {
            foreach (var body in new[] {
                Border("1,2").Replace("<Border>", "<Border x:Name='border'>"),
                Border("1,2").Replace("<Border>", "<Border x:Uid='border'>"),
                Border("1,2").Replace("<Border>", "<Border Loaded='OnLoaded'>"),
                Border("1,2").Replace("<Border>", "<Border x:Load='False'>"),
                Border("1,2").Replace("<Border>", "<Border Grid.Row='1'>"),
                Border("1,2").Replace("<Border>", "<Border local:Behavior.Enabled='True'>"),
                Border("1,2").Replace("<Border>", "<Border Background='{ThemeResource Brush}'>"),
                Border("1,2").Replace("<Border>", "<Border Width='{Binding Width}'>"),
                "<ContentControl><ContentControl.ContentTemplate><DataTemplate>" + Border("1,2") +
                    "</DataTemplate></ContentControl.ContentTemplate></ContentControl>",
                "<Grid><Grid.Resources>" + Border("1,2").Replace("<Border>", "<Border x:Key='border'>") +
                    "</Grid.Resources></Grid>",
                "<Grid xml:space='preserve'>" + Border("1,2") + "</Grid>" })
            {
                Assert.AreEqual(0, Items(Analyze(Page(body, "xmlns:local='using:Unmodeled'")), "Edits").Length, body);
            }
        }

        [TestMethod]
        public void ConditionalObjectsAndPropertiesAreNotLowered()
        {
            var namespaces = "xmlns:conditional='http://schemas.microsoft.com/winfx/2006/xaml/presentation?" +
                "IsTypePresent(Microsoft.UI.Xaml.Controls.Border)'";
            foreach (var body in new[] {
                Border("1,2").Replace("Thickness", "conditional:Thickness"),
                Border("1,2").Replace("Border.Padding", "conditional:Border.Padding"),
                "<conditional:Grid>" + Border("1,2") + "</conditional:Grid>" })
            {
                Assert.AreEqual(0, Items(Analyze(Page(body, namespaces)), "Edits").Length, body);
            }
        }

        [TestMethod]
        public void LoweringIsDeterministicAndIdempotent()
        {
            var source = Page(Border("1,2") + Border("3,4"));
            var first = Rewrite(source, true, out var editor);
            Assert.AreEqual(first, Rewrite(source, true, out _));
            CollectionAssert.AreEqual(new[] { "Lowered", "Lowered" }, editor.OptimizationReasons);
            Assert.AreEqual(0, Items(Analyze(first), "Edits").Length);
        }

        [TestMethod]
        public void GateIsOffByDefaultAndOnlyEnabledInNormalPassTwo()
        {
            var compilerType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CompileXamlInternal");
            var compiler = compilerType.CreateInstance();
            var gate = compilerType.GetProperty("ShouldOptimizeXaml", true);
            var flags = compilerType.GetProperty("FeatureControlFlags");
            Assert.AreEqual(false, gate.GetValue(compiler, null));
            flags.SetValue(compiler, Enum.Parse(flags.PropertyType, "EnableXamlCompilerOptimizations"), null);
            Assert.AreEqual(true, gate.GetValue(compiler, null));
            compilerType.GetProperty("IsPass1").SetValue(compiler, true, null);
            Assert.AreEqual(false, gate.GetValue(compiler, null));
            compilerType.GetProperty("IsPass1").SetValue(compiler, false, null);
            compilerType.GetProperty("IsDesignTimeBuild").SetValue(compiler, true, null);
            Assert.AreEqual(false, gate.GetValue(compiler, null));
        }

        [TestMethod]
        public void EnablingAndDisablingInvalidateTheExistingSavedStateFingerprint()
        {
            var compilerType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CompileXamlInternal");
            var compiler = compilerType.CreateInstance();
            var state = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.SavedStateManager").CreateInstance();
            compilerType.GetProperty("SaveState", true).SetValue(compiler, state, null);
            compilerType.GetProperty("IsPass1").SetValue(compiler, true, null);
            var flags = compilerType.GetProperty("FeatureControlFlags");
            var changed = compilerType.GetMethod("DidFeatureControlFlagsChange", BindingFlags.Instance | BindingFlags.NonPublic);
            foreach (var value in new[] { "Nothing", "EnableXamlCompilerOptimizations", "Nothing" })
            {
                flags.SetValue(compiler, Enum.Parse(flags.PropertyType, value), null);
                Assert.AreEqual(true, changed.Invoke(compiler, null), value);
                Assert.AreEqual(false, changed.Invoke(compiler, null), value);
            }
        }

        [TestMethod]
        public void OptimizationSelectionSurvivesPassOneAndSavedStateReload()
        {
            var compilerType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.CompileXamlInternal");
            var compiler = compilerType.CreateInstance();
            var stateType = new ProxyHelper("Microsoft.UI.Xaml.Markup.Compiler.SavedStateManager");
            var load = stateType.GetMethod("Load", BindingFlags.Public | BindingFlags.Static);
            var save = stateType.GetMethod("Save");
            var flags = compilerType.GetProperty("FeatureControlFlags");
            var pass1 = compilerType.GetProperty("IsPass1");
            var designTime = compilerType.GetProperty("IsDesignTimeBuild");
            var stateProperty = compilerType.GetProperty("SaveState", true);
            var changed = compilerType.GetMethod("DidFeatureControlFlagsChange", BindingFlags.Instance | BindingFlags.NonPublic);
            var record = compilerType.GetMethod("RecordXamlOptimizationState", BindingFlags.Instance | BindingFlags.NonPublic);
            var path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N") + ".xml");
            var state = load.Invoke(null, new object[] { path });
            stateProperty.SetValue(compiler, state, null);
            try
            {
                foreach (var value in new[] { "EnableXamlCompilerOptimizations", "Nothing" })
                {
                    flags.SetValue(compiler, Enum.Parse(flags.PropertyType, value), null);
                    pass1.SetValue(compiler, true, null);
                    Assert.AreEqual(true, changed.Invoke(compiler, null));
                    record.Invoke(compiler, null);
                    save.Invoke(state, null);
                    state = load.Invoke(null, new object[] { path });
                    stateProperty.SetValue(compiler, state, null);
                    Assert.AreEqual(false, changed.Invoke(compiler, null));

                    pass1.SetValue(compiler, false, null);
                    designTime.SetValue(compiler, true, null);
                    record.Invoke(compiler, null);
                    designTime.SetValue(compiler, false, null);
                    Assert.AreEqual(true, changed.Invoke(compiler, null));
                    Assert.AreEqual(true, changed.Invoke(compiler, null));
                    record.Invoke(compiler, null);
                    save.Invoke(state, null);
                    state = load.Invoke(null, new object[] { path });
                    stateProperty.SetValue(compiler, state, null);
                    Assert.AreEqual(false, changed.Invoke(compiler, null));
                }
                Assert.IsFalse(File.ReadAllText(path).Contains("XamlCompilerOptimizationsEnabledAtLastPass2"));
            }
            finally
            {
                File.Delete(path);
            }
        }

        [TestMethod]
        public void ApplicationEditorDoesNotRunOptimizationRules()
        {
            var xaml = "<Application x:Class='OptimizationTests.App' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
                "xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'><Application.Resources>" +
                Border("1,2").Replace("<Border>", "<Border x:Key='border'>") +
                "</Application.Resources></Application>";
            var root = helper.LoadXamlDom(xaml, schema);
            var classInfo = helper.HarvestClassCodeInfo(".", root, false, true);
            var fileInfo = helper.HarvestFileCodeInfo(".", false, classInfo, root);
            var editor = new XamlConnectionIdRewriter();
            editor.EnableOptimizations(root);
            var output = editor.Parse(xaml, classInfo, fileInfo);
            Assert.IsTrue(output.Contains("<Thickness>1,2</Thickness>"));
            Assert.AreEqual(0, editor.OptimizationReasons.Length);
        }
    }
}
