// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Reflection;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class HarvesterTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        /// <summary>
        /// Simple calling of the Harvester.
        /// </summary>
        [TestMethod]
        public void Harvester_Simple()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.MyPage'>
    <Grid>
        <Button x:Name='btn' Click='ClickHdlr' />
    </Grid>
</Page>";

            CompilerDomRootToken domTree = _testHelper.LoadXamlDom(xaml, SchemaMode.ManagedRuntime);
            XamlFileCodeInfo codeInfo = _testHelper.Harvest(".", domTree, false, false);
        }

        /// <summary>
        /// Test that the x:Class element must be in a namespace
        /// ie have a '.' in the name.
        /// The Validator should guard for this so the Harvester doesn't see it but it does not at this time
        /// </summary>
        [TestMethod]
        public void Harvester_BadClass0()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyPage'>
    <Grid>
        <Button x:Name='btn' Click='ClickHdlr' />
    </Grid>
</Page>";
            string errorMessage = null;
            CompilerDomRootToken domTree = _testHelper.LoadXamlDom(xaml, SchemaMode.ManagedRuntime);
            try
            {
                XamlFileCodeInfo codeInfo = _testHelper.Harvest(".", domTree, false, false);
            }
            catch (TargetInvocationException ex)
            {
                errorMessage = ex.InnerException.Message;
            }
            Assert.IsNotNull(errorMessage, "Bad x:Class Value should have caused an error");
        }

        /// <summary>
        /// Test that the x:Class element does not have an empty part of the namespace.
        /// The Validator should guard for this so the Harvester doesn't see it but it does not at this time
        /// </summary>
        [TestMethod]
        public void Harvester_BadClass1()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff..MyPage'>
    <Grid>
        <Button x:Name='btn' Click='ClickHdlr' />
    </Grid>
</Page>";
            string errorMessage = null;
            CompilerDomRootToken domTree = _testHelper.LoadXamlDom(xaml, SchemaMode.ManagedRuntime);
            try
            {
                XamlFileCodeInfo codeInfo = _testHelper.Harvest(".", domTree, false, false);
            }
            catch (TargetInvocationException ex)
            {
                errorMessage = ex.InnerException.Message;
            }
            Assert.IsNotNull(errorMessage, "Bad x:Class Value should have caused an error");
        }

        /// <summary>
        /// Test that the x:Class element doesn't have spaces in it.
        /// The Validator should guard for this so the Harvester doesn't see it but it does not at this time
        /// </summary>
        [TestMethod]
        public void Harvester_BadClass2()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.Other thing.MyPage'>
    <Grid>
        <Button x:Name='btn' Click='ClickHdlr' />
    </Grid>
</Page>";
            string errorMessage = null;
            CompilerDomRootToken domTree = _testHelper.LoadXamlDom(xaml, SchemaMode.ManagedRuntime);
            try
            {
                XamlFileCodeInfo codeInfo = _testHelper.Harvest(".", domTree, false, false);
            }
            catch (TargetInvocationException ex)
            {
                errorMessage = ex.InnerException.Message;
            }
            Assert.IsNotNull(errorMessage, "Bad x:Class Value should have caused an error");
        }

        /// <summary>
        /// Test that the x:Class element doesn't have other illegal characters in it.
        /// The Validator should guard for this so the Harvester doesn't see it but it does not at this time
        /// </summary>
        [TestMethod]
        public void Harvester_BadClass3()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.Other@thing.MyPage'>
    <Grid>
        <Button x:Name='btn' Click='ClickHdlr' />
    </Grid>
</Page>";
            string errorMessage = null;
            CompilerDomRootToken domTree = _testHelper.LoadXamlDom(xaml, SchemaMode.ManagedRuntime);
            try
            {
                XamlFileCodeInfo codeInfo = _testHelper.Harvest(".", domTree, false, false);
            }
            catch (TargetInvocationException ex)
            {
                errorMessage = ex.InnerException.Message;
            }
            Assert.IsNotNull(errorMessage, "Bad x:Class Value should have caused an error");
        }

        public void Harvester_xBindInsideTemplate(string xaml)
        {
            var schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);
            CompilerDomRootToken domTree = _testHelper.LoadXamlDom(xaml, schema);
            XamlFileCodeInfo codeInfo = _testHelper.Harvest(".", domTree, false, false);
            string[] expectedErrors =
            {
                "WMC1111",  // Templates containing x:Bind need a DataType to be specified using 'x:DataType'
            };
            string result = _testHelper.MatchErrors(schema.SchemaErrors, null, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void Harvester_xBindInsideControlTemplateNoTargetType()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.MyPage'>
    <Grid>
      <Button>
        <Button.Template>
            <ControlTemplate>
                <TextBlock Text='{x:Bind Tag}'/>
            </ControlTemplate>
        </Button.Template>
      </Button>
    </Grid>
</Page>";
            Harvester_xBindInsideTemplate(xaml);
        }

        [TestMethod]
        public void Harvester_xBindOnControlTemplate()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.MyPage'>
    <Grid>
      <Button>
        <Button.Template>
            <ControlTemplate TargetType='Button' >
                <TextBlock Text='{x:Bind Tag}'/>
            </ControlTemplate>
        </Button.Template>
      </Button>
    </Grid>
</Page>";
            var schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);
            CompilerDomRootToken domTree = _testHelper.LoadXamlDom(xaml, schema);
            XamlFileCodeInfo codeInfo = _testHelper.Harvest(".", domTree, false, false);
            string[] expectedErrors =
            {
            };
            string result = _testHelper.MatchErrors(schema.SchemaErrors, null, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void Harvester_xBindInsideItemsPanelTemplate()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.MyPage'>
    <Grid>
        <GridView>
            <GridView.ItemsPanel>
                <ItemsPanelTemplate>
                    <Grid ColumnSpacing='{x:Bind Test}'/>
                </ItemsPanelTemplate>
            </GridView.ItemsPanel>
        </GridView>
    </Grid>
</Page>";
            Harvester_xBindInsideTemplate(xaml);
        }
    }
}
