// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using System.Linq;
using System.Threading;
using Windows.Foundation.Metadata;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using MUXControls.TestAppUtils;
using PlatformConfiguration = Common.PlatformConfiguration;
using OSVersion = Common.OSVersion;
using System.Collections.Generic;
using XamlControlsResources = Microsoft.UI.Xaml.Controls.XamlControlsResources;
using Windows.UI.Xaml.Markup;
using System;
using Microsoft.UI.Xaml.Controls;
using System.Text;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class CommonStylesApiTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyAllThemesContainSameResourceKeys()
        {
            bool dictionariesContainSameElements = true;
            RunOnUIThread.Execute(() =>
            {
                var resourceDictionaries = new XamlControlsResources() { ControlsResourcesVersion = ControlsResourcesVersion.Version2 };
                Log.Comment("ThemeDictionaries");

                var defaultThemeDictionary = resourceDictionaries.ThemeDictionaries["Default"] as ResourceDictionary;

                foreach (var dictionaryName in resourceDictionaries.ThemeDictionaries.Keys)
                {
                    // Skip the Default theme dictionary
                    if (dictionaryName.ToString() == "Default")
                    {
                        continue;
                    }

                    Log.Comment("Comparing against " + dictionaryName.ToString());
                    var themeDictionary = resourceDictionaries.ThemeDictionaries[dictionaryName] as ResourceDictionary;

                    bool allKeysInDefaultExistInDictionary = ResourceDictionariesContainSameKeys(defaultThemeDictionary, "Default", themeDictionary, dictionaryName.ToString());
                    bool allKeysInDictionaryExistInDefault = ResourceDictionariesContainSameKeys(themeDictionary, dictionaryName.ToString(), defaultThemeDictionary, "Default");

                    dictionariesContainSameElements &= (allKeysInDefaultExistInDictionary && allKeysInDictionaryExistInDefault);
                }

                Verify.AreEqual(0, resourceDictionaries.MergedDictionaries.Count, "MergedDictionaries is not empty");
            });

            Verify.IsTrue(dictionariesContainSameElements, "Dictionaries don't contain the same keys");
        }

        bool ResourceDictionariesContainSameKeys(ResourceDictionary expectedDictionary, string expectedDictionaryName, ResourceDictionary actualDictionary, string actualDictionaryName)
        {
            List<string> missingKeysInActualDictionary = new List<string>();
            foreach (var entry in expectedDictionary)
            {
                if (!actualDictionary.ContainsKey(entry.Key))
                {
                    missingKeysInActualDictionary.Add(entry.Key.ToString());
                }
            }

            if (missingKeysInActualDictionary.Count > 0)
            {
                Log.Comment("Keys found in " + expectedDictionaryName + " but not in " + actualDictionaryName);
                foreach (var missingKey in missingKeysInActualDictionary)
                {
                    Log.Comment("* " + missingKey);
                }
            }

            return (missingKeysInActualDictionary.Count == 0);
        }

        [TestMethod]
        public void VerifyNoBaselineResourceWereRemovedFromPreviousStableReleaseInV2Styles()
        {
            EnsureNoMissingThemeResources(
                BaselineResources.BaselineResourcesList2dot5Stable,
                new XamlControlsResources() { ControlsResourcesVersion = ControlsResourcesVersion.Version2 });
        }

        [TestMethod]
        public void VerifyNoBaselineResourceWereRemovedFromPreviousStableReleaseInV1Styles()
        {
            EnsureNoMissingThemeResources(
                BaselineResources.BaselineResourcesList2dot5Stable,
                new XamlControlsResources() { ControlsResourcesVersion = ControlsResourcesVersion.Version1 });
        }

        private void EnsureNoMissingThemeResources(IList<string> baseline, XamlControlsResources dictionaryToVerify)
        {
            var actualResourcesKeys = new HashSet<string>();

            RunOnUIThread.Execute(() =>
            {
                var resourceDictionaries = dictionaryToVerify;

                foreach (var dictionaryName in resourceDictionaries.ThemeDictionaries.Keys)
                {
                    var themeDictionary = resourceDictionaries.ThemeDictionaries[dictionaryName] as ResourceDictionary;

                    foreach (var entry in themeDictionary)
                    {
                        string entryKey = entry.Key as string;
                        if (!actualResourcesKeys.Contains(entryKey))
                        {
                            actualResourcesKeys.Add(entryKey);
                        }
                    }
                }

                foreach (var entry in resourceDictionaries)
                {
                    string entryKey = entry.Key as string;
                    if (!actualResourcesKeys.Contains(entryKey))
                    {
                        actualResourcesKeys.Add(entryKey);
                    }
                }
            });

            StringBuilder missingKeysList = new StringBuilder();

            bool allBaselineResourceKeysExist = true;
            foreach (var baselineResourceKey in baseline)
            {
                if (!actualResourcesKeys.Contains(baselineResourceKey))
                {
                    missingKeysList.Append(baselineResourceKey + ", ");
                    allBaselineResourceKeysExist = false;
                }
            }

            Verify.IsTrue(allBaselineResourceKeysExist, "List of missing resource keys: " + missingKeysList.ToString());
        }

        public void DumpThemeResources()
        {
            RunOnUIThread.Execute(() =>
            {
                var resourceDictionary = new XamlControlsResources() { ControlsResourcesVersion = ControlsResourcesVersion.Version2 };
                
                Log.Comment("ThemeDictionaries");
                foreach (var key in resourceDictionary.ThemeDictionaries.Keys)
                {
                    Log.Comment("* " + key.ToString());

                    var themeDictionary = resourceDictionary.ThemeDictionaries[key] as ResourceDictionary;
                    foreach (var entry in themeDictionary)
                    {
                        Log.Comment("\t*" + entry.ToString());
                    }
                }

                Log.Comment("Entries in Resource Dictionary");
                foreach (var entry in resourceDictionary)
                {
                    Log.Comment("* " + entry.ToString());
                }
            });
        }

        [TestMethod]
        public void VerifyUseCompactResourcesAPI()
        {
            //Verify there is no crash and TreeViewItemMinHeight is not the same when changing UseCompactResources.
            RunOnUIThread.Execute(() =>
            {
                var dict = new XamlControlsResources();
                var height = dict["TreeViewItemMinHeight"].ToString();

                dict.UseCompactResources = true;
                var compactHeight = dict["TreeViewItemMinHeight"].ToString();
                Verify.AreNotEqual(height, compactHeight, "Height in Compact is not the same as default");
                Verify.AreEqual("24", compactHeight, "Height in 24 in Compact");

                dict.UseCompactResources = false;
                var height2 = dict["TreeViewItemMinHeight"].ToString();
                Verify.AreEqual(height, height2, "Height are the same after disabled compact");
            });

            MUXControlsTestApp.Utilities.IdleSynchronizer.Wait();
        }

        [TestMethod]
        public void CornerRadiusFilterConverterTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                Log.Comment("Corner radius is only available on RS5+");
                return;
            }

            RunOnUIThread.Execute(() => {
                var root = (StackPanel)XamlReader.Load(
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' 
                             xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                             xmlns:primitives='using:Microsoft.UI.Xaml.Controls.Primitives'> 
                            <StackPanel.Resources>
                                <primitives:CornerRadiusFilterConverter x:Key='TopCornerRadiusFilterConverter' Filter='Top' Scale='2'/>
                                <primitives:CornerRadiusFilterConverter x:Key='RightCornerRadiusFilterConverter' Filter='Right'/>
                                <primitives:CornerRadiusFilterConverter x:Key='BottomCornerRadiusFilterConverter' Filter='Bottom'/>
                                <primitives:CornerRadiusFilterConverter x:Key='LeftCornerRadiusFilterConverter' Filter='Left'/>
                                <CornerRadius x:Key='testCornerRadius'>6,6,6,6</CornerRadius>
                            </StackPanel.Resources>
                            <Grid x:Name='TopRadiusGrid'
                                CornerRadius='{Binding Source={StaticResource testCornerRadius}, Converter={StaticResource TopCornerRadiusFilterConverter}}'>
                            </Grid>
                            <Grid x:Name='RightRadiusGrid'
                                CornerRadius='{Binding Source={StaticResource testCornerRadius}, Converter={StaticResource RightCornerRadiusFilterConverter}}'>
                            </Grid>
                            <Grid x:Name='BottomRadiusGrid'
                                CornerRadius='{Binding Source={StaticResource testCornerRadius}, Converter={StaticResource BottomCornerRadiusFilterConverter}}'>
                            </Grid>
                            <Grid x:Name='LeftRadiusGrid'
                                CornerRadius='{Binding Source={StaticResource testCornerRadius}, Converter={StaticResource LeftCornerRadiusFilterConverter}}'>
                            </Grid>
                       </StackPanel>");

                var topRadiusGrid = (Grid)root.FindName("TopRadiusGrid");
                var rightRadiusGrid = (Grid)root.FindName("RightRadiusGrid");
                var bottomRadiusGrid = (Grid)root.FindName("BottomRadiusGrid");
                var leftRadiusGrid = (Grid)root.FindName("LeftRadiusGrid");

                Verify.AreEqual(new CornerRadius(12, 12, 0, 0), topRadiusGrid.CornerRadius);
                Verify.AreEqual(new CornerRadius(0, 6, 6, 0), rightRadiusGrid.CornerRadius);
                Verify.AreEqual(new CornerRadius(0, 0, 6, 6), bottomRadiusGrid.CornerRadius);
                Verify.AreEqual(new CornerRadius(6, 0, 0, 6), leftRadiusGrid.CornerRadius);
            });
        }

        //Task 30789390: Re-enable disabled tests
        //[TestMethod]
        public void VerifyVisualTreeForControlsInCommonStyles()
        {
            var controlsToVerify = new List<string> {
                "AppBarButton", "AppBarToggleButton", "Button", "CheckBox",
                "CommandBar", "ContentDialog", "DatePicker", "FlipView", "ListViewItem",
                "PasswordBox", "Pivot", "PivotItem", "RichEditBox", "Slider", "SplitView",
                "TextBox", "TimePicker", "ToolTip", "ToggleButton", "ToggleSwitch"};

            bool failed = false;

            foreach (var control in controlsToVerify)
            {
                try
                {
                    Log.Comment($"Verify visual tree for {control}");
                    VisualTreeTestHelper.VerifyVisualTree(xaml: XamlStringForControl(control), verificationFileNamePrefix: control);
                }
                catch (Exception e)
                {
                    failed = true;
                    Log.Error(e.Message);
                }
            }

            if(failed)
            {
                Verify.Fail("One or more visual tree verification failed, see details above");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled due to #2210: Unreliable test: CommonStylesApiTests.VerifyVisualTreeForCommandBarOverflowMenu
        public void VerifyVisualTreeForCommandBarOverflowMenu()
        {
            StackPanel root = null;
            CommandBar commandBar = null;
            UIElement overflowContent = null;

            RunOnUIThread.Execute(() => {
                root = (StackPanel)XamlReader.Load(
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' 
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                            <CommandBar x:Name='TestCommandBar'>
                                <AppBarButton Icon='AddFriend'/>
                                <AppBarButton Icon='World' Label='World'/>
                                <AppBarToggleButton Icon='Volume' Label='Volume'/>
                                <CommandBar.SecondaryCommands>
                                    <AppBarButton Label='Like'/>
                                    <AppBarButton Label='Dislike'/>
                                    <AppBarToggleButton Label='Toggle'/>
                                </CommandBar.SecondaryCommands>
                                <CommandBar.Content>
                                    <TextBlock Text='Hello World' Margin='12'/>
                                </CommandBar.Content>
                            </CommandBar>
                      </StackPanel>");

                commandBar = (CommandBar)root.FindName("TestCommandBar");
                Verify.IsNotNull(commandBar);
                Content = root;
                Content.UpdateLayout();
                commandBar.IsOpen = true;
                Content.UpdateLayout();
                var popup = VisualTreeHelper.GetOpenPopups(Window.Current).Last();
                Verify.IsNotNull(popup);
                overflowContent = popup.Child;
            });

            var visualTreeDumperFilter = new VisualTreeDumper.DefaultFilter();
            visualTreeDumperFilter.PropertyNameAllowedList.Remove("MaxWidth");
            visualTreeDumperFilter.PropertyNameAllowedList.Remove("MaxHeight");
            VisualTreeTestHelper.VerifyVisualTree(root: overflowContent, verificationFileNamePrefix: "CommandBarOverflowMenu", filter: visualTreeDumperFilter);
        }

        private string XamlStringForControl(string controlName)
        {
            return $@"<Grid Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                          <{controlName} />
                   </Grid>";
        }
    }

    class ControlVisualTreeTestFilter : VisualTreeDumper.DefaultFilter
    {
        public override bool ShouldVisitProperty(string propertyName)
        {
            return base.ShouldVisitProperty(propertyName) && !propertyName.Contains("RenderSize");
        }
    }

    [TestClass]
    public class CommonStylesVisualTreeTestSamples
    {
        //Task 30789390: Re-enable disabled tests
        //[TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // The default theme is different on OneCore, leading to a test failure.
        public void VerifyVisualTreeForAppBarAndAppBarToggleButton()
        {
            if (!PlatformConfiguration.IsOsVersion(OSVersion.Redstone5))
            {
                return;
            }

            var xaml = @"<Grid Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                            <StackPanel>
                                <AppBarButton Icon='Accept' Label='Accept'/>
                                <AppBarToggleButton Icon='Dislike' Label='Dislike'/>
                            </StackPanel>
                       </Grid>";
            VisualTreeTestHelper.VerifyVisualTree(xaml: xaml, 
                verificationFileNamePrefix: "VerifyVisualTreeForAppBarAndAppBarToggleButton");
        }

        [TestMethod]
        public void VerifyVisualTreeExampleLoadAndVerifyForAllThemes()
        {
            if (!PlatformConfiguration.IsOsVersion(OSVersion.Redstone5))
            {
                return;
            }

            var xaml = @"<Grid Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                       </Grid>";
            VisualTreeTestHelper.VerifyVisualTree(xaml: xaml, 
                verificationFileNamePrefix: "VerifyVisualTreeExampleLoadAndVerifyForAllThemes",
                theme: Theme.All);
        }

        [TestMethod]
        public void VerifyVisualTreeExampleLoadAndVerifyForDarkThemeWithCustomName()
        {
            if (!PlatformConfiguration.IsOsVersion(OSVersion.Redstone5))
            {
                return;
            }

            var xaml = @"<Grid Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                       </Grid>";
            UIElement root = VisualTreeTestHelper.SetupVisualTree(xaml);
            RunOnUIThread.Execute(() =>
            {
                (root as FrameworkElement).RequestedTheme = ElementTheme.Dark;
            });
            VisualTreeTestHelper.VerifyVisualTree(root: root,
                verificationFileNamePrefix: "VerifyVisualTreeExampleLoadAndVerifyForDarkThemeWithCustomName");
        }

        [TestMethod]
        public void VerifyVisualTreeExampleForLightTheme()
        {
            if (!PlatformConfiguration.IsOsVersion(OSVersion.Redstone5))
            {
                return;
            }

            var xaml = @"<Grid Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                       </Grid>";
            UIElement root = VisualTreeTestHelper.SetupVisualTree(xaml);
            VisualTreeTestHelper.VerifyVisualTree(root: root,
                verificationFileNamePrefix: "VerifyVisualTreeExampleForLightTheme",
                theme: Theme.Light);
        }

        // TODO: fix failing tests after color updates[TestMethod]
        // [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // The default theme is different on OneCore, leading to a test failure.
        public void VerifyVisualTreeExampleWithCustomerFilter()
        {
            if (!PlatformConfiguration.IsOsVersion(OSVersion.Redstone5))
            {
                return;
            }

            var xaml = @"<Grid Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                        <TextBlock Text='Abc' />
                       </Grid>";

            VisualTreeTestHelper.VerifyVisualTree(xaml: xaml,
                verificationFileNamePrefix: "VerifyVisualTreeExampleWithCustomerFilter",
                filter: new CustomizedFilter());
        }

        [TestMethod]
        // [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // The default theme is different on OneCore, leading to a test failure.
        public void VerifyVisualTreeExampleWithCustomerPropertyValueTranslator()
        {
            if (!PlatformConfiguration.IsOsVersion(OSVersion.Redstone5))
            {
                return;
            }

            var xaml = @"<Grid Width='400' Height='400' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> 
                            <TextBlock Text='Abc' />
                       </Grid>";

            VisualTreeTestHelper.VerifyVisualTree(xaml: xaml,
                verificationFileNamePrefix: "VerifyVisualTreeExampleWithCustomerPropertyValueTranslator",
                translator: new CustomizedTranslator());
        }

        class CustomizedFilter : VisualTreeDumper.IFilter
        {

            private static readonly string[] _propertyNamePostfixBlockList = new string[] { "Property", "Transitions", "Template", "Style", "Selector" };

            private static readonly string[] _propertyNameBlockList = new string[] { "Interactions", "ColumnDefinitions", "RowDefinitions",
            "Children", "Resources", "Transitions", "Dispatcher", "TemplateSettings", "ContentTemplate", "ContentTransitions",
            "ContentTemplateSelector", "Content", "ContentTemplateRoot", "XYFocusUp", "XYFocusRight", "XYFocusLeft", "Parent",
            "Triggers", "RequestedTheme", "XamlRoot", "IsLoaded", "BaseUri", "Resources"};

            private static readonly Dictionary<string, string> _knownPropertyValueDict = new Dictionary<string, string> {
                {"Padding", "0,0,0,0"},
                {"IsTabStop", "False" },
                {"IsEnabled", "True"},
                {"IsLoaded", "True" },
                {"HorizontalContentAlignment", "Center" },
                {"FontSize", "14" },
                {"TabIndex", "2147483647" },
                {"AllowFocusWhenDisabled", "False" },
                {"CharacterSpacing", "0" },
                {"BorderThickness", "0,0,0,0"},
                {"FocusState", "Unfocused"},
                {"IsTextScaleFactorEnabled", "True" },
                {"UseSystemFocusVisuals","False" },
                {"RequiresPointer","Never" },
                {"IsFocusEngagementEnabled","False" },
                {"IsFocusEngaged","False" },
                {"ElementSoundMode","Default" },
                {"CornerRadius","0,0,0,0" },
                {"BackgroundSizing","InnerBorderEdge" },
                {"Width","NaN" },
                {"Name","" },
                {"MinWidth","0" },
                {"MinHeight","0" },
                {"MaxWidth","∞" },
                {"MaxHeight","∞" },
                {"Margin","0,0,0,0" },
                {"Language","en-US" },
                {"HorizontalAlignment","Stretch" },
                {"Height","NaN" },
                {"FlowDirection","LeftToRight" },
                {"RequestedTheme","Default" },
                {"FocusVisualSecondaryThickness","1,1,1,1" },
                {"FocusVisualPrimaryThickness","2,2,2,2" },
                {"FocusVisualMargin","0,0,0,0" },
                {"AllowFocusOnInteraction","True" },
                {"Visibility","Visible" },
                {"UseLayoutRounding","True" },
                {"RenderTransformOrigin","0,0" },
                {"AllowDrop","False" },
                {"Opacity","1" },
                {"ManipulationMode","System" },
                {"IsTapEnabled","True" },
                {"IsRightTapEnabled","True" },
                {"IsHoldingEnabled","True" },
                {"IsHitTestVisible","True" },
                {"IsDoubleTapEnabled","True" },
                {"CanDrag","False" },
                {"IsAccessKeyScope","False" },
                {"ExitDisplayModeOnAccessKeyInvoked","True" },
                {"AccessKey","" },
                {"KeyTipHorizontalOffset","0" },
                {"XYFocusRightNavigationStrategy","Auto" },
                {"HighContrastAdjustment","Application" },
                {"TabFocusNavigation","Local" },
                {"XYFocusUpNavigationStrategy","Auto" },
                {"XYFocusLeftNavigationStrategy","Auto" },
                {"XYFocusKeyboardNavigation","Auto" },
                {"XYFocusDownNavigationStrategy","Auto" },
                {"KeyboardAcceleratorPlacementMode","Auto" },
                {"CanBeScrollAnchor","False" },
                {"Translation","<0, 0, 0>" },
                {"Scale","<1, 1, 1>" },
                {"RotationAxis","<0, 0, 1>" },
                {"CenterPoint","<0, 0, 0>" },
                {"Rotation","0" },
                {"TransformMatrix","{ {M11:1 M12:0 M13:0 M14:0} {M21:0 M22:1 M23:0 M24:0} {M31:0 M32:0 M33:1 M34:0} {M41:0 M42:0 M43:0 M44:1} }"},
            };

            public bool ShouldVisitPropertyValuePair(string propertyName, string value)
            {
                string v = _knownPropertyValueDict.ContainsKey(propertyName) ? _knownPropertyValueDict[propertyName] : VisualTreeDumper.ValueNULL;
                return !(v.Equals(value) || (!string.IsNullOrEmpty(value) && value.StartsWith("Exception")));
            }

            public bool ShouldVisitElement(string elementName)
            {
                return true;
            }

            public bool ShouldVisitProperty(string propertyName)
            {
                return (_propertyNamePostfixBlockList.Where(item => propertyName.EndsWith(item)).Count()) == 0 &&
                    !_propertyNameBlockList.Contains(propertyName);
            }
        }

        class CustomizedTranslator : VisualTreeDumper.DefaultPropertyValueTranslator // Add prefix MyValue to all Value
        {
            public override string PropertyValueToString(string propertyName, object value)
            {
                return "MyValue" + base.PropertyValueToString(propertyName, value);
            }
        }
    }
}
