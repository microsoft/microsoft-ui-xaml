// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Text;
using System.Collections.Generic;
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

        /// <summary>
        /// Simple 'just test some XAML' basic test.
        /// </summary>
        [TestMethod]
        // Create a dependencies that cause MsTest.exe to copy the specified file to the Test folder.
        [DeploymentItem(@"LibManagedDll.dll")]
        [DeploymentItem(@"LibManagedDllSatellite.dll")]
        [DeploymentItem(@"LibManagedWinmd.Winmd")]
        [DeploymentItem(@"Microsoft.UI.Xaml.Markup.Compiler.dll")]
        [DeploymentItem(ProxyHelper.WinUIWinmdFile)]
        [DeploymentItem(@"GenXbf.dll")]
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
