// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using Common;
using MUXControlsTestApp.Utilities;

using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using RatingControl = Microsoft.UI.Xaml.Controls.RatingControl;
using PersonPicture = Microsoft.UI.Xaml.Controls.PersonPicture;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class ThemeResourcesTests : ApiTestBase
    {
        [ClassInitialize]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) { }

        [TestMethod]
        // Isolate this test because other tests might have run in this context and either loaded the brushes 
        // already (thereby invalidating what we're trying to test) or our changes will adversly affect other tests.
        // Because of the test isolation we cannot use the base classes Content property as it may not be initialized.
        [TestProperty("IsolationLevel", "Method")]
        public void VerifyOverrides()
        {
            RatingControl ratingControl = null;
            PersonPicture personPicture = null;
            Slider slider = null;
            Grid root = null;

            RunOnUIThread.Execute(() =>
            {
                var appResources = Application.Current.Resources;
                // 1) Override WinUI defined brush in App.Resources.
                appResources["RatingControlCaptionForeground"] = new SolidColorBrush(Colors.Orange);

                // 2) Override system brush used by WinUI ThemeResource.

                ((ResourceDictionary)appResources.ThemeDictionaries["Light"])["SystemAltHighColor"] = Colors.Green;
                ((ResourceDictionary)appResources.ThemeDictionaries["Default"])["SystemAltHighColor"] = Colors.Green;
                ((ResourceDictionary)appResources.ThemeDictionaries["HighContrast"])["SystemColorButtonTextColor"] = Colors.Green;

                // 3) Override brush name used by a system control
                appResources["SliderTrackValueFill"] = new SolidColorBrush(Colors.Purple);

                root = new Grid {
                    Background = new SolidColorBrush(Colors.AntiqueWhite),
                };

                StackPanel panel = new StackPanel { Orientation = Orientation.Vertical };
                panel.Children.Add(slider = new Slider());

                panel.Children.Add(ratingControl = new RatingControl() { Value = 2 });
                panel.Children.Add(personPicture = new PersonPicture());

                root.Children.Add(panel);
                // Add an element over top to prevent stray mouse input from interfering.
                root.Children.Add(new Button {
                    Background = new SolidColorBrush(Color.FromArgb(30, 0, 255, 0)),
                    HorizontalAlignment = HorizontalAlignment.Stretch,
                    VerticalAlignment = VerticalAlignment.Stretch
                });

                MUXControlsTestApp.App.TestContentRoot = root;
            });
            IdleSynchronizer.TryWait();

            //System.Threading.Tasks.Task.Delay(TimeSpan.FromSeconds(20)).Wait();

            RunOnUIThread.Execute(() =>
            {
                // 1) Verify that overriding WinUI defined brushes in App.Resources works.
                Verify.AreEqual(Colors.Orange, ((SolidColorBrush)ratingControl.Foreground).Color,
                    "Verify RatingControlCaptionForeground override in Application.Resources gets picked up by WinUI control");

                // 2) Verify that overriding a system color used by a WinUI control works.
                Verify.AreEqual(Colors.Green, ((SolidColorBrush)personPicture.Foreground).Color,
                    "Verify PersonPictureForegroundThemeBrush (which uses SystemAltHighColor) overridden in Application.Resources gets picked up by WinUI control");

                // 3) Verify that overriding a system brush used by a system control works.
                if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone1))
                {
                    // Below code is comment because of bug 19180323 and we expect the code to be enabled again after test case is moved to nuget testapp

                    //Verify.AreEqual(Colors.Purple, ((SolidColorBrush)slider.Foreground).Color,
                    //    "Verify Slider (which uses SliderTrackValueFill as its .Foreground) overridden in Application.Resources gets picked up by Slider control");
                }
                else
                {
                    // Before RS1, we used to reference system brushes directly.
                }

                Log.Comment("Setting Window.Current.Content = null");
                MUXControlsTestApp.App.TestContentRoot = null;
            });
            IdleSynchronizer.TryWait();
        }

        // TO DO [Task 30818605]: Disabling to replace with reflection-based tests in release test app
        // [TestMethod]
        public void VerifyRS2DefaultStyleDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone1))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS2DefaultStyleDictionaries(), "Microsoft.UI.Xaml/Themes/rs2_generic.xaml");
            }
        }

        [TestMethod]
        public void VerifyRS3DefaultStyleDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS3DefaultStyleDictionaries(), "Microsoft.UI.Xaml/Themes/rs3_generic.xaml");
            }
        }

        [TestMethod]
        public void VerifyRS4DefaultStyleDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS4DefaultStyleDictionaries(), "Microsoft.UI.Xaml/Themes/rs4_generic.xaml");
            }
        }

        [TestMethod]
        public void VerifyRS5DefaultStyleDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS5DefaultStyleDictionaries(), "Microsoft.UI.Xaml/Themes/rs5_generic.xaml");
            }
        }

        [TestMethod]
        public void Verify19H1DefaultStyleDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone5))
            {
                VerifyDictionariesWereMergedCorrectly(Get19H1DefaultStyleDictionaries(), "Microsoft.UI.Xaml/Themes/19h1_generic.xaml");
            }
        }

        [TestMethod]
        public void VerifyRS2ThemeResourceDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone1))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS2ThemeResourceDictionaries(), "Microsoft.UI.Xaml/Themes/rs2_themeresources.xaml");
            }
        }

        [TestMethod]
        public void VerifyRS3ThemeResourceDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS3ThemeResourceDictionaries(), "Microsoft.UI.Xaml/Themes/rs3_themeresources.xaml");
            }
        }

        [TestMethod]
        public void VerifyRS4ThemeResourceDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS4ThemeResourceDictionaries(), "Microsoft.UI.Xaml/Themes/rs4_themeresources.xaml");
            }
        }

        [TestMethod]
        public void VerifyRS5ThemeResourceDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                VerifyDictionariesWereMergedCorrectly(GetRS5ThemeResourceDictionaries(), "Microsoft.UI.Xaml/Themes/rs5_themeresources.xaml");
            }
        }

        [TestMethod]
        public void Verify19H1ThemeResourceDictionariesWereMergedCorrectly()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone5))
            {
                VerifyDictionariesWereMergedCorrectly(Get19H1ThemeResourceDictionaries(), "Microsoft.UI.Xaml/Themes/19h1_themeresources.xaml");
            }
        }

        #region Dictionary verification

        List<string> keysWithDifferentValues = new List<string>();

        private void VerifyDictionariesWereMergedCorrectly(List<string> dictionaries, string mergedDictionary)
        {
            RunOnUIThread.Execute(() =>
            {
                keysWithDifferentValues.Clear();
                TestDictionariesAgainstMergedDictionary(dictionaries, mergedDictionary);

                if (keysWithDifferentValues.Count > 0)
                {
                    string errorMessage = Environment.NewLine + "Verification FAILED.  The following keys were not the same in the merged dictionaries as in the unmerged dictionaries:" + Environment.NewLine;

                    foreach (string key in keysWithDifferentValues)
                    {
                        errorMessage += "    " + key;
                    }

                    errorMessage += Environment.NewLine;

                    Log.Comment(errorMessage);
                }
                else
                {
                    Log.Comment(Environment.NewLine + "Verification succeeded!" + Environment.NewLine);
                }
            });
        }

        private void TestDictionariesAgainstMergedDictionary(List<string> dictionaries, string mergedDictionary)
        {
            ResourceDictionary parsedMergedDictionary = new ResourceDictionary() { Source = new Uri("ms-appx:///" + mergedDictionary) };

            foreach (string dictionary in dictionaries)
            {
                Log.Comment(Environment.NewLine + "Testing {0} against {1}..." + Environment.NewLine, dictionary, mergedDictionary);
                ResourceDictionary parsedDictionary = new ResourceDictionary() { Source = new Uri("ms-appx:///" + dictionary) };

                List<string> keysToCheck = new List<string>();

                foreach (object themeDictionaryKey in parsedDictionary.ThemeDictionaries.Keys)
                {
                    string themeDictionaryKeyString = themeDictionaryKey as string;

                    ResourceDictionary themeDictionary = parsedDictionary.ThemeDictionaries[themeDictionaryKeyString] as ResourceDictionary;
                    keysToCheck.AddRange(SafeRetrieveKeys(themeDictionary).Where((key) => !keysToCheck.Contains(key)));
                }

                foreach (ResourceDictionary innerMergedDictionary in parsedDictionary.MergedDictionaries)
                {
                    keysToCheck.AddRange(SafeRetrieveKeys(innerMergedDictionary).Where((key) => !keysToCheck.Contains(key)));
                }
                
                keysToCheck.AddRange(SafeRetrieveKeys(parsedDictionary).Where((key) => !keysToCheck.Contains(key)));

                foreach (string key in keysToCheck)
                {
                    Log.Comment("Comparing key '{0}' between dictionaries...", key);

                    if (CompareObjects(parsedDictionary[key], parsedMergedDictionary[key], 0) == false)
                    {
                        keysWithDifferentValues.Add(key);
                    }
                }
            }
        }

        // There's a bug in RS1 where trying to iterate through a ResourceDictionary containing an element with no key
        // causes E_FAIL to be returned.  We'll just ignore those failures.
        private static IEnumerable<string> SafeRetrieveKeys(ResourceDictionary dictionary)
        {
            IEnumerator<object> keyEnumerator = dictionary.Keys.GetEnumerator();

            bool shouldContinue = true;

            while (shouldContinue)
            {
                string keyString = null;

                try
                {
                    shouldContinue = keyEnumerator.MoveNext();

                    if (!shouldContinue)
                    {
                        break;
                    }

                    keyString = keyEnumerator.Current as string;
                }
                catch (COMException e)
                {
                    if (e.HResult == -2147467259) // E_FAIL
                    {
                        shouldContinue = false;
                    }
                    else
                    {
                        throw;
                    }
                }

                if (keyString != null)
                {
                    yield return keyString;
                }
            }
        }

        // For whatever reason, our version of System.Type doesn't contain an IsValueType property,
        // so we need to do things the hard way.
        static Type[] valueTypes = new Type[]
        {
            typeof(Enum),
            typeof(String),
            typeof(Char),
            typeof(Guid),
            typeof(Boolean),
            typeof(Byte),
            typeof(Int16),
            typeof(Int32),
            typeof(Int64),
            typeof(Single),
            typeof(Double),
            typeof(Decimal),
            typeof(SByte),
            typeof(UInt16),
            typeof(UInt32),
            typeof(UInt64),
            typeof(DateTime),
            typeof(DateTimeOffset),
            typeof(TimeSpan),
        };

        // Some properties can end up creating a loop if we try to then reference their properties,
        // or are super noisy and take way too long to evaluate. We'll ignore them.
        static string[] ignoredProperties = new string[]
        {
            "Inverse",
            "RelativeTransform",
            "Transform",
        };

        private bool CompareObjects(object value, object mergedValue, int indentation)
        {
            // We'll first test equality.  Failure doesn't necessarily mean that the objects aren't the same,
            // since we may be dealing with a reference type, but success means we can immediately quit.
            if (value == mergedValue)
            {
                // We'll still call verify to log the values so we can make sure they look right.
                Verify.AreEqual(value, mergedValue);
            }
            else
            {
                // If one value is null and the other isn't, then we know we can immediately fail.
                if ((value == null && mergedValue != null) || (value != null && mergedValue == null))
                {
                    Verify.Fail("One value was null while the other wasn't, so these values aren't equal.");
                    return false;
                }

                // If the types aren't even the same, then we can just early-out and fail.
                Type type = value.GetType();
                Type mergedType = mergedValue.GetType();

                // If one value is null and the other isn't, then we know we can immediately fail.
                if (type != mergedType)
                {
                    Verify.Fail(string.Format("Types '{0}' and '{1}' do not match.", type.Name, mergedType.Name));
                    return false;
                }

                // If the two values weren't equal, but they're value types, then we can fal - testing for equality should've worked.
                // We'll call Verify.AreEqual() to get information about what the values were.
                if (valueTypes.Any((valueType) => valueType.IsAssignableFrom(type)))
                {
                    Verify.AreEqual(value, mergedValue);
                }
                else
                {
                    DependencyObject valueObject = value as DependencyObject;
                    DependencyObject mergedValueObject = mergedValue as DependencyObject;

                    PropertyInfo[] propertyInfoList = null;

                    if (valueObject != null && mergedValueObject != null)
                    {
                        propertyInfoList = type.GetProperties(BindingFlags.Static | BindingFlags.Public | BindingFlags.FlattenHierarchy).Where((pi) => pi.PropertyType == typeof(DependencyProperty)).ToArray();
                    }
                    else
                    {
                        propertyInfoList = type.GetProperties(BindingFlags.Instance | BindingFlags.Public | BindingFlags.FlattenHierarchy);
                    }

                    // If we've got a dependency object that we're checking, then we'll consider all of its dependency properties.
                    // We'll ignore any that aren't set.
                    foreach (PropertyInfo propertyInfo in propertyInfoList)
                    {
                        if (ignoredProperties.Contains(propertyInfo.Name))
                        {
                            continue;
                        }

                        object property = null;
                        object mergedProperty = null;

                        if (valueObject != null && mergedValueObject != null)
                        {
                            DependencyProperty propertyDP = propertyInfo.GetValue(valueObject) as DependencyProperty;

                            property = valueObject.ReadLocalValue(propertyDP);
                            mergedProperty = mergedValueObject.ReadLocalValue(propertyDP);

                            if (property == DependencyProperty.UnsetValue && mergedProperty == DependencyProperty.UnsetValue)
                            {
                                // If both values are unset, then we can just ignore this property.
                                continue;
                            }
                            else if ((property == DependencyProperty.UnsetValue && mergedProperty != DependencyProperty.UnsetValue) ||
                                (property != DependencyProperty.UnsetValue && mergedProperty == DependencyProperty.UnsetValue))
                            {
                                // If one value is unset and another is set, then we've found a difference, and can stop.
                                return false;
                            }
                        }
                        else
                        {
                            property = propertyInfo.GetValue(value);
                            mergedProperty = propertyInfo.GetValue(mergedValue);
                        }

                        string comparisonComment = "Comparing property '{0}' on objects...";

                        for (int i = 0; i < indentation; i++)
                        {
                            comparisonComment = "  " + comparisonComment;
                        }

                        Log.Comment(comparisonComment, propertyInfo.Name);
                        bool succeeded = CompareObjects(property, mergedProperty, indentation + 1);

                        if (!succeeded)
                        {
                            return false;
                        }
                    }

                    // Finally, we should check to see whether or not this is a collection.
                    // If it is, we should iterate over its items to make sure they're all equal too.
                    ICollection collection = value as ICollection;
                    ICollection mergedCollection = mergedValue as ICollection;

                    if (collection != null && mergedCollection != null)
                    {
                        List<object> list = new List<object>();
                        List<object> mergedList = new List<object>();

                        foreach (object o in collection)
                        {
                            list.Add(o);
                        }

                        foreach (object o in mergedCollection)
                        {
                            mergedList.Add(o);
                        }

                        for (int i = 0; i < list.Count; i++)
                        {
                            string comparisonComment = "Comparing element '{0}' in list...";

                            for (int j = 0; j < indentation; j++)
                            {
                                comparisonComment = "  " + comparisonComment;
                            }

                            Log.Comment(comparisonComment, i);
                            bool succeeded = CompareObjects(list[i], mergedList[i], indentation + 1);

                            if (!succeeded)
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            return true;
        }

        #endregion
    }
}
