// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class TypeCollectorTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
        public void TypeCollector_Simple()
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

            //XamlSchemaCodeInfo scheamInfo = collector.SchemaInfo;

            string result = _testHelper.MatchErrors(schema.SchemaErrors, null, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TypeCollector_Types()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <dll:SimpleClass x:Key='key'/>
        <dll:CollectionHolder x:Key='key1'/>
        <dll:ArrayHolder x:Key='key2'/>
        <dll:NullableTypeHolder x:Key='key3'/>
    </Page.Resources>
</Page>";
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.LoadUserDll);
            TypeInfoCollector collector = _testHelper.CollectTypes(xaml, schema);

            //XamlSchemaCodeInfo scheamInfo = collector.SchemaInfo;

            string result = _testHelper.MatchErrors(schema.SchemaErrors, null, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TypeCollector_Styles()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <Style x:Key='style' TargetType='dll:SimpleClass'>
            <Setter Property='Prop1' Value='hello' />
        </Style>
    </Page.Resources>
</Page>";
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.LoadUserDll);
            TypeInfoCollector collector = _testHelper.CollectTypes(xaml, schema);

            //XamlSchemaCodeInfo scheamInfo = collector.SchemaInfo;

            string result = _testHelper.MatchErrors(schema.SchemaErrors, null, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TypeCollector_Indexer()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <dll:HasIndexer x:Key='key'/>
        <dll:HasInstanceItem x:Key='key1'/>
        <SolidColorBrush x:Key='key2' dll:HasAttachableItem.Item='Hello'>Red</SolidColorBrush>
        <dll:HasIndexerRenamed x:Key='key3'/>
    </Page.Resources>
</Page>";

            // Validate the XAML
            var schema = _testHelper.LoadSchema(SchemaMode.LoadUserDll);
            var validator = _testHelper.ValidateXAML(xaml, schema);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);

            // Collect the types
            TypeInfoCollector collector = _testHelper.CollectTypes(xaml, schema);
            result = _testHelper.MatchErrors(schema.SchemaErrors, null, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TypeCollector_RootLogs_PropertyPathNames()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Grid>
     <ComboBox DisplayMemberPath='DisplayMemberPath1' SelectedValuePath='SelectedValuePath'/>
     <AutoSuggestBox TextMemberPath='TextMemberPath'/>
     <Button>
        <Button.Flyout>
            <ListPickerFlyout DisplayMemberPath='DisplayMemberPath2'/>
        </Button.Flyout>
     </Button>
  </Grid>
</Page>";

            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.LoadUserDll);
            TypeInfoCollector collector = _testHelper.CollectTypes(xaml, schema);
            var ppNames = collector.RootLog.PropertyPathNames;
            Assert.IsTrue(
                ppNames.Count == 4 &&
                ppNames[0] == "DisplayMemberPath1" &&
                ppNames[1] == "SelectedValuePath" &&
                ppNames[2] == "TextMemberPath" &&
                ppNames[3] == "DisplayMemberPath2");
        }
    }
}
