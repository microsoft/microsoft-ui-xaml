// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    /// <summary>
    /// Codegen and generated-binding execution tests for x:Bind function bindings,
    /// including null template data and preservation of instance-dependent behavior.
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
                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                xmlns:dll='using:LibManagedDll'>{0}</Page>",
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

        private TemplateBindings CreateTemplateBindings(
            string dataType,
            string itemBinding = null,
            string staticBinding = "dll:BindPathParserClass.GetTipOfTheDayStatic()",
            int phase = 0)
        {
            string code = GenerateBindings(
                "<ContentControl><ContentControl.ContentTemplate><DataTemplate " + dataType + ">" +
                "<TextBlock " + (phase == 0 ? "" : "x:Phase='" + phase + "' ") +
                (staticBinding == null ? "" : "Text='{x:Bind " + staticBinding + "}' ") +
                (itemBinding == null ? "" : "Tag='{x:Bind " + itemBinding + "}'") +
                "/></DataTemplate></ContentControl.ContentTemplate></ContentControl>",
                CodeGenLanguage.CSharp);

            // Isolate the emitted connector and setters so the tests execute compiler output
            // without having to host the surrounding page or the real dependency-property system.
            var classes = CSharpSyntaxTree.ParseText(code).GetRoot().DescendantNodes()
                .OfType<ClassDeclarationSyntax>().ToList();
            var bindings = classes.Single(type => type.Members.OfType<MethodDeclarationSyntax>()
                .Any(method => method.Identifier.ValueText == "DataContextChangedHandler"));
            var setters = classes.Single(type => type.Identifier.ValueText == "XamlBindingSetters");
            var connection = bindings.Members.OfType<MethodDeclarationSyntax>()
                .Single(method => method.Identifier.ValueText == "Connect")
                .DescendantNodes().OfType<CaseSwitchLabelSyntax>().Single();
            int connectionId = (int)((LiteralExpressionSyntax)connection.Value).Token.Value;

            // Execute the emitted binding class, including its setters and function dispatch.
            // Only the framework surface is substituted; no binding methods are rewritten.
            string source = @"
namespace WinRT
{
    public static class CastExtensions
    {
        public static T As<T>(object value) { return (T)value; }
    }
}
namespace Microsoft.UI.Xaml
{
    public class DependencyProperty { }
    public delegate void DependencyPropertyChangedCallback(DependencyObject sender, DependencyProperty property);
    public class DependencyObject
    {
        private readonly System.Collections.Generic.Dictionary<long, DependencyPropertyChangedCallback> callbacks =
            new System.Collections.Generic.Dictionary<long, DependencyPropertyChangedCallback>();
        private long nextToken;
        public long RegisterPropertyChangedCallback(DependencyProperty property, DependencyPropertyChangedCallback callback)
        {
            callbacks.Add(++nextToken, callback);
            return nextToken;
        }
        public void UnregisterPropertyChangedCallback(DependencyProperty property, long token)
        {
            callbacks.Remove(token);
        }
        protected void NotifyPropertyChanged(DependencyProperty property)
        {
            foreach (var callback in new System.Collections.Generic.List<DependencyPropertyChangedCallback>(callbacks.Values))
            {
                callback(this, property);
            }
        }
    }
    public class FrameworkElement : DependencyObject
    {
        public static DependencyProperty TagProperty { get; } = new DependencyProperty();
        private object tag;
        public object Tag
        {
            get { return tag; }
            set
            {
                if (!object.Equals(tag, value))
                {
                    tag = value;
                    NotifyPropertyChanged(TagProperty);
                }
            }
        }
        public event System.Action<FrameworkElement, DataContextChangedEventArgs> DataContextChanged;
    }
    public class DataTemplate { }
    public class DataContextChangedEventArgs
    {
        public object NewValue { get; set; }
    }
}
namespace Microsoft.UI.Xaml.Controls
{
    public class TextBlock : Microsoft.UI.Xaml.FrameworkElement
    {
        public string Text { get; set; } = string.Empty;
    }
    public class ContainerContentChangingEventArgs
    {
        public object Item { get; set; }
        public int ItemIndex { get; set; }
        public uint Phase { get; set; }
    }
}
namespace Microsoft.UI.Xaml.Markup
{
    public interface IComponentConnector { }
    public static class XamlBindingHelper
    {
        public static void SuspendRendering(Microsoft.UI.Xaml.FrameworkElement element) { }
        public static void ResumeRendering(Microsoft.UI.Xaml.FrameworkElement element) { }
        public static object ConvertValue(System.Type type, object value)
        {
            if (value == null || type.IsInstanceOfType(value)) { return value; }
            throw new System.InvalidOperationException(""Unexpected binding conversion."");
        }
    }
}
namespace LibManagedDll
{
    public class AnotherClassForPathing
    {
        public void SetText(object value) { }
    }
    public struct ValueTypeBindingItem
    {
        public static int ReceiverAccesses;
        public AnotherClassForPathing Receiver
        {
            get
            {
                ReceiverAccesses++;
                return new AnotherClassForPathing();
            }
        }
    }
    public class BindPathParserClass
    {
        public static string StaticText = ""static text"";
        public static string GetTipOfTheDayStatic() { return StaticText; }
        public static string StringPropertyStatic
        {
            get { return StaticText; }
            set { StaticText = value; }
        }
        public static int StaticArgumentCalls;
        public static string FormatTitleStatic(string value)
        {
            StaticArgumentCalls++;
            return ""Title: "" + value;
        }
        public static int StaticBindBackCalls;
        public static void SetTextStatic(object value)
        {
            StaticBindBackCalls++;
            StaticText = (string)value;
        }
        public string FormatTitle(string value) { return ""Title: "" + value; }
        public string StringProperty { get; set; }
" + setters.ToFullString() + bindings.WithBaseList(null).ToFullString() + @"
    }
}";
            var references = new[]
            {
                typeof(object).Assembly.Location,
                typeof(Enumerable).Assembly.Location,
                typeof(System.CodeDom.Compiler.GeneratedCodeAttribute).Assembly.Location,
            }.Distinct().Select(path => MetadataReference.CreateFromFile(path));
            var compilation = CSharpCompilation.Create(
                "TemplateBindings_" + Guid.NewGuid().ToString("N"),
                new[] { CSharpSyntaxTree.ParseText(source) },
                references,
                new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary));
            using (var output = new MemoryStream())
            {
                var result = compilation.Emit(output);
                Assert.IsTrue(result.Success, "Generated template bindings did not compile:" +
                    Environment.NewLine + string.Join(Environment.NewLine,
                        result.Diagnostics.Where(diagnostic => diagnostic.Severity == DiagnosticSeverity.Error)));
                return new TemplateBindings(Assembly.Load(output.ToArray()), bindings.Identifier.ValueText, connectionId);
            }
        }

        private sealed class TemplateBindings
        {
            private readonly Type _modelType;
            private readonly Type _bindingsType;
            private readonly object _bindings;
            private readonly object _target;
            private readonly Type _eventArgsType;

            public TemplateBindings(Assembly assembly, string bindingsName, int connectionId)
            {
                _modelType = assembly.GetType("LibManagedDll.BindPathParserClass", true);
                _bindingsType = _modelType.GetNestedType(bindingsName, BindingFlags.NonPublic);
                _bindings = Activator.CreateInstance(_bindingsType, true);
                _target = Activator.CreateInstance(assembly.GetType("Microsoft.UI.Xaml.Controls.TextBlock", true));
                _eventArgsType = assembly.GetType("Microsoft.UI.Xaml.DataContextChangedEventArgs", true);
                // Match GetBindingConnector's setup order before delivering data-context changes.
                _bindingsType.GetMethod("SetDataRoot").Invoke(_bindings, new object[] { null });
                _bindingsType.GetMethod("Connect").Invoke(_bindings, new[] { (object)connectionId, _target });
            }

            public string Text => (string)_target.GetType().GetProperty("Text").GetValue(_target);
            public object Tag => _target.GetType().GetProperty("Tag").GetValue(_target);
            public string StaticText => (string)_modelType.GetField("StaticText").GetValue(null);
            public int StaticArgumentCalls => (int)_modelType.GetField("StaticArgumentCalls").GetValue(null);
            public int StaticBindBackCalls => (int)_modelType.GetField("StaticBindBackCalls").GetValue(null);

            public void SetStaticText(string text)
            {
                _modelType.GetField("StaticText").SetValue(null, text);
            }

            public object CreateItem(string text)
            {
                object item = Activator.CreateInstance(_modelType);
                _modelType.GetProperty("StringProperty").SetValue(item, text);
                return item;
            }

            public string GetItemText(object item)
            {
                return (string)_modelType.GetProperty("StringProperty").GetValue(item);
            }

            public void SetTag(object value)
            {
                _target.GetType().GetProperty("Tag").SetValue(_target, value);
            }

            public object CreateValueItem()
            {
                return Activator.CreateInstance(_modelType.Assembly.GetType("LibManagedDll.ValueTypeBindingItem", true));
            }

            public int ReceiverAccesses => (int)_modelType.Assembly
                .GetType("LibManagedDll.ValueTypeBindingItem", true).GetField("ReceiverAccesses").GetValue(null);

            public void ResetReceiverAccesses()
            {
                _modelType.Assembly.GetType("LibManagedDll.ValueTypeBindingItem", true)
                    .GetField("ReceiverAccesses").SetValue(null, 0);
            }

            public void ChangeDataContext(object value)
            {
                // The substitute framework has no inheritance engine; deliver its notification explicitly.
                object args = Activator.CreateInstance(_eventArgsType);
                _eventArgsType.GetProperty("NewValue").SetValue(args, value);
                _bindingsType.GetMethod("DataContextChangedHandler").Invoke(_bindings, new[] { _target, args });
            }

            public int ProcessBindings(object item, int phase)
            {
                object[] arguments = { item, 0, phase, -1 };
                _bindingsType.GetMethod("ProcessBindings", new[]
                {
                    typeof(object), typeof(int), typeof(int), typeof(int).MakeByRefType()
                }).Invoke(_bindings, arguments);
                return (int)arguments[3];
            }

            public void Recycle()
            {
                _bindingsType.GetMethod("Recycle").Invoke(_bindings, null);
            }

            public void StopTracking()
            {
                _bindingsType.GetMethod("StopTracking").Invoke(_bindings, null);
            }
        }

        [TestMethod]
        public void FunctionBinding_StaticFunctionInTemplateWithNullData()
        {
            var bindings = CreateTemplateBindings("");
            bindings.ChangeDataContext(null);

            Assert.AreEqual("static text", bindings.Text,
                "A static function binding must run even when the template has no data item.");
        }

        [TestMethod]
        public void FunctionBinding_StaticUntypedTemplateIgnoresUnrelatedDataRoots()
        {
            var bindings = CreateTemplateBindings("");
            int update = 0;
            // Change the static result too: retaining old text must not hide a skipped update
            // when the unused item enters or leaves an untyped template.
            foreach (object item in new object[] { "content", null, new object(), null })
            {
                string expected = "independent update " + update++;
                bindings.SetStaticText(expected);
                bindings.ChangeDataContext(item);
                Assert.AreEqual(expected, bindings.Text);
            }
        }

        [TestMethod]
        public void FunctionBinding_StaticUntypedComponentIgnoresUnrelatedItems()
        {
            foreach (object item in new object[] { "content", null, new object() })
            {
                var bindings = CreateTemplateBindings("", phase: 1);
                // Component entry points must omit the same unused conversion without
                // bypassing phase scheduling or evaluating the phase-one binding early.
                Assert.AreEqual(1, bindings.ProcessBindings(item, 0));
                Assert.AreEqual("", bindings.Text);
                Assert.AreEqual(-1, bindings.ProcessBindings(item, 1));
                Assert.AreEqual("static text", bindings.Text);
            }
        }

        [TestMethod]
        public void FunctionBinding_StaticTemplatesKeepRequiredItemConversions()
        {
            foreach (CodeGenLanguage language in new[] { CodeGenLanguage.CSharp, CodeGenLanguage.CppWinRT })
            {
                foreach (string template in new[]
                {
                    // An explicit DataTemplate item type is not the synthetic untyped fallback.
                    "<DataTemplate x:DataType='DataTemplate'><TextBlock Text='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic()}'/></DataTemplate>",
                    "<DataTemplate><TextBlock Text='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic()}' Tag='{x:Bind}'/></DataTemplate>",
                    "<DataTemplate><TextBlock Text='{x:Bind dll:BindPathParserClass.FormatObjectStatic(DispatcherQueue)}'/></DataTemplate>",
                    "<DataTemplate x:DataType='dll:ValueTypeBindingItem'><TextBlock Tag='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic(), Mode=TwoWay, BindBack=Receiver.SetText}'/></DataTemplate>"
                })
                {
                    string code = GenerateBindings("<ContentControl><ContentControl.ContentTemplate>" +
                        template + "</ContentControl.ContentTemplate></ContentControl>", language);
                    // Checking the component method catches accidentally applying the no-item
                    // specialization to a typed root, item argument, or reverse-only receiver.
                    AssertGenerated(code, language == CodeGenLanguage.CSharp
                        ? @"this\.Update_\((?:\([^)]+\)item|(?:global::)?WinRT\.CastExtensions\.As<[^>]+>\(item\)), 1 << phase\);"
                        : @"Update_\(::winrt::unbox_value<[^>]+>\(item\)\s*, 1 << phase\);");
                }
            }
        }

        [TestMethod]
        public void FunctionBinding_OrdinaryTemplatePreservesItemBinding()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'", "StringProperty, Mode=TwoWay", staticBinding: null);
            object first = bindings.CreateItem("first");
            object second = bindings.CreateItem("second");
            bindings.ChangeDataContext(first);
            Assert.AreEqual("first", bindings.Tag);
            bindings.ChangeDataContext(second);
            Assert.AreEqual("second", bindings.Tag);
            bindings.SetTag("edited");
            Assert.AreEqual("edited", bindings.GetItemText(second));
            Assert.AreEqual("first", bindings.GetItemText(first));
        }

        [TestMethod]
        public void FunctionBinding_StaticFunctionInTypedTemplateWithNullData()
        {
            var bindings = CreateTemplateBindings("x:DataType='x:String'");
            bindings.ChangeDataContext(null);

            Assert.AreEqual("static text", bindings.Text,
                "Declaring x:DataType must not make a static binding depend on a data item.");
        }

        [TestMethod]
        public void FunctionBinding_StaticFunctionInTemplatePreservesInstanceNullSafety()
        {
            var bindings = CreateTemplateBindings("x:DataType='dll:BindPathParserClass'", "StringProperty");
            bindings.ChangeDataContext(null);
            Assert.AreEqual("static text", bindings.Text);
            Assert.IsNull(bindings.Tag);

            bindings.ChangeDataContext(bindings.CreateItem("item text"));
            Assert.AreEqual("item text", bindings.Tag);

            bindings.SetStaticText("after null");
            bindings.ChangeDataContext(null);
            Assert.AreEqual("after null", bindings.Text);
            Assert.AreEqual("item text", bindings.Tag,
                "A missing data item must not evaluate an instance-dependent path.");
        }

        [TestMethod]
        public void FunctionBinding_StaticFunctionInTemplatePreservesValueTypeNullSafety()
        {
            var bindings = CreateTemplateBindings("x:DataType='x:Int32'", "");
            bindings.ChangeDataContext(null);
            Assert.AreEqual("static text", bindings.Text);
            Assert.IsNull(bindings.Tag, "A missing value-type item is not its default value.");

            bindings.ChangeDataContext(7);
            Assert.AreEqual(7, bindings.Tag);

            bindings.SetStaticText("after null");
            bindings.ChangeDataContext(null);
            Assert.AreEqual("after null", bindings.Text);
            Assert.AreEqual(7, bindings.Tag,
                "A missing value-type item must not replace the last value with zero.");
        }

        [TestMethod]
        public void FunctionBinding_StaticFunctionInTemplateDoesNotReuseMissingItem()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'",
                "dll:BindPathParserClass.FormatTitleStatic(StringProperty)");
            bindings.ChangeDataContext(null);
            Assert.AreEqual("static text", bindings.Text);
            Assert.AreEqual(0, bindings.StaticArgumentCalls);

            bindings.ChangeDataContext(bindings.CreateItem("item text"));
            Assert.AreEqual("Title: item text", bindings.Tag);
            int callsWithItem = bindings.StaticArgumentCalls;
            Assert.IsTrue(callsWithItem > 0);

            bindings.SetStaticText("after null");
            bindings.ChangeDataContext(null);
            Assert.AreEqual("after null", bindings.Text);
            Assert.AreEqual(callsWithItem, bindings.StaticArgumentCalls,
                "A static function with an item-dependent argument must not reuse the previous item.");
        }

        [TestMethod]
        public void FunctionBinding_StaticPropertyInTemplateWithNullData()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='x:String'",
                staticBinding: "dll:BindPathParserClass.StringPropertyStatic");
            bindings.ChangeDataContext(null);

            Assert.AreEqual("static text", bindings.Text);
        }

        [TestMethod]
        public void FunctionBinding_StaticTemplateDoesNotWriteToRemovedItem()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'", "StringProperty, Mode=TwoWay");
            object item = bindings.CreateItem("item text");
            bindings.ChangeDataContext(item);
            bindings.ChangeDataContext(null);
            bindings.SetTag("edited without an item");

            Assert.AreEqual("item text", bindings.GetItemText(item),
                "Editing a target after the item is removed must not write to the previous item.");

            var independent = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'",
                "dll:BindPathParserClass.StringPropertyStatic, Mode=TwoWay");
            independent.ChangeDataContext(null);
            independent.SetTag("edited static property");
            Assert.AreEqual("edited static property", independent.StaticText,
                "Independent static TwoWay bindings must remain active without an item.");
        }

        [TestMethod]
        public void FunctionBinding_StaticBindBackDoesNotInitializeTemplateWithoutItem()
        {
            // A static reverse handler does not make the instance-based forward path independent.
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'",
                "FormatTitle(StringProperty), Mode=TwoWay, BindBack=dll:BindPathParserClass.SetTextStatic",
                staticBinding: null);
            bindings.ChangeDataContext(null);
            bindings.SetTag("edited without an item");
            Assert.AreEqual(0, bindings.StaticBindBackCalls,
                "A BindBack-only static root must not initialize a template that has never received an item.");

            bindings.ChangeDataContext(bindings.CreateItem("item text"));
            Assert.AreEqual("Title: item text", bindings.Tag);
            bindings.SetTag("edited with an item");
            Assert.AreEqual(1, bindings.StaticBindBackCalls);
            Assert.AreEqual("edited with an item", bindings.StaticText,
                "The static BindBack must still run after the instance-dependent binding is initialized.");
        }

        [TestMethod]
        public void FunctionBinding_StaticInitializationDoesNotEnableItemDependentBindBack()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'",
                "FormatTitle(StringProperty), Mode=TwoWay, BindBack=dll:BindPathParserClass.SetTextStatic");
            bindings.ChangeDataContext(null);
            bindings.SetTag("edited without an item");
            Assert.AreEqual(0, bindings.StaticBindBackCalls,
                "Initializing an unrelated static binding must not enable an item-dependent binding's BindBack.");
            Assert.AreEqual("static text", bindings.StaticText);
        }

        [TestMethod]
        public void FunctionBinding_StaticArgumentInitializationDoesNotEnableBindBack()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'",
                "dll:BindPathParserClass.FormatTitleStatic(StringProperty), Mode=TwoWay, BindBack=dll:BindPathParserClass.SetTextStatic");
            bindings.ChangeDataContext(null);
            bindings.SetTag("edited without an item");
            Assert.AreEqual(0, bindings.StaticBindBackCalls,
                "A static receiver does not make item-dependent function arguments independent.");
            Assert.AreEqual(0, bindings.StaticArgumentCalls);
        }

        [TestMethod]
        public void FunctionBinding_StaticInitializationDoesNotWriteBackFirstItem()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'",
                "FormatTitle(StringProperty), Mode=TwoWay, BindBack=dll:BindPathParserClass.SetTextStatic");
            bindings.ChangeDataContext(null);
            bindings.ChangeDataContext(bindings.CreateItem("first item"));
            Assert.AreEqual("Title: first item", bindings.Tag);
            Assert.AreEqual(0, bindings.StaticBindBackCalls,
                "The first item-to-target update must not acquire a reverse write from earlier static initialization.");
            bindings.SetTag("legitimate edit");
            Assert.AreEqual(1, bindings.StaticBindBackCalls);
            Assert.AreEqual("legitimate edit", bindings.StaticText);
        }

        [TestMethod]
        public void FunctionBinding_StaticInitializationPreservesIndependentStaticTwoWayBinding()
        {
            var independent = CreateTemplateBindings(
                "x:DataType='dll:BindPathParserClass'",
                "dll:BindPathParserClass.GetTipOfTheDayStatic(), Mode=TwoWay, BindBack=dll:BindPathParserClass.SetTextStatic");
            independent.ChangeDataContext(null);
            independent.SetTag("static edit");
            Assert.AreEqual(1, independent.StaticBindBackCalls);
            Assert.AreEqual("static edit", independent.StaticText);
        }

        [TestMethod]
        public void FunctionBinding_StaticInitializationPreservesItemBindBackLifecycle()
        {
            foreach (string staticBinding in new[] { null, "dll:BindPathParserClass.GetTipOfTheDayStatic()" })
            {
                var bindings = CreateTemplateBindings(
                    "x:DataType='dll:BindPathParserClass'",
                    "FormatTitle(StringProperty), Mode=TwoWay, BindBack=dll:BindPathParserClass.SetTextStatic",
                    staticBinding);
                bindings.ChangeDataContext(null);
                bindings.ChangeDataContext(bindings.CreateItem("first"));
                Assert.AreEqual(0, bindings.StaticBindBackCalls);
                bindings.SetTag("first edit");
                Assert.AreEqual(1, bindings.StaticBindBackCalls);

                bindings.ChangeDataContext(null);
                bindings.SetTag("edit after removal");
                Assert.AreEqual(2, bindings.StaticBindBackCalls,
                    "An initialized binding with an independent BindBack receiver keeps its existing lifetime.");
                bindings.ChangeDataContext(bindings.CreateItem("replacement"));
                Assert.AreEqual("Title: replacement", bindings.Tag);
                Assert.AreEqual(3, bindings.StaticBindBackCalls,
                    "Replacing an item must preserve the established forward/reverse ordering.");

                bindings.Recycle();
                Assert.AreEqual(-1, bindings.ProcessBindings(bindings.CreateItem("recycled"), 0));
                Assert.AreEqual("Title: recycled", bindings.Tag);
                Assert.AreEqual(4, bindings.StaticBindBackCalls);
                bindings.SetTag("recycled edit");
                Assert.AreEqual(5, bindings.StaticBindBackCalls);

                bindings.StopTracking();
                bindings.ChangeDataContext(null);
                bindings.SetTag("edit before reinitialization");
                Assert.AreEqual(5, bindings.StaticBindBackCalls);
                bindings.ChangeDataContext(bindings.CreateItem("reinitialized"));
                Assert.AreEqual(5, bindings.StaticBindBackCalls);
                bindings.SetTag("reinitialized edit");
                Assert.AreEqual(6, bindings.StaticBindBackCalls);
            }
        }

        [TestMethod]
        public void FunctionBinding_StaticTemplateDoesNotEvaluateBindBackOnlyReceiverWithoutItem()
        {
            var bindings = CreateTemplateBindings(
                "x:DataType='dll:ValueTypeBindingItem'",
                "dll:BindPathParserClass.GetTipOfTheDayStatic(), Mode=TwoWay, BindBack=Receiver.SetText");
            bindings.ChangeDataContext(bindings.CreateValueItem());
            bindings.ChangeDataContext(null);
            bindings.ResetReceiverAccesses();
            bindings.SetTag("edited without an item");

            Assert.AreEqual(0, bindings.ReceiverAccesses,
                "A BindBack-only receiver must not be evaluated on a missing value-type item.");
        }

        [TestMethod]
        public void FunctionBinding_StaticTemplateRestoresDeferredNamedSource_CppWinRT()
        {
            string code = GenerateDeferredNamedSourceTemplateBindingsCppWinRT();

            AssertGenerated(code,
                @"void UpdateStaticBindings\(\) override\s*\{\s*UpdateStaticBindings\(this->NOT_PHASED\);\s*\}");
            var updater = Regex.Match(code,
                @"void UpdateStaticBindings\(int32_t phase\)\s*\{(?<body>[\s\S]*?)\r?\n        \}");
            Assert.IsTrue(updater.Success, "The null-data static updater was not generated.");
            AssertGenerated(updater.Groups["body"].Value, @"\bUpdate_source\(");
        }

        [TestMethod]
        public void FunctionBinding_FullTemplateRebindRestoresDeferredNamedSource_CppWinRT()
        {
            string code = GenerateDeferredNamedSourceTemplateBindingsCppWinRT();

            var updater = Regex.Match(code,
                @"void Update_\(::winrt::LibManagedDll::BindPathParserClass obj, int32_t phase\)\s*\{(?<body>[\s\S]*?)\r?\n        \}");
            Assert.IsTrue(updater.Success, "The full-root updater was not generated.");
            AssertGenerated(updater.Groups["body"].Value, @"\bUpdate_source\(");
        }

        [TestMethod]
        public void FunctionBinding_NamedOnlyTemplatePreservesNullDataBehavior_CppWinRT()
        {
            string code = GenerateDeferredNamedSourceTemplateBindingsCppWinRT(false);

            Assert.IsFalse(code.Contains("UpdateStaticBindings() override"),
                "A named-only template must not gain null-item initialization.");
            Assert.IsFalse(code.Contains("_hasDataRoot"),
                "An unaffected template must not opt into missing-item root tracking.");
        }

        [TestMethod]
        public void FunctionBinding_NamedOnlyTemplateBindBackPreservesLegacyValidity_CppWinRT()
        {
            string code = GenerateBindings(
                "<ContentControl><ContentControl.ContentTemplate><DataTemplate x:DataType='dll:ValueTypeBindingItem'>" +
                "<StackPanel><dll:NamedElementForPathing x:Name='helper'/>" +
                "<TextBlock Tag='{x:Bind helper.Format(helper.Value), Mode=TwoWay, BindBack=Receiver.SetText}'/>" +
                "</StackPanel></DataTemplate></ContentControl.ContentTemplate></ContentControl>",
                CodeGenLanguage.CppWinRT);

            AssertGenerated(code, @"if \(IsInitialized\(\)\)");
            Assert.IsFalse(code.Contains("_hasDataRoot"),
                "Named-only reverse bindings must retain their existing root-validity behavior.");
        }

        private string GenerateDeferredNamedSourceTemplateBindingsCppWinRT(bool includeStaticBinding = true)
        {
            return GenerateBindings(
                "<ContentControl><ContentControl.ContentTemplate><DataTemplate x:DataType='dll:BindPathParserClass'>" +
                "<StackPanel><TextBlock x:Name='source' x:DeferLoadStrategy='Lazy' Text='before null'/>" +
                "<TextBlock Text='{x:Bind source.Text, Mode=OneWay}'/>" +
                (includeStaticBinding ? "<TextBlock Text='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic()}'/>" : "") +
                "</StackPanel></DataTemplate></ContentControl.ContentTemplate></ContentControl>",
                CodeGenLanguage.CppWinRT);
        }

        [TestMethod]
        public void FunctionBinding_StaticFunctionInTemplate_CppWinRT()
        {
            string code = GenerateBindings(
                "<ContentControl><ContentControl.ContentTemplate><DataTemplate x:DataType='dll:BindPathParserClass'>" +
                "<TextBlock Text='{x:Bind dll:BindPathParserClass.FormatTitleStatic(StringProperty)}'/>" +
                "</DataTemplate></ContentControl.ContentTemplate></ContentControl>",
                CodeGenLanguage.CppWinRT);

            AssertGenerated(code, @"void UpdateStaticBindings\(\) override");
            AssertGenerated(code, @"if \(!this->_hasDataRoot\)\s*\{\s*return false;");
        }

        [TestMethod]
        public void FunctionBinding_NonStaticScopesKeepLegacyRootValidity_CppWinRT()
        {
            // Static members on a page/control template do not make those scopes null-item
            // DataTemplates; ordinary item templates must also keep their existing lifecycle.
            foreach (string xaml in new[]
            {
                "<StackPanel><TextBlock Tag='{x:Bind StringProperty, Mode=TwoWay}'/>" +
                    "<TextBlock Text='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic()}'/></StackPanel>",
                "<ContentControl><ContentControl.ContentTemplate><DataTemplate x:DataType='dll:BindPathParserClass'>" +
                    "<TextBlock Tag='{x:Bind StringProperty, Mode=TwoWay}'/>" +
                    "</DataTemplate></ContentControl.ContentTemplate></ContentControl>",
                "<Button><Button.Template><ControlTemplate TargetType='Button'><StackPanel>" +
                    "<TextBlock Tag='{x:Bind Tag, Mode=TwoWay}'/>" +
                    "<TextBlock Text='{x:Bind dll:BindPathParserClass.GetTipOfTheDayStatic()}'/>" +
                    "</StackPanel></ControlTemplate></Button.Template></Button>"
            })
            {
                string code = GenerateBindings(xaml, CodeGenLanguage.CppWinRT);
                Assert.IsFalse(code.Contains("_hasDataRoot"),
                    "Root-validity state must be limited to DataTemplates with forward static bindings.");
                Assert.IsFalse(code.Contains("UpdateStaticBindings() override"),
                    "An unaffected scope must not gain null-item initialization.");
            }
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
        public void FunctionBinding_UpdatesWhenArgumentIsNull_CppCX()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind FormatTitle(StringProperty)}'/>", CodeGenLanguage.Cpp);

            AssertGenerated(code,
                @"void Update_StringProperty\(::Platform::String\^ obj, int phase\)\s*\{\s*" +
                @"this->Update_M_FormatTitle_\d+\(phase\);\s*\}");
        }

        [TestMethod]
        public void FunctionBinding_UpdatesWhenArgumentIsNull_CppWinRT()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind FormatTitle(StringProperty)}'/>", CodeGenLanguage.CppWinRT);

            AssertGenerated(code,
                @"void Update_StringProperty\(::winrt::hstring obj, int32_t phase\)\s*\{\s*" +
                @"Update_M_FormatTitle_\d+\(phase\);\s*\}");
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
        public void FunctionBinding_RetrievesInstanceWhenArgumentIsOutsideItsPath_CppCX()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind InnerClass.Format(StringProperty)}'/>", CodeGenLanguage.Cpp);

            AssertGenerated(code,
                @"::LibManagedDll::AnotherClassForPathing\^ instance;\s*" +
                @"if \(!TryGet_InnerClass\(instance\) \|\| instance == nullptr\) \{ return; \}\s*" +
                @"::Platform::String\^ result = instance->Format\(p0\);");
        }

        [TestMethod]
        public void FunctionBinding_RetrievesInstanceWhenArgumentIsOutsideItsPath_CppWinRT()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind InnerClass.Format(StringProperty)}'/>", CodeGenLanguage.CppWinRT);

            AssertGenerated(code,
                @"::winrt::LibManagedDll::AnotherClassForPathing instance = nullptr;\s*" +
                @"if \(!TryGet_InnerClass\(instance\) \|\| !instance\) \{ return; \}\s*" +
                @"::winrt::hstring result = instance\.Format\(p0\);");
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

        /// <summary>
        /// A path that starts at a named element inside a template is rooted at the element root, a
        /// synthetic step standing in for the namescope rather than for a value, so it has to be
        /// read straight off the bindings class instead of being walked to through that root.
        /// </summary>
        private const string NamedElementTemplate = @"
            <ContentControl>
                <ContentControl.ContentTemplate>
                    <DataTemplate x:DataType='dll:BindPathParserClass'>
                        <Grid>
                            <dll:NamedElementForPathing x:Name='helper'/>
                            {0}
                        </Grid>
                    </DataTemplate>
                </ContentControl.ContentTemplate>
            </ContentControl>";

        [TestMethod]
        public void FunctionBinding_RetrievesNamedElementInstanceInTemplate()
        {
            string code = GenerateBindings(
                String.Format(NamedElementTemplate, "<TextBlock Text='{x:Bind helper.Format(StringProperty)}'/>"),
                CodeGenLanguage.CSharp);

            AssertGenerated(code,
                @"private bool TryGet_helper\(out global::LibManagedDll\.NamedElementForPathing val\)\s*\{\s*" +
                @"val = this\.obj\d+;\s*return true;\s*\}");
        }

        [TestMethod]
        public void FunctionBinding_RetrievesNamedElementInstanceInTemplate_VisualBasic()
        {
            string code = GenerateBindings(
                String.Format(NamedElementTemplate, "<TextBlock Text='{x:Bind helper.Format(StringProperty)}'/>"),
                CodeGenLanguage.VisualBasic);

            AssertGenerated(code,
                @"Private Function TryGet_helper\(<Global\.System\.Runtime\.InteropServices\.Out\(\)> ByRef val As " +
                @"Global\.LibManagedDll\.NamedElementForPathing\) As Boolean\s*" +
                @"val = Me\.obj\d+\s*Return True\s*End Function");
        }

        [TestMethod]
        public void FunctionBinding_RetrievesNamedElementInstanceInTemplate_CppCX()
        {
            string code = GenerateBindings(
                String.Format(NamedElementTemplate, "<TextBlock Text='{x:Bind helper.Format(StringProperty)}'/>"),
                CodeGenLanguage.Cpp);

            AssertGenerated(code,
                @"bool TryGet_helper\(::LibManagedDll::NamedElementForPathing\^& val\)\s*\{\s*" +
                @"val = this->obj\d+;\s*return true;\s*\}");
        }

        [TestMethod]
        public void FunctionBinding_RetrievesNamedElementInstanceInTemplate_CppWinRT()
        {
            string code = GenerateBindings(
                String.Format(NamedElementTemplate, "<TextBlock Text='{x:Bind helper.Format(StringProperty)}'/>"),
                CodeGenLanguage.CppWinRT);

            // The element root has no expression of its own, so reaching the field through it used
            // to produce a stray leading '.' and an accessor call on what is a plain field.
            AssertGenerated(code,
                @"bool TryGet_helper\(::winrt::LibManagedDll::NamedElementForPathing& val\)\s*\{\s*" +
                @"val = obj\d+;\s*return true;\s*\}");
        }

        /// <summary>
        /// The same expression backs the reverse assignment of a two way binding, which reaches a
        /// named element in a template without any function binding being involved.
        /// </summary>
        [TestMethod]
        public void TwoWayBinding_AssignsThroughNamedElementInTemplate_CppWinRT()
        {
            string code = GenerateBindings(
                String.Format(NamedElementTemplate, "<TextBox Text='{x:Bind helper.Value, Mode=TwoWay}'/>"),
                CodeGenLanguage.CppWinRT);

            AssertGenerated(code, @"if \(obj\d+ != nullptr\)");
            AssertGenerated(code, @"obj\d+\.Value\(obj\d+\.Text\(\)\);");
        }

        [TestMethod]
        public void FunctionBinding_RetrievesNamedElementArgumentInTemplate()
        {
            string code = GenerateBindings(
                String.Format(NamedElementTemplate, "<TextBlock Text='{x:Bind FormatTitle(helper.Value)}'/>"),
                CodeGenLanguage.CSharp);

            // The step for the named element itself is read directly, and the step below it is
            // walked to through that one as usual.
            AssertGenerated(code,
                @"private bool TryGet_helper\(out global::LibManagedDll\.NamedElementForPathing val\)\s*\{\s*" +
                @"val = this\.obj\d+;\s*return true;\s*\}");
            AssertGenerated(code,
                @"private bool TryGet_helper_Value\(out global::System\.String val\)\s*\{\s*" +
                @"global::LibManagedDll\.NamedElementForPathing obj;\s*" +
                @"if \(TryGet_helper\(out obj\) && obj != null\)");
        }

        /// <summary>
        /// A named element argument is rooted at the element root rather than at the data root, so
        /// it can schedule the binding while the data root the function is invoked on is still
        /// null. The two roots are distinct steps that share an empty code name.
        /// </summary>
        [TestMethod]
        public void FunctionBinding_RetrievesDataRootForNamedElementArgumentInTemplate()
        {
            string code = GenerateBindings(
                String.Format(NamedElementTemplate, "<TextBlock Text='{x:Bind FormatTitle(helper.Value)}'/>"),
                CodeGenLanguage.CSharp);

            AssertGenerated(code,
                @"global::LibManagedDll\.BindPathParserClass instance;\s*" +
                @"if \(!TryGet_\(out instance\) \|\| instance == null\) \{ return; \}\s*" +
                @"global::System\.String result = instance\.FormatTitle\(p0\);");
        }

        /// <summary>
        /// A function whose argument is the very step it is invoked on is still scheduled by that
        /// step becoming null, so the instance has to be retrieved even though the argument and the
        /// instance share a path.
        /// </summary>
        [TestMethod]
        public void FunctionBinding_RetrievesInstanceWhenArgumentIsTheInstance()
        {
            string code = GenerateBindings(
                "<TextBlock Text='{x:Bind InnerClass.Describe(InnerClass)}'/>", CodeGenLanguage.CSharp);

            AssertGenerated(code,
                @"global::LibManagedDll\.AnotherClassForPathing instance;\s*" +
                @"if \(!TryGet_InnerClass\(out instance\) \|\| instance == null\) \{ return; \}\s*" +
                @"global::System\.String result = instance\.Describe\(p0\);");
        }
    }
}
