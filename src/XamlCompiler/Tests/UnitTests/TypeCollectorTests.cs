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

        [TestMethod]
        public void TypeCollector_RefReturnProperty()
        {
            // Regression test for a ref-return property (e.g. "public ref int RefReturnProperty").
            // Its reflected return type is "System.Int32&" (IsByRef). This must never leak into the
            // generated XamlTypeInfo type table, otherwise codegen emits typeof(global::System.Int32&)
            // which is invalid C#/VB and breaks the build (CS1026/CS1525/CS1002/CS1513).
            // See https://github.com/microsoft/microsoft-ui-xaml/issues/8293.
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <dll:MyRefReturnClass x:Key='key'/>
    </Page.Resources>
</Page>";
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.LoadUserDll);
            TypeInfoCollector collector = _testHelper.CollectTypes(xaml, schema);

            string result = _testHelper.MatchErrors(schema.SchemaErrors, null, null, null);
            Assert.IsNull(result, result);

            // No ByRef type should ever be present in the generated type table.
            foreach (var entry in collector.SchemaInfo.TypeTable)
            {
                Assert.IsFalse(
                    entry.SystemName != null && entry.SystemName.EndsWith("&"),
                    "ByRef type leaked into the XamlTypeInfo type table: " + entry.SystemName);
            }

            // Positive check: legitimate (non-ByRef) member types are still collected, so an
            // over-broad guard that drops valid members can't pass this test. NormalProperty's
            // "System.Int32" return type must be present in the type table.
            bool foundInt32 = false;
            foreach (var entry in collector.SchemaInfo.TypeTable)
            {
                if (entry.SystemName == "System.Int32")
                {
                    foundInt32 = true;
                    break;
                }
            }
            Assert.IsTrue(
                foundInt32,
                "Expected the non-ByRef 'System.Int32' member type to be collected into the type table.");
        }
    }
}
