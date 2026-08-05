// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Win8Xaml.CompilerProxies;

namespace UnitTests
{
    [TestClass]
    public class XbfGeneratorTests
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        [TestMethod]
#if DO_NOT_USE_GENXBF
        [Ignore]
#endif
        public void CanCallXbfGen()
        {
            string genericXaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyNamespace.MyClass'>
    <Grid>
        <Button x:Name='btn' Click='ClickHandler' {0}/>
        <Button x:Name='btn2' Click='ClickHandler' Loaded='LoadedHandler' />
        <Button x:Name='btn3' x:FieldModifier='public' />
    </Grid>
</Page>";

            string rs1Xaml = string.Format(genericXaml, "");
            string rs3Xaml = string.Format(genericXaml, "XYFocusDownNavigationStrategy = 'NavigationDirectionDistance'");

            // RS3 Xaml should generate just fine in Latest
            var xbgGenerator = _testHelper.GenerateXbf(new Version(KnownVersions.Latest), rs3Xaml);
            Assert.AreEqual(0, xbgGenerator.XbfErrors.Count);

            // RS1 Xaml should generate just fine in Latest
            xbgGenerator = _testHelper.GenerateXbf(new Version(KnownVersions.Latest), rs1Xaml);
            Assert.AreEqual(0, xbgGenerator.XbfErrors.Count);
        }

        [TestMethod]
        [Ignore]
        public void IsTemplateCollectedOnce()
        {
            string genericXaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    x:Class='MyNamespace.MyClass'>
    <Grid>
        <Grid.Resources>
            <ControlTemplate x:Name='namedControlTemplate' TargetType='Button'>
                <StackPanel>
                    <TextBox Text='hello world' Height='{x:Bind Height}' />
                </StackPanel>
            </ControlTemplate>

            <DataTemplate x:Name='namedDataTemplate' x:DataType='Button'>
                <StackPanel>
                    <TextBox Text='hello world' Height='{x:Bind Height}' />
                </StackPanel>
            </DataTemplate>
        </Grid.Resources>
        <Button x:Name='btn' Click='ClickHandler' />
    </Grid>
</Page>";

            // If a template was harvested twice and had two different connection IDs on the same template,
            // GenXBF would error due to a duplicate attribute assignment.  Verify we only collected
            // each one once by verifying there are no errors.
            var xbgGenerator = _testHelper.GenerateXbf(new Version(KnownVersions.Latest), genericXaml);
            Assert.AreEqual(0, xbgGenerator.XbfErrors.Count);
        }
    }
}
