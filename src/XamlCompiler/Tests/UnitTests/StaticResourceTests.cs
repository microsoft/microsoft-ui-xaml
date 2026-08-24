// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTests
{
    [TestClass]
    public class StaticResourceTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
        public void LookupStyleInResources01()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Page.Resources>
    <Style x:Key='bs' TargetType='TextBox'>
      <Setter Property='Background' Value='Blue' />
    </Style>
  </Page.Resources>

    <Grid>
      <Grid.Resources>
        <Style x:Key='tbs' TargetType='Button' BasedOn='{StaticResource bs}'>
          <Setter Property='Foreground' Value='White' />
        </Style>
      </Grid.Resources>
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
        public void LookupStyleInResources02()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
  <Page.Resources>
    <Style x:Key='tbs' TargetType='Button' BasedOn='{StaticResource bs}'>
        <Setter Property='Foreground' Value='White' />
    </Style>
    <Style x:Key='bs' TargetType='TextBox'>
      <Setter Property='Background' Value='Blue' />
    </Style>
  </Page.Resources>

    <Grid>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            // There should be no error because the bad style comes after the usage.
            // There could be a warning about "BasedOn not found" but we haven't implemented that.

            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }
    
    }
}
