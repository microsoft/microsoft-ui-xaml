// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;
using System.Reflection;
using System.Collections.Generic;

namespace UnitTests
{
    [TestClass]
    public class SchemaTests
    {
        TestHelper _testHelper;
        const string WINFX2006 = "http://schemas.microsoft.com/winfx/2006/xaml/presentation";

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
        public void Thrown_XamlSchemaError_AmbiguousCollectionAdd()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <dll:MultiAdd x:Key='key'>
            <x:String>this</x:String>
        </dll:MultiAdd>
    </Page.Resources>
</Page>";
            XamlDomValidator validator = null;
            Exception caughtException = null;
            try
            {
                validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            }
            catch (TargetInvocationException tiex)
            {
                // This error is thrown from System.Xaml.   We discovered this issue to close to 
                // release of version 1.0 of WinRT to address the issue.  Hopefully in future releases
                // we can address this and the error will not throw through to the user.

                // When this is fixed this test will fail and should be rewritten or removed.
                caughtException = tiex;
            }
            Assert.IsNotNull(caughtException, "Missing Ambigious Add() Exception from System.Xaml");
        }

        [TestMethod]
        public void XamlSchemaError_AmbiguousAttachablePropertyProvider()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>

<Grid>
    <Button Content='Hello'  Height='30' Width='100'
                dll:AttachablePropertyProviderWithMultipleSets.Foo='2' />
</Grid>
</Page>";
            // Run it past the Validator just to check it and 'protect' the Collector.
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.LoadUserDll);
            var validator = _testHelper.ValidateXAML(xaml, schema);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);

            TypeInfoCollector collector = _testHelper.CollectTypes(xaml, schema);

            string result2 = _testHelper.MatchErrors(schema.SchemaErrors, null, null, null);
            Assert.IsNull(result2, result2);
        }

        [TestMethod]
        public void Schema_Binding()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button Content='{Binding Foo}' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Touch code coverage of the WS Significant Collections
        /// and the Line Break.
        /// </summary>
        [TestMethod]
        public void Schema_BreakLineAndWhiteSpaceSC()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <TextBlock> this is text <LineBreak/> more text </TextBlock>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Touch Code coverage for the UI Primitive types
        /// </summary>
        [TestMethod]
        public void Schema_PointRectSize()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <Point x:Key='p'>77,77</Point>
        <Rect  x:Key='r'>1,2,3,4</Rect>
        <Size  x:Key='s'>100,100</Size>
    </Page.Resources>
    <Grid>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void Schema_ObsureSystemTypeNotInTheTop4()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:diag='using:System.Diagnostics'
    xmlns:sx='using:System.Xml'
>
    <Page.Resources>
        <diag:Stopwatch x:Key='sw'/>
        <sx:NameTable x:Key='nt' />
    </Page.Resources>

    <Grid>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Touch Code coverage for the UI Primitive types
        /// </summary>
        [TestMethod]
        public void Schema_UiPrimitives()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:dll='using:LibManagedDll'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

    <Page.Resources>
      <dll:UiPrimitives x:Key='prim' Thickness='4'/>
      <dll:UiPrimitives x:Key='prim2' Point='4,5.1'/>
      <dll:UiPrimitives x:Key='prim3' Rect='4.1, 3.2, 3.3, 9'/>
      <dll:UiPrimitives x:Key='prim4' Size='4.0,6'/>
      <dll:UiPrimitives x:Key='prim5' Orientation='Vertical'/>
    </Page.Resources>

    <Grid>
    </Grid>

</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Touch Code coverage for Content Property
        /// </summary>
        [TestMethod]
        public void Schema_CustomContentProperty()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <dll:GoodCpaClass>
        <x:String>Hello</x:String>
    </dll:GoodCpaClass>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Touch Code coverage for Template Property
        /// </summary>
        [TestMethod]
        public void Schema_Template()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Button>
        <Button.Template>
            <ControlTemplate>
                <Rectangle Fill='Red'/>
            </ControlTemplate>
        </Button.Template>
    </Button>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        // This test is necessary to drive coverage of LookupKeyType()
        // the compiler doesn't check the key type.  Because it insists that all keys be "text".
        [TestMethod]
        public void Schema_DriveCodeCoverageOf_KeyType()
        {
            SchemaMode[] schemaModes = new SchemaMode[2] { SchemaMode.ManagedRuntime, SchemaMode.NativeRuntime };
            foreach(SchemaMode smode in schemaModes)
            {
                DirectUISchemaContext schema = _testHelper.LoadSchema(smode);
                XamlTypeName typeName = new XamlTypeName(WINFX2006, "ResourceDictionary");
                XamlType xamlTypeRD = schema.GetXamlType(typeName);
                XamlType xamlTypeKey = xamlTypeRD.KeyType;

                Assert.AreEqual("Object", xamlTypeKey.Name);
            }
        }

        // This issue is something that should be fixed
        // But we are driving code coverage and I want to get in to this error path.
        [TestMethod]
        public void Schema_DriveCodeCoverageOf_KeyType2()
        {
            DirectUISchemaContext schema = null;
            XamlTypeName typeName = null;
            XamlType dictWith2Adds = null;
            XamlType xamlTypeKey = null;

            string result = null;
            string good = null;

            good = "Cannot determine the item type of dictionary type 'LibManagedDll.DictionaryWith2Adds' because it has more than one Add method or IDictionary<K,V> implementation. To make this dictionary type usable in XAML, add a public Add(object,object) method, implement System.Collections.IDictionary or a single System.Collections.Generic.IDictionary<K,V>.";
            result = "did not throw exception";
            try
            {
                schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime | SchemaMode.LoadUserDll);
                typeName = new XamlTypeName("using:LibManagedDll", "DictionaryWith2Adds");
                dictWith2Adds = schema.GetXamlType(typeName);
                xamlTypeKey = dictWith2Adds.KeyType;
            }
            catch (Exception ex)
            {
                result = ex.InnerException.Message;
            }
            Assert.AreEqual(good, result);
        }

        [TestMethod]
        public void Schema_DriveCodeCoverageOf_IsNullable()
        {
            SchemaMode[] schemaModes = new SchemaMode[2] { SchemaMode.ManagedRuntime, SchemaMode.NativeRuntime };
            foreach (SchemaMode smode in schemaModes)
            {
                DirectUISchemaContext schema = _testHelper.LoadSchema(smode);
                XamlTypeName typeName = new XamlTypeName(WINFX2006, "ToggleButton");
                XamlType xamlTypeTB = schema.GetXamlType(typeName);

                XamlMember xamlMemberIsChecked = xamlTypeTB.GetMember("IsChecked");
                XamlType xamlTypeNullableBool = xamlMemberIsChecked.Type;

                XamlMember xamlMemberWidth = xamlTypeTB.GetMember("Width");
                XamlType xamlTypeDouble = xamlMemberWidth.Type;

                bool yes = xamlTypeNullableBool.IsNullable;
                bool no = xamlTypeDouble.IsNullable;

                Assert.AreEqual(true, yes);
                Assert.AreEqual(false, no);
            }
        }

        [TestMethod]
        public void Schema_DriveCodeCoverageOf_SchemaErrors()
        {
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.NativeRuntime);
            Type type = null;
            string result1 = "Null Type Did Not Throw";
            string goodR1 = "Null Type Threw";

            try
            {
                schema.GetXamlType(type);
            }
            catch (TargetInvocationException tiex)
            {
                if (tiex.InnerException.GetType() == typeof(ArgumentNullException))
                {
                    result1 = goodR1;
                }
            }
            Assert.AreEqual(goodR1, result1);

            string result2 = "Bad Schema Namespace URI Did Not Throw";
            string goodR2 = "Bad Schema Namespace URI Threw";
            try
            {
                schema.GetAllXamlTypes("http://Nothing");
            }
            catch (TargetInvocationException tiex)
            {
                if (tiex.InnerException.GetType() == typeof(NotImplementedException))
                {
                    result2 = goodR2;
                }
            }
            Assert.AreEqual(goodR2, result2);
        }

        [TestMethod]
        public void Schema_DriveCodeCoverageOf_BindingAndRelativeSource()
        {
            SchemaMode[] schemaModes = new SchemaMode[2] { SchemaMode.ManagedRuntime, SchemaMode.NativeRuntime };
            foreach (SchemaMode smode in schemaModes)
            {
                DirectUISchemaContext schema = _testHelper.LoadSchema(smode);
                XamlTypeName typeName = new XamlTypeName(WINFX2006, "Binding");
                XamlType binding = schema.GetXamlType(typeName);
                bool b = binding.IsMarkupExtension;
                XamlType bmert = binding.MarkupExtensionReturnType;
                Assert.AreEqual("Binding", binding.Name);
                Assert.AreEqual(true, b);
                Assert.AreEqual("Object", bmert.Name);

                typeName = new XamlTypeName(WINFX2006, "RelativeSource");
                XamlType relativeSource = schema.GetXamlType(typeName);
                XamlType rsmert = relativeSource.MarkupExtensionReturnType;

                Assert.AreEqual("RelativeSource", relativeSource.Name);
                Assert.AreEqual("RelativeSource", rsmert.Name);

            }
        }

        // Use of the "x:Lang" attribute in XAML should be good enough
        // to get into the DirectUI LookupAliasesProperties() for "Lang"
        // But it does not.
        [TestMethod]
        public void Schema_DriveCodeCoverage_Lang()
        {
            SchemaMode[] schemaModes = new SchemaMode[2] { SchemaMode.ManagedRuntime, SchemaMode.NativeRuntime };
            foreach (SchemaMode smode in schemaModes)
            {
                DirectUISchemaContext schema = _testHelper.LoadSchema(smode);
                XamlTypeName typeName = new XamlTypeName(WINFX2006, "Button");
                XamlType button = schema.GetXamlType(typeName);
                XamlMember lang = button.GetAliasedProperty(XamlLanguage.Lang);

                Assert.AreEqual("Language", lang.Name);

                typeName = new XamlTypeName(WINFX2006, "Inline");
                XamlType inline = schema.GetXamlType(typeName);
                lang = inline.GetAliasedProperty(XamlLanguage.Lang);

                Assert.AreEqual("Language", lang.Name);

            }
        }

        [TestMethod]
        public void Schema_DriveCodeCoverage_IsWhitespaceSignificantCollection()
        {
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.NativeRuntime);
            XamlTypeName typeName = new XamlTypeName(WINFX2006, "InlineCollection");
            XamlType inlineCollection = schema.GetXamlType(typeName);
            bool b = inlineCollection.IsWhitespaceSignificantCollection;

            Assert.AreEqual(true, b);
        }

        [TestMethod]
        public void Schema_DriveCodeCoverage_AllowedContentTypes()
        {
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.NativeRuntime);
            XamlTypeName typeName = new XamlTypeName(WINFX2006, "ResourceDictionary");
            XamlType resourceDictionary = schema.GetXamlType(typeName);
            IList<XamlType> types = resourceDictionary.AllowedContentTypes;

            Assert.AreEqual(0, types.Count);
        }

        [TestMethod]
        public void Schema_DriveCodeCoverage_Inlines()
        {
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.NativeRuntime);
            XamlTypeName typeName = new XamlTypeName(WINFX2006, "Inline");
            XamlType inline = schema.GetXamlType(typeName);
            XamlMember member = inline.GetAliasedProperty(XamlLanguage.Lang);

            Assert.AreEqual("Language", member.Name);
        }

        [TestMethod]
        public void Schema_GetAllXamlTypes()
        {
            int count = 0;
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);
            foreach (XamlType xamlType in schema.GetAllXamlTypes(WINFX2006))
            {
                count += 1;
            }

            Assert.IsTrue(count > 1400);
        }

        [TestMethod]
        public void Schema_GetAllMembers()
        {
            int count = 0;
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);
            XamlTypeName xamlTypeName = new XamlTypeName(WINFX2006, "Style");
            XamlType xamlType = schema.GetXamlType(xamlTypeName);
            List<string> memberNames = new List<string>();
            foreach (XamlMember member in xamlType.GetAllMembers())
            {
                count += 1;
                memberNames.Add(member.Name);
            }
            String[] shouldBeNames = new string[] { "BasedOn", "IsSealed", "Setters", "TargetType", "Dispatcher" };
            _testHelper.AssertListsAreEqual("property", memberNames, shouldBeNames);
        }

        [TestMethod]
        public void Schema_GetAllAttachableMembers()
        {
            int count = 0;
            DirectUISchemaContext schema = _testHelper.LoadSchema(SchemaMode.ManagedRuntime);
            XamlTypeName xamlTypeName = new XamlTypeName(WINFX2006, "Grid");
            XamlType xamlType = schema.GetXamlType(xamlTypeName);
            List<string> attachableMemberNames = new List<string>();
            foreach (XamlMember member in xamlType.GetAllAttachableMembers())
            {
                count += 1;
                attachableMemberNames.Add(member.Name);
            }
            String[] shouldBeNames = new string[] { "Row", "Column", "RowSpan", "ColumnSpan" };
            _testHelper.AssertListsAreEqual("attachable property", attachableMemberNames, shouldBeNames);
        }

    }
}
