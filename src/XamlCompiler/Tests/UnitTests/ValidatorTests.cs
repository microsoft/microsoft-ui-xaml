// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    /// <summary>
    /// Summary description for ValidatorTests
    /// </summary>
    [TestClass]
    public class ValidatorTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
        public void WMC0001_UnknownObject()
        {
            string xaml = @"
<PageNot
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
</PageNot>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedErrors =
            {
                "WMC0001",  // Unknown type 'PageNot' in XML namespace 'http://schemas.microsoft.com/winfx/2006/xaml/presentation'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0001_UnknownObject2()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button>
            <dll:ClassNot />
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedErrors =
            {
                "WMC0001",  // Unknown type 'ClassNot' in XML namespace 'using:LibManagedDll'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0001_UnknownObject3()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <ButtonNot />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            // Really should not return WMC0020 "cannot assign to collection".  (future improvement?)
            string[] expectedErrors =
            {
                "WMC0001",  // Unknown type 'ButtonNot' in XML namespace 'http://schemas.microsoft.com/winfx/2006/xaml/presentation'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0001_UnknownObject4()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button Content='{Unknown}' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            // Really should not return WMC0020 "cannot assign to collection".  (future improvement?)
            string[] expectedErrors =
            {
                "WMC0001",  // Unknown type 'Unknown' in XML namespace 'http://schemas.microsoft.com/winfx/2006/xaml/presentation'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0005_NonPublicType()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button>
            <dll:InternalType/>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            // Error WMC0005 is currently inaccessible, masked by the less usefull WMC0001.  (future improvement?)
            string[] expectedErrors =
            {
                //"WMC0005",  // Cannot access non-public type 'InternalType' in XMLnamespace 'using:LibManagedDll'
                "WMC0001",  // Unknown type 'InternalType' in XML namespace 'using:LibManagedDll'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0010_UnknownAttachableMember()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button>
            <Button Content='Hello' Grid.NotRow='Blue' />
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            // Error WMC0005 is currently inaccessible, masked by the less usefull WMC0001.  (future improvement?)
            string[] expectedErrors =
            {
                "WMC0010",  // unknown attachable member 'Grid.NotRow' on element Button
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0011_UnknownMember()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
      <Button Content='Hello' BGrnd='Blue' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC0011",  // Unknown member 'BGrnd' on element 'Button'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void UnknownLocalMembersAreAllowedInPass1()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <dll:SomeType SomeProp='Hello'/>
        <Button dll:SomeProvider.SomeAP='foo' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, isPass1: true);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0015_CantAssign()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button Content='Hello'>
            <Button.Background>
                <Color>Red</Color>
            </Button.Background>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC0015",  // Can't assign 'Color' into property 'Background', type must be assignable to 'Brush'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0020_CantAddToCollectionProperty()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Brush>Red</Brush>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0020",  // Cannot assign 'Brush' into the collection property 'Children', type must be 'UIElement'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0021_CantAddToCollectionObject()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button>
            <dll:CollectionHolder>
                <dll:CollectionHolder.IntListProp>
                    <dll:IntList>
                        <x:String>Test</x:String>
                    </dll:IntList>
                </dll:CollectionHolder.IntListProp>
            </dll:CollectionHolder>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0021",  // Cannot add 'String' into the collection object 'IntList', type must be 'Int32'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0025_DictionaryItemsCannotBeText()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <x:String x:Key='abc'>ABC</x:String>
        Foo
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0025",  // dictionary Items cannot bo Text: 'Foo'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0026_CantAddToDictionaryProperty()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button>
            <dll:CollectionHolder>
                <dll:CollectionHolder.ButtonDictionaryProp>
                    <Button x:Key='sss'/>
                    <x:String x:Key='aaa'>ABC</x:String>
                </dll:CollectionHolder.ButtonDictionaryProp>
            </dll:CollectionHolder>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0026",  // Cannot add 'String' into the dictionary property 'ButtonDictionaryProp', type must be 'Button'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0027_CantAddToDictionaryObject()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button>
            <dll:CollectionHolder>
                <dll:CollectionHolder.ButtonDictionaryProp>
                    <dll:ButtonDictionary>
                        <Button x:Key='sss'/>
                        <x:String x:Key='aaa'>ABC</x:String>
                    </dll:ButtonDictionary>
                </dll:CollectionHolder.ButtonDictionaryProp>
            </dll:CollectionHolder>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0027",  // Cannot add 'String' into the dictionary object 'ButtonDictionary', type must be 'Button'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0030_IdPropertiesMustBeText()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <x:Name>
            <Color>Red</Color>
        </x:Name>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0030",  // Values for 'Name' property must be Text
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0035_DuplicationAssignment()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Grid.Height>400</Grid.Height>
        <Grid.Height>500</Grid.Height>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0035",  // Duplicate assignment to the 'Height' property of the 'Grid' object
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0040_BadName()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid x:Name='$$#' />
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0040",  // The value '$$#' is an invalid value from 'Name' on object 'Grid'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0045_CantNameValueTypes()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button>
            <Color x:Name='namedColor'>Red</Color>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0045",  // Type 'Color', and 'Value Types" in general, cannot use x:Name
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0050_ReadOnlyProperty()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <dll:ReadOnlyHolder x:Key='k0' ReadOnlyStringProp='55'>

            <dll:ReadOnlyHolder.ReadOnlySimpleClassProp>
                <dll:SimpleClass />
            </dll:ReadOnlyHolder.ReadOnlySimpleClassProp>

        </dll:ReadOnlyHolder>
    </Page.Resources>

</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedErrors =
            {
                "WMC0050",  // Cannot assign '55' into the read only property 'ReadOnlyStringProp'
                "WMC0050",  // Cannot assign 'SimpleClass' into the read only property 'ReadOnlySimpleClassProp'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0055_CantAssignTextToProperty()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button>
            <dll:CollectionHolder>
                <dll:CollectionHolder.IntListProp>
                    43
                </dll:CollectionHolder.IntListProp>
            </dll:CollectionHolder>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value '43' into the property 'IntListProp' of type 'IntList'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void CanAssignTextToPrimitiveIntegerProperties()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <Button>
            <dll:PrimitiveHolder
                Int16Prop='4'
                UInt16Prop='4'
                Char16Prop='A'
                Int32Prop='4'
                Int64Prop='4'
                UInt32Prop='4'
                UInt64Prop='4' />
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors = { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0060_XamlValidationDictionaryKeyError()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <SolidColorBrush Color='Red'/>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0060",  // Dictionary Item '{0}' must have a Key attribute
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC1500_XamlDeprecated()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

<Grid>
<GridView>
<GridView.GroupStyle>
<GroupStyle ContainerStyle='{x:Null}'>
</GroupStyle>
</GridView.GroupStyle>
</GridView>
</Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            Assert.IsTrue(validator.Warnings.Count == 1 && validator.Errors.Count == 0);
            Assert.IsTrue(validator.Warnings[0].ErrorCode == "WMC1500"); // XamlValidationWarningDeprecated
        }

        [TestMethod]
        public void WMC1501_XamlExperimental()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <ResourceDictionary>
            <dll:ExperimentalClass x:Key='a' />
        </ResourceDictionary>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            Assert.IsTrue(validator.Warnings.Count == 1 && validator.Errors.Count == 0);
            Assert.IsTrue(validator.Warnings[0].ErrorCode == "WMC1501"); // XamlValidationWarningExperimental
        }

        [TestMethod]
        public void WMC0001_UnresolvedAssemblyForwardedType()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <ResourceDictionary>
            <dll:ForwardedType x:Key='a' />
        </ResourceDictionary>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0001",
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0003_UnresolvedAssemblyForwardedTypeInherited()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <ResourceDictionary>
            <dll:InheritForwardedType x:Key='a' />
        </ResourceDictionary>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0003",
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0003_UnresolvedAssemblyForwardedTypeOnMember()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <ResourceDictionary>
            <dll:NonForwardedClass x:Key='a' />
        </ResourceDictionary>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            //Although NonForwardedClass has a property which uses a dangling forwarded type,
            //as long as we don't use the member we shouldn't have any errors
            string[] expectedErrors =
            {
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0003_UnresolvedAssemblyForwardedTypeOnUsedMember()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <ResourceDictionary>
            <dll:NonForwardedClass ForwardedTypeMember='{x:Null}' x:Key='a' />
        </ResourceDictionary>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0003",
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0908_XamlValidationDataTypeShouldBeUsedForDataTemplateOnly()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <ResourceDictionary>
            <ControlTemplate TargetType='Button' x:DataType='Grid' x:Key='ControlTemplate'>
                <TextBlock Text='{x:Bind Tag}'/>
            </ControlTemplate>
        </ResourceDictionary>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            Assert.IsTrue(validator.Errors.Count == 1 && validator.Warnings.Count == 0);
            Assert.IsTrue(validator.Errors[0].ErrorCode == "WMC0908"); // XamlValidationDataTypeOnlyAllowedOnDataTemplate
        }

        [TestMethod]
        public void WMC0065_XamlValidationDictionaryKeyError()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <SolidColorBrush x:Key='myKey' Color='Red'/>
        <Style x:Key='myKey' TargetType='Button'/>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0065",  // Dictionary Item '{0}' has duplicate key '{1}'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0070_XamlValidationErrorBadCPA()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <dll:BadCpaClass/>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0070",  // Invalid ContentPropertyAttribute on type '{0}', property '{1}' is not found
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0075_XamlValidationErrorMissingCPA()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <dll:MissingCPA>
            <Button />
        </dll:MissingCPA>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            // should not throw WMC0011, future inprovement.
            string[] expectedErrors =
            {
                "WMC0075",  // Missing Content Property definition for Element '{0}' to receive content '{1}'
                "WMC0011",  // Unknown member '_UnknownContent' on element 'MissingCPA'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0080_XamlValidationErrorStyleMustHaveTargetType()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Grid.Style>
            <Style />
        </Grid.Style>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0080",  // Style object must specify a String value for the TargetType property
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0085_XamlValidationErrorSetterMissingField()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Grid.Style>
            <Style TargetType='Grid'>
                <Setter Property='Height'              />
                <Setter                     Value='55' />

                <Setter Property='Height'>
                    <Setter.Value/>
                </Setter>

                <Setter Value='55'>
                    <Setter.Property/>
                </Setter>
            </Style>
        </Grid.Style>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0085",  // Setter object must specify a String value for the Property property.
                "WMC0086",  // Setter object must specify a value for the Value Property.
                "WMC0086",  // Setter object must specify a value for the Value Property.
                "WMC0085",  // Setter object must specify a String value for the Property property.
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0090_XamlValidationErrorSetterUnknownMember()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <Style TargetType='Button' x:Key='foo'>
            <Setter Property='Background' Value='Red' />
            <Setter Property='Margin2' Value='34' />
            <Setter Property='Foreground' Value='{x:Null}' />
        </Style>
    </Page.Resources>
    <Grid>
        <Grid.Style>
            <Style TargetType='Button'>
                <Setter Property='HeightFrom' Value='55' />
                <Setter Property='Grid.Row2'   Value='2' />
            </Style>
        </Grid.Style>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0090",  // Unknown 'Margin2'
                "WMC0090",  // Unknown member 'HeightFrom' on element 'Button'
                "WMC0091",  // Unknown attachable member 'Grid.Row' on element 'Button'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0095_XamlValidationErrorSetterSetterPropertyMustBeDP()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Page.Resources>
        <Style x:Key='key' TargetType='dll:SimpleClass'>
            <Setter Property='Prop1' Value='Hello' />
        </Style>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0095",  // Property 'Prop1' must be a DependencyProperty to be set with a Setter
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0100_XamlValidationErrorNotConstructibleObject()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <dll:AbstractBaseClass />
        <dll:NoZeroArgumentCtor />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            // There are two reasons to get this error, we test both.
            string[] expectedErrors =
            {
                "WMC0100",  // XAML AbstractBaseClass type cannot be constructed. ... etc
                "WMC0100",  // XAML NoZeroArgumentCtor type cannot be constructed. ... etc
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0105_XamlCompilerTypeMustHaveANamespace()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:'>
    <Page.Resources>
        <dll:OutSideAllNSes x:Key='key'/>
    </Page.Resources>

</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            // There are two reasons to get this error, we test both.
            string[] expectedErrors =
            {
                "WMC0105",  // Invalid class name 'OutSideAllNSes'. Types must be in a namespace to be referenced in XAML; they cannot be declared in the global namespace.
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0110_XamlValidationErrorUnknownStyleTargetType()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <Style x:Key='key' TargetType='Grid3'>
            <Setter Property='Height' Value='33' />
        </Style>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0110", // Unknown target type 'Grid3'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }


        // Error WMC0115 is unreachable, by design.  Error 0080 in the validator
        // it supposed to catch these and error 0115 is in the Type Collector if
        // something was missed.   Currently I think nothing is missed.
        // Note: These validator tests don't invoke the TypeInfoCollector
        //      so these test (in this module) can not hit WMC0115.
        [TestMethod]
        public void WMC0115_XamlCompilerErrorProcessingStyle()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <Style x:Key='key' />
        <Style x:Key='key1'>
            <Setter Property='Height' Value='33' />
        </Style>
        <Style x:Key='key2'>
            <Style.TargetType/>
        </Style>

    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            // error WMC0115 is unreachable
            string[] expectedErrors =
            {
                "WMC0080",  // Style object must specify a String value for the TargetType property
                "WMC0080",  // Style object must specify a String value for the TargetType property
                "WMC0080",  // Style object must specify a String value for the TargetType property
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0120_XamlCompileErrorInvalidPropertyType()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <dll:BadProps EnumProp='{StaticResource someEnum}' SignedByte='{StaticResource SByte}' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0120",  // Type Enum on property EnumProp is invalid.
                "WMC0121"   // Type SByte on property SignedByte is invalid.  Signed Char is not a valid WinRT type.
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0015_XamlValidationErrorEventValuesMustBeAssignableFromRoutedEventHandler()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Grid.Loaded><Button/></Grid.Loaded>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0015",  // Cannot assign 'Button' into property 'Loaded', type must be assignable to 'RoutedEventHandler'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0125_XamlValidationErrorEventValuesMustBeText()
        {
            string xaml = @"
<Page 
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button Click=''>Hello world</Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0125",  // Invalid value for '{0}'. Event values must be text
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0140_XamlValidationErrorStyleBasedOnMustBeStyle()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Page.Resources>
    <SolidColorBrush x:Key='green' Color='Green' />
    <Style x:Key='tbs' TargetType='TextBox' BasedOn='{StaticResource green}'>
      <Setter Property='Foreground' Value='White' />
    </Style>
  </Page.Resources>

    <Grid>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0140",  // Style BasedOn property must be a Style.  StaticResource 'green' resolves to a 'SolidColorBrush' not Style
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void XamlValidationStyleBasedOnCustomResourceIsLegal()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Page.Resources>
    <Style x:Key='tbs' TargetType='TextBox' BasedOn='{CustomResource MyCustomResource}'>
      <Setter Property='Foreground' Value='White' />
    </Style>
  </Page.Resources>

    <Grid>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0145_XamlValidationErrorStyleBasedOnBadStyleTargetType()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Page.Resources>
    <Style x:Key='bs' TargetType='Button'>
      <Setter Property='Background' Value='Blue' />
    </Style>
    <Style x:Key='tbs' TargetType='TextBox' BasedOn='{StaticResource bs}'>
      <Setter Property='Foreground' Value='White' />
    </Style>
  </Page.Resources>

    <Grid>      
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0145",  // Style TargetType 'TextBox' must be assignable to the BasedOn Style's TargetType 'Button'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0145_XamlValidationErrorStyleBasedOnBadStyleTargetTypeWithoutKey()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Page.Resources>
    <Style x:Name='bs' TargetType='Button'>
      <Setter Property='Background' Value='Blue' />
    </Style>
    <Style x:Key='tbs' TargetType='TextBox' BasedOn='{StaticResource bs}'>
      <Setter Property='Foreground' Value='White' />
    </Style>
  </Page.Resources>

    <Grid>      
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0145",  // Style TargetType 'TextBox' must be assignable to the BasedOn Style's TargetType 'Button'
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0905_XamlValidationErrorValidFieldModifier()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
      <Button x:Name='Button1' x:FieldModifier='protected' />
      <Button x:Name='Button2' x:FieldModifier='public' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);

        }

        [TestMethod]
        public void WMC0905_XamlValidationErrorInvalidFieldModifier()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <StackPanel>
        <Button x:Name='_button1' x:FieldModifier='public' />
        <Button x:Name='_button2' x:FieldModifier='garbage' />
        <Button                   x:FieldModifier='silly' />
    </StackPanel>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0905", // Invalid FieldModifier value: 'garbage' on '_button2'.  Must be private, public, protected, internal or friend.
                "WMC0905", // Invalid FieldModifier value: 'silly' on 'Button'.  Must be private, public, protected, internal or friend.
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0906_XamlValidationErrorLoadInvalidValue()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <StackPanel>
        <Button x:Load='SomeInvalidValue' x:Name='blah' />
    </StackPanel>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0906",
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0907_XamlValidationErrorMissingNameForLoad()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <StackPanel>
        <Button x:Load='True' />
    </StackPanel>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0907", // Element must have x:Name attribute specified since it uses x:Load.
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0913_XamlValidationErrorCannotHaveLoad()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <StackPanel>
        <StackPanel.Resources>
            <Button x:Load='True' x:Name='thisIsInADictionaryAndShouldntWork' />
        </StackPanel.Resources>
    </StackPanel>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0913", // x:Load can only be used on UIElement or FlyoutBase type elements that are not direct children of a resource dictionary
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0913_XamlValidation_LoadOnDTRoot()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <DataTemplate x:Key='FooBar' x:DataType='Grid'>
            <TextBlock x:Load='{x:Bind Tag}' x:Name='TB' Text='Foo'/>
        </DataTemplate>
    </Page.Resources>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0913", // x:Load on the root of a DT is invalid
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0913_XamlValidation_LoadOnPage()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Load='True'
    x:Name='Foo'>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0913", // x:Load on a page or user control
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0914_XamlValidationErrorLoadDeferMutex()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <StackPanel>
        <Button x:Load='True' x:Name='test' x:DeferLoadStrategy='Lazy' />
    </StackPanel>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0914", // An element cannot have both x:DeferLoadStrategy and x:Load set.
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC1118_XamlValidationErrorLoadTwoWay()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <StackPanel>
        <Button x:Load='{x:Bind Tag, Mode=TwoWay}' x:Name='test'/>
    </StackPanel>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC1118", // TwoWay binding target 'Load' must be a dependency property.
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void CheckIllegalXAttributesAreIllegal()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

    <Grid x:Arguments='a' x:AsyncRecords='10' x:ClassAttribute='foo' x:ClassModifier='cmod'
            x:Code='code' x:Members='mem' x:Subclass='sc' x:SynchronousMode='sync'
            x:TypeArguments='ta' x:FactoryMethod='fm'>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
                "WMC0011", // Unknown Member
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void StyleSetterDottedProperties0()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

    <Grid>
        <Grid.Style>
            <Style TargetType='FrameworkElement'>
                <Setter Property='ScrollViewer.IsDeferredScrollingEnabled' Value='True' />
                <Setter Property='Grid.Row' Value='1' />
            </Style>
        </Grid.Style>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void StyleSetterTargetInsteadOfProperty()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

    <Grid>
        <Grid.Style>
            <Style TargetType='FrameworkElement'>
                <Setter Target='Grid.Row' Value='1' />
            </Style>
        </Grid.Style>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void StyleSetterDottedProperties1()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

    <Grid>
        <Grid.Style>
            <Style TargetType='FrameworkElement'>
                <Setter Property='ScrollViewer.IsDeferredScrollingEnabled' Value='True' />
                <Setter Property='Button.Height' Value='1' />
            </Style>
        </Grid.Style>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0091"  // Unknown Attachable Member  (Button on FE)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void InvalidEnumValue()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel Orientation='NotVertical'>
            <Button/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void InvalidEnumValue2()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <StackPanel Orientation='Vertical'>
            <ContentControl>
                <dll:MyEnumTestClass MyEnumValue='MyEnum1,MyEnum2' />
            </ContentControl>
            <ContentControl>
                            <!-- Only this one should fail -->
                <dll:MyEnumTestClass MyEnumValue='MyEnum1,MyEnum2XX' />
            </ContentControl>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void InvalidEnumValue3()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid>
        <StackPanel Orientation='Vertical'>
            <ContentControl>
                <dll:MyEnumTestClass MyEnumValue='2' />
            </ContentControl>
            <ContentControl>
                <dll:MyEnumTestClass MyEnumValue='mYenUm1' />
            </ContentControl>
            <ContentControl>
                <dll:MyEnumTestClass MyEnumValue='mYenUm1,mYenuM2' />
            </ContentControl>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);

            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TextBox_InvalidEnumValue1()
        {
            //Test for TextBox with invalid enum value "WrapWholeWords"
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <TextBox TextWrapping = 'WrapWholeWords'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TextBox_InvalidEnumValue2()
        {
            //Test for TextBox with invalid enum value (WrapWholeWords case insensitivity check)
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <TextBox TextWrapping = 'WrapwhoLeWoRds'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TextBox_InvalidEnumValue3()
        {
            //Test for TextBox with invalid enum value (WrapWholeWorsds integer value check)
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <TextBox TextWrapping = '3'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TextBox_ValidEnumValue1()
        {
            //Test for TextBox with valid enum value "Wrap"
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <TextBox TextWrapping = 'Wrap'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void RichEditBox_InvalidEnumValue1()
        {
            //Test for RichEditBox with invalid enum value "WrapWholeWords"
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <RichEditBox TextWrapping = 'WrapWholeWords'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void RichEditBox_InvalidEnumValue2()
        {
            //Test for RichEditBox with invalid enum value (WrapWholeWords case insensitivity check)
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <RichEditBox TextWrapping = 'WrapwhoLeWords'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void RichEditBox_InvalidEnumValue3()
        {
            //Test for RichEditBox with invalid enum value (WrapWholeWords integer value check)
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <RichEditBox TextWrapping = '3'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0055",  // Cannot assign text value (bad enum value)
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void RichEditBox_ValidEnumValue1()
        {
            //Test for RichEditBox with valid enum value
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <RichEditBox TextWrapping = 'NoWrap'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TextBlock_ValidEnumValue1()
        {
            //Test for TextBlock with valid enum value
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <TextBlock TextWrapping = 'WrapWholeWords'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void RichTextBlock_ValidEnumValue1()
        {
            //Test for RichTextBlock with valid enum value
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <StackPanel>
            <RichTextBlock TextWrapping = 'WrapWholeWords'/>
        </StackPanel>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void CanAssignToNullableSystemProperties()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>

    <Grid>
        <ToggleButton IsChecked='true' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        // The compiler allows nullable types on custom types
        [TestMethod]
        public void CanAssignToNullableCustomProperties()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid >
        <dll:NullableTypeHolder  NullableInt='4'/>
        <dll:NullableTypeHolder>
            <dll:NullableTypeHolder.NullableInt>
                10
            </dll:NullableTypeHolder.NullableInt>
        </dll:NullableTypeHolder>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll, false, KnownVersions.RS5);

            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0151_TypeNotPresentInMinVersion()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'
    >    
    <Page.Resources>
        <dll:TypeNotInMinVersion x:Key='k0' />
    </Page.Resources>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedWarnings =
            {
                "WMC0151",  // TypeNotInMinVersion's contract version was higher than the min version's
            };
            string result = _testHelper.MatchErrors(validator, null, expectedWarnings);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        [Ignore]
        // From the comment in XamlDomValidator.ValidateTypePresentInMinVersion,
        // it seems that this test case is not actually relevant, since this error
        // will only ever be raised if a previously supported OS contract now no longer
        // exists (which should never be the case)
        public void WMC0152_NonExistingContractInMinVersion()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'
    >    
    <Page.Resources>
        <dll:NonExistingContractInMinVersion x:Key='k0' />
    </Page.Resources>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedWarnings =
            {
                "WMC0152",  // NonExistingContractInMinVersion is defined in a contract that doesn't exist at all in the min version
            };
            string result = _testHelper.MatchErrors(validator, null, expectedWarnings);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0151_MemberNotPresentInMinVersion()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'
    >    
    <Page.Resources>
        <dll:MemberNotPresentInMinVersion InvalidMember='deadbeef' x:Key='k0' />
    </Page.Resources>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedWarnings =
            {
                "WMC0151",  // The InvalidMember property on MemberNotPresentInMinVersion is defined in a contract that is higher than the min version's, so we can't set it
            };
            string result = _testHelper.MatchErrors(validator, null, expectedWarnings);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0151_InvalidMemberNotUsed()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'
    >    
    <Page.Resources>
        <dll:MemberNotPresentInMinVersion x:Key='k0' />
    </Page.Resources>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            //The InvalidMember property on MemberNotPresentInMinVersion is defined in a contract that is higher than the min version's, but since we never set it and the type is otherwise
            //valid we shouldn't have any errors
            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0155_ListViewCollectionCreatedWithSuccinctSyntax()
        {
            //Creating a valid ListView collection object using SuccinctCollectionSyntax
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    >   
    <ListView Items=""'Hello', 'world'"" />
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC0155_ColumnDefinitionsCollectionCreatedWithSuccinctSyntax()
        {
            //Creating a valid set of ColumnDefinitions using SuccinctCollectionSyntax
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    >  
    <Grid ColumnDefinitions=""100, Auto, *"" />
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        public void WMC0155_SuccinctSyntaxStringHelper(string inputString, string [] expectedErrors = null)
        {
            string xaml = System.String.Format(@"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    > 
    <ListView Items=""{0}"" />
</Page>
",inputString);
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            expectedErrors = expectedErrors?? new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);


        }

        [TestMethod]
        public void WMC0155_CheckSyntacticallyValidInput()
        {
            //Valid, values separated by comma
            WMC0155_SuccinctSyntaxStringHelper("1, '3,4', hello");

            //Valid, process escaped characters
            WMC0155_SuccinctSyntaxStringHelper("'hello, world\\'', hi");

            //Valid, process escaped characters and trim whitespace
            WMC0155_SuccinctSyntaxStringHelper("hello \\' world, 1 2");

            //Valid, process escaped characters
            WMC0155_SuccinctSyntaxStringHelper("1,\\[3,4\\]");

            //Valid, empty value 
            WMC0155_SuccinctSyntaxStringHelper("''"); 

        }

        [TestMethod]
        public void WMC0155_CheckSyntacticallyInvalidInput()
        {
            string[] expectedErrors = {
                "WMC0155"
            };
            //Unescaped bracket 
            WMC0155_SuccinctSyntaxStringHelper("1,[3,4]", expectedErrors);

            //Unmatched quote
            WMC0155_SuccinctSyntaxStringHelper("1,'2", expectedErrors); 
            WMC0155_SuccinctSyntaxStringHelper("1, 2'", expectedErrors);

            // No comma between values
            WMC0155_SuccinctSyntaxStringHelper("'1''2'", expectedErrors);

            //Extra single quote
            WMC0155_SuccinctSyntaxStringHelper("1, ''3'", expectedErrors); 
            WMC0155_SuccinctSyntaxStringHelper("1, '3''", expectedErrors);
            WMC0155_SuccinctSyntaxStringHelper("'1'', 3", expectedErrors);

            //Missing single quote
            WMC0155_SuccinctSyntaxStringHelper("1', '2'", expectedErrors);
            WMC0155_SuccinctSyntaxStringHelper("'1', 2'", expectedErrors);
            WMC0155_SuccinctSyntaxStringHelper("'1', '2", expectedErrors);
            WMC0155_SuccinctSyntaxStringHelper("'1, '2'", expectedErrors);

            //Comma at the end
            WMC0155_SuccinctSyntaxStringHelper("'1',", expectedErrors);

        }

        [TestMethod]
        public void WMC0155_ListViewDuplicateError()
        {
            //Creating a ListView combining both syntaxes, should result in an error
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    > 
    <ListView Items=""'Hello', 'world'"">
        < ListView.Items >
            < x:string> Hello </ x:string>
            < x:string> World </ x:string>
        </ ListView.Items >
    </ ListView >
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            string[] expectedErrors = {
                "WMC0155"
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void MinVersionNoCheckWithConditionals()
        {
            //If someone uses a conditional, we shouldn't do any min version validation.  All of these should fail unless they were using conditional markup.
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'
    xmlns:rs2 = 'using:LibManagedDll?IsApiContractPresent(Windows.Foundation.UniversalApiContract,4,0)'
     >    
    <Page.Resources>
        <rs2:MemberNotPresentInMinVersion InvalidMember='deadbeef' x:Key='k0' />
        <dll:MemberNotPresentInMinVersion rs2:InvalidMember='deadbeef' x:Key='k1' />
        <rs2:TypeNotInMinVersion x:Key='k2' />
        <rs2:NonExistingContractInMinVersion x:Key='k3' />
    </Page.Resources>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.LoadUserDll);
            //The InvalidMember property on MemberNotPresentInMinVersion is defined in a contract that is higher than the min version's, but since we never set it and the type is otherwise
            //valid we shouldn't have any errors
            string[] expectedErrors = new string[] { };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        // The compiler allows naming an element using X:Name and Name at the same time
        [TestMethod]
        public void CannotNameAnElementTwice()
        {
            // First Case : x:Name->Name
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid x:Name='Mango' Name='Apple'>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0046",  // Cannot name an element twice
            };

            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);

            // Second case Name->x:Name
            xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid Name='Apple' x:Name='Mango'>
    </Grid>
</Page>";
            validator = _testHelper.ValidateXAML(xaml);

            result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        // The compiler validator skips checking if an element's name is already used
        [TestMethod]
        public void ElementNameShouldBeUnique()
        {
            // First Case 
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button Name='Mango'>Red</Button>
        <Button x:Name= 'Mango' Name='Mango'>Red</Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string[] expectedErrors =
            {
                "WMC0046",  // Cannot name an element twice
                "WMC0047",   // Element Name is already used
                "WMC0047"   // Element Name is already used
            };

            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);

            // Second case 
            xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid Name='Apple'>
        <Button x:Name='Apple'>Red</Button>
    </Grid>
</Page>";
            validator = _testHelper.ValidateXAML(xaml);
            expectedErrors = new string[]
            {
                "WMC0047"   // Element Name is already used
            };

            result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);

            // Third case 
            xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
        <DataTemplate x:Key='first'>
            <Grid x:Name='Apple'/>
        </DataTemplate>
        <DataTemplate x:Key='second'>
            <Grid x:Name='Apple'/>
        </DataTemplate>
    </Page.Resources>
    <Grid>
        <Button x:Name='Apple'/>
    </Grid>
</Page>";
            validator = _testHelper.ValidateXAML(xaml);
            expectedErrors = new string[] { };

            result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void TwoWayXBindTargetMustBeADependencyProperty()
        {
            string xaml = @"
<Page
    x:Class='Foo.Bar'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    >
  <Grid>
       <TextBox SelectedText='{x:Bind Tag, Mode=TwoWay}'/>
    </Grid>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC1118", // TwoWay binding target 'SelectedText' must be a dependency property
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void XBindInResourceDictionaryWithoutCodeBehind()
        {
            string xaml = @"
<ResourceDictionary
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:local='using:App1'
    >
    <DataTemplate x:Key='FooBar' x:DataType='Grid'>
        <TextBlock Text='{x:Bind Tag}'/>
    </DataTemplate>
</ResourceDictionary>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC1119",
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void XBindNullTargetValueOnNonNullableTypes()
        {
            string xaml = @"
<Page
    x:Class='Foo.Bar'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    >
  <Grid>
       <TextBox Width='{x:Bind Width, TargetNullValue=1}'/>
    </Grid>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC1120",
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void XBindInsideXBind()
        {
            string xaml = @"
<Page
    x:Class='Foo.Bar'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    >
  <Grid>
       <TextBox Text='{x:Bind Tag, TargetNullValue={x:Bind Tag}}'/>
    </Grid>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC1122",
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void XDeferLoadStrategyOnValidAndInvalidElements()
        {
            string xaml = @"
<Page
    x:Class='Foo.Bar'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    >
    <Page.Resources>
        <Button x:Name='evilButton' x:DeferLoadStrategy='Lazy'/>
        <DataTemplate x:Key='clickDataTemplate'>
            <StackPanel>
                <Button x:Name='goodButton' x:DeferLoadStrategy='Lazy'/>
            </StackPanel>
        </DataTemplate>
    </Page.Resources>
    
    <StackPanel>
        <TextBlock>
            <TextBlock.Foreground>
                <SolidColorBrush Color='Red' x:DeferLoadStrategy='Lazy' x:Name='evilBrush'/>
            </TextBlock.Foreground>
        </TextBlock>
        <TextBlock x:Name='goodTextBlock' x:DeferLoadStrategy='Lazy'/>
    </StackPanel>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC0913",  // evilButton
                "WMC0913",  // evilBrush
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void DefaultBindModeInvalidValue()
        {
            string xaml = @"
<Page
    x:Class='Foo.Bar'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:DefaultBindMode='4d3d3d3'
    >    
    <StackPanel x:DefaultBindMode='oyster'>
        <TextBlock>
            <TextBlock.Foreground>
                <SolidColorBrush Color='Red' x:Name='evilBrush' x:DefaultBindMode='celery man'/>
            </TextBlock.Foreground>
        </TextBlock>
        <TextBlock x:Name='goodTextBlock'/>
    </StackPanel>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC0917",  // Page
                "WMC0917",  // StackPanel
                "WMC0917",  // evilBrush
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void DefaultBindModeValidValues()
        {
            string xaml = @"
<Page
    x:Class='Foo.Bar'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:DefaultBindMode='oneWAY'
    >    
    <StackPanel x:DefaultBindMode='ONEtime'>
        <TextBlock>
            <TextBlock.Foreground>
                <SolidColorBrush Color='Red' x:Name='evilBrush' x:DefaultBindMode='twoway'/>
            </TextBlock.Foreground>
        </TextBlock>
        <TextBlock x:Name='goodTextBlock'/>
    </StackPanel>
</Page>
";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void WMC1123_xBindOnControlTemplate()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyStuff.MyPage'>
    <Grid>
      <Button>
        <Button.Template>
            <ControlTemplate TargetType='Button' Grid.Row='{x:Bind}' >
                <TextBlock Text='{x:Bind Tag}'/>
            </ControlTemplate>
        </Button.Template>
      </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC1123",  // x:Bind only supported inside ControlTemplate
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void xBindInDataTemplateInControlTemplate()
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
                <Grid>
                  <Grid.Resources>
                     <DataTemplate x:Key='DT' x:DataType='Grid'>
                        <TextBlock Width='{x:Bind Width}'/>
                      </DataTemplate>
                  </Grid.Resources>
                </Grid>
            </ControlTemplate>
        </Button.Template>
      </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }
    }


}
