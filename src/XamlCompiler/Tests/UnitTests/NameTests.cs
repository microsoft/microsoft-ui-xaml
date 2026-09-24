// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class NameTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
        public void IncorrectNameValue()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid> <Button x:Name='{Binding}'/> </Grid>
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
        public void CorrectNameValue()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button x:Name='SomeThing'/>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /*Testing Name value with Unicode Characters case.
         Needed to confirm 'Name' string value validation is
         working for unicode characters.*/

        // 1. Grammatically correct unicode value
        [TestMethod]
        public void CorrectUniCodeNameValue()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Button x:Name='اهلا'/>
        <Button x:Name='لکھائی' />
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        
        // 2. Grammatically incorrect unicode value
        // Unicode string contains spaces
        [TestMethod]
        public void IncorrectUniCodeNameValue()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid> <Button x:Name='  اهلا'/> </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors =
            {
                "WMC0040",  // Invalid Value for 'Name' property
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }


        [TestMethod]
        public void ValidKeyValues()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
      <Button x:Key='Some Thing'/>
      <Button x:Key='12mn'/>
      <Button x:Key=' LeadingSpace'/>
      <Button x:Key='TrailingSpace '/>
      <Button x:Key='$#@]{} Total Garbage, but allowed' />
      <Button x:Key='لعمرك ما ضاقت بلاد بأهلها و لكن أحلام الرجال تضيق'/>
    </Page.Resources> 
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void InvalidKeyValues()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
      <Button x:Key='' />
    </Page.Resources> 
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string[] expectedErrors = 
            {
                "WMC0030"   // Values for 'Key' property must be Text
            };
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        [TestMethod]
        public void NullkeyValue()
        {
            string testName = null;
            Assert.IsFalse(XamlDomValidator.IsValidKeyIdentifierName(testName), @"Cannot have a null name");
        }

        [TestMethod]
        public void NullNameValue()
        {
            string testName = null;
            Assert.IsFalse(XamlDomValidator.IsValidIdentifierName(testName), @"Cannot have a null name");
        }

        [TestMethod]
        public void StartWithNumNameValue()
        {
            string testName = @"1abc";
            Assert.IsFalse(XamlDomValidator.IsValidIdentifierName(testName), @"Cannot have a name start with number");
        }

        [TestMethod]
        public void UniqueNames()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
      <Button x:Name='same'>Red</Button>
      <Button x:Name='same'>Yellow</Button>
      <Button x:Name='same'>Green</Button>
    </Grid> 
</Page>";
            string[] expectedErrors =
            {
                "WMC0047",   // Element Name is already used
                "WMC0047"   // Element Name is already used
            };

            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Test that we check for duplicate keys in a dictionary
        /// </summary>
        [TestMethod]
        public void Dictionary_UniqueKeys01()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
      <Brush x:Key='Some Thing'>Red</Brush>
      <Brush x:Key='12mm'>Yellow</Brush>
      <Brush x:Key='12mm'>Green</Brush>
    </Page.Resources> 
</Page>";
            string[] expectedErrors =
            {
                "WMC0065",  // Dictionary Item has Duplicate Key
            };

            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }


        /// <summary>
        /// TargetType is sometimes uses as a Key
        /// Check that we check for duplicate TargetTypes in a dictionary.
        /// </summary>
        [TestMethod]
        public void Dictionary_UniqueKeys02()
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
        <Style TargetType='Button' >
            <Setter Property='Background' Value='Cyan' />
        </Style>
    </Page.Resources> 
</Page>";
            string[] expectedErrors =
            {
                "WMC0065",  // Dictionary Item has Duplicate Key
            };

            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Check that duplicate target types are allowed if x:Key is used
        /// because x:Key overrides TargetType as the key value.
        /// </summary>
        [TestMethod]
        public void Dictionary_UniqueKeys03()
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
        <Style TargetType='Button' x:Key='OtherStyle'>
            <Setter Property='Background' Value='Cyan' />
        </Style>
    </Page.Resources> 
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Check that duplicate target types are allowed if x:Key is used
        /// because x:Name overrides TargetType as the key value.
        /// because x:Name is an alternate to x:Key
        /// </summary>
        [TestMethod]
        public void Dictionary_UniqueKeys04()
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
        <Style TargetType='Button' x:Name='OtherStyle'>
            <Setter Property='Background' Value='Cyan' />
        </Style>
    </Page.Resources> 
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Confirm that TargetType='Button' is a differnet key from x:Key='Button'
        /// </summary>
        [TestMethod]
        public void Dictionary_UniqueKeys05()
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
        <Style TargetType='Grid' x:Key='Button'>
            <Setter Property='Background' Value='Cyan' />
        </Style>
    </Page.Resources> 
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        /// <summary>
        /// Confrim that x:Key='Same' is a duplicate key to x:Name='Same'
        /// </summary>
        [TestMethod]
        public void Dictionary_UniqueKeys06()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Page.Resources>
      <Brush  x:Key='Same'>Yellow</Brush>
      <Brush x:Name='Same'>Green</Brush>
    </Page.Resources> 
</Page>";
            string[] expectedErrors =
            {
                "WMC0065",  // Dictionary Item has Duplicate Key
            };

            var validator = _testHelper.ValidateXAML(xaml);
            string result = _testHelper.MatchErrors(validator, expectedErrors, null);
            Assert.IsNull(result, result);
        }

    }
}
