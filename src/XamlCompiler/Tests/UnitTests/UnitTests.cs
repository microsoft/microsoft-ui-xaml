// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Text;
using System.Collections.Generic;
using System.Xml;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class UnitTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
        public void TestProxies()
        {
            _testHelper.TestProxies();
        }

        [TestMethod]
        public void XamlTypeUniverseDisposalIsSharedAcrossProxies()
        {
            var universe = new XamlTypeUniverse(false);
            var alias = new XamlTypeUniverse(universe.Instance);

            universe.Dispose();
            universe.Dispose();
            alias.Dispose();

            AssertObjectDisposed(() => Assert.IsNotNull(alias.Instance));
            AssertObjectDisposed(() => alias.GetSystemAssembly());
            AssertObjectDisposed(() => Assert.IsFalse(alias.IsSystemAssemblyLoaded));
        }

        [TestMethod]
        public void DirectUISystemProxyExposesUnderlyingCollections()
        {
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);
            DirectUISystem system = schema.DirectUISystem;

            Assert.IsTrue(system.PlatformAssemblies.Count > 0);
            Assert.AreEqual(system.PlatformAssemblies.Count, system.XamlTypeUniverses.Count);
            foreach (DirectUIAssembly assembly in system.PlatformAssemblies)
            {
                Assert.IsNotNull(assembly.WrappedAssembly);
            }
            foreach (XamlTypeUniverse universe in system.XamlTypeUniverses)
            {
                Assert.IsNotNull(universe.Instance);
            }
        }

        private static void AssertObjectDisposed(Action action)
        {
            try
            {
                action();
                Assert.Fail("Expected ObjectDisposedException.");
            }
            catch (ObjectDisposedException)
            {
            }
        }

        /// <summary>
        /// Simple 'just test some XAML' basic test.
        /// </summary>
        [TestMethod]
        public void Basic01()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <Style TargetType='Button' >
            <Setter Property='Background' Value='Red' />
        </Style>
        <SolidColorBrush x:Key='myBrush'>Cyan</SolidColorBrush>
    </Page.Resources>

    <Grid>
        <Grid.RowDefinitions>
            <RowDefinition/>
            <RowDefinition/>
            <RowDefinition/>
        </Grid.RowDefinitions>
        <Border Grid.Row='0' BorderBrush='Black' BorderThickness='2'>
            <Button Content='OK'>
                <Button.Background>
                    <Brush>Cyan</Brush>
                </Button.Background>
            </Button>
        </Border>
        <Border Grid.Row='1' BorderBrush='Black'>
            <Border.BorderThickness>
                <Thickness>2</Thickness>
            </Border.BorderThickness>
            <Button >
                <Button.Background>
                    <SolidColorBrush>Orange</SolidColorBrush>
                </Button.Background>
                OK
            </Button>
        </Border>
        <StackPanel Orientation='Horizontal' Grid.Row='2'>
            <Border BorderBrush='Black'/>
            <Button Content='Press' />
        </StackPanel>
    </Grid>
</Page>";

            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void ThemeResource()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button Content='Hello' Background='{ThemeResource someName}' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void StringOnGrid()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <x:String>foo</x:String>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC0020",  // can't assign String into Grid's Children UIElementCollection. 
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WriteOnlyProperties()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>

    <Page.Resources>
        <dll:WriteOnlyHolder x:Key='foo' PrivateSetStringProp='private' WOStringProp='setOnly'/>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void ThicknessOnBorder()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
      <Border CornerRadius='3'>
            <Border.BorderThickness>
                <Thickness>2</Thickness>
            </Border.BorderThickness>
      </Border>
    </Grid>

</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void CheckStaticResourceAndNull()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button Foreground='{x:Null}'/>
        <Button Background='{x:Null}'/>
        <Button Content='{x:Null}'/>
        <Button Content='{StaticResource foo}'/>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void CheckStaticResourceAndNullElements()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Grid.Resources>
            <StaticResource ResourceKey='dict' />
        </Grid.Resources>
        <Button>
            <Button.Background>
                <NullExtension />
            </Button.Background>
            <Button.Content>
                <StaticResourceExtension ResourceKey='foo' />
            </Button.Content>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }


        [TestMethod]
        public void CheckTypeExtensionFails()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Grid.Style>
            <Style TargetType='{x:Type Grid}' />
        </Grid.Style>
    </Grid>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC0001",  // Unknown Type 'Type'
                "WMC0080",  // Style object must specify a String value for the TargetType property
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void FindProjectedUiXamlStructs()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Page.Resources>
    <Thickness x:Key='thicknessKey'>3</Thickness>
  </Page.Resources>

  <Grid>

  </Grid>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);

            validator = _testHelper.ValidateXAML(xaml);
            result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TestAllowedContentTypes()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

  <TextBlock>
    <Run>This is a Run</Run>
    this is just text
  </TextBlock>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);

            validator = _testHelper.ValidateXAML(xaml);
            result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void XamlRewrite_XBindInTextBlockRunsInDataTemplate()
        {
            AssertTextBlockRunConnectionIds(
                "<TextBlock><Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/></TextBlock>");
        }

        [TestMethod]
        public void XamlRewrite_TextBlockOpeningTagBoundaries()
        {
            string[] textBlocks =
            {
                "<TextBlock><Run Text='{x:Bind Name}'/><Run Text='{x:Bind Width}'/></TextBlock>",
                "<TextBlock>prefix <Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/> suffix</TextBlock>",
                "<TextBlock>\n<Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/>\n</TextBlock>",
                "<TextBlock\n>\n<Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/>\n</TextBlock>",
                "<TextBlock ><Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/></TextBlock>",
                "<TextBlock Tag='preserved'><Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/></TextBlock>",
                "<p:TextBlock><p:Run Text='{x:Bind Name}'/> <p:Run Text='{x:Bind Width}'/></p:TextBlock>",
                "<TextBlock><Span><Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/></Span></TextBlock>"
            };

            foreach (string textBlock in textBlocks)
            {
                AssertTextBlockRunConnectionIds(textBlock);
            }
        }

        [TestMethod]
        public void XamlRewrite_MultilineOpeningTagAfterComment()
        {
            AssertTextBlockRunConnectionIds(
                "<!-- earlier > --><TextBlock\n><Run Text='{x:Bind Name}'/> <Run Text='{x:Bind Width}'/></TextBlock>");
        }

        // Analyze bindings and rewrite the supplied TextBlock inside a DataTemplate, then verify
        // valid XML, correctly placed connection IDs, and preservation of text and unrelated attributes.
        private void AssertTextBlockRunConnectionIds(string textBlock)
        {
            const string presentationNamespace = "http://schemas.microsoft.com/winfx/2006/xaml/presentation";
            const string xamlNamespace = "http://schemas.microsoft.com/winfx/2006/xaml";
            string xaml = string.Format(@"
<Page
    x:Class='MyNamespace.MyClass'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:p='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <ListView>
        <ListView.ItemTemplate>
            <DataTemplate x:DataType='Button'>
                {0}
            </DataTemplate>
        </ListView.ItemTemplate>
    </ListView>
</Page>", textBlock);

            // Require valid input markup and resolvable binding paths before exercising the rewriter.
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);
            CompilerDomRootToken domRoot = _testHelper.LoadXamlDom(xaml, schema);
            XamlDomValidator validator = _testHelper.ValidateXamlDom(domRoot, false);
            Assert.AreEqual(0, validator.Errors.Count, textBlock);
            XamlClassCodeInfo classCodeInfo = _testHelper.HarvestClassCodeInfo(".", domRoot, false, false);
            XamlFileCodeInfo fileCodeInfo = _testHelper.HarvestFileCodeInfo(".", false, classCodeInfo, domRoot);
            classCodeInfo.AddXamlFileInfo(fileCodeInfo);
            foreach (BindUniverse universe in classCodeInfo.BindUniverses)
            {
                var bindingErrors = new List<XamlCompileError>(universe.Parse(classCodeInfo));
                string result = _testHelper.MatchErrors(bindingErrors, null, null, null);
                Assert.IsNull(result, result);
            }

            // Rewriting must produce output without compiler errors.
            var rewriter = new XamlConnectionIdRewriter();
            string rewrittenXaml = rewriter.Parse(xaml, classCodeInfo, fileCodeInfo);
            Assert.AreEqual(0, rewriter.Errors.Count, textBlock);
            Assert.AreEqual(0, schema.SchemaErrors.Count, textBlock);
            Assert.IsNotNull(rewrittenXaml, textBlock);

            // Parsing catches malformed output, including duplicate connection-ID attributes.
            var originalDocument = new XmlDocument { PreserveWhitespace = true };
            originalDocument.LoadXml(xaml);
            var rewrittenDocument = new XmlDocument { PreserveWhitespace = true };
            rewrittenDocument.LoadXml(rewrittenXaml);

            var originalTextBlock = (XmlElement)originalDocument.GetElementsByTagName("TextBlock", presentationNamespace)[0];
            var rewrittenTextBlock = (XmlElement)rewrittenDocument.GetElementsByTagName("TextBlock", presentationNamespace)[0];
            // Preserve literal text, allowing the rewriter's existing line-ending normalization.
            Assert.AreEqual(originalTextBlock.InnerText.Replace("\r\n", "\n"),
                rewrittenTextBlock.InnerText.Replace("\r\n", "\n"), textBlock);
            // Unrelated attributes, such as Tag, must remain unchanged.
            foreach (XmlAttribute attribute in originalTextBlock.Attributes)
            {
                Assert.AreEqual(attribute.Value,
                    rewrittenTextBlock.GetAttribute(attribute.LocalName, attribute.NamespaceURI), textBlock);
            }

            // The template root and its two Runs must receive distinct, nonempty connection IDs.
            string rootConnectionId = rewrittenTextBlock.GetAttribute("ConnectionId", xamlNamespace);
            Assert.IsFalse(string.IsNullOrEmpty(rootConnectionId), textBlock);
            var connectionIds = new HashSet<string> { rootConnectionId };
            XmlNodeList runs = rewrittenTextBlock.GetElementsByTagName("Run", presentationNamespace);
            Assert.AreEqual(2, runs.Count, textBlock);
            foreach (XmlElement run in runs)
            {
                string connectionId = run.GetAttribute("ConnectionId", xamlNamespace);
                Assert.IsFalse(string.IsNullOrEmpty(connectionId), textBlock);
                Assert.IsTrue(connectionIds.Add(connectionId), textBlock);
                // Generated binding code supplies Text; x:Bind must not remain in the XBF input.
                Assert.IsFalse(run.HasAttribute("Text"), textBlock);
            }
        }

        [TestMethod]
        public void TouchTheXTypes()
        {
            // The x: types are:
            // x:Null, x:String, x:Int32, x:Double, and x:Boolean
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Grid>
        <Button Background='{Null}'>
            <Button.Height>
                <x:Double>55</x:Double>
            </Button.Height>
            <Grid.Row>
                <x:Int32>1</x:Int32>
            </Grid.Row>
            <Button.Content>
                <x:String>Press Me</x:String>
            </Button.Content>
            <Button.IsTabStop>
                <x:Boolean>false</x:Boolean>
            </Button.IsTabStop>
        </Button>
    </Grid>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);

            validator = _testHelper.ValidateXAML(xaml, SchemaMode.NativeRuntime);
            result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);

        }

    }
}
