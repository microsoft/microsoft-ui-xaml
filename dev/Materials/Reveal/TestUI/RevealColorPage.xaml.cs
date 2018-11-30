// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

#if !BUILD_WINDOWS
using RevealBrush = Microsoft.UI.Xaml.Media.RevealBrush;
using RevealTestApi = Microsoft.UI.Private.Media.RevealTestApi;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class RevealColorPage : TestPage
    {
        public RevealColorPage()
        {
            this.InitializeComponent();
            //_revealState.BorderLight.InnerConeColor = Colors.Red;
        }

        private RevealTestApi _revealState = new RevealTestApi();

        public RevealTestApi RevealState
        {
            get { return _revealState; }
        }

        public static IEnumerable<string> BackgroundBrushNames
        {
            get
            {
                return new List<string>() {
                    "SystemControlBackgroundBaseLowRevealBackgroundBrush",                    
                    "SystemControlTransparentRevealBackgroundBrush",
                    "SystemControlHighlightAccentRevealBackgroundBrush",
                    "SystemControlHighlightAccent3RevealBackgroundBrush",
                    "SystemControlHighlightAccent2RevealBackgroundBrush",
                    "SystemControlHighlightListMediumRevealBackgroundBrush",
                    "SystemControlHighlightListLowRevealBackgroundBrush", 
                    "SystemControlBackgroundBaseMediumLowRevealBaseLowBackgroundBrush",
                    "SystemControlHighlightBaseMediumLowRevealAccentBackgroundBrush",
                    "SystemControlHighlightListMediumRevealListLowBackgroundBrush",
                    "SystemControlHighlightAccent3RevealAccent2BackgroundBrush" };
            }
        }

        public static IEnumerable<string> BorderBrushNames
        {
            get
            {
                return new List<string>() {
                    "SystemControlBackgroundAccentRevealBorderBrush",
                    "SystemControlBackgroundBaseHighRevealBorderBrush",
                    "SystemControlBackgroundBaseLowRevealBorderBrush",
                    "SystemControlBackgroundBaseMediumRevealBorderBrush",
                    "SystemControlBackgroundBaseMediumHighRevealBorderBrush",
                    "SystemControlBackgroundBaseMediumLowRevealBorderBrush",
                    "SystemControlBackgroundChromeBlackHighRevealBorderBrush",
                    "SystemControlBackgroundChromeBlackMediumRevealChromeBorderBrush",
                    "SystemControlBackgroundChromeMediumRevealBorderBrush",
                    "SystemControlBackgroundChromeMediumLowRevealBorderBrush",
                    "SystemControlBackgroundChromeWhiteRevealBorderBrush",
                    "SystemControlTransparentRevealListLowBorderBrush",
                    "SystemControlBackgroundListLowRevealBorderBrush",
                    "SystemControlBackgroundListMediumRevealBorderBrush",
                    "SystemControlTransparentRevealBorderBrush",
                    "SystemControlBackgroundTransparentRevealBorderBrush",
                    "SystemControlHighlightTransparentRevealBorderBrush",
                    "SystemControlHighlightAltTransparentRevealBorderBrush",
                    "SystemControlHighlightAccentRevealBorderBrush",
                    "SystemControlHighlightBaseHighRevealBorderBrush",
                    "SystemControlHighlightBaseLowRevealBorderBrush",
                    "SystemControlHighlightBaseMediumRevealBorderBrush",
                    "SystemControlHighlightBaseMediumHighRevealBorderBrush",
                    "SystemControlHighlightBaseMediumLowRevealBorderBrush" };
            }
        }

        public static string GetBrushName(string fullName)
        {
            //SystemControlTransparentRevealBackgroundBrush
            //SystemControlBackgroundTransparentRevealBorderBrush
            if (fullName == null || fullName.Length <= 24)
                return string.Empty;

            return fullName.Substring(19, fullName.Length - 24);
        }

        public IEnumerable<KeyValuePair<string, Brush>> RevealBorderBrushes
        {
            get
            {
                foreach (var brushName in BorderBrushNames)
                {
                    var brush = Application.Current.Resources[brushName] as Brush;
                    if (brush != null)
                        yield return new KeyValuePair<string, Brush>(GetBrushName(brushName), brush);
                }
            }
        }

        public IEnumerable<KeyValuePair<string, Brush>> RevealBackgroundBrushes
        {
            get
            {
                foreach (var brushName in BackgroundBrushNames)
                {
                    var brush = Application.Current.Resources[brushName] as Brush;
                    if (brush != null)
                        yield return new KeyValuePair<string, Brush>(GetBrushName(brushName), brush);
                }
            }
        }

        // TODO: use Verify and have a helper to bubble failures back to test app
        private string _checkBrushResult = "";
        private void BrushCheckFailed(string message)
        {
            if (_checkBrushResult != "") _checkBrushResult += ";";
            _checkBrushResult += message;
        }

        private void BrushCheckFailed(string format, object arg0 = null, object arg1 = null)
        {
            BrushCheckFailed(string.Format(format, arg0, arg1));
        }

        private void OnCheckBrushResourcesClicked(object sender, RoutedEventArgs args)
        {
            using (var setup = new MaterialSetupHelper())
            {
                // Load up the resource dictionaries and do some consistency checks on them. Verify:
                // 1) All same resource names exist in all theme dictionaries (rs1, rs2) x (light, dark, hc)
                // 2) If a resource is a RevealBrush in Light that it's the same type of RevealBrush in Dark
                // 3) All RevealBrushes in Light should have TargetTheme=Light, same for Dark.
                // 4) Border-named brushes should be RevealBorderBrush, same for Background.

                try
                {
                    _checkBrushResult = "";

                    // RS1 should have no implicit keys.
                    int implicitKeys = RS1RevealThemeResourcesLight.Where(x => !(x.Key is string)).Count();
                    if (implicitKeys > 0)
                    {
                        BrushCheckFailed("Rs1 should have no implicit keys (had {0})", implicitKeys);
                    }

                    var rs1Keys = RS1RevealThemeResourcesLight.Keys.ToList();
                    var rs2Keys = RS2RevealThemeResourcesLight.Keys.ToList();

                    var uniqueKeysInRS1 = RS1RevealThemeResourcesLight.Where(x => !rs2Keys.Contains(x.Key)).ToList();
                    var uniqueKeysInRS2 = RS2RevealThemeResourcesLight.Where(x => !rs1Keys.Contains(x.Key)).Where(x => x.Key is string).ToList();

                    if (uniqueKeysInRS2.Count > 0)
                    {
                        BrushCheckFailed("RS2 should not have any keys that RS1 does not");
                    }

                    // Grab all the theme dictionaries -- for RS1 tack on any keys not in a theme dictionary and treat them as if they're in that theme dictionary.
                    var rs1dark = ((ResourceDictionary)RS1RevealThemeResourcesDark.ThemeDictionaries["Default"]).Concat(uniqueKeysInRS1).ToDictionary(x => x.Key, x => x.Value);
                    var rs1light = ((ResourceDictionary)RS1RevealThemeResourcesLight.ThemeDictionaries["Light"]).Concat(uniqueKeysInRS1).ToDictionary(x => x.Key, x => x.Value);
                    var rs1hc = ((ResourceDictionary)RS1RevealThemeResourcesLight.ThemeDictionaries["HighContrast"]).Concat(uniqueKeysInRS1).ToDictionary(x => x.Key, x => x.Value);
                    var rs2dark = ((ResourceDictionary)RS2RevealThemeResourcesDark.ThemeDictionaries["Default"]).ToDictionary(x => x.Key, x => x.Value);
                    var rs2light = ((ResourceDictionary)RS2RevealThemeResourcesLight.ThemeDictionaries["Light"]).ToDictionary(x => x.Key, x => x.Value);
                    var rs2hc = ((ResourceDictionary)RS2RevealThemeResourcesLight.ThemeDictionaries["HighContrast"]).ToDictionary(x => x.Key, x => x.Value);

                    // Choose the rs1 light keys as "truth" and check everyone else has those keys and doesn't have keys they shouldn't.
                    var expectedKeys = rs1light.Keys;
                    Action<string, Dictionary<object, object>> checkDictionary = (which, dict) =>
                        {
                            var unexpectedEntries = dict.Where(x => !expectedKeys.Contains(x.Key)).ToList();

                            foreach (var unexpected in unexpectedEntries)
                            {
                                BrushCheckFailed("Dictionary {0} has an entry it should not: {1}", which, unexpected.Key);
                            }

                            var missingEntries = expectedKeys.Where(x => !dict.ContainsKey(x)).ToList();
                            foreach (var missing in missingEntries)
                            {
                                BrushCheckFailed("Dictionary {0} should have had entry '{1}' but was missing", which, missing);
                            }
                        };

                    checkDictionary("rs1light", rs1light);
                    checkDictionary("rs2light", rs2light);

                    checkDictionary("rs1dark", rs1dark);
                    checkDictionary("rs2dark", rs2dark);

                    // NOTE: Doing this will cause the High Contrast brushes to load ThemeResource from Light/Dark instead of HighContrast.
                    // At the moment there's nothing we can do about that. But we can still check the keys in the dictionaries.
                    checkDictionary("rs1hc", rs1hc);
                    checkDictionary("rs2hc", rs2hc);

                    // Make sure there are no reveal brushes in high contrast or rs1.
                    foreach (var entry in rs1hc.Concat(rs1dark).Concat(rs1light))
                    {
                        var brush = entry.Value as RevealBrush;
                        if (brush != null)
                        {
                            BrushCheckFailed("RS1 theme dictionary had a RevealBrush entry {0}", entry.Key);
                        }
                    }

                    // Don't log failures for issues where the brush is intentionally different in RS1.
                    var brushesWithFallbackIssues = new HashSet<string> {
                    "SystemControlHighlightAccent2RevealBackgroundBrush",
                    "SystemControlHighlightAccent3RevealBackgroundBrush",
                    "SystemControlHighlightAccent3RevealAccent2BackgroundBrush",
                    "ListViewItemRevealBackgroundSelected",
                    "ListViewItemRevealBackgroundSelectedPointerOver",
                    "ListViewItemRevealBackgroundSelectedPressed",
                    "ComboBoxItemRevealBackgroundSelected",
                    "ComboBoxItemRevealBackgroundSelectedUnfocused",
                    "ComboBoxItemRevealBackgroundSelectedPointerOver",
                    "ComboBoxItemRevealBackgroundSelectedPressed",
                    "GridViewItemRevealBackgroundSelectedPointerOver",
                    "GridViewItemRevealBackgroundSelectedPressed",
                };

                    // Trying to dig the resources out of the dictionary simply doesn't work. We have to load XAML and put the element in the tree.
                    string snippetBegin = "<Grid RequestedTheme=\"{0}\" " +
                        " xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">" +
                        " <Grid.Resources><ResourceDictionary Source=\"{1}\"/></Grid.Resources>\r\n";

                    string snippetChildren = "";
                    foreach (var lightEntry in rs2light)
                    {
                        if (lightEntry.Value is RevealBrush)
                        {
                            snippetChildren += string.Format("<Grid Name=\"{0}\" Background=\"{{ThemeResource {0}}}\"/>\r\n", lightEntry.Key);
                        }
                    }

                    string snippetEnd = "</Grid>";
                    string rs1darksnippet = string.Format(snippetBegin, "Dark", "ms-appx:///RevealBrush_rs1_themeresources.xaml") + snippetChildren + snippetEnd;
                    string rs2darksnippet = string.Format(snippetBegin, "Dark", "ms-appx:///RevealBrush_rs2_themeresources.xaml") + snippetChildren + snippetEnd;
                    string rs1lightsnippet = string.Format(snippetBegin, "Light", "ms-appx:///RevealBrush_rs1_themeresources.xaml") + snippetChildren + snippetEnd;
                    string rs2lightsnippet = string.Format(snippetBegin, "Light", "ms-appx:///RevealBrush_rs2_themeresources.xaml") + snippetChildren + snippetEnd;
                    var darkGridRS1 = (Grid)XamlReader.Load(rs1darksnippet);
                    ResourceRootDarkRS1.Children.Add(darkGridRS1);
                    var darkGridRS2 = (Grid)XamlReader.Load(rs2darksnippet);
                    ResourceRootDarkRS2.Children.Add(darkGridRS2);
                    var lightGridRS1 = (Grid)XamlReader.Load(rs1lightsnippet);
                    ResourceRootLightRS1.Children.Add(lightGridRS1);
                    var lightGridRS2 = (Grid)XamlReader.Load(rs2lightsnippet);
                    ResourceRootLightRS2.Children.Add(lightGridRS2);

                    Dictionary<string, Grid> rs1DarkBrushes = new Dictionary<string, Grid>();
                    Dictionary<string, Grid> rs2DarkBrushes = new Dictionary<string, Grid>();
                    Dictionary<string, Grid> rs1LightBrushes = new Dictionary<string, Grid>();
                    Dictionary<string, Grid> rs2LightBrushes = new Dictionary<string, Grid>();

                    foreach (Grid grid in darkGridRS1.Children)
                    {
                        rs1DarkBrushes[grid.Name] = grid;
                    }

                    foreach (Grid grid in darkGridRS2.Children)
                    {
                        rs2DarkBrushes[grid.Name] = grid;
                    }

                    foreach (Grid grid in lightGridRS1.Children)
                    {
                        rs1LightBrushes[grid.Name] = grid;
                    }

                    foreach (Grid grid in lightGridRS2.Children)
                    {
                        rs2LightBrushes[grid.Name] = grid;
                    }

                    foreach (var lightEntry in rs1LightBrushes)
                    {
                        var darkRS1Brush = rs1DarkBrushes[(string)lightEntry.Key].Background as SolidColorBrush;
                        var darkBrush = rs2DarkBrushes[(string)lightEntry.Key].Background as RevealBrush;
                        var lightRS1Brush = rs1LightBrushes[(string)lightEntry.Key].Background as SolidColorBrush;
                        var lightBrush = rs2LightBrushes[(string)lightEntry.Key].Background as RevealBrush;

                        // Check that TargetTheme is set correctly.
                        if (lightBrush.TargetTheme != ApplicationTheme.Light)
                        {
                            BrushCheckFailed("RevealBrush for light theme did not have TargetTheme=Light: {0}", lightEntry.Key);
                        }

                        // Make sure that their fallback colors match their correspondents in RS1.
                        if (lightBrush.FallbackColor != lightRS1Brush.Color && !brushesWithFallbackIssues.Contains(lightEntry.Key))
                        {
                            BrushCheckFailed("FallbackColor for light theme did not match rs1 color: {0}", lightEntry.Key);
                        }

                        if (darkBrush == null || darkBrush.GetType() != lightBrush.GetType())
                        {
                            BrushCheckFailed("Light entry was RevealBrush but dark entry was not (or their types weren't the same): {0}", lightEntry.Key);
                        }
                        else
                        {
                            if (darkBrush.TargetTheme != ApplicationTheme.Dark)
                            {
                                BrushCheckFailed("RevealBrush for dark theme did not have TargetTheme=Dark: {0}", lightEntry.Key);
                            }
                            else
                            {
                                if (darkBrush.FallbackColor != darkRS1Brush.Color && !brushesWithFallbackIssues.Contains(lightEntry.Key))
                                {
                                    BrushCheckFailed("FallbackColor for dark theme did not match rs1 color: {0}", lightEntry.Key);
                                }
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    BrushCheckFailed("ERROR: " + e.ToString());
                }

                // Set at the end to make sure we only fire valuechanged once.
                CheckBrushesResult.TextWrapping = TextWrapping.Wrap;
                CheckBrushesResult.Text = _checkBrushResult;
            }
        }

        public string Blah
        {
            get
            {
                return "ListViewItemRevealBackground";
            }
        }
    }
}
