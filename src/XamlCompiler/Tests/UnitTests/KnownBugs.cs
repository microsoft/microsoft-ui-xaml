// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace UnitTests
{
    [TestClass]
    public class KnownBugs
    {
        TestHelper _testHelper;

        [TestInitialize]
        public void SchemaInit()
        {
            _testHelper = new TestHelper();
        }

        // Fixed: Unable to sometimes set some attachable properties with a Style
        [TestMethod]
        public void B461925_DottedPropertiesInSetters()
        {
            string xaml = @"
<Page
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <Grid>
        <Grid.Style>
            <Style TargetType='Grid'>
                <Setter Property='Grid.Row' Value='2' />
            </Style>
        </Grid.Style>
        <Button>
            <Button.Style>
                <Style TargetType='Button'>
                    <Setter Property='FrameworkElement.Height' Value='55' />
                </Style>
            </Button.Style>
        </Button>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml);

            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        // Schema doesn't handle Dictionaries with two Add methods.
        // Only one can be chosen, We should decide how to choose the right one.
        // We could still error if we still can't decide which to choose.
        // Currently System.Xaml throws, this test would test the clean error message
        // it *should* return.
        [TestMethod]
        [Ignore]
        public void Schema_b467742_Dictionary2AddsManaged()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:dll='using:LibManagedDll'>
    <Grid >
        <dll:DictionaryHolder>
            <dll:DictionaryHolder.DictionaryWith2Adds>
                <Button x:Key='theButton' />
                <x:String x:Key='theString'>Hello</x:String>
            </dll:DictionaryHolder.DictionaryWith2Adds>
        </dll:DictionaryHolder>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.ManagedRuntime | SchemaMode.LoadUserDll);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }

        // Schema doesn't handle Dictionaries with two Add methods.
        // Only one can be chosen, We should decide how to choose the right one.
        // We could still error if we still can't decide which to choose.
        // Currently System.Xaml throws, this test would test the clean error message
        // it *should* return.
        [TestMethod]
        [Ignore]
        public void Schema_b467742_Dictionary2AddsNative()
        {
            string xaml = @"
<Page
    x:Class='BlankCs01.BlankPage'
    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
    xmlns:wmd='using:LibManagedWinmd'>
    <Grid >
        <wmd:SchemaTypeHolder>
            <wmd:SchemaTypeHolder.DictionaryWith2Adds>
                <Button x:Key='theButton' />
                <x:String x:Key='theString'>Hello</x:String>
            </wmd:SchemaTypeHolder.DictionaryWith2Adds>
        </wmd:SchemaTypeHolder>
    </Grid>
</Page>";
            var validator = _testHelper.ValidateXAML(xaml, SchemaMode.NativeRuntime | SchemaMode.LoadUserWinmd);
            string result = _testHelper.MatchErrors(validator, null, null);
            Assert.IsNull(result, result);
        }
    }
}
