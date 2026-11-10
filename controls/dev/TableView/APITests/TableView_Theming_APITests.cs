// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Media.Imaging;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using Windows.Foundation;
using Windows.UI;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewThemingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 13 of the TableView API test plan: theming, resources, density and high contrast.
    //
    // Four dictionaries own distinct things, and a test that looks in the wrong one finds nothing:
    //
    //   * CommonStyles\TabularSurfaces_themeresources.xaml - the brush palette. Every TabularSurface*
    //     key, in Default / Light / HighContrast. The canonical source.
    //   * TableView\TableView_themeresources.xaml - the metric tokens ONLY: row heights, font sizes,
    //     cell and header padding, the density variants, gripper width. Brushes are deliberately
    //     excluded; mirroring them would collide on duplicate keys during theme-XBF emission.
    //   * ResizeGripper\ResizeGripper_themeresources.xaml - the gripper's own styling keys
    //     (ResizeGripperSeparator*), so a gripper resolves them without TableView.
    //   * TableView\TableView.xaml (root) - last-resort re-resolving {ThemeResource} fallbacks for
    //     hosts that do not merge TabularSurfaces.
    //
    // All of them reach a consumer through TabularControlsResources, which the test app must merge
    // explicitly: App.xaml merges only XamlControlsResources. That is also why
    // CommonStylesTests.VerifyAllThemesContainSameResourceKeys does NOT cover any of these keys -
    // it builds a XamlControlsResources. VerifyTabularSurfaceKeysExistInEveryThemeDictionary below
    // is the only parity guard these keys have.
    [TestClass]
    public class TableViewThemingTests : TableViewApiTestBase
    {
        #region 13.1 Style resolution

        // TableView's own default style is already proven by VerifyTemplatePartsAfterTemplateApplication
        // in TableViewTests.cs - that test cannot pass unless the style applied. The value here is the
        // container types nothing else touches.
        //
        // Asserts the template AND a setter rather than just Style != null: a style can be attached and
        // still not be the intended one.
        [TestMethod]
        public void VerifyRowAndGroupHeaderDefaultStylesResolve()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                if (row == null)
                {
                    Verify.Fail("A grouped TableView should realize at least one TableViewRow.");
                    return;
                }

                Verify.IsNotNull(row.Template, "TableViewRow must resolve its default ControlTemplate.");
                Verify.AreEqual(new Thickness(0, 0, 0, 1), row.BorderThickness,
                    "TableViewRow's default style sets BorderThickness to 0,0,0,1, the bottom grid line the default GridLinesVisibility keeps.");

                var groupHeader = FindVisualChildrenByType<TableViewGroupHeader>(tableView).FirstOrDefault();
                if (groupHeader == null)
                {
                    Verify.Fail("A grouped TableView should realize at least one TableViewGroupHeader.");
                    return;
                }

                Verify.IsNotNull(groupHeader.Template, "TableViewGroupHeader must resolve its default ControlTemplate.");

                Verify.AreEqual(new Thickness(4, 10, 12, 10), groupHeader.Padding,
                    "TableViewGroupHeader's default style must take Padding from TableViewGroupHeaderPadding.");
            });
        }

        // Both types are MUX_INTERNAL but projected into the test app as Microsoft.UI.Private.Controls;
        // TableView_Sizing_APITests.cs already constructs a ResizeGripper the same way.
        [TestMethod]
        public void VerifySortIndicatorAndResizeGripperDefaultStylesResolve()
        {
            SortIndicator indicator = null;
            ResizeGripper gripper = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                indicator = new SortIndicator();
                gripper = new ResizeGripper { Width = 8, Height = 24 };

                var host = new StackPanel();
                host.Children.Add(indicator);
                host.Children.Add(gripper);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(indicator.Template,
                    "SortIndicator must resolve its default ControlTemplate, otherwise the sort glyph renders nothing.");
                Verify.IsNotNull(gripper.Template,
                    "ResizeGripper must resolve its default ControlTemplate, otherwise the resize affordance is invisible while still being draggable.");

                // A setter, not just the template: proves it is this style and not some other one.
                Verify.IsFalse(gripper.IsTabStop,
                    "The default ResizeGripper style sets IsTabStop=False; the header cell is the keyboard target.");
                Verify.IsFalse(indicator.IsHitTestVisible,
                    "The default SortIndicator style sets IsHitTestVisible=False; it is decorative.");
            });
        }

        #endregion

        #region 13.2 Theme resources

        // The single highest-value test in this category. A key present in Default but missing from
        // HighContrast is a CRASH at resolve time under that theme, and it is invisible in whichever
        // theme the author happened to be working in.
        //
        // Models CommonStylesTests.VerifyAllThemesContainSameResourceKeys, which does not reach these
        // keys at all - it builds a XamlControlsResources, and TableView's resources live in
        // TabularControlsResources. Covers the TableView* metric tokens in the same pass: same failure
        // mode, and no reason to walk the dictionaries twice.
        [TestMethod]
        public void VerifyTabularSurfaceKeysExistInEveryThemeDictionary()
        {
            RunOnUIThread.Execute(() =>
            {
                var resources = new TabularControlsResources();

                var themeDictionaries = new Dictionary<string, ResourceDictionary>();
                foreach (var themeName in new[] { "Default", "Light", "HighContrast" })
                {
                    if (!resources.ThemeDictionaries.ContainsKey(themeName))
                    {
                        Verify.Fail($"TabularControlsResources must declare a '{themeName}' theme dictionary. " +
                            "Once any merged dictionary declares Light, XAML stops treating Default as a Light fallback, " +
                            "so a missing theme silently drops every key from that theme.");
                        return;
                    }

                    var dictionary = resources.ThemeDictionaries[themeName] as ResourceDictionary;
                    if (dictionary == null)
                    {
                        Verify.Fail($"The '{themeName}' entry in TabularControlsResources.ThemeDictionaries must be a ResourceDictionary.");
                        return;
                    }

                    themeDictionaries[themeName] = dictionary;
                    Log.Comment($"{themeName}: {dictionary.Keys.Count} keys.");
                }

                var defaultKeys = CollectKeys(themeDictionaries["Default"]);

                // Sanity floor: an empty Default dictionary would make every comparison below pass.
                Verify.IsGreaterThan(defaultKeys.Count, 0,
                    "The Default theme dictionary must not be empty - TabularControlsResources failed to load its source.");

                foreach (var themeName in new[] { "Light", "HighContrast" })
                {
                    var themeKeys = CollectKeys(themeDictionaries[themeName]);

                    // Report names, not counts. A bare "counts differ" failure leaves the next person
                    // diffing three dictionaries by hand.
                    var missingFromTheme = defaultKeys.Except(themeKeys).OrderBy(key => key).ToList();
                    var missingFromDefault = themeKeys.Except(defaultKeys).OrderBy(key => key).ToList();

                    Verify.AreEqual(0, missingFromTheme.Count,
                        $"Keys defined in Default but missing from {themeName}: {string.Join(", ", missingFromTheme)}");
                    Verify.AreEqual(0, missingFromDefault.Count,
                        $"Keys defined in {themeName} but missing from Default: {string.Join(", ", missingFromDefault)}");
                }

                // Both families must actually be present, or the parity check above is vacuously true
                // for whichever one failed to merge.
                Verify.IsTrue(defaultKeys.Any(key => key.StartsWith("TabularSurface", StringComparison.Ordinal)),
                    "The brush palette from TabularSurfaces_themeresources.xaml must be part of TabularControlsResources.");
                Verify.IsTrue(defaultKeys.Contains("TableViewRowMinHeightCompact"),
                    "The metric tokens from TableView_themeresources.xaml must be part of TabularControlsResources.");
            });
        }

        // Reasoned from the high-contrast requirement itself rather than a spec sentence: the dictionary
        // is written this way throughout today, so this pins an existing, deliberate property.
        //
        // A hardcoded colour under high contrast ignores the user's chosen scheme. For a user who
        // requires high contrast that is not cosmetic - it can make a column of text unreadable against
        // its own background.
        [TestMethod]
        public void VerifyHighContrastBrushesUseSystemColors()
        {
            RunOnUIThread.Execute(() =>
            {
                var resources = new TabularControlsResources();
                EnsureTabularControlsResources();
                var highContrast = resources.ThemeDictionaries["HighContrast"] as ResourceDictionary;
                if (highContrast == null)
                {
                    Verify.Fail("TabularControlsResources must declare a HighContrast theme dictionary.");
                    return;
                }

                // Outside HC, the framework's system brushes intentionally contain placeholder
                // colors. Compare shared objects, not raw system colors from a different theme.
                var systemBrushes = SystemColorNames.Select(name => name + "Brush")
                    .Where(key => Application.Current.Resources.ContainsKey(key))
                    .Select(key => Application.Current.Resources[key] as SolidColorBrush)
                    .Where(brush => brush != null).ToList();
                Verify.IsGreaterThan(systemBrushes.Count, 0,
                    "Shared SystemColor* brushes must resolve from the application resources.");

                var unsharedBrushes = new List<string>();
                int checkedBrushes = 0;

                foreach (var key in CollectKeys(highContrast))
                {
                    // Only the canonical palette entries; metric and component-specific
                    // resources are covered separately.
                    if (!key.StartsWith("TabularSurface", StringComparison.Ordinal) ||
                        !key.EndsWith("Brush", StringComparison.Ordinal))
                    {
                        continue;
                    }

                    var brush = highContrast[key] as SolidColorBrush;
                    Verify.IsNotNull(brush, $"{key} must resolve to a SolidColorBrush.");
                    if (brush == null)
                    {
                        continue;
                    }

                    checkedBrushes++;

                    if (!systemBrushes.Any(shared => ReferenceEquals(shared, brush)))
                    {
                        unsharedBrushes.Add(key);
                    }
                }

                Verify.IsGreaterThan(checkedBrushes, 0,
                    "The HighContrast dictionary must contain SolidColorBrush entries; finding none means it did not load.");

                Verify.AreEqual(0, unsharedBrushes.Count,
                    "Every high-contrast brush must reuse a shared SystemColor* brush. Unshared: " +
                    string.Join(", ", unsharedBrushes));
            });
        }

        // Gridline and outer-frame values mirror Common_themeresources_any.xaml.
        [TestMethod]
        public void VerifyGridLineAndOuterBorderBrushDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();

                var dark = RequireBrush(resources, "Default", "TabularSurfaceGridLineBrush");
                var light = RequireBrush(resources, "Light", "TabularSurfaceGridLineBrush");
                var darkBorder = RequireBrush(resources, "Default", "TabularSurfaceOuterBorderBrush");
                var lightBorder = RequireBrush(resources, "Light", "TabularSurfaceOuterBorderBrush");
                var darkHeader = RequireBrush(resources, "Default", "TabularSurfaceHeaderSeparatorBrush");
                var lightHeader = RequireBrush(resources, "Light", "TabularSurfaceHeaderSeparatorBrush");
                var highContrast = RequireBrush(resources, "HighContrast", "TabularSurfaceGridLineBrush");
                var highContrastHeader = RequireBrush(resources, "HighContrast", "TabularSurfaceHeaderSeparatorBrush");
                var highContrastSection = RequireBrush(resources, "HighContrast", "TabularSurfaceGroupHeaderBorderBrush");
                var highContrastBorder = RequireBrush(resources, "HighContrast", "TabularSurfaceOuterBorderBrush");
                if (dark == null || light == null || darkBorder == null || lightBorder == null || darkHeader == null || lightHeader == null || highContrast == null || highContrastHeader == null || highContrastSection == null || highContrastBorder == null)
                {
                    return;
                }

                Verify.AreEqual(Color.FromArgb(0x15, 0xFF, 0xFF, 0xFF), dark.Color,
                    "Dark TabularSurfaceGridLineBrush must be DividerStrokeColorDefault (#15FFFFFF).");
                Verify.AreEqual(Color.FromArgb(0x0F, 0x00, 0x00, 0x00), light.Color,
                    "Light TabularSurfaceGridLineBrush must be DividerStrokeColorDefault (#0F000000).");
                Verify.AreEqual(Color.FromArgb(0x15, 0xFF, 0xFF, 0xFF), darkBorder.Color,
                    "The default Dark outer border must use DividerStrokeColorDefault (#15FFFFFF).");
                Verify.AreEqual(Color.FromArgb(0x0F, 0x00, 0x00, 0x00), lightBorder.Color,
                    "The default Light outer border must use DividerStrokeColorDefault (#0F000000).");
                Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorGrayTextColorBrush"), highContrastBorder),
                    "High Contrast outer borders must match the body's system disabled color.");
                Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorGrayTextColorBrush"), highContrast),
                    "High Contrast body gridlines must use the system disabled color.");
                Verify.AreEqual(highContrast.Color, highContrastHeader.Color,
                    "High Contrast header lines must match the body's system disabled color.");
                Verify.AreEqual(highContrast.Color, highContrastSection.Color,
                    "High Contrast section dividers must match the body's system disabled color.");
                var highContrastResources = (ResourceDictionary)resources.ThemeDictionaries["HighContrast"];
                Verify.AreEqual(new Thickness(0, 0, 0, 1),
                    (Thickness)highContrastResources["TabularSurfaceGroupHeaderBorderThickness"],
                    "High Contrast section headers must have only a bottom divider, not an extra top rule.");
                Verify.AreEqual(dark.Color, darkHeader.Color, "Dark header lines must match the body grid lines (DividerStrokeColorDefault).");
                Verify.AreEqual(light.Color, lightHeader.Color, "Light header lines must match the body grid lines (DividerStrokeColorDefault).");
            });
        }

        [TestMethod]
        public void VerifyBorderlessOuterBorderPreservesHighContrast()
        {
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();
                foreach (var theme in new[] { "Default", "Light", "HighContrast" })
                {
                    var brush = RequireBrush(resources, theme, "TabularSurfaceBorderlessOuterBorderBrush");
                    if (brush == null)
                    {
                        continue;
                    }

                    if (theme == "HighContrast")
                    {
                        Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorGrayTextColorBrush"), brush),
                            "The borderless option must preserve the High Contrast gray-text frame.");
                    }
                    else
                    {
                        Verify.AreEqual((byte)0, brush.Color.A,
                            "The borderless option must hide the frame in Light/Dark.");
                    }
                }

                var style = resources["BorderlessTableViewStyle"] as Style;
                Verify.IsNotNull(style, "TabularControlsResources must expose the borderless style.");
                if (style != null)
                {
                    Verify.AreEqual(1, style.Setters.Count, "The borderless style must change only BorderBrush.");
                    Verify.AreEqual(Control.BorderBrushProperty, (style.Setters[0] as Setter)?.Property);
                }
            });
        }

        [TestMethod]
        public void VerifyDisabledSelectionIndicatorReusesSharedBrushAcrossThemes()
        {
            Grid host = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                host = (Grid)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                    "<Border BorderBrush='{ThemeResource TabularSurfaceSelectionIndicatorDisabledBrush}' />" +
                    "<Border BorderBrush='{ThemeResource AccentFillColorDisabledBrush}' />" +
                    "</Grid>");
                LoadContent(host);
            });
            IdleSynchronizer.Wait();

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
            {
                RunOnUIThread.Execute(() => host.RequestedTheme = theme);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var indicator = ((Border)host.Children[0]).BorderBrush as SolidColorBrush;
                    var shared = ((Border)host.Children[1]).BorderBrush as SolidColorBrush;
                    Verify.IsNotNull(indicator);
                    Verify.IsNotNull(shared);
                    Verify.IsTrue(ReferenceEquals(shared, indicator),
                        $"{theme}: disabled selection must reuse the shared accent-disabled brush.");
                    Verify.AreEqual(RequireThemeColor(theme == ElementTheme.Dark ? "Default" : "Light",
                        "AccentFillColorDisabled"), indicator.Color,
                        $"{theme}: the shared brush must follow the requested theme.");
                });
            }
        }

        [TestMethod]
        public void VerifyOuterBorderTransparencyPreservesLayoutAndCustomization()
        {
            TableView tableView = null;
            Rect headerBounds = default;
            SolidColorBrush customBrush = null;
            Style borderlessStyle = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                borderlessStyle = (Style)new TabularControlsResources()["BorderlessTableViewStyle"];
                customBrush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                tableView = CreateThemingTable();
                tableView.RequestedTheme = ElementTheme.Light;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var header = GetHeaderCell(tableView, 0);
                headerBounds = header.TransformToVisual(tableView).TransformBounds(
                    new Rect(0, 0, header.ActualWidth, header.ActualHeight));
            });

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
            {
                RunOnUIThread.Execute(() => tableView.RequestedTheme = theme);
                IdleSynchronizer.Wait();
                foreach (var showBorder in new[] { true, false, true })
                {
                    RunOnUIThread.Execute(() => tableView.Style = showBorder ? null : borderlessStyle);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        Verify.AreEqual(theme, tableView.ActualTheme, "The table must follow the requested theme.");
                        var expected = showBorder
                            ? (theme == ElementTheme.Dark
                                ? Color.FromArgb(0x15, 0xFF, 0xFF, 0xFF)
                                : Color.FromArgb(0x0F, 0x00, 0x00, 0x00))
                            : Microsoft.UI.Colors.Transparent;
                        var brush = tableView.BorderBrush as SolidColorBrush;
                        Verify.IsNotNull(brush);
                        if (showBorder)
                        {
                            Verify.AreEqual(expected, brush.Color, "The default outer frame must be visible.");
                        }
                        else
                        {
                            Verify.AreEqual((byte)0, brush.Color.A, "The borderless style must hide the frame.");
                        }
                        var frame = VisualTreeHelper.GetChild(tableView, 0) as Border;
                        Verify.IsNotNull(frame, "The default template must retain its outer Border.");
                        Verify.AreEqual(new Thickness(1), frame.BorderThickness, "Keep the existing layout inset.");
                        Verify.AreEqual(tableView.BorderBrush, frame.BorderBrush, "The frame must remain template-bound.");
                        var header = GetHeaderCell(tableView, 0);
                        Verify.AreEqual(headerBounds, header.TransformToVisual(tableView).TransformBounds(
                            new Rect(0, 0, header.ActualWidth, header.ActualHeight)),
                            "Switching border styles or themes must not shift or resize header content.");
                    });
                }
            }

            RunOnUIThread.Execute(() =>
            {
                tableView.BorderBrush = customBrush;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var frame = VisualTreeHelper.GetChild(tableView, 0) as Border;
                Verify.AreEqual(customBrush, frame.BorderBrush, "A local BorderBrush must customize the visible border.");
                var header = GetHeaderCell(tableView, 0);
                Verify.AreEqual(headerBounds, header.TransformToVisual(tableView).TransformBounds(
                    new Rect(0, 0, header.ActualWidth, header.ActualHeight)),
                    "Restoring the border must not shift or resize header content.");
            });
        }

        [TestMethod]
        public void VerifyTerminalTrailingGridLinesStayHiddenWithBorderlessStyle()
        {
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            {
                foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
                {
                    TableView tableView = null;
                    Style borderlessStyle = null;
                    Rect terminalHeaderBounds = default;
                    RunOnUIThread.Execute(() =>
                    {
                        EnsureTabularControlsResources();
                        borderlessStyle = (Style)new TabularControlsResources()["BorderlessTableViewStyle"];
                        tableView = CreateThemingTable();
                        tableView.FlowDirection = flow;
                        tableView.RequestedTheme = theme;
                        foreach (var column in tableView.Columns)
                        {
                            column.Width = new GridLength(1, GridUnitType.Star);
                        }
                        LoadContent(tableView);
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var header = GetHeaderCell(tableView, 1);
                        terminalHeaderBounds = header.TransformToVisual(tableView).TransformBounds(
                            new Rect(0, 0, header.ActualWidth, header.ActualHeight));
                    });

                    foreach (var showBorder in new[] { true, false, true, false })
                    {
                        RunOnUIThread.Execute(() => tableView.Style = showBorder ? null : borderlessStyle);
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var context = $"{flow}, {theme}, border visible={showBorder}";
                            var header = GetHeaderCell(tableView, 1);
                            Verify.AreEqual(Visibility.Collapsed,
                                ((Border)header.FindVisualChildByName("PART_GridLine")).Visibility,
                                $"The terminal header divider must stay hidden at the viewport edge ({context}).");
                            Verify.AreEqual(Visibility.Visible,
                                ((Border)GetHeaderCell(tableView, 0).FindVisualChildByName("PART_GridLine")).Visibility,
                                $"The interior header divider must remain visible ({context}).");
                            Verify.AreEqual(terminalHeaderBounds, header.TransformToVisual(tableView).TransformBounds(
                                new Rect(0, 0, header.ActualWidth, header.ActualHeight)),
                                $"The border toggle must preserve column geometry ({context}).");
                            var rows = FindVisualChildrenByType<TableViewRow>(tableView).ToList();
                            Verify.IsGreaterThan(rows.Count, 0, "The table must realize rows.");
                            foreach (var row in rows)
                            {
                                Verify.IsNull(GetRowCell(row, 1).BorderBrush,
                                    $"The terminal body divider must stay hidden at the viewport edge ({context}).");
                                Verify.IsNotNull(GetRowCell(row, 0).BorderBrush,
                                    $"The interior body divider must remain visible ({context}).");
                            }
                        });
                    }

                    RunOnUIThread.Execute(() =>
                    {
                        foreach (var column in tableView.Columns)
                        {
                            column.Width = new GridLength(80);
                        }
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        Verify.AreEqual(Visibility.Visible,
                            ((Border)GetHeaderCell(tableView, 1).FindVisualChildByName("PART_GridLine")).Visibility,
                            "A column ending before the viewport edge must retain its divider.");
                        var rows = FindVisualChildrenByType<TableViewRow>(tableView).ToList();
                        Verify.IsGreaterThan(rows.Count, 0, "The table must realize rows.");
                        foreach (var row in rows)
                        {
                            Verify.IsNotNull(GetRowCell(row, 1).BorderBrush,
                                "A body cell ending before the viewport edge must retain its divider.");
                        }
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyTerminalBottomGridLineTracksArrangedFrameAndBorderVisibility()
        {
            foreach (var density in new[] { TableViewDensity.Standard, TableViewDensity.Compact, TableViewDensity.ExtraCompact })
            {
                TableView table = null;
                TableViewRow lastRow = null;
                Rect rowBounds = default;
                RunOnUIThread.Execute(() =>
                {
                    var items = Enumerable.Range(0, 40)
                        .Select(i => new Person { Name = $"Person {i}", Role = "Engineer" }).ToList();
                    table = CreateTableView(itemsSource: TableViewSource.From(items), height: 241.3333333333333);
                    table.Density = density;
                    table.RequestedTheme = ElementTheme.Light;
                    table.HighContrastAdjustment = ElementHighContrastAdjustment.None;
                    table.BorderBrush = new SolidColorBrush(Microsoft.UI.Colors.Black);
                    table.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
                    table.GridLinesVisibility = TableViewGridLinesVisibility.All;
                    table.Resources["TabularSurfaceGridLineBrush"] = new SolidColorBrush(Microsoft.UI.Colors.Red);
                    table.Columns[0].Width = new GridLength(1, GridUnitType.Star);
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => GetBodyScroller(table).ChangeView(null, double.MaxValue, null, true));
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    lastRow = FindVisualChildrenByType<TableViewRow>(table)
                        .Single(row => (row.DataContext as Person)?.Name == "Person 39");
                    rowBounds = lastRow.TransformToVisual(table).TransformBounds(
                        new Rect(0, 0, lastRow.ActualWidth, lastRow.ActualHeight));
                    var inner = (FrameworkElement)table.FindVisualChildByName("PART_LayoutRoot");
                    Verify.IsNotNull(inner, "The default template exposes its arranged inner frame.");
                    var innerBounds = inner.TransformToVisual(table).TransformBounds(
                        new Rect(0, 0, inner.ActualWidth, inner.ActualHeight));
                    Verify.IsLessThanOrEqual(Math.Abs(innerBounds.Bottom - rowBounds.Bottom),
                        1.03125 / table.XamlRoot.RasterizationScale,
                        "Precondition: the final row must meet the frame within scroll rounding.");
                    var separator = (Border)lastRow.FindVisualChildByName("PART_GridLineBorder");
                    Verify.AreEqual(0.0, separator.BorderThickness.Bottom,
                        $"{density}: only the outer frame should draw the terminal bottom edge.");
                });

                foreach (var gapPixels in new[] { 1.0, 3.0, 0.0 })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        lastRow.RenderTransform = new TranslateTransform
                        {
                            Y = -gapPixels / table.XamlRoot.RasterizationScale
                        };
                        table.BorderBrush = new SolidColorBrush(Microsoft.UI.Colors.Black);
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        Verify.AreEqual(gapPixels <= 1.0 ? 0.0 : 1.0,
                            ((Border)lastRow.FindVisualChildByName("PART_GridLineBorder")).BorderThickness.Bottom,
                            "Coalesce a scroll-rounding gap, but do not hide a distinct data boundary.");
                    });
                    if (gapPixels == 1.0)
                    {
                        var image = RenderElementPixels(table);
                        var sawFrame = false;
                        for (var y = Math.Max(0, image.Height - 15); y < image.Height; ++y)
                        {
                            var pixel = ReadRenderedPixel(image, image.Width / 2, y);
                            Verify.AreNotEqual(Microsoft.UI.Colors.Red, pixel,
                                "The terminal edge must not paint a second row-separator stroke.");
                            sawFrame |= pixel == Microsoft.UI.Colors.Black;
                        }
                        Verify.IsTrue(sawFrame, "The remaining terminal stroke must be the outer frame.");
                    }
                }

                foreach (var showBorder in new[] { false, true, false, true })
                {
                    RunOnUIThread.Execute(() => table.BorderBrush = new SolidColorBrush(
                        showBorder ? Microsoft.UI.Colors.Black : Microsoft.UI.Colors.Transparent));
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var separator = (Border)lastRow.FindVisualChildByName("PART_GridLineBorder");
                        Verify.AreEqual(showBorder ? 0.0 : 1.0, separator.BorderThickness.Bottom,
                            "A transparent frame must not replace the row's own separator.");
                        var bounds = lastRow.TransformToVisual(table).TransformBounds(
                            new Rect(0, 0, lastRow.ActualWidth, lastRow.ActualHeight));
                        Verify.AreEqual(rowBounds, bounds,
                            "Coalescing the border must not resize or move the row.");
                    });
                }

                RunOnUIThread.Execute(() => GetBodyScroller(table).ChangeView(null, 0, null, true));
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var firstRow = FindVisualChildrenByType<TableViewRow>(table)
                        .Single(row => (row.DataContext as Person)?.Name == "Person 0");
                    Verify.AreEqual(1.0,
                        ((Border)firstRow.FindVisualChildByName("PART_GridLineBorder")).BorderThickness.Bottom,
                        "Scrolling away must retain ordinary row separators.");
                });
            }
        }

        [TestMethod]
        public void VerifyTerminalBottomGridLinePreservesEmptySpace()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateThemingTable();
                table.BorderBrush = new SolidColorBrush(Microsoft.UI.Colors.Black);
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var lastRow = FindVisualChildrenByType<TableViewRow>(table)
                    .Single(row => (row.DataContext as Person)?.Name == "Mei");
                var bounds = lastRow.TransformToVisual(table).TransformBounds(
                    new Rect(0, 0, lastRow.ActualWidth, lastRow.ActualHeight));
                Verify.IsGreaterThan(table.ActualHeight - bounds.Bottom, 10.0,
                    "Precondition: there must be genuine empty space below the data.");
                Verify.AreEqual(1.0,
                    ((Border)lastRow.FindVisualChildByName("PART_GridLineBorder")).BorderThickness.Bottom,
                    "The final data row keeps its separator when it does not meet the frame.");
            });
        }

        [TestMethod]
        public void VerifyOuterBorderAndHeaderRuleUseTheirOwnBrushes()
        {
            var border = Color.FromArgb(0xFF, 0x20, 0x40, 0xC0);
            var gridLine = Color.FromArgb(0xFF, 0xC0, 0x40, 0x20);
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceOuterBorderBrush", new SolidColorBrush(border));
                SetApplicationResource("TabularSurfaceHeaderSeparatorBrush", new SolidColorBrush(gridLine));

                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(border, (tableView.BorderBrush as SolidColorBrush)?.Color ?? default,
                        "The outer border must come from TabularSurfaceOuterBorderBrush.");

                    var headerRow = tableView.FindVisualChildByName("PART_HeaderGridLine") as Border;
                    Verify.IsNotNull(headerRow, "The template must contain PART_HeaderGridLine.");
                    Verify.AreEqual(gridLine, (headerRow?.BorderBrush as SolidColorBrush)?.Color ?? default,
                        "The header bottom rule must come from TabularSurfaceHeaderSeparatorBrush.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    ClearApplicationResource("TabularSurfaceOuterBorderBrush");
                    ClearApplicationResource("TabularSurfaceHeaderSeparatorBrush");
                });
            }
        }

        // TableView-dev-spec.md:227-231 makes overriding a TabularSurface* key THE supported way to
        // restyle the table, in place of a monolithic style. If the control caches or hardcodes the
        // brush instead of resolving it, that story is fiction.
        //
        // The gridline brush is held in the per-instance resource cache, so override BEFORE load;
        // overriding afterwards tests the invalidation path, which is
        // VerifyThemeChangeAfterLoadReResolvesRowsAndHeaders' job.
        [TestMethod]
        public void VerifyGridLineBrushOverrideChangesRenderedSeparators()
        {
            var expected = Color.FromArgb(0xFF, 0xFF, 0x00, 0x99);
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceGridLineBrush", new SolidColorBrush(expected));

                tableView = CreateThemingTable();
                Verify.AreEqual(TableViewGridLinesVisibility.All, tableView.GridLinesVisibility,
                    "This test relies on the default including vertical lines; the per-cell separator is where the brush lands.");

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                    if (row == null)
                    {
                        Verify.Fail("The TableView should realize at least one row.");
                        return;
                    }

                    var wrapper = GetRowCell(row, 0);
                    var brush = wrapper.BorderBrush as SolidColorBrush;
                    if (brush == null)
                    {
                        Verify.Fail("A cell wrapper should carry a SolidColorBrush vertical separator when GridLinesVisibility includes Vertical.");
                        return;
                    }

                    Verify.AreEqual(expected, brush.Color,
                        "An app-scope TabularSurfaceGridLineBrush override must reach the rendered cell separator.");
                });
            }
            finally
            {
                // Application.Resources is UI-thread affine; a finally block runs on the test thread.
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceGridLineBrush"));
            }
        }

        [TestMethod]
        public void VerifyHeaderGridLinesAreAlwaysVisibleIndependentOfBodyGridLines()
        {
            var separatorColor = Color.FromArgb(0xFF, 0x12, 0x9E, 0x5A);
            var cases = new[]
            {
                TableViewGridLinesVisibility.None,
                TableViewGridLinesVisibility.Horizontal,
                TableViewGridLinesVisibility.Vertical,
                TableViewGridLinesVisibility.All,
            };

            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceHeaderSeparatorBrush", new SolidColorBrush(separatorColor));

                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            try
            {
                foreach (var body in cases)
                {
                    foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            tableView.GridLinesVisibility = body;
                            tableView.FlowDirection = flow;
                            Content.UpdateLayout();
                        });

                        IdleSynchronizer.Wait();

                        RunOnUIThread.Execute(() =>
                        {
                            var context = $"body {body}, flow {flow}";
                            bool bodyVertical = body == TableViewGridLinesVisibility.All || body == TableViewGridLinesVisibility.Vertical;
                            bool bodyHorizontal = body == TableViewGridLinesVisibility.All || body == TableViewGridLinesVisibility.Horizontal;

                            var separator = GetHeaderCell(tableView, 0).FindVisualChildByName("PART_GridLine") as Border;
                            if (separator == null)
                            {
                                Verify.Fail("Every header cell must contain its separator.");
                                return;
                            }

                            Verify.AreEqual(Visibility.Visible, separator.Visibility,
                                $"Header column separators remain visible regardless of body gridlines ({context}).");
                            Verify.AreEqual(VerticalAlignment.Stretch, separator.VerticalAlignment, "Header separators are full height.");
                            Verify.AreEqual(separatorColor, (separator.Background as SolidColorBrush)?.Color ?? default,
                                "Header separators paint TabularSurfaceHeaderSeparatorBrush.");

                            var headerRow = tableView.FindVisualChildByName("PART_HeaderGridLine") as Border;
                            Verify.IsNotNull(headerRow, "The template must contain PART_HeaderGridLine.");
                            if (headerRow != null)
                            {
                                Verify.AreEqual(new Thickness(0, 0, 0, 1), headerRow.BorderThickness,
                                    $"The header bottom rule is a 1px overlay ({context}).");
                                Verify.AreEqual(separatorColor, (headerRow.BorderBrush as SolidColorBrush)?.Color ?? default,
                                    $"The header bottom rule remains visible ({context}).");
                            }

                            var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                            if (row == null)
                            {
                                Verify.Fail("The TableView should realize at least one row.");
                                return;
                            }

                            Verify.AreEqual(bodyHorizontal, row.BorderThickness.Bottom > 0.0,
                                $"Row dividers must follow GridLinesVisibility ({context}).");
                            Verify.AreEqual(bodyVertical, GetRowCell(row, 0).BorderThickness.Right > 0.0,
                                $"Body column separators must follow GridLinesVisibility ({context}).");
                        });
                    }
                }
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceHeaderSeparatorBrush"));
            }
        }

        [TestMethod]
        public void VerifyHeaderGridLinesSurviveTemplateAndColumnChanges()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateThemingTable();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.None;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
            {
                RunOnUIThread.Execute(() =>
                {
                    tableView.RequestedTheme = theme;
                    tableView.FlowDirection = theme == ElementTheme.Dark ? FlowDirection.RightToLeft : FlowDirection.LeftToRight;
                    tableView.Columns[0].Visibility = Visibility.Collapsed;
                    var template = tableView.Template;
                    tableView.Template = null;
                    tableView.Template = template;
                    tableView.ApplyTemplate();
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var headerRow = tableView.FindVisualChildByName("PART_HeaderGridLine") as Border;
                    Verify.IsNotNull(headerRow);
                    Verify.IsNotNull(headerRow.BorderBrush, "Body None must not hide the header rule after retemplating.");
                    var visibleHeaders = FindVisualChildrenByType<TableViewColumnHeader>(tableView)
                        .Where(header => header.Visibility == Visibility.Visible).ToList();
                    Verify.AreEqual(tableView.Columns.Count - 1, visibleHeaders.Count);
                    foreach (var columnHeader in visibleHeaders)
                    {
                        var separator = columnHeader.FindVisualChildByName("PART_GridLine") as Border;
                        Verify.IsNotNull(separator, "Visible headers must realize their separators after retemplating.");
                        if (columnHeader != visibleHeaders.Last())
                        {
                            Verify.AreEqual(Visibility.Visible, separator.Visibility,
                                "Interior separators remain visible after retemplating, including in RTL.");
                        }
                    }

                    tableView.Columns[0].Visibility = Visibility.Visible;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var headerRow = tableView.FindVisualChildByName("PART_HeaderGridLine") as Border;
                    var separator = GetHeaderCell(tableView, 0).FindVisualChildByName("PART_GridLine") as Border;
                    Verify.IsNotNull(headerRow.BorderBrush, "Showing horizontal gridlines must restore the live themed brush.");
                    Verify.AreEqual(Visibility.Visible, separator.Visibility, "Showing vertical gridlines must restore rebuilt separators.");
                    Verify.AreEqual(TableViewGridLinesVisibility.None, tableView.GridLinesVisibility,
                        "Header changes must leave body gridlines unchanged.");
                });
            }
        }

        [TestMethod]
        public void VerifyTransparentHeaderSeparatorBrushHidesHeaderLines()
        {
            var transparent = Color.FromArgb(0x00, 0xFF, 0xFF, 0xFF);
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceHeaderSeparatorBrush", new SolidColorBrush(transparent));

                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var separator = GetHeaderCell(tableView, 0).FindVisualChildByName("PART_GridLine") as Border;
                    Verify.IsNotNull(separator, "Every header cell must contain its separator.");
                    Verify.AreEqual(transparent, (separator?.Background as SolidColorBrush)?.Color ?? default,
                        "Overriding TabularSurfaceHeaderSeparatorBrush with Transparent must hide the header separators.");

                    var headerRow = tableView.FindVisualChildByName("PART_HeaderGridLine") as Border;
                    Verify.IsNotNull(headerRow, "The template must contain PART_HeaderGridLine.");
                    Verify.AreEqual(transparent, (headerRow?.BorderBrush as SolidColorBrush)?.Color ?? default,
                        "Overriding TabularSurfaceHeaderSeparatorBrush with Transparent must hide the header bottom rule.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceHeaderSeparatorBrush"));
            }
        }

        [TestMethod]
        public void VerifyGridLineBrushAppOverrideWinsInsideElementTheme()
        {
            var expected = Color.FromArgb(0xFF, 0x66, 0x22, 0xAA);
            TableView tableView = null;
            Grid host = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceGridLineBrush", new SolidColorBrush(expected));

                tableView = CreateThemingTable();
                host = new Grid { RequestedTheme = ElementTheme.Light };
                host.Children.Add(tableView);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var actual = RequireCellSeparatorColor(tableView, "with an app override inside a Light subtree");

                    Verify.AreEqual(expected, actual,
                        "A direct app-scope TabularSurfaceGridLineBrush override must win even when the table is inside an element-scoped theme.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceGridLineBrush"));
            }
        }

        [TestMethod]
        public void VerifyElementThemeDictionaryBeatsFartherApplicationResource()
        {
            var fartherAppColor = Color.FromArgb(0xFF, 0x66, 0x22, 0xAA);
            var nearerElementThemeColor = Color.FromArgb(0xFF, 0x11, 0xCC, 0x77);
            TableView tableView = null;
            Grid host = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceGridLineBrush", new SolidColorBrush(fartherAppColor));

                tableView = CreateThemingTable();
                host = new Grid { RequestedTheme = ElementTheme.Light };
                host.Resources = new ResourceDictionary();
                host.Resources.ThemeDictionaries["Light"] = new ResourceDictionary
                {
                    ["TabularSurfaceGridLineBrush"] = new SolidColorBrush(nearerElementThemeColor),
                };
                host.Children.Add(tableView);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var actual = RequireCellSeparatorColor(tableView, "with a nearer element theme dictionary and a farther app resource");

                    Verify.AreEqual(nearerElementThemeColor, actual,
                        "A nearer element-scoped ThemeDictionaries entry must win over a farther plain Application.Resources entry.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceGridLineBrush"));
            }
        }

        // Framework order is own entry, then MergedDictionaries last-to-first. An own entry that is the
        // same object as an EARLIER (non-winning) merged entry must still be own; deciding own-ness by
        // "matches any merged entry" would skip it and return the winning merged entry instead.
        [TestMethod]
        public void VerifyOwnEntryBeatsMergedEntryEvenWhenAnotherMergedEntryMatches()
        {
            var ownColor = Color.FromArgb(0xFF, 0x11, 0xAA, 0x55);
            var winningMergedColor = Color.FromArgb(0xFF, 0xAA, 0x33, 0x11);
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                var ownBrush = new SolidColorBrush(ownColor);
                tableView = CreateThemingTable();
                var host = new Grid();
                host.Resources = new ResourceDictionary { ["TabularSurfaceGridLineBrush"] = ownBrush };
                host.Resources.MergedDictionaries.Add(new ResourceDictionary { ["TabularSurfaceGridLineBrush"] = ownBrush });
                host.Resources.MergedDictionaries.Add(new ResourceDictionary { ["TabularSurfaceGridLineBrush"] = new SolidColorBrush(winningMergedColor) });
                host.Children.Add(tableView);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var actual = RequireCellSeparatorColor(tableView, "with an own entry shared by a non-winning merged dictionary");
                Verify.AreEqual(ownColor, actual,
                    "An own ResourceDictionary entry must win over MergedDictionaries, even when an earlier merged dictionary holds the same object.");
            });
        }

        // Under Dark the framework picks ThemeDictionaries["Dark"] ahead of ["Default"], per element.
        // The differing "Default" entry stops a lookup that only probes "Default" from passing.
        [TestMethod]
        public void VerifyElementDarkThemeDictionaryOverrideWins()
        {
            var darkColor = Color.FromArgb(0xFF, 0x22, 0x88, 0xDD);
            var defaultColor = Color.FromArgb(0xFF, 0xDD, 0x44, 0x11);
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                tableView = CreateThemingTable();
                var host = new Grid { RequestedTheme = ElementTheme.Dark };
                host.Resources = new ResourceDictionary();
                host.Resources.ThemeDictionaries["Dark"] = new ResourceDictionary
                {
                    ["TabularSurfaceGridLineBrush"] = new SolidColorBrush(darkColor),
                };
                host.Resources.ThemeDictionaries["Default"] = new ResourceDictionary
                {
                    ["TabularSurfaceGridLineBrush"] = new SolidColorBrush(defaultColor),
                };
                host.Children.Add(tableView);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(ElementTheme.Dark, tableView.ActualTheme, "Precondition: the table must be in a Dark subtree.");

                var actual = RequireCellSeparatorColor(tableView, "with an element-scoped Dark theme dictionary");
                Verify.AreEqual(darkColor, actual,
                    "Under Dark, an ancestor's ThemeDictionaries[\"Dark\"] entry must win over its [\"Default\"] entry and the TableView default.");
            });
        }

        [TestMethod]
        public void VerifyGridLineBrushSameColorOverrideKeepsObjectAndOpacity()
        {
            var themeColor = Color.FromArgb(0x0F, 0x00, 0x00, 0x00);
            SolidColorBrush expectedBrush = null;
            TableView tableView = null;
            Grid host = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                expectedBrush = new SolidColorBrush(themeColor) { Opacity = 0.42 };
                SetApplicationResource("TabularSurfaceGridLineBrush", expectedBrush);

                tableView = CreateThemingTable();
                host = new Grid { RequestedTheme = ElementTheme.Light };
                host.Children.Add(tableView);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                    if (row == null)
                    {
                        Verify.Fail("The TableView should realize at least one row.");
                        return;
                    }

                    var wrapper = GetRowCell(row, 0);
                    var brush = wrapper.BorderBrush as SolidColorBrush;
                    if (brush == null)
                    {
                        Verify.Fail("A cell wrapper should carry a SolidColorBrush vertical separator when GridLinesVisibility includes Vertical.");
                        return;
                    }

                    Verify.AreEqual(themeColor, brush.Color,
                        "The override deliberately uses the same color as the Light theme default.");
                    Verify.AreEqual(expectedBrush.Opacity, brush.Opacity,
                        "A direct app-scope brush override must not be discarded just because its Color equals the theme default.");
                    Verify.AreSame(expectedBrush, brush,
                        "The rendered separator must keep the exact app-scope brush object so non-color brush properties are preserved.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceGridLineBrush"));
            }
        }

        // TableView-dev-spec.md:123 names both keys as part of what "the primitive owns", which makes
        // them a published surface rather than an internal detail. If they are inert a host cannot match
        // the separator to its design language.
        [TestMethod]
        public void VerifyResizeGripperSeparatorResourcesApply()
        {
            var expectedColor = Color.FromArgb(0xFF, 0x00, 0xCC, 0x44);
            var expectedThickness = new Thickness(0, 0, 5, 0);
            ResizeGripper gripper = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("ResizeGripperSeparatorBrush", new SolidColorBrush(expectedColor));
                SetApplicationResource("ResizeGripperSeparatorThickness", expectedThickness);

                gripper = new ResizeGripper { Width = 8, Height = 24 };
                LoadContent(gripper);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var brush = gripper.BorderBrush as SolidColorBrush;
                    if (brush == null)
                    {
                        Verify.Fail("The default ResizeGripper style must set BorderBrush from ResizeGripperSeparatorBrush.");
                        return;
                    }

                    Verify.AreEqual(expectedColor, brush.Color,
                        "An app-scope ResizeGripperSeparatorBrush override must win over the primitive's own theme dictionary.");

                    // The named state target from the template contract in ResizeGripper.idl. The
                    // OrientationStates group applies the thickness, and DragOrientation defaults to
                    // Horizontal, so the horizontal key is the one in play.
                    var separator = gripper.FindVisualChildByName("Separator") as Border;
                    if (separator == null)
                    {
                        Verify.Fail("The ResizeGripper template must contain the 'Separator' part its visual states target by name.");
                        return;
                    }

                    Verify.AreEqual(expectedThickness, separator.BorderThickness,
                        "An app-scope ResizeGripperSeparatorThickness override must reach the separator through the Horizontal visual state.");
                });
            }
            finally
            {
                // Application.Resources is UI-thread affine; a finally block runs on the test thread.
                RunOnUIThread.Execute(() =>
                {
                    ClearApplicationResource("ResizeGripperSeparatorBrush");
                    ClearApplicationResource("ResizeGripperSeparatorThickness");
                });
            }
        }

        [TestMethod]
        public void VerifyResizeGripperSeparatorShowsOnlyWhileDragging()
        {
            ResizeGripper gripper = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                gripper = new ResizeGripper { Width = 8, Height = 24 };
                LoadContent(gripper);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var separator = gripper.FindVisualChildByName("Separator") as Border;
                if (separator == null)
                {
                    Verify.Fail("The ResizeGripper template must contain the 'Separator' part its visual states target by name.");
                    return;
                }

                Verify.AreEqual(separator, VisualTreeHelper.GetChild(gripper, 0),
                    "'Separator' must be the template root.");
                Verify.IsNull(separator.Child, "The separator Border must not wrap another element.");
                Verify.IsNotNull(separator.Background, "A null Background would stop the gripper from hit-testing.");

                var rest = separator.BorderBrush as SolidColorBrush;
                Verify.IsTrue(rest != null && rest.Color.A == 0, "The separator must be hidden in the Normal state.");

                VisualStateManager.GoToState(gripper, "PointerOver", false);
                var hover = separator.BorderBrush as SolidColorBrush;
                Verify.IsTrue(hover != null && hover.Color.A == 0, "Hover must not draw the separator.");

                VisualStateManager.GoToState(gripper, "Pressed", false);
                Verify.AreEqual(gripper.BorderBrush, separator.BorderBrush,
                    "The Pressed state must show the separator with the gripper's BorderBrush.");

                VisualStateManager.GoToState(gripper, "Normal", false);
                rest = separator.BorderBrush as SolidColorBrush;
                Verify.IsTrue(rest != null && rest.Color.A == 0, "Returning to Normal must hide the separator again.");
            });
        }

        [TestMethod]
        public void VerifyResizeGripperDefaultSeparatorBrushesResolve()
        {
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();

                var expectedLight = RequireThemeColor("Light", "ControlStrongStrokeColorDefault");
                var expectedDark = RequireThemeColor("Default", "ControlStrongStrokeColorDefault");
                Verify.AreNotEqual(expectedLight, expectedDark,
                    "ControlStrongStrokeColorDefault must differ between Light and Dark, or a swap could not be detected.");

                var light = RequireBrush(resources, "Light", "ResizeGripperSeparatorBrush");
                Verify.AreEqual(expectedLight, light.Color,
                    "The Light ResizeGripperSeparatorBrush must be the neutral ControlStrongStrokeColorDefault.");

                var dark = RequireBrush(resources, "Default", "ResizeGripperSeparatorBrush");
                Verify.AreEqual(expectedDark, dark.Color,
                    "The Dark ResizeGripperSeparatorBrush must be the neutral ControlStrongStrokeColorDefault.");

                var accents = new[] { "SystemAccentColorDark1", "SystemAccentColorLight2" }
                    .Select(RequireApplicationColor).ToList();
                Verify.IsFalse(accents.Contains(light.Color) || accents.Contains(dark.Color),
                    "The gripper separator must not use an accent colour.");

                var highContrast = RequireBrush(resources, "HighContrast", "ResizeGripperSeparatorBrush");
                var systemColors = CollectSystemColors();
                Verify.IsGreaterThan(systemColors.Count, 0,
                    "SystemColor* resources must resolve from the application resources before validating HighContrast.");
                Verify.IsTrue(systemColors.Contains(highContrast.Color),
                    "The HighContrast default ResizeGripperSeparatorBrush must resolve through a system colour, not a hardcoded accent or cross-dictionary StaticResource alias.");
            });
        }

        [TestMethod]
        public void VerifySelectionIndicatorBrushFollowsAccentFillTiers()
        {
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();

                var expectedLight = RequireApplicationColor("SystemAccentColorDark1");
                var expectedDark = RequireApplicationColor("SystemAccentColorLight2");
                Verify.AreNotEqual(expectedLight, expectedDark,
                    "SystemAccentColorDark1 and SystemAccentColorLight2 must differ, or a Light/Dark swap could not be detected.");

                Verify.AreEqual(expectedLight, RequireBrush(resources, "Light", "TabularSurfaceSelectionIndicatorBrush").Color,
                    "The Light TabularSurfaceSelectionIndicatorBrush must follow AccentFillColorDefaultBrush's Light tier, SystemAccentColorDark1.");
                Verify.AreEqual(expectedDark, RequireBrush(resources, "Default", "TabularSurfaceSelectionIndicatorBrush").Color,
                    "The Default/Dark TabularSurfaceSelectionIndicatorBrush must follow AccentFillColorDefaultBrush's Dark tier, SystemAccentColorLight2.");

                foreach (var themeName in new[] { "Default", "Light", "HighContrast" })
                {
                    var dictionary = (ResourceDictionary)resources.ThemeDictionaries[themeName];
                    Verify.IsTrue(dictionary.ContainsKey("TabularSurfaceHeaderForegroundBrush"),
                        $"{themeName} must declare the header foreground independently of body text.");
                    Verify.IsTrue(dictionary.ContainsKey("TabularSurfaceHeaderSeparatorBrush"),
                        $"{themeName} must keep TabularSurfaceHeaderSeparatorBrush.");
                }
            });
        }

        [TestMethod]
        public void VerifyHeaderPointerStateBrushesResolveInEveryTheme()
        {
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();

                foreach (var themeName in new[] { "Default", "Light" })
                {
                    Verify.AreEqual(
                        RequireBrush(resources, themeName, "TabularSurfaceRowBackgroundPointerOverBrush").Color,
                        RequireBrush(resources, themeName, "TabularSurfaceHeaderBackgroundPointerOverBrush").Color,
                        $"'{themeName}' header pointer-over fill must match the row pointer-over fill.");
                    Verify.AreEqual(
                        RequireBrush(resources, themeName, "TabularSurfaceRowBackgroundPressedBrush").Color,
                        RequireBrush(resources, themeName, "TabularSurfaceHeaderBackgroundPressedBrush").Color,
                        $"'{themeName}' header pressed fill must match the row pressed fill.");

                    Verify.AreEqual((byte)0, RequireBrush(resources, themeName, "TabularSurfaceHeaderBackgroundBrush").Color.A,
                        $"'{themeName}' header band must be transparent at rest.");
                    var restForeground = RequireBrush(resources, themeName, "TabularSurfaceHeaderForegroundBrush").Color;
                    Verify.AreEqual(RequireThemeColor(themeName, "TextFillColorSecondary"), restForeground,
                        $"'{themeName}' headers must use secondary text.");
                    Verify.AreEqual(restForeground,
                        RequireBrush(resources, themeName, "TabularSurfaceHeaderForegroundPointerOverBrush").Color,
                        $"'{themeName}' header text must keep its rest colour on hover.");
                    Verify.AreEqual(restForeground,
                        RequireBrush(resources, themeName, "TabularSurfaceHeaderForegroundPressedBrush").Color,
                        $"'{themeName}' header text must keep its rest colour when pressed.");
                }

                var systemColors = CollectSystemColors();
                Verify.IsGreaterThan(systemColors.Count, 0,
                    "SystemColor* resources must resolve from the application resources before validating HighContrast.");

                var highlight = RequireBrush(resources, "HighContrast", "TabularSurfaceRowBackgroundPointerOverBrush").Color;
                var highlightText = RequireBrush(resources, "HighContrast", "TabularSurfaceRowForegroundPointerOverBrush").Color;
                foreach (var state in new[] { "PointerOver", "Pressed" })
                {
                    var background = RequireBrush(resources, "HighContrast", $"TabularSurfaceHeaderBackground{state}Brush").Color;
                    var foreground = RequireBrush(resources, "HighContrast", $"TabularSurfaceHeaderForeground{state}Brush").Color;
                    Verify.IsTrue(systemColors.Contains(background) && systemColors.Contains(foreground),
                        $"HighContrast header {state} brushes must resolve through system colours.");
                    Verify.AreEqual(highlight, background, $"HighContrast header {state} fill must be SystemColorHighlightColor.");
                    Verify.AreEqual(highlightText, foreground, $"HighContrast header {state} text must be SystemColorHighlightTextColor.");
                }
            });
        }

        // If the sort glyph cannot be themed it may end up invisible against a customised header
        // background.
        [TestMethod]
        public void VerifySortIndicatorForegroundResourceApplies()
        {
            var expected = Color.FromArgb(0xFF, 0x33, 0x66, 0xFF);
            SortIndicator indicator = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("SortIndicatorForeground", new SolidColorBrush(expected));

                indicator = new SortIndicator { Direction = SortIndicatorDirection.Ascending };
                LoadContent(indicator);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var foreground = indicator.Foreground as SolidColorBrush;
                    if (foreground == null)
                    {
                        Verify.Fail("The default SortIndicator style must set Foreground from SortIndicatorForeground.");
                        return;
                    }

                    Verify.AreEqual(expected, foreground.Color,
                        "An app-scope SortIndicatorForeground override must reach the indicator.");

                    // The glyph template-binds Foreground, so the override has to survive the binding too.
                    var glyph = indicator.FindVisualChildByName("GlyphIcon") as Microsoft.UI.Xaml.Shapes.Path;
                    if (glyph == null)
                    {
                        Verify.Fail("The SortIndicator template must contain the GlyphIcon vector Path part.");
                        return;
                    }

                    var glyphForeground = glyph.Fill as SolidColorBrush;
                    Verify.IsNotNull(glyphForeground, "GlyphIcon must template-bind the control's Foreground.");
                    Verify.AreEqual(expected, glyphForeground.Color,
                        "The glyph itself must render with the overridden foreground, not just the control property.");
                });
            }
            finally
            {
                // Application.Resources is UI-thread affine; a finally block runs on the test thread.
                RunOnUIThread.Execute(() => ClearApplicationResource("SortIndicatorForeground"));
            }
        }

        // TableView-dev-spec.md:176 states values "re-resolve across theme (and high-contrast) changes".
        //
        // The per-instance resource cache is real and deliberate - density metrics and the gridline
        // brush are both cached - so this is the specific risk the invalidation path exists to cover,
        // not a hypothetical. Without it a table stays light-themed inside a dark app until recreated.
        [TestMethod]
        public void VerifyThemeChangeAfterLoadReResolvesRowsAndHeaders()
        {
            TableView tableView = null;
            Grid host = null;
            Color lightGridLine = default;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                host = new Grid { RequestedTheme = ElementTheme.Light };
                host.Children.Add(tableView);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                lightGridLine = RequireCellSeparatorColor(tableView, "in Light");

                Verify.AreEqual(Color.FromArgb(0x0F, 0x00, 0x00, 0x00), lightGridLine,
                    "The light-theme gridline must resolve to #0F000000 before the theme switch, or the rest of this test proves nothing.");

                host.RequestedTheme = ElementTheme.Dark;
                host.UpdateLayout();
            });

            // A theme change reaches realized rows one full layout + idle pass after the property
            // changes, so settle twice.
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                host.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var darkGridLine = RequireCellSeparatorColor(tableView, "in Dark");

                Verify.AreNotEqual(lightGridLine, darkGridLine,
                    "The gridline brush must change with the theme; an unchanged value means the per-instance resource cache " +
                    "was not invalidated on the theme change.");
                Verify.AreEqual(Color.FromArgb(0x15, 0xFF, 0xFF, 0xFF), darkGridLine,
                    "After switching to Dark the gridline must re-resolve to the dark TabularSurfaceGridLineBrush.");
            });
        }

        // Adopted from PR !15971489's SortIndicator_ThemeSwitchMidState, which has it as an interaction
        // test. Written here as an API test because both triggers are programmatic.
        //
        // Spec is silent on this - it pins robustness, not a stated contract - so treat a failure as a
        // real finding but expect a spec conversation with it.
        [TestMethod]
        public void VerifyThemeChangeDuringSortIndicatorTransitionIsCoherent()
        {
            SortIndicator indicator = null;
            Grid host = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                indicator = new SortIndicator();
                host = new Grid { RequestedTheme = ElementTheme.Light };
                host.Children.Add(indicator);

                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Both in the same turn, before the visual-state transition started by the first has any
                // chance to complete. That interleaving is the whole point of the test.
                indicator.Direction = SortIndicatorDirection.Descending;
                host.RequestedTheme = ElementTheme.Dark;
                host.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                host.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortIndicatorDirection.Descending, indicator.Direction,
                    "The interrupted transition must leave the final direction intact.");

                var layoutRoot = indicator.FindVisualChildByName("LayoutRoot") as FrameworkElement;
                if (layoutRoot == null)
                {
                    Verify.Fail("The SortIndicator template must contain the LayoutRoot part whose Opacity the control drives.");
                    return;
                }

                Verify.AreEqual(1.0, layoutRoot.Opacity,
                    "A sorted indicator must end fully opaque. A value between 0 and 1 means the transition was stranded " +
                    "mid-animation by the theme change.");

                var glyph = indicator.FindVisualChildByName("GlyphIcon") as Microsoft.UI.Xaml.Shapes.Path;
                if (glyph == null)
                {
                    Verify.Fail("The SortIndicator template must contain the GlyphIcon part.");
                    return;
                }

                Verify.IsNotNull(glyph.Data,
                    "The vector geometry must still be set after the interrupted transition.");

                var foreground = indicator.Foreground as SolidColorBrush;
                Verify.IsNotNull(foreground,
                    "The indicator must still carry a resolved foreground brush after the theme change - a null brush renders nothing.");
                Verify.AreEqual(foreground.Color, ((SolidColorBrush)glyph.Fill).Color,
                    "The vector fill must follow the resolved foreground after the theme change.");
            });
        }

        // Legacy fill states remain available for custom templates; default HC rows instead use
        // HighContrast* outline states that preserve the resting foreground and adjustment policy.
        [TestMethod]
        public void VerifyPointerOverAndPressedRowsSuppressHighContrastBackplate()
        {
            foreach (var state in new[] { "PointerOver", "Pressed", "Selected", "SelectedPointerOver", "SelectedPressed" })
            {
                TableView tableView = null;
                TableViewRow row = null;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    row = GetRealizedRows(tableView).FirstOrDefault();
                    if (row == null)
                    {
                        Verify.Fail("The TableView should realize at least one row.");
                        return;
                    }

                    Verify.IsTrue(VisualStateManager.GoToState(row, state, false), $"The row template must define the {state} state.");
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var presenter = row?.FindVisualChildByName("PART_CellForegroundPresenter") as ContentPresenter;
                    if (presenter == null)
                    {
                        Verify.Fail("The row template must contain PART_CellForegroundPresenter.");
                        return;
                    }

                    Verify.AreEqual(ElementHighContrastAdjustment.None, presenter.HighContrastAdjustment,
                        $"The {state} state must turn off the High Contrast text backplate.");
                });
            }
        }

        [TestMethod]
        public void VerifyHighContrastRowOutlinesPreserveContentAndLayout()
        {
            bool highContrast = false;
            RunOnUIThread.Execute(() => highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast);
            foreach (var density in DensityCases)
            {
                TableView tableView = null;
                TableViewRow row = null;
                Grid cell = null;
                TextBlock appText = null;
                Button appButton = null;
                SolidColorBrush restingBackground = null;
                SolidColorBrush restingForeground = null;
                Brush buttonBackground = null;
                Brush appForeground = null;
                Thickness restingBackgroundMargin = default;
                double rowHeight = 0;
                double cellWidth = 0;
                double cellHeight = 0;
                ElementHighContrastAdjustment adjustment = default;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateGroupedThemingTable();
                    tableView.Density = density.Density;
                    tableView.IsTextScaleFactorEnabled = false;
                    tableView.FlowDirection = FlowDirection.RightToLeft;
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    row = GetRealizedRows(tableView).First();
                    cell = GetRowCell(row, 0);
                    appForeground = new SolidColorBrush(RequireApplicationColor("SystemColorButtonTextColor"));
                    appText = new TextBlock { Text = "App-owned content", Foreground = appForeground };
                    appButton = new Button { Content = appText };
                    cell.Children.Add(appButton);
                    tableView.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var presenter = (ContentPresenter)row.FindVisualChildByName("PART_CellForegroundPresenter");
                    var background = (Border)row.FindVisualChildByName("PART_RootBorder");
                    restingBackground = (SolidColorBrush)background.Background;
                    restingBackgroundMargin = background.Margin;
                    restingForeground = (SolidColorBrush)presenter.Foreground;
                    adjustment = presenter.HighContrastAdjustment;
                    buttonBackground = appButton.Background;
                    rowHeight = row.ActualHeight;
                    cellWidth = cell.ActualWidth;
                    cellHeight = cell.ActualHeight;
                });

                foreach (var state in new[] { "HighContrastPointerOver", "HighContrastPressed",
                    "HighContrastSelected", "HighContrastSelectedPointerOver", "HighContrastSelectedPressed",
                    "HighContrastSelected", "HighContrastPointerOver", "Normal" })
                {
                    // Enter from a legacy fill state to verify that its fill, foreground inversion,
                    // and backplate suppression are all removed, not just hidden by a new stroke.
                    RunOnUIThread.Execute(() => Verify.IsTrue(VisualStateManager.GoToState(row, "Selected", false)));
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() => Verify.IsTrue(VisualStateManager.GoToState(row, state, false)));
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var outline = (Border)row.FindVisualChildByName("PART_SelectionIndicator");
                        var background = (Border)row.FindVisualChildByName("PART_RootBorder");
                        var presenter = (ContentPresenter)row.FindVisualChildByName("PART_CellForegroundPresenter");
                        Verify.AreEqual(state == "Normal" ? 0.0 : 1.0, outline.Opacity);
                        if (state != "Normal")
                        {
                            var colorKey = state == "HighContrastSelectedPointerOver" || state == "HighContrastSelectedPressed"
                                ? "SystemColorButtonTextColor" : "SystemColorHighlightColor";
                            // WinUI's SystemColor*Brush resources are placeholders outside HC.
                            // Exercise transitions there too, but validate actual HC palette colors in HC.
                            var expected = highContrast ? RequireApplicationColor(colorKey)
                                : ((SolidColorBrush)Application.Current.Resources[colorKey + "Brush"]).Color;
                            Verify.AreEqual(expected, ((SolidColorBrush)outline.BorderBrush).Color,
                                $"{state} must use the approved system outline color.");
                        }
                        Verify.AreEqual(restingBackground.Color, ((SolidColorBrush)background.Background).Color,
                            "HC interaction must preserve the resting row background, without a Highlight fill.");
                        Verify.AreEqual(restingBackgroundMargin, background.Margin,
                            "HC interaction must preserve the resting row background inset.");
                        Verify.AreEqual(new Thickness(0), background.BorderThickness);
                        Verify.AreEqual(restingForeground.Color, ((SolidColorBrush)presenter.Foreground).Color,
                            "Outline-only states must not invert the inherited text.");
                        Verify.AreEqual(adjustment, presenter.HighContrastAdjustment);
                        Verify.AreEqual(new Thickness(1), outline.BorderThickness);
                        Verify.AreEqual(new Thickness(3, 2, 3, 2), outline.Margin);
                        Verify.AreEqual(row.CornerRadius, outline.CornerRadius);
                        Verify.IsFalse(outline.IsHitTestVisible);
                        Verify.AreEqual(rowHeight, row.ActualHeight);
                        Verify.AreEqual(cellWidth, cell.ActualWidth);
                        Verify.AreEqual(cellHeight, cell.ActualHeight);
                        Verify.AreEqual(appForeground, appText.Foreground, "App-owned text must keep its explicit foreground.");
                        Verify.AreEqual(buttonBackground, appButton.Background, "Native child controls keep their own chrome.");
                        Verify.IsTrue(appButton.IsEnabled && appButton.IsHitTestVisible);
                        Verify.IsFalse(row.IsSelected, "Visual hover/press must not change actual row selection.");
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyRowSelectionUsesThemeSpecificOutlineStates()
        {
            TableView tableView = null;
            bool highContrast = false;
            RunOnUIThread.Execute(() =>
            {
                highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => tableView.Select(0));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var row = GetRealizedRows(tableView).First();
                Verify.IsTrue(row.IsSelected);
                Verify.AreEqual(highContrast ? "HighContrastSelected" : "Selected",
                    GetCurrentVisualState(row, "CommonStates"), "Real selection must choose the correct theme's state.");
                var outline = (Border)row.FindVisualChildByName("PART_SelectionIndicator");
                Verify.AreEqual(1.0, outline.Opacity);
                row.IsEnabled = false;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var row = GetRealizedRows(tableView).First();
                Verify.AreEqual("SelectedDisabled", GetCurrentVisualState(row, "CommonStates"));
                row.IsEnabled = true;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var row = GetRealizedRows(tableView).First();
                Verify.AreEqual(highContrast ? "HighContrastSelected" : "Selected",
                    GetCurrentVisualState(row, "CommonStates"), "Re-enabling must restore the correct selected outline.");
                tableView.DeselectAll();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var row = GetRealizedRows(tableView).First();
                Verify.IsFalse(row.IsSelected);
                Verify.AreEqual("Normal", GetCurrentVisualState(row, "CommonStates"));
                Verify.AreEqual(0.0, ((Border)row.FindVisualChildByName("PART_SelectionIndicator")).Opacity);
            });
        }

        [TestMethod]
        public void VerifyHighContrastRowStatesFallBackForLegacyTemplates()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var row = GetRealizedRows(tableView).First();
                var root = (FrameworkElement)VisualTreeHelper.GetChild(row, 0);
                var group = VisualStateManager.GetVisualStateGroups(root).Single(g => g.Name == "CommonStates");
                foreach (var state in group.States.Where(s => s.Name.StartsWith("HighContrast", StringComparison.Ordinal)).ToList())
                {
                    group.States.Remove(state);
                }
                tableView.Select(0);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
                Verify.AreEqual("Selected", GetCurrentVisualState(GetRealizedRows(tableView).First(), "CommonStates"),
                    "An older template must retain its selection state when HC-specific states are absent."));
        }

        [TestMethod]
        public void VerifyHighContrastGroupHeadersUseButtonText()
        {
            bool highContrast = false;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;
                var resources = new TabularControlsResources();
                Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorButtonTextColorBrush"),
                    RequireBrush(resources, "HighContrast", "TabularSurfaceGroupHeaderForegroundBrush")),
                    "The HC section-header foreground must map to ButtonText.");
                foreach (var key in new[] { "TabularSurfaceGroupHeaderBackgroundBrush", "TabularSurfaceGroupHeaderInteractionBorderBrush" })
                {
                    Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorButtonFaceColorBrush"),
                        RequireBrush(resources, "HighContrast", key)),
                        "The HC resting section background and interaction border must pair ButtonFace with ButtonText.");
                }
            });

            foreach (var strong in new[] { false, true })
            {
                TableView tableView = null;
                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateGroupedThemingTable();
                    if (strong)
                    {
                        tableView.Resources[typeof(TableViewGroupHeader)] =
                            (Style)new TabularControlsResources()["StrongTableViewGroupHeaderStyle"];
                    }
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();

                foreach (var state in new[] { "Normal", "PointerOver", "Pressed", "Disabled", "Normal" })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                        Verify.IsTrue(VisualStateManager.GoToState(header, state, false));
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                        var presenter = (ContentPresenter)header.FindVisualChildByName("PART_ContentPresenter");
                        var chevron = (FontIcon)header.FindVisualChildByName("PART_ExpanderIcon");
                        var count = (TextBlock)header.FindVisualChildByName("TableViewGroupHeaderItemCountText");
                        var title = FindVisualChildrenByType<TextBlock>(presenter).First(text => text != count);
                        var foreground = ((SolidColorBrush)presenter.Foreground).Color;
                        var context = $"strong={strong}, {state}";
                        if (highContrast)
                        {
                            var resource = state == "Normal" ? "SystemColorButtonTextColor" :
                                state == "Disabled" ? "SystemColorGrayTextColor" : "SystemColorHighlightTextColor";
                            Verify.AreEqual(RequireApplicationColor(resource), foreground, context);
                            var plate = (Border)header.FindVisualChildByName("PART_Root");
                            var backgroundResource = state == "Normal" || state == "Disabled"
                                ? "SystemColorButtonFaceColor" : "SystemColorHighlightColor";
                            Verify.AreEqual(RequireApplicationColor(backgroundResource),
                                ((SolidColorBrush)plate.Background).Color, $"Section background: {context}");
                            Verify.AreEqual(RequireApplicationColor(backgroundResource),
                                ((SolidColorBrush)plate.BorderBrush).Color, $"Section interaction border: {context}");
                        }
                        Verify.AreEqual(foreground, ((SolidColorBrush)title.Foreground).Color, $"Section title: {context}");
                        Verify.AreEqual(foreground, ((SolidColorBrush)count.Foreground).Color, $"Section count: {context}");
                        Verify.AreEqual(foreground, ((SolidColorBrush)chevron.Foreground).Color, $"Section chevron: {context}");
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyHighContrastGroupHeaderPairingWithDistinctSystemColors()
        {
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var appResources = Application.Current.Resources;
                var brushes = new Dictionary<string, SolidColorBrush>
                {
                    ["SystemColorWindowColorBrush"] = new SolidColorBrush(Color.FromArgb(255, 12, 24, 36)),
                    ["SystemColorButtonFaceColorBrush"] = new SolidColorBrush(Color.FromArgb(255, 48, 60, 72)),
                    ["SystemColorButtonTextColorBrush"] = new SolidColorBrush(Color.FromArgb(255, 224, 236, 248)),
                    ["SystemColorGrayTextColorBrush"] = new SolidColorBrush(Color.FromArgb(255, 120, 132, 144)),
                };
                var previous = brushes.Keys.ToDictionary(key => key,
                    key => appResources.Keys.Contains(key) ? appResources[key] : null);
                try
                {
                    foreach (var entry in brushes)
                    {
                        appResources[entry.Key] = entry.Value;
                    }
                    var resources = new TabularControlsResources();
                    foreach (var key in new[] { "TabularSurfaceGroupHeaderBackgroundBrush", "TabularSurfaceGroupHeaderInteractionBorderBrush" })
                    {
                        Verify.IsTrue(ReferenceEquals(brushes["SystemColorButtonFaceColorBrush"],
                            RequireBrush(resources, "HighContrast", key)),
                            "The shared ButtonFace brush must be used even when it differs from Window in a custom palette.");
                    }
                    Verify.IsTrue(ReferenceEquals(brushes["SystemColorButtonTextColorBrush"],
                        RequireBrush(resources, "HighContrast", "TabularSurfaceGroupHeaderForegroundBrush")));
                    foreach (var key in new[] { "TabularSurfaceGridLineBrush", "TabularSurfaceHeaderSeparatorBrush",
                        "TabularSurfaceGroupHeaderBorderBrush", "TabularSurfaceOuterBorderBrush",
                        "TabularSurfaceBorderlessOuterBorderBrush" })
                    {
                        Verify.IsTrue(ReferenceEquals(brushes["SystemColorGrayTextColorBrush"],
                            RequireBrush(resources, "HighContrast", key)),
                            $"{key} must reuse the shared GrayText brush.");
                    }
                }
                finally
                {
                    foreach (var entry in previous)
                    {
                        if (entry.Value == null)
                        {
                            appResources.Remove(entry.Key);
                        }
                        else
                        {
                            appResources[entry.Key] = entry.Value;
                        }
                    }
                }
            });
        }

        [TestMethod]
        public void VerifyGroupHeaderCountCanBeHiddenAndRestored()
        {
            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            foreach (var density in DensityCases)
            {
                TableView table = null;
                RunOnUIThread.Execute(() =>
                {
                    table = CreateGroupedThemingTable();
                    table.RequestedTheme = theme;
                    table.Density = density.Density;
                    table.ShowGroupHeaderCount = false;
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                foreach (var enabled in new[] { true, false })
                foreach (var show in new[] { false, true, false, true })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        table.IsEnabled = enabled;
                        table.ShowGroupHeaderCount = show;
                        var template = table.Template;
                        table.Template = null;
                        table.Template = template;
                        table.ApplyTemplate();
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        foreach (var header in FindVisualChildrenByType<TableViewGroupHeader>(table))
                        {
                            var count = (TextBlock)header.FindVisualChildByName("TableViewGroupHeaderItemCountText");
                            var info = (TableViewGroupInfo)header.Content;
                            Verify.IsNotNull(count);
                            Verify.AreEqual(show ? Visibility.Visible : Visibility.Collapsed, count.Visibility,
                                $"{theme}, {density.Density}, enabled={enabled}, show={show}");
                            Verify.AreEqual(info.ItemCountText, count.Text, "Hiding the count must not discard its live data.");
                            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(header);
                            if (show)
                            {
                                Verify.IsTrue(peer.GetName().Contains(info.ItemCountText));
                            }
                            else
                            {
                                Verify.AreEqual(info.KeyText, peer.GetName(), "Hidden counts must also be omitted from the default automation name.");
                                var root = (StackPanel)header.FindVisualChildByName("TableViewDefaultGroupHeaderContentRoot");
                                var key = (TextBlock)root.Children[0];
                                Verify.IsLessThanOrEqual(Math.Abs(root.DesiredSize.Width - key.DesiredSize.Width),
                                    1.0 / table.XamlRoot.RasterizationScale,
                                    "A collapsed count must not leave the inter-label gap.");
                            }
                            AutomationProperties.SetName(header, "Custom section name");
                            Verify.AreEqual("Custom section name", peer.GetName());
                            header.ClearValue(AutomationProperties.NameProperty);
                        }
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyGroupHeaderCountOptionPreservesCustomContentTemplates()
        {
            TableView table = null;
            const string CustomTemplateXaml =
                @"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                                xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
                      <StackPanel x:Name=""TableViewDefaultGroupHeaderContentRoot"" Orientation=""Horizontal"">
                          <TextBlock Text=""{Binding KeyText}"" />
                          <TextBlock x:Name=""TableViewGroupHeaderItemCountText"" Text=""{Binding ItemCountText}""
                                     Visibility=""{Binding ElementName=TableViewDefaultGroupHeaderContentRoot, Path=Visibility}""
                                     Opacity=""0.25"" />
                      </StackPanel>
                  </DataTemplate>";
            foreach (var useStyle in new[] { false, true })
            {
                RunOnUIThread.Execute(() =>
                {
                    table = CreateGroupedThemingTable();
                    table.ShowGroupHeaderCount = false;
                    var template = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(CustomTemplateXaml);
                    if (useStyle)
                    {
                        var baseStyle = new Style(typeof(TableViewGroupHeader));
                        baseStyle.Setters.Add(new Setter(ContentControl.ContentTemplateProperty, template));
                        table.Resources[typeof(TableViewGroupHeader)] =
                            new Style(typeof(TableViewGroupHeader)) { BasedOn = baseStyle };
                    }
                    else
                    {
                        table.GroupHeaderTemplate = template;
                    }
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                foreach (var show in new[] { false, true, false })
                {
                    RunOnUIThread.Execute(() => table.ShowGroupHeaderCount = show);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        foreach (var header in FindVisualChildrenByType<TableViewGroupHeader>(table))
                        {
                            var count = (TextBlock)header.FindVisualChildByName("TableViewGroupHeaderItemCountText");
                            Verify.AreEqual(Visibility.Visible, count.Visibility, "Custom content remains app-owned.");
                            Verify.IsNotNull(count.GetBindingExpression(UIElement.VisibilityProperty),
                                "A copied default name does not transfer ownership of the app's binding.");
                            Verify.AreEqual(0.25, count.Opacity, "Do not change a custom count's opacity.");
                        }
                    });
                }
                RunOnUIThread.Execute(() =>
                {
                    if (useStyle)
                    {
                        table.Resources.Remove(typeof(TableViewGroupHeader));
                    }
                    else
                    {
                        table.GroupHeaderTemplate = null;
                    }
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    foreach (var header in FindVisualChildrenByType<TableViewGroupHeader>(table))
                    {
                        var count = (TextBlock)header.FindVisualChildByName("TableViewGroupHeaderItemCountText");
                        Verify.AreEqual(Visibility.Collapsed, count.Visibility, "Restoring the default template reapplies the option.");
                    }
                });
            }
        }

        [TestMethod]
        public void VerifyGroupHeaderCountRefreshHonorsReentrantChanges()
        {
            TableView table = null;
            TextBlock count = null;
            long token = 0;
            bool restored = false;
            RunOnUIThread.Execute(() =>
            {
                table = CreateGroupedThemingTable();
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                var header = FindVisualChildrenByType<TableViewGroupHeader>(table).First();
                count = (TextBlock)header.FindVisualChildByName("TableViewGroupHeaderItemCountText");
                token = count.RegisterPropertyChangedCallback(UIElement.VisibilityProperty, (s, e) =>
                {
                    if (!restored && count.Visibility == Visibility.Collapsed)
                    {
                        restored = true;
                        table.ShowGroupHeaderCount = true;
                    }
                });
                try { table.ShowGroupHeaderCount = false; }
                finally { count.UnregisterPropertyChangedCallback(UIElement.VisibilityProperty, token); }
                Verify.IsTrue(restored);
                Verify.IsTrue(table.ShowGroupHeaderCount);
                foreach (var current in FindVisualChildrenByType<TableViewGroupHeader>(table))
                {
                    Verify.AreEqual(Visibility.Visible,
                        ((TextBlock)current.FindVisualChildByName("TableViewGroupHeaderItemCountText")).Visibility);
                    Verify.IsTrue(FrameworkElementAutomationPeer.CreatePeerForElement(current).GetName()
                        .Contains(((TableViewGroupInfo)current.Content).ItemCountText));
                }
            });
        }

        [TestMethod]
        public void VerifyDensityRestampsAgainAfterReentrantFontChange()
        {
            TableView table = null;
            TextBlock text = null;
            bool changed = false;
            RunOnUIThread.Execute(() =>
            {
                table = CreateGroupedThemingTable();
                table.IsTextScaleFactorEnabled = false;
                table.Resources["TableViewCellFontSize"] = 20.0;
                table.Resources["TableViewCellFontSizeExtraCompact"] = 12.0;
                table.Density = TableViewDensity.ExtraCompact;
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                text = RequireFirstCellTextBlock(table);
                var token = text.RegisterPropertyChangedCallback(TextBlock.FontSizeProperty, (s, e) =>
                {
                    if (!changed && text.FontSize == 20.0)
                    {
                        changed = true;
                        table.Density = TableViewDensity.ExtraCompact;
                    }
                });
                try { table.Density = TableViewDensity.Standard; }
                finally { text.UnregisterPropertyChangedCallback(TextBlock.FontSizeProperty, token); }
                Verify.IsTrue(changed);
                Verify.AreEqual(TableViewDensity.ExtraCompact, table.Density);
                Verify.AreEqual(12.0, text.FontSize, "The replay must reread the final density's typography.");
                Verify.AreEqual(24.0, GetRealizedRows(table).First().MinHeight,
                    "The current row must not retain the interrupted pass's minimum height.");
            });
            SettleLayout(table);
        }

        [TestMethod]
        public void VerifySectionInteractionPlateMatchesRowVerticalInsets()
        {
            foreach (var density in DensityCases)
            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            foreach (var hcSeparators in new[] { false, true })
            foreach (var lines in new[] { TableViewGridLinesVisibility.None, TableViewGridLinesVisibility.All })
            {
                TableView table = null;
                TableViewGroupHeader section = null;
                TableViewRow row = null;
                Grid sectionChrome = null;
                Border sectionPlate = null;
                FrameworkElement content = null;
                Rect contentBounds = default;
                double height = 0;
                Thickness focusMargin = default;
                Thickness dividerThickness = default;
                RunOnUIThread.Execute(() =>
                {
                    table = CreateGroupedThemingTable();
                    table.Density = density.Density;
                    table.RequestedTheme = theme;
                    table.FlowDirection = flow;
                    table.GridLinesVisibility = lines;
                    table.IsTextScaleFactorEnabled = false;
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    section = FindVisualChildrenByType<TableViewGroupHeader>(table).First();
                    row = GetRealizedRows(table).First();
                    // Exercise custom top/bottom separator geometry even when Windows itself is not in HC.
                    section.BorderThickness = hcSeparators ? new Thickness(0, 1, 0, 1) : new Thickness(0);
                    sectionChrome = (Grid)section.FindVisualChildByName("PART_RowChrome");
                    sectionPlate = (Border)section.FindVisualChildByName("PART_Root");
                    content = (FrameworkElement)section.FindVisualChildByName("PART_ContentHost");
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    contentBounds = content.TransformToVisual(section).TransformBounds(
                        new Rect(0, 0, content.ActualWidth, content.ActualHeight));
                    height = section.ActualHeight;
                    focusMargin = section.FocusVisualMargin;
                    dividerThickness = ((Border)section.FindVisualChildByName("PART_GridLineBorder")).BorderThickness;
                });

                foreach (var state in new[] { "PointerOver", "Pressed", "Normal", "Disabled" })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        Verify.IsTrue(VisualStateManager.GoToState(section, state, false));
                        Verify.IsTrue(VisualStateManager.GoToState(row, state, false));
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var context = $"{density.Density}, {theme}, {flow}, HC separators={hcSeparators}, {lines}, {state}";
                        Verify.AreEqual(height, section.ActualHeight, $"Do not shrink the section header's layout or hit target: {context}");
                        Verify.AreEqual(contentBounds, content.TransformToVisual(section).TransformBounds(
                            new Rect(0, 0, content.ActualWidth, content.ActualHeight)),
                            $"Do not move section text or chevron: {context}");
                        Verify.AreEqual(focusMargin, section.FocusVisualMargin, context);
                        Verify.AreEqual(dividerThickness, ((Border)section.FindVisualChildByName("PART_GridLineBorder")).BorderThickness, context);
                        if (state == "PointerOver" || state == "Pressed")
                        {
                            var rowChrome = (Grid)row.FindVisualChildByName("PART_RowChrome");
                            var rowPlate = (Border)row.FindVisualChildByName("PART_RootBorder");
                            var sectionBounds = sectionPlate.TransformToVisual(sectionChrome).TransformBounds(
                                new Rect(0, 0, sectionPlate.ActualWidth, sectionPlate.ActualHeight));
                            var rowBounds = rowPlate.TransformToVisual(rowChrome).TransformBounds(
                                new Rect(0, 0, rowPlate.ActualWidth, rowPlate.ActualHeight));
                            var tolerance = 1.0 / table.XamlRoot.RasterizationScale;
                            Verify.IsLessThanOrEqual(Math.Abs(sectionBounds.Top - rowBounds.Top), tolerance,
                                $"Section and row hover plates must have the same top gap inside their separator-free chrome: {context}");
                            Verify.IsLessThanOrEqual(Math.Abs(sectionChrome.ActualHeight - sectionBounds.Bottom
                                - rowChrome.ActualHeight + rowBounds.Bottom), tolerance,
                                $"Section and row hover plates must have the same bottom gap inside their separator-free chrome: {context}");
                            Verify.AreEqual(0.0, sectionBounds.Left, $"Keep section horizontal geometry unchanged: {context}");
                            Verify.AreEqual(sectionChrome.ActualWidth, sectionBounds.Width, context);
                        }
                        else
                        {
                            Verify.AreEqual(new Thickness(0), sectionPlate.Margin,
                                $"Rest/disabled section plates must retain their baseline geometry: {context}");
                        }
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyGroupHeaderBackplatesMatchRowsAndFollowTheme()
        {
            TableView tableView = null;
            bool highContrast = false;

            RunOnUIThread.Execute(() =>
            {
                highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;
                tableView = CreateGroupedThemingTable();
                LoadContent(tableView);

                var resources = new TabularControlsResources();
                Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorButtonFaceColorBrush"),
                    RequireBrush(resources, "HighContrast", "TabularSurfaceGroupHeaderBackgroundBrush")),
                    "High Contrast section headers must pair ButtonFace with ButtonText.");
            });

            IdleSynchronizer.Wait();

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
            {
                RunOnUIThread.Execute(() => tableView.RequestedTheme = theme);
                IdleSynchronizer.Wait();

                foreach (var state in new[] { "Normal", "PointerOver", "Pressed", "Normal" })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                        var row = GetRealizedRows(tableView).First();
                        Verify.IsTrue(VisualStateManager.GoToState(header, state, false));
                        Verify.IsTrue(VisualStateManager.GoToState(row,
                            highContrast && state != "Normal" ? "HighContrast" + state : state, false));
                    });

                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                        var row = GetRealizedRows(tableView).First();
                        var headerRoot = (Border)header.FindVisualChildByName("PART_Root");
                        var rowRoot = (Border)row.FindVisualChildByName("PART_RootBorder");
                        var headerFill = headerRoot.Background as SolidColorBrush;
                        var rowFill = rowRoot.Background as SolidColorBrush;
                        Verify.IsNotNull(headerFill);
                        Verify.IsNotNull(rowFill);
                        if (highContrast)
                        {
                            Verify.AreEqual(RequireApplicationColor("SystemColorWindowColor"), rowFill.Color,
                                "HC body rows must keep their resting background during interaction.");
                            Verify.AreEqual(RequireApplicationColor(state == "Normal"
                                ? "SystemColorButtonFaceColor" : "SystemColorHighlightColor"), headerFill.Color,
                                "HC sections retain their approved filled interaction, independently of body outlines.");
                        }
                        else
                        {
                            Verify.AreEqual(rowFill.Color, headerFill.Color,
                                $"{theme}/{state}: section headers must use the unselected row backplate.");
                        }
                        Verify.AreEqual(rowRoot.CornerRadius, headerRoot.CornerRadius,
                            $"{theme}/{state}: section and row backplate corners must match.");
                        if (state == "Normal" && !highContrast)
                        {
                            Verify.AreEqual((byte)0, headerFill.Color.A,
                                $"{theme}: section headers must have no resting backplate.");
                        }
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyStrongGroupHeaderStyleChangesOnlyWeightAndCanBeToggledLive()
        {
            TableView tableView = null;
            Style strongStyle = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();
                strongStyle = resources["StrongTableViewGroupHeaderStyle"] as Style;
                Verify.IsNotNull(strongStyle);
                Verify.AreEqual(1, strongStyle.Setters.Count, "The strong section style changes only weight.");
                Verify.AreEqual(Control.FontWeightProperty, ((Setter)strongStyle.Setters[0]).Property);
                tableView = CreateGroupedThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            foreach (var strong in new[] { false, true, false })
            {
                RunOnUIThread.Execute(() =>
                {
                    if (strong)
                    {
                        tableView.Resources[typeof(TableViewGroupHeader)] = strongStyle;
                    }
                    else
                    {
                        tableView.Resources.Remove(typeof(TableViewGroupHeader));
                    }
                });

                IdleSynchronizer.Wait();

                foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
                {
                    RunOnUIThread.Execute(() => tableView.RequestedTheme = theme);
                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                        var presenter = (ContentPresenter)header.FindVisualChildByName("PART_ContentPresenter");
                        var count = (TextBlock)header.FindVisualChildByName("TableViewGroupHeaderItemCountText");
                        var title = FindVisualChildrenByType<TextBlock>(presenter).First(text => text != count);
                        var expectedWeight = (ushort)(strong ? 600 : 400);
                        var themeName = theme == ElementTheme.Light ? "Light" : "Default";
                        var resources = new TabularControlsResources();
                        Verify.IsNotNull(header.Template, "Both weights retain the built-in template.");
                        Verify.AreEqual(expectedWeight, header.FontWeight.Weight);
                        Verify.AreEqual(expectedWeight, title.FontWeight.Weight);
                        Verify.AreEqual(expectedWeight, count.FontWeight.Weight);
                        Verify.AreEqual(14.0, title.FontSize, "Section typography size is unchanged.");
                        Verify.AreEqual(RequireBrush(resources, themeName, "TabularSurfaceGroupHeaderForegroundBrush").Color,
                            ((SolidColorBrush)title.Foreground).Color, "Weight must not change the title color.");
                        Verify.AreEqual(((SolidColorBrush)title.Foreground).Color, ((SolidColorBrush)count.Foreground).Color,
                            "The count must keep inheriting the title foreground.");
                        Verify.IsLessThan(Math.Abs(count.Opacity - (theme == ElementTheme.Light ? 0.7 : 0.77)), 0.000001,
                            "The count's existing secondary treatment is unchanged.");
                        Verify.AreEqual((ushort)400, GetRealizedRows(tableView).First().FontWeight.Weight,
                            "Section weight must not change data-row weight.");
                        Verify.AreEqual((ushort)400, RequireColumnHeader(tableView, 0).FontWeight.Weight,
                            "Section weight must remain independent of column-header weight.");
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyPointerOverAndPressedGroupHeadersSuppressHighContrastBackplate()
        {
            foreach (var state in new[] { "PointerOver", "Pressed" })
            {
                TableView tableView = null;
                TableViewGroupHeader groupHeader = null;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateGroupedThemingTable();
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    groupHeader = FindVisualChildrenByType<TableViewGroupHeader>(tableView).FirstOrDefault();
                    if (groupHeader == null)
                    {
                        Verify.Fail("A grouped TableView should realize at least one TableViewGroupHeader.");
                        return;
                    }

                    Verify.IsTrue(VisualStateManager.GoToState(groupHeader, state, false), $"The group header template must define the {state} state.");
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    foreach (var partName in new[] { "PART_ContentPresenter", "PART_ExpanderIcon" })
                    {
                        var part = groupHeader?.FindVisualChildByName(partName) as UIElement;
                        if (part == null)
                        {
                            Verify.Fail($"The group header template must contain {partName}.");
                            return;
                        }

                        Verify.AreEqual(ElementHighContrastAdjustment.None, part.HighContrastAdjustment,
                            $"The {state} state must turn off the High Contrast text backplate on {partName}.");
                    }
                });
            }
        }

        [TestMethod]
        public void VerifyHighContrastHoveredAndPressedHeadersSuppressBackplate()
        {
            foreach (var state in new[] { "HighContrastPointerOver", "HighContrastPressed" })
            {
                TableView tableView = null;
                TableViewColumnHeader header = null;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    header = RequireColumnHeader(tableView, 0);
                    Verify.IsTrue(VisualStateManager.GoToState(header, state, false), $"The header template must define the {state} state.");
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var presenter = header?.FindVisualChildByName("PART_ContentPresenter") as UIElement;
                    if (presenter == null)
                    {
                        Verify.Fail("The header template must contain PART_ContentPresenter.");
                        return;
                    }

                    Verify.AreEqual(ElementHighContrastAdjustment.None, presenter.HighContrastAdjustment,
                        $"The {state} state must turn off the High Contrast text backplate on the header content.");
                });
            }
        }

        [TestMethod]
        public void VerifyDefaultGroupHeaderCountInheritsForegroundThatFollowsStates()
        {
            TableView tableView = null;
            TableViewGroupHeader groupHeader = null;
            TextBlock count = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceGroupHeaderCountOpacity", 0.5);

                tableView = CreateGroupedThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            Color PresenterColor() =>
                ((groupHeader.FindVisualChildByName("PART_ContentPresenter") as ContentPresenter)?.Foreground as SolidColorBrush)?.Color ?? default;
            Color CountColor() => (count.Foreground as SolidColorBrush)?.Color ?? default;

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    groupHeader = FindVisualChildrenByType<TableViewGroupHeader>(tableView).FirstOrDefault();
                    count = groupHeader?.FindVisualChildByName("TableViewGroupHeaderItemCountText") as TextBlock;
                    if (count == null)
                    {
                        Verify.Fail("The default group header content must contain the item-count TextBlock.");
                        return;
                    }

                    Verify.AreEqual(DependencyProperty.UnsetValue, count.ReadLocalValue(TextBlock.ForegroundProperty),
                        "The count must inherit Foreground rather than hold its own brush.");
                    Verify.AreEqual(0.5, count.Opacity, "The count must be dimmed with TabularSurfaceGroupHeaderCountOpacity.");
                    Verify.AreEqual(PresenterColor(), CountColor(), "At rest the count must use the header's Foreground.");

                    var contentPresenter = groupHeader.FindVisualChildByName("PART_ContentPresenter") as ContentPresenter;
                    var key = contentPresenter == null ? null :
                        FindVisualChildrenByType<TextBlock>(contentPresenter).FirstOrDefault(t => t != count);
                    Verify.IsNotNull(key, "The default group header content must contain the key TextBlock.");
                    Verify.AreEqual(DependencyProperty.UnsetValue, key?.ReadLocalValue(TextBlock.FontWeightProperty),
                        "Key text weight must inherit from the header, not a local value.");
                    Verify.AreEqual((ushort)400, key?.FontWeight.Weight ?? (ushort)0,
                        "Key text must use regular body typography.");
                    Verify.AreEqual(TextWrapping.NoWrap, key?.TextWrapping ?? TextWrapping.Wrap, "Key text stays on one line.");

                    Verify.IsTrue(VisualStateManager.GoToState(groupHeader, "PointerOver", false), "PointerOver must exist.");
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(PresenterColor(), CountColor(), "PointerOver must recolor the count with the key text.");
                    Verify.IsTrue(VisualStateManager.GoToState(groupHeader, "Disabled", false), "Disabled must exist.");
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(PresenterColor(), CountColor(), "Disabled must give the count the same disabled brush as the key text.");
                    Verify.IsTrue(VisualStateManager.GoToState(groupHeader, "Normal", false), "Normal must exist.");
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() => Verify.AreEqual(PresenterColor(), CountColor(),
                    "Returning to Normal must restore the rest brush."));
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceGroupHeaderCountOpacity"));
            }
        }

        [TestMethod]
        public void VerifyDisabledGroupHeaderCountIsNotDimmedTwice()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceGroupHeaderCountOpacity", 0.5);

                tableView = CreateGroupedThemingTable();
                tableView.IsEnabled = false;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            TableViewGroupHeader header = null;
            TextBlock Count() => header?.FindVisualChildByName("TableViewGroupHeaderItemCountText") as TextBlock;

            // IsEnabledChanged is raised asynchronously; the header's own handler was attached first.
            using (var enabledChanged = new global::System.Threading.AutoResetEvent(false))
            {
                try
                {
                    RunOnUIThread.Execute(() =>
                    {
                        header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).FirstOrDefault();
                        var count = Count();
                        if (count == null)
                        {
                            Verify.Fail("The default group header content must contain the item-count TextBlock.");
                            return;
                        }

                        Verify.AreEqual(1.0, count.Opacity, "A header loaded disabled must not dim its count on top of the disabled brush.");
                        header.IsEnabledChanged += (s, e) =>
                        {
                            try { enabledChanged.Set(); } catch (ObjectDisposedException) { }
                        };
                        tableView.IsEnabled = true;
                    });

                    Verify.IsTrue(enabledChanged.WaitOne(TimeSpan.FromSeconds(5)), "IsEnabledChanged must be raised.");
                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        Verify.AreEqual(0.5, Count().Opacity, "Enabling must restore TabularSurfaceGroupHeaderCountOpacity.");
                        tableView.IsEnabled = false;
                    });

                    Verify.IsTrue(enabledChanged.WaitOne(TimeSpan.FromSeconds(5)), "IsEnabledChanged must be raised.");
                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() => Verify.AreEqual(1.0, Count().Opacity,
                        "Disabling must not dim the count on top of the disabled brush."));
                }
                finally
                {
                    RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceGroupHeaderCountOpacity"));
                }
            }
        }

        [TestMethod]
        public void VerifyDisabledGroupHeaderCountFollowsContentTemplateChanges()
        {
            TableView tableView = null;
            DataTemplate customTemplate = null;
            const string CustomTemplateXaml =
                @"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                                xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
                      <StackPanel Orientation=""Horizontal"">
                          <TextBlock Text=""{Binding KeyText}"" />
                          <TextBlock x:Name=""TableViewGroupHeaderItemCountText"" Text=""{Binding ItemCountText}"" Opacity=""0.25"" />
                      </StackPanel>
                  </DataTemplate>";

            List<TextBlock> Counts() => FindVisualChildrenByType<TableViewGroupHeader>(tableView)
                .Select(header => header.FindVisualChildByName("TableViewGroupHeaderItemCountText") as TextBlock)
                .ToList();

            void VerifyCounts(double expected, string message)
            {
                var counts = Counts();
                Verify.IsGreaterThan(counts.Count, 0, "Precondition: group headers are realized.");
                foreach (var count in counts)
                {
                    if (count == null)
                    {
                        Verify.Fail("Every group header must contain the item-count TextBlock.");
                        return;
                    }
                    Verify.AreEqual(expected, count.Opacity, message);
                }
            }

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    EnsureTabularControlsResources();
                    SetApplicationResource("TabularSurfaceGroupHeaderCountOpacity", 0.5);
                    customTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(CustomTemplateXaml);

                    tableView = CreateGroupedThemingTable();
                    tableView.IsEnabled = false;
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    VerifyCounts(1.0, "Precondition: a disabled default count is not dimmed twice.");
                    tableView.GroupHeaderTemplate = customTemplate;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    VerifyCounts(0.25, "An app template's same-named TextBlock must keep its own Opacity.");
                    tableView.GroupHeaderTemplate = null;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() => VerifyCounts(1.0,
                    "A default count realized again while disabled must not be dimmed twice."));
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceGroupHeaderCountOpacity"));
            }
        }

        [TestMethod]
        public void VerifyGroupHeaderCountFollowsAppForegroundStyle()
        {
            var white = Color.FromArgb(0xFF, 0xFF, 0xFF, 0xFF);
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateGroupedThemingTable();
                tableView.RequestedTheme = ElementTheme.Light;
                var style = new Style(typeof(TableViewGroupHeader));
                style.Setters.Add(new Setter(Control.BackgroundProperty, new SolidColorBrush(Color.FromArgb(0xFF, 0x00, 0x5F, 0xB8))));
                style.Setters.Add(new Setter(Control.ForegroundProperty, new SolidColorBrush(white)));
                tableView.Resources[typeof(TableViewGroupHeader)] = style;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var groupHeader = FindVisualChildrenByType<TableViewGroupHeader>(tableView).FirstOrDefault();
                var count = groupHeader?.FindVisualChildByName("TableViewGroupHeaderItemCountText") as TextBlock;
                if (count == null)
                {
                    Verify.Fail("The default group header content must contain the item-count TextBlock.");
                    return;
                }

                Verify.AreEqual(white, (count.Foreground as SolidColorBrush)?.Color ?? default,
                    "An app's header Foreground must reach the count so it stays readable on the app's Background.");
            });
        }

        #endregion

        #region 13.3 Density

        [TestMethod]
        public void VerifyGroupHeaderDensityGeometryAndTypography()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedThemingTable();
                tableView.IsTextScaleFactorEnabled = false;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                foreach (var direction in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
                {
                    foreach (var testCase in DensityCases)
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            tableView.RequestedTheme = theme;
                            tableView.FlowDirection = direction;
                            tableView.Density = testCase.Density;
                            tableView.UpdateLayout();
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                            var expectedHeight = testCase.RowMinHeight;
                            var verticalPadding = testCase.CellPadding.Top;
                            Verify.AreEqual(expectedHeight, header.MinHeight, "Section-header density minimum.");
                            Verify.AreEqual(expectedHeight, header.ActualHeight, "Unscaled section-header rendered height.");
                            Verify.AreEqual(new Thickness(4, verticalPadding, 12, verticalPadding), header.Padding,
                                "Section-header padding follows live density changes.");
                            var presenter = (ContentPresenter)header.FindVisualChildByName("PART_ContentPresenter");
                            var gutter = (FrameworkElement)header.FindVisualChildByName("PART_ExpanderGutter");
                            var root = tableView.XamlRoot.Content;
                            var headerBounds = header.TransformToVisual(root).TransformBounds(
                                new Rect(0, 0, header.ActualWidth, header.ActualHeight));
                            var contentBounds = presenter.TransformToVisual(root).TransformBounds(
                                new Rect(0, 0, presenter.ActualWidth, presenter.ActualHeight));
                            var leadingInset = direction == FlowDirection.LeftToRight
                                ? contentBounds.Left - headerBounds.Left : headerBounds.Right - contentBounds.Right;
                            Verify.AreEqual(20.0, leadingInset, "Section content starts 20px from the leading edge, including in RTL.");
                            Verify.AreEqual(12.0, gutter.ActualWidth, "The expander uses a 12px gutter.");
                            var title = FindVisualChildrenByType<TextBlock>(header)
                                .First(text => text.Text == ((TableViewGroupInfo)header.Content).KeyText);
                            Verify.AreEqual(testCase.FontSize, title.FontSize,
                                "Section titles match the cells' font size at every density.");
                            Verify.AreEqual(Microsoft.UI.Text.FontWeights.Normal, title.FontWeight,
                                "Section titles use regular rather than semibold text.");
                            var row = GetRealizedRows(tableView).First();
                            var cellText = (TextBlock)GetCellContent(GetRowCell(row, 0));
                            var titleBounds = title.TransformToVisual(root).TransformBounds(
                                new Rect(0, 0, title.ActualWidth, title.ActualHeight));
                            var cellBounds = cellText.TransformToVisual(root).TransformBounds(
                                new Rect(0, 0, cellText.ActualWidth, cellText.ActualHeight));
                            var titleStart = direction == FlowDirection.LeftToRight ? titleBounds.Left : titleBounds.Right;
                            var cellStart = direction == FlowDirection.LeftToRight
                                ? cellBounds.Left + cellText.Padding.Left : cellBounds.Right - cellText.Padding.Right;
                            Verify.IsLessThanOrEqual(Math.Abs(titleStart - cellStart), 0.5 / tableView.XamlRoot.RasterizationScale,
                                "Only the first cell's text start aligns with the section title, including in RTL.");
                            Verify.AreEqual(GetAlignedContentPadding(row, testCase.TextPadding), cellText.Padding,
                                "Section alignment preserves the authored display-text padding.");
                            Verify.AreEqual(0.0, GetRowCell(row, 1).Padding.Left, "Other cells receive no section inset.");
                            var count = (TextBlock)header.FindVisualChildByName("TableViewGroupHeaderItemCountText");
                            Verify.IsNotNull(count, "Density and typography changes preserve item counts.");
                            Verify.AreEqual(testCase.FontSize, count.FontSize,
                                "Section counts match the cells' font size at every density.");
                            Verify.IsTrue(header.IsTabStop && header.UseSystemFocusVisuals,
                                "Section headers retain native keyboard focus.");
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifySectionAlignmentClearsAndMovesWithFirstVisibleCell()
        {
            TableView table = null;
            TableViewSource source = null;
            Grid root = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateThemingTable();
                table.IsTextScaleFactorEnabled = false;
                foreach (var column in table.Columns)
                {
                    column.Width = new GridLength(200);
                }
                source = TableViewSource.From(MakeItems());
                table.ItemsSource = source;
                root = new Grid { FlowDirection = FlowDirection.LeftToRight };
                root.Children.Add(table);
                LoadContent(root);
            });
            IdleSynchronizer.Wait();

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            foreach (var density in DensityCases)
            {
                double TextStart(TextBlock text)
                {
                    var bounds = text.TransformToVisual(root).TransformBounds(
                        new Rect(0, 0, text.ActualWidth, text.ActualHeight));
                    return flow == FlowDirection.LeftToRight
                        ? bounds.Left + text.Padding.Left : bounds.Right - text.Padding.Right;
                }
                TextBlock CellText(int index) => (TextBlock)GetCellContent(GetRowCell(GetRealizedRows(table).First(), index));
                TextBlock SectionTitle() => (TextBlock)FindVisualChildrenByType<TableViewGroupHeader>(table)
                    .First().FindVisualChildByName("TableViewGroupHeaderKeyText");
                double firstStart = 0;
                double secondStart = 0;
                RunOnUIThread.Execute(() =>
                {
                    table.RequestedTheme = theme;
                    table.FlowDirection = flow;
                    table.Density = density.Density;
                    source.ClearGroupBy();
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    firstStart = TextStart(CellText(0));
                    secondStart = TextStart(CellText(1));
                    Verify.AreEqual(0.0, GetRowCell(GetRealizedRows(table).First(), 0).Padding.Left);
                    source.GroupBy(item => ((Person)item).Role);
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    var context = $"{theme}, {flow}, {density.Density}";
                    Verify.IsLessThanOrEqual(Math.Abs(TextStart(CellText(0)) - TextStart(SectionTitle())),
                        0.5 / table.XamlRoot.RasterizationScale, $"Section first-cell alignment: {context}.");
                    Verify.AreEqual(secondStart, TextStart(CellText(1)), $"Other column text must not move: {context}.");
                    Verify.AreEqual(density.RowMinHeight, GetRealizedRows(table).First().ActualHeight);
                    table.Columns[0].Visibility = Visibility.Collapsed;
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    var row = GetRealizedRows(table).First();
                    Verify.AreEqual(0.0, GetRowCell(row, 0).Padding.Left, "The hidden cell must release the reservation.");
                    Verify.IsLessThanOrEqual(Math.Abs(TextStart(CellText(1)) - TextStart(SectionTitle())),
                        0.5 / table.XamlRoot.RasterizationScale, "The new first visible cell aligns with the section.");
                    table.Columns[0].Visibility = Visibility.Visible;
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(0.0, GetRowCell(GetRealizedRows(table).First(), 1).Padding.Left,
                        "Restoring the first column clears the second column's reservation.");
                    source.ClearGroupBy();
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(firstStart, TextStart(CellText(0)), "Removing sections restores the original first-cell alignment.");
                    Verify.AreEqual(secondStart, TextStart(CellText(1)), "Other cells keep their original alignment.");
                    Verify.AreEqual(0.0, GetRowCell(GetRealizedRows(table).First(), 0).Padding.Left);
                });
            }
        }

        [TestMethod]
        public void VerifySectionAlignmentFitsAutoColumnContent()
        {
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            foreach (var density in DensityCases)
            {
                TableView table = null;
                TableViewSource source = null;
                double flatWidth = 0;
                RunOnUIThread.Execute(() =>
                {
                    table = CreateThemingTable();
                    table.IsTextScaleFactorEnabled = false;
                    table.FlowDirection = flow;
                    table.Density = density.Density;
                    table.Columns[0].Width = GridLength.Auto;
                    table.Columns[1].Width = new GridLength(200);
                    source = TableViewSource.From(new List<Person>
                    {
                        new Person { Name = "A section row with a long name", Role = "Design" }
                    });
                    table.ItemsSource = source;
                    LoadContent(table);
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    flatWidth = table.Columns[0].ActualWidth;
                    source.GroupBy(item => ((Person)item).Role);
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    var cell = GetRowCell(GetRealizedRows(table).Single(), 0);
                    Verify.AreEqual(8.0, cell.Padding.Left);
                    Verify.AreEqual(GridUnitType.Auto, table.Columns[0].Width.GridUnitType);
                    Verify.IsGreaterThanOrEqual(table.Columns[0].ActualWidth,
                        flatWidth + cell.Padding.Left - 0.5 / table.XamlRoot.RasterizationScale,
                        $"Auto width includes the section inset without clipping content: {flow}, {density.Density}.");
                });
            }
        }

        [TestMethod]
        public void VerifySectionAlignmentUsesAuthoredMetricResources()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateGroupedThemingTable();
                table.Resources["TableViewGroupExpanderSize"] = 20.0;
                table.Resources["TableViewGroupExpanderSpacing"] = 8.0;
                table.Resources["TableViewGroupHeaderTextPadding"] = new Thickness(4, 9, 0, 11);
                table.Resources["TableViewCellTextPadding"] = new Thickness(16, 9, 16, 11);
                table.Density = TableViewDensity.Compact;
                table.Density = TableViewDensity.Standard;
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                var root = table.XamlRoot.Content;
                var title = (TextBlock)FindVisualChildrenByType<TableViewGroupHeader>(table)
                    .First().FindVisualChildByName("TableViewGroupHeaderKeyText");
                var row = GetRealizedRows(table).First();
                var text = (TextBlock)GetCellContent(GetRowCell(row, 0));
                var titleStart = title.TransformToVisual(root).TransformPoint(new Point()).X;
                var cellStart = text.TransformToVisual(root).TransformPoint(new Point()).X + text.Padding.Left;
                Verify.IsLessThanOrEqual(Math.Abs(titleStart - cellStart), 0.5 / table.XamlRoot.RasterizationScale,
                    "Alignment must derive from the actual metric resources, not a hard-coded 8px inset.");
                Verify.AreEqual(16.0, text.Padding.Left, "Preserve the authored cell-text padding.");
                Verify.AreEqual(20.0, GetRowCell(row, 0).Padding.Left, "36px section text start minus 16px cell padding.");
                Verify.AreEqual(0.0, GetRowCell(row, 1).Padding.Left);
            });
        }

        [TestMethod]
        public void VerifyGroupHeaderStyleCycleReportsNativeStyleError()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateGroupedThemingTable();
                // Keep headers unrealized so this tests our probes, not native live-style application.
                table.Template = (ControlTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    @"<ControlTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                        xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
                        <ItemsRepeater x:Name=""PART_RowsRepeater"" Visibility=""Collapsed"" />
                      </ControlTemplate>");
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                var first = new Style(typeof(TableViewGroupHeader));
                var second = new Style(typeof(TableViewGroupHeader)) { BasedOn = first };
                first.BasedOn = second;
                var expected = new Style(typeof(TableViewGroupHeader));
                expected.BasedOn = new Style(typeof(TableViewGroupHeader)) { BasedOn = expected };
                Exception nativeError = null;
                try { expected.Seal(); }
                catch (Exception ex) { nativeError = ex; }
                Verify.IsNotNull(nativeError);
                var repeater = GetRowsRepeater(table);
                var selector = (DataTemplateSelector)repeater.ItemTemplate;
                var groupItem = repeater.ItemsSourceView.GetAt(0);
                try
                {
                    table.Resources[typeof(TableViewGroupHeader)] = first;
                    Exception error = null;
                    try { selector.SelectTemplate(groupItem); }
                    catch (Exception ex) { error = ex; }
                    Verify.IsNotNull(error, "A cyclic group style must fail instead of hanging during setter probes.");
                    Verify.AreEqual(nativeError.HResult, error.HResult, "Preserve native invalid-style error semantics.");
                }
                finally
                {
                    table.Resources.Remove(typeof(TableViewGroupHeader));
                    first.BasedOn = null;
                    expected.BasedOn = null;
                }
                var valid = new Style(typeof(TableViewGroupHeader));
                valid.Setters.Add(new Setter(Control.PaddingProperty, new Thickness(16)));
                for (int i = 0; i < 100; ++i)
                {
                    valid = new Style(typeof(TableViewGroupHeader)) { BasedOn = valid };
                }
                try
                {
                    table.Resources[typeof(TableViewGroupHeader)] = valid;
                    Verify.IsNotNull(selector.SelectTemplate(groupItem),
                        "A valid deep chain must not be rejected by an arbitrary depth limit.");
                }
                finally { table.Resources.Remove(typeof(TableViewGroupHeader)); }
            });
        }

        [TestMethod]
        public void VerifyGroupHeaderDensityPreservesAppMetrics()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedThemingTable();
                var baseStyle = new Style(typeof(TableViewGroupHeader));
                baseStyle.Setters.Add(new Setter(FrameworkElement.MinHeightProperty, 64.0));
                baseStyle.Setters.Add(new Setter(Control.PaddingProperty, new Thickness(16, 8, 16, 8)));
                tableView.Resources[typeof(TableViewGroupHeader)] = new Style(typeof(TableViewGroupHeader)) { BasedOn = baseStyle };
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            foreach (var testCase in DensityCases)
            {
                RunOnUIThread.Execute(() => tableView.Density = testCase.Density);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                    Verify.AreEqual(64.0, header.MinHeight, "A BasedOn Style's MinHeight wins over density.");
                    Verify.AreEqual(new Thickness(16, 8, 16, 8), header.Padding, "A BasedOn Style's Padding wins over density.");
                    header.MinHeight = 72;
                    header.Padding = new Thickness(20, 12, 20, 12);
                    tableView.Density = TableViewDensity.Standard;
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                    Verify.AreEqual(72.0, header.MinHeight, "Local MinHeight wins over density.");
                    Verify.AreEqual(new Thickness(20, 12, 20, 12), header.Padding, "Local Padding wins over density.");
                    header.ClearValue(FrameworkElement.MinHeightProperty);
                    header.ClearValue(Control.PaddingProperty);
                });
                IdleSynchronizer.Wait();
            }
        }

        [TestMethod]
        public void VerifyGroupHeaderDensityPreservesLocalMetricsAfterLoad()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedThemingTable();
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                foreach (var header in FindVisualChildrenByType<TableViewGroupHeader>(tableView))
                {
                    header.MinHeight = 72;
                    header.Padding = new Thickness(20, 12, 20, 12);
                }
            });
            foreach (var testCase in DensityCases)
            {
                RunOnUIThread.Execute(() => tableView.Density = testCase.Density);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                    Verify.AreEqual(72.0, header.MinHeight, "A new local MinHeight replaces the density-owned value.");
                    Verify.AreEqual(new Thickness(20, 12, 20, 12), header.Padding,
                        "A new local Padding replaces the density-owned value.");
                });
            }
        }

        [TestMethod]
        public void VerifyGroupHeaderDensityGrowsForLargerText()
        {
            foreach (var testCase in DensityCases)
            foreach (var fontSize in new[] { 28.0, 42.0 })
            {
                TableView tableView = null;
                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateGroupedThemingTable();
                    tableView.Density = testCase.Density;
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    foreach (var header in FindVisualChildrenByType<TableViewGroupHeader>(tableView))
                    {
                        header.FontSize = fontSize;
                    }
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                    var presenter = (ContentPresenter)header.FindVisualChildByName("PART_ContentPresenter");
                    var minimum = testCase.RowMinHeight;
                    Verify.IsTrue(header.IsTextScaleFactorEnabled, "Section headers retain system text scaling.");
                    Verify.IsGreaterThan(header.ActualHeight, minimum, "Larger section text grows beyond the density minimum.");
                    Verify.IsGreaterThanOrEqual(header.ActualHeight, presenter.ActualHeight,
                        "The presenter includes its text padding and must fit inside the section.");
                    foreach (var text in FindVisualChildrenByType<TextBlock>(presenter))
                    {
                        var top = text.TransformToVisual(header).TransformPoint(new Point()).Y;
                        Verify.IsGreaterThanOrEqual(top, 0.0, "The complete line box starts inside the section.");
                        Verify.IsLessThanOrEqual(top + text.ActualHeight, header.ActualHeight,
                            "Larger section text keeps its complete line box inside the section.");
                    }
                });
            }
        }

        // Density is one of the better-specified areas in the control: the presets are literal values in
        // TableView_themeresources.xaml rather than constants in code.
        //
        // The row Style has no MinHeight setter, so each preset must reach the row from code.
        [TestMethod]
        public void VerifyRowMinHeightFollowsDensity()
        {
            foreach (var testCase in DensityCases)
            {
                TableView tableView = null;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.Density = testCase.Density;
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                    if (row == null)
                    {
                        Verify.Fail($"The TableView should realize at least one row at Density={testCase.Density}.");
                        return;
                    }

                    Verify.AreEqual(testCase.RowMinHeight, row.MinHeight,
                        $"Density={testCase.Density} must give rows a MinHeight of {testCase.RowMinHeight}, from " +
                        "TableView_themeresources.xaml, applied by the row in code.");
                });
            }
        }

        // TableView-dev-spec.md:83 states it directly: "Header cells use the density row minimum height,
        // so the header band matches body rows.
        [TestMethod]
        public void VerifyHeaderBandHeightFollowsDensity()
        {
            foreach (var testCase in DensityCases)
            {
                TableView tableView = null;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.Density = testCase.Density;
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var headerCell = GetHeaderCell(tableView, 0);
                    if (headerCell == null)
                    {
                        return;
                    }

                    Verify.AreEqual(testCase.RowMinHeight, headerCell.MinHeight,
                        $"Density={testCase.Density} must give header cells the same MinHeight as body rows " +
                        $"({testCase.RowMinHeight}), so the header band lines up with the grid it labels.");

                    var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                    if (row == null)
                    {
                        Verify.Fail($"The TableView should realize at least one row at Density={testCase.Density}.");
                        return;
                    }

                    Verify.AreEqual(row.MinHeight, headerCell.MinHeight,
                        "Header cells and body rows must agree on height at every density.");
                });
            }
        }

        // Horizontal padding is identical across all three densities by design - density is vertical
        // only - so a test asserting horizontal change would be asserting a bug.
        [TestMethod]
        public void VerifyCellAndHeaderPaddingFollowDensity()
        {
            foreach (var testCase in DensityCases)
            {
                TableView tableView = null;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.Density = testCase.Density;
                    // Sortable headers add the sort indicator's width to their trailing padding; this
                    // test is about density padding, so compare against an unsortable header.
                    tableView.Columns[0].CanSort = false;
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var cellText = RequireFirstCellTextBlock(tableView);
                    if (cellText == null)
                    {
                        return;
                    }

                    Verify.AreEqual(GetAlignedContentPadding(GetRealizedRows(tableView).First(), testCase.TextPadding), cellText.Padding,
                        $"Density={testCase.Density} redistributes padding without changing its total.");

                    var headerPresenter = GetHeaderPresenter(tableView, 0);
                    Verify.AreEqual(testCase.TextPadding, headerPresenter.Padding,
                        $"Density={testCase.Density} optically aligns only the header label.");
                });
            }
        }

        [TestMethod]
        public void VerifyAuthoredTextPaddingResources()
        {
            RunOnUIThread.Execute(() =>
            {
                var resources = new TabularControlsResources();
                foreach (var theme in new[] { "Light", "Default", "HighContrast" })
                {
                    var dictionary = (ResourceDictionary)resources.ThemeDictionaries[theme];
                    Verify.IsFalse(dictionary.ContainsKey("TableViewCellTextVerticalOffset"));
                    Verify.IsFalse(dictionary.ContainsKey("TableViewHeaderTextVerticalOffset"));
                    foreach (var density in DensityCases)
                    {
                        Verify.AreEqual(density.TextPadding, dictionary["TableViewCellTextPadding" + density.ResourceSuffix]);
                        Verify.AreEqual(density.TextPadding, dictionary["TableViewHeaderTextPadding" + density.ResourceSuffix]);
                        Verify.AreEqual(new Thickness(0, density.TextPadding.Top, 0, density.TextPadding.Bottom),
                            dictionary["TableViewGroupHeaderTextPadding" + density.ResourceSuffix]);
                    }
                }
            });
        }

        [TestMethod]
        public void VerifyAuthoredTextPaddingOverridesAndLiveHeaderContent()
        {
            foreach (var density in DensityCases)
            foreach (var useOverride in new[] { false, true })
            {
                TableView table = null;
                TableViewColumnHeader header = null;
                var textPadding = useOverride ? new Thickness(16, 0, 20, 4) : density.TextPadding;
                var sectionPadding = new Thickness(0, textPadding.Top, 0, textPadding.Bottom);
                RunOnUIThread.Execute(() =>
                {
                    table = CreateGroupedThemingTable();
                    table.Density = density.Density;
                    if (useOverride)
                    {
                        table.Resources["TableViewCellTextPadding" + density.ResourceSuffix] = textPadding;
                        table.Resources["TableViewHeaderTextPadding" + density.ResourceSuffix] = textPadding;
                        table.Resources["TableViewGroupHeaderTextPadding" + density.ResourceSuffix] = sectionPadding;
                    }
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var row = GetRealizedRows(table).First();
                    var text = RequireFirstCellTextBlock(table);
                    header = RequireColumnHeader(table, 0);
                    var presenter = GetHeaderPresenter(table, 0);
                    Verify.AreEqual(density.RowMinHeight, row.ActualHeight);
                    Verify.AreEqual(density.RowMinHeight, header.ActualHeight);
                    Verify.AreEqual(density.CellPadding, header.Padding, "Optical alignment must not change control padding.");
                    Verify.AreEqual(GetAlignedContentPadding(row, textPadding), text.Padding);
                    Verify.AreEqual(textPadding, presenter.Padding);
                    var section = FindVisualChildrenByType<TableViewGroupHeader>(table).First();
                    Verify.AreEqual(GetAlignedContentPadding(section, sectionPadding),
                        ((ContentPresenter)section.FindVisualChildByName("PART_ContentPresenter")).Padding);
                    Verify.AreEqual(DependencyProperty.UnsetValue, text.ReadLocalValue(UIElement.RenderTransformProperty));
                    Verify.AreEqual(DependencyProperty.UnsetValue,
                        ((TextBlock)header.Content).ReadLocalValue(UIElement.RenderTransformProperty));
                    header.Padding = new Thickness(20, 3, 28, 7);
                    table.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(header.Padding, GetHeaderPresenter(table, 0).Padding,
                        "Generated label padding must follow a live control-padding change.");
                    header.Content = new TextBlock { Text = "App-owned replacement" };
                    table.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(header.Padding, GetHeaderPresenter(table, 0).Padding,
                        "Replacing generated content must restore the template's unadjusted padding.");
                });
            }
        }

        [TestMethod]
        public void VerifyGeneratedTextLayoutStaysInsideSeparators()
        {
            foreach (var density in DensityCases)
            foreach (var lines in new[] { TableViewGridLinesVisibility.All, TableViewGridLinesVisibility.None })
            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                TableView table = null;
                TableViewColumnHeader header = null;
                TableViewRow row = null;
                FrameworkElement cell = null;
                FrameworkElement chrome = null;
                TableViewGroupHeader section = null;
                FrameworkElement sectionChrome = null;
                TextBlock sectionText = null;
                RunOnUIThread.Execute(() =>
                {
                    table = CreateGroupedThemingTable();
                    table.Density = density.Density;
                    table.RequestedTheme = theme;
                    table.GridLinesVisibility = lines;
                    table.CanUserSortColumns = false;
                    table.SelectionMode = TableViewSelectionMode.None;
                    table.ShowGroupHeaderCount = false;
                    table.IsTextScaleFactorEnabled = false;
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    header = RequireColumnHeader(table, 0);
                    row = GetRealizedRows(table).First();
                    cell = GetRowCell(row, 0);
                    chrome = (FrameworkElement)row.FindVisualChildByName("PART_RowChrome");
                    section = FindVisualChildrenByType<TableViewGroupHeader>(table).First();
                    sectionChrome = (FrameworkElement)section.FindVisualChildByName("PART_RowChrome");
                    sectionText = FindVisualChildrenByType<TextBlock>(section)
                        .First(text => text.Text == ((TableViewGroupInfo)section.Content).KeyText);
                });
                foreach (var weight in new[] { Microsoft.UI.Text.FontWeights.Normal, Microsoft.UI.Text.FontWeights.SemiBold })
                foreach (var label in new[] { "HIM", "Name" })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        ((TextBlock)header.Content).Text = label;
                        header.FontWeight = weight;
                        table.FontWeight = weight;
                        var text = RequireFirstCellTextBlock(table);
                        text.Text = label;
                        sectionText.Text = label;
                        section.FontWeight = weight;
                        table.UpdateLayout();
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        foreach (var target in new FrameworkElement[] { header, cell, section })
                        {
                            var text = target == header ? (TextBlock)header.Content :
                                target == cell ? RequireFirstCellTextBlock(table) : sectionText;
                            double interiorTop = 0;
                            double interiorBottom;
                            if (target == header)
                            {
                                interiorBottom = header.ActualHeight -
                                    ((Border)table.FindVisualChildByName("PART_HeaderGridLine")).BorderThickness.Bottom;
                                Verify.AreEqual(density.TextPadding, GetHeaderPresenter(table, 0).Padding);
                            }
                            else
                            {
                                var targetChrome = target == cell ? chrome : sectionChrome;
                                interiorTop = targetChrome.TransformToVisual(target).TransformPoint(new Point()).Y;
                                interiorBottom = interiorTop + targetChrome.ActualHeight;
                                if (target == cell)
                                {
                                    Verify.AreEqual(GetAlignedContentPadding(row, density.TextPadding), text.Padding);
                                }
                                else
                                {
                                    Verify.AreEqual(GetAlignedContentPadding(section,
                                        new Thickness(0, density.TextPadding.Top, 0, density.TextPadding.Bottom)),
                                        ((ContentPresenter)section.FindVisualChildByName("PART_ContentPresenter")).Padding);
                                }
                            }
                            var context = $"{density.Density}, {lines}, {theme}, weight={weight.Weight}, {label}, {target.GetType().Name}";
                            Verify.AreEqual(TextLineBounds.Full, text.TextLineBounds, context);
                            Verify.AreEqual(density.RowMinHeight, target.ActualHeight, context);
                            var bounds = text.TransformToVisual(target).TransformBounds(new Rect(
                                text.Padding.Left, text.Padding.Top,
                                text.ActualWidth - text.Padding.Left - text.Padding.Right,
                                text.ActualHeight - text.Padding.Top - text.Padding.Bottom));
                            var tolerance = 1 / target.XamlRoot.RasterizationScale + 0.001;
                            Log.Comment($"{context}: text={bounds}, interior={interiorTop}..{interiorBottom}.");
                            Verify.IsGreaterThan(bounds.Width, 0.0, context);
                            Verify.IsGreaterThan(bounds.Height, 0.0, context);
                            Verify.IsGreaterThanOrEqual(bounds.Top, interiorTop - tolerance,
                                $"The full text layout must fit below the top separator ({context}).");
                            Verify.IsLessThanOrEqual(bounds.Bottom, interiorBottom + tolerance,
                                $"The full text layout must fit inside the separator bounds ({context}).");
                        }
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyDisplayOpticalAlignmentPreservesFullHeightAndCustomContent()
        {
            foreach (var density in DensityCases)
            foreach (var textPadding in new[] { density.TextPadding, density.CellPadding })
            {
                TableView table = null;
                RunOnUIThread.Execute(() =>
                {
                    var resources = new TabularControlsResources();
                    foreach (var theme in new[] { "Light", "Default", "HighContrast" })
                    {
                        Verify.AreEqual(density.TextPadding, ((ResourceDictionary)resources.ThemeDictionaries[theme])[
                            "TableViewCellTextPadding" + density.ResourceSuffix]);
                    }
                    table = CreateThemingTable();
                    table.Density = density.Density;
                    table.Resources["TableViewCellTextPadding" + density.ResourceSuffix] = textPadding;
                    table.Columns.Add(new TableViewTemplateColumn
                    {
                        Header = "Custom",
                        CellTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                            @"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                                <TextBlock Text=""Custom content"">
                                    <TextBlock.RenderTransform><TranslateTransform Y=""3"" /></TextBlock.RenderTransform>
                                </TextBlock>
                              </DataTemplate>")
                    });
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                foreach (var fontSize in new[] { 28.0, 42.0 })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var text = RequireFirstCellTextBlock(table);
                        text.FontSize = fontSize;
                        text.Text = "\u00C1gj \u00C5p \u4E2D\u6587";
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var row = GetRealizedRows(table).First();
                        var text = RequireFirstCellTextBlock(table);
                        Verify.AreEqual(DependencyProperty.UnsetValue, text.ReadLocalValue(UIElement.RenderTransformProperty));
                        Verify.AreEqual(TextLineBounds.Full, text.TextLineBounds);
                        Verify.IsTrue(text.IsTextScaleFactorEnabled);
                        Verify.IsGreaterThan(row.ActualHeight, density.RowMinHeight, "Large text must still grow the row.");
                        Verify.AreEqual(GetAlignedContentPadding(row, textPadding), text.Padding,
                            "Display text must use authored padding plus divider compensation.");
                        var bounds = text.TransformToVisual(row).TransformBounds(new Rect(0, 0, text.ActualWidth, text.ActualHeight));
                        Verify.IsGreaterThanOrEqual(bounds.Top + text.Padding.Top, 0.0, "Keep the complete font line box inside the row.");
                        Verify.IsLessThanOrEqual(bounds.Bottom - text.Padding.Bottom, row.ActualHeight, "Do not clip accents or descenders.");
                        var customText = FindVisualChildrenByType<TextBlock>(GetRowCell(row, 2)).Single();
                        Verify.AreEqual(3.0, ((TranslateTransform)customText.RenderTransform).Y,
                            "Do not reposition app-owned cell-template content.");
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyGeneratedHeaderTextLayoutFollowsTypography()
        {
            foreach (var density in DensityCases)
            {
                TableView table = null;
                TableViewColumnHeader header = null;
                TextBlock text = null;
                RunOnUIThread.Execute(() =>
                {
                    table = CreateThemingTable();
                    table.Density = density.Density;
                    table.CanUserSortColumns = false;
                    table.IsTextScaleFactorEnabled = false;
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    header = RequireColumnHeader(table, 0);
                    text = (TextBlock)header.Content;
                });
                foreach (var label in new[] { "Name", "Role", "Status" })
                foreach (var weight in new[] { Microsoft.UI.Text.FontWeights.Normal, Microsoft.UI.Text.FontWeights.SemiBold })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        text.Text = label;
                        header.FontWeight = weight;
                        table.UpdateLayout();
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var context = $"{density.Density}, {label}, weight={weight.Weight}";
                        Verify.AreEqual(density.RowMinHeight, header.ActualHeight, context);
                        Verify.AreEqual(TextLineBounds.Full, text.TextLineBounds, context);
                        Verify.AreEqual(header.FontSize, text.FontSize, "Generated text must inherit header typography.");
                        Verify.AreEqual(weight, text.FontWeight, "Generated text must follow live font-weight changes.");
                        Verify.AreEqual(VerticalAlignment.Center, text.VerticalAlignment, context);
                        Verify.AreEqual(density.TextPadding, GetHeaderPresenter(table, 0).Padding, context);
                        Verify.IsFalse(text.IsTextTrimmed, $"Short labels must fit without trimming ({context}).");
                        var bounds = text.TransformToVisual(header).TransformBounds(
                            new Rect(0, 0, text.ActualWidth, text.ActualHeight));
                        var tolerance = 1 / header.XamlRoot.RasterizationScale + 0.001;
                        Verify.IsGreaterThan(bounds.Width, 0.0, context);
                        Verify.IsGreaterThan(bounds.Height, 0.0, context);
                        Verify.IsGreaterThanOrEqual(bounds.Top, -tolerance, context);
                        Verify.IsLessThanOrEqual(bounds.Bottom, header.ActualHeight + tolerance, context);
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyHeaderOpticalAlignmentPreservesFullHeightAndCustomContent()
        {
            foreach (var density in DensityCases)
            {
                TableView table = null;
                TextBlock appContent = null;
                TranslateTransform appTransform = null;
                RunOnUIThread.Execute(() =>
                {
                    var resources = new TabularControlsResources();
                    foreach (var theme in new[] { "Light", "Default", "HighContrast" })
                    {
                        Verify.AreEqual(density.TextPadding, ((ResourceDictionary)resources.ThemeDictionaries[theme])[
                            "TableViewHeaderTextPadding" + density.ResourceSuffix]);
                    }
                    table = CreateThemingTable();
                    table.Density = density.Density;
                    appTransform = new TranslateTransform { Y = 3 };
                    appContent = new TextBlock { Text = "App-owned content", RenderTransform = appTransform };
                    table.Columns[1].Header = appContent;
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                foreach (var fontSize in new[] { 28.0, 42.0 })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var header = RequireColumnHeader(table, 0);
                        header.FontSize = fontSize;
                        ((TextBlock)header.Content).Text = "\u00C1gj \u00C5p \u4E2D\u6587";
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var header = RequireColumnHeader(table, 0);
                        var text = (TextBlock)header.Content;
                        Verify.AreEqual(DependencyProperty.UnsetValue, text.ReadLocalValue(UIElement.RenderTransformProperty));
                        Verify.AreEqual(TextLineBounds.Full, text.TextLineBounds);
                        Verify.IsTrue(text.IsTextScaleFactorEnabled);
                        Verify.IsGreaterThan(header.ActualHeight, density.RowMinHeight, "Large text must still grow the header.");
                        var bounds = text.TransformToVisual(header).TransformBounds(new Rect(0, 0, text.ActualWidth, text.ActualHeight));
                        Verify.IsGreaterThanOrEqual(bounds.Top, 0.0, "The complete line box must remain inside the header.");
                        Verify.IsLessThanOrEqual(bounds.Bottom, header.ActualHeight, "Do not clip accents or descenders to optically center the label.");
                        Verify.AreEqual(appContent, RequireColumnHeader(table, 1).Content);
                        Verify.AreSame(appTransform, appContent.RenderTransform, "App-owned header content must retain its authored positioning.");
                        Verify.AreEqual(3.0, appTransform.Y);
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyDensityRenderedHeightsAndContentInsets()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                tableView.IsTextScaleFactorEnabled = false;
                tableView.Columns[0].CanSort = false;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            foreach (var testCase in DensityCases)
            {
                foreach (var lines in new[] { TableViewGridLinesVisibility.All, TableViewGridLinesVisibility.None })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        tableView.Density = testCase.Density;
                        tableView.GridLinesVisibility = lines;
                        tableView.UpdateLayout();
                    });
                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        var row = GetRealizedRows(tableView).First();
                        var cell = GetRowCell(row, 0);
                        var text = RequireFirstCellTextBlock(tableView);
                        var header = GetHeaderCell(tableView, 0);
                        var presenter = GetHeaderPresenter(tableView, 0);
                        var headerBand = (FrameworkElement)tableView.FindVisualChildByName("PART_HeaderRow");
                        var context = $"{testCase.Density}, {lines}";
                        Verify.AreEqual(testCase.RowMinHeight, row.ActualHeight, $"Rendered row height: {context}.");
                        Verify.AreEqual(testCase.RowMinHeight, header.ActualHeight, $"Rendered header height: {context}.");
                        Verify.AreEqual(testCase.RowMinHeight, headerBand.ActualHeight, $"Header rule must not add height: {context}.");

                        var bodyOrigin = text.TransformToVisual(cell).TransformPoint(new Point());
                        var headerOrigin = presenter.TransformToVisual(header).TransformPoint(new Point());
                        Verify.AreEqual(12.0, bodyOrigin.X + text.Padding.Left, $"Body text starts 12px inside the cell: {context}.");
                        Verify.AreEqual(12.0, headerOrigin.X + presenter.Padding.Left, $"Header and body text align: {context}.");
                        var roundingTolerance = 1.0 / tableView.XamlRoot.RasterizationScale;
                        Verify.AreEqual(DependencyProperty.UnsetValue, text.ReadLocalValue(UIElement.RenderTransformProperty));
                        Verify.IsLessThanOrEqual(Math.Abs(row.ActualHeight - text.ActualHeight - 2 * bodyOrigin.Y),
                            roundingTolerance, $"The full padded line box remains vertically centered: {context}.");
                        Verify.IsLessThanOrEqual(Math.Abs(header.ActualHeight - presenter.ActualHeight - 2 * headerOrigin.Y),
                            roundingTolerance, $"Padded header text is vertically centered: {context}.");
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyDensityRowsGrowForLargerText()
        {
            foreach (var testCase in DensityCases)
            {
                TableView tableView = null;
                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.Density = testCase.Density;
                    Verify.IsTrue(tableView.IsTextScaleFactorEnabled, "Density must not disable system text scaling.");
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    RequireFirstCellTextBlock(tableView).FontSize = 28;
                    ((TableViewColumnHeader)GetHeaderCell(tableView, 0)).FontSize = 28;
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var row = GetRealizedRows(tableView).First();
                    var text = RequireFirstCellTextBlock(tableView);
                    var header = GetHeaderCell(tableView, 0);
                    Verify.IsTrue(text.IsTextScaleFactorEnabled, "Generated text must keep system text scaling.");
                    Verify.IsGreaterThan(row.ActualHeight, testCase.RowMinHeight, "Larger text must grow the row beyond its density minimum.");
                    Verify.IsGreaterThan(header.ActualHeight, testCase.RowMinHeight, "Larger text must grow the header.");
                    Verify.IsGreaterThanOrEqual(row.ActualHeight, text.ActualHeight, "The row must fit the padded text rather than clip it.");
                    Verify.IsLessThanOrEqual(row.ActualHeight - text.ActualHeight, 1.0 / tableView.XamlRoot.RasterizationScale,
                        "Extra row space must be no more than layout rounding.");
                });
            }
        }

        [TestMethod]
        public void VerifyFocusMarginsKeepBaselineExceptHighContrastDropdown()
        {
            bool highContrast = false;
            RunOnUIThread.Execute(() =>
            {
                highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;
                var resources = new TabularControlsResources();
                foreach (var theme in new[] { "Light", "Default", "HighContrast" })
                {
                    var dictionary = (ResourceDictionary)resources.ThemeDictionaries[theme];
                    Verify.AreEqual(new Thickness(0),
                        (Thickness)dictionary["TableViewFocusVisualMargin"], theme);
                    Verify.AreEqual(new Thickness(0, -1, 0, -1),
                        (Thickness)dictionary["TableViewRowFocusVisualMargin"], theme);
                    Verify.AreEqual(new Thickness(theme == "HighContrast" ? 0 : -1),
                        (Thickness)dictionary["TableViewHeaderFlyoutButtonFocusVisualMargin"], theme);
                }
            });
            Log.Comment($"System High Contrast: {highContrast}");

            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedThemingTable();
                tableView.IsTextScaleFactorEnabled = false;
                tableView.Columns[0].HeaderFlyout = new MenuFlyout();
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                foreach (var testCase in DensityCases)
                {
                    foreach (var lines in new[] { TableViewGridLinesVisibility.All, TableViewGridLinesVisibility.None })
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            tableView.RequestedTheme = theme;
                            tableView.FlowDirection = theme == ElementTheme.Dark ? FlowDirection.RightToLeft : FlowDirection.LeftToRight;
                            tableView.Density = testCase.Density;
                            tableView.GridLinesVisibility = lines;
                        });
                        IdleSynchronizer.Wait();

                        foreach (var surface in new[] { "Row", "Cell", "Column", "Section", "Dropdown" })
                        {
                            FrameworkElement target = null;
                            RunOnUIThread.Execute(() =>
                            {
                                var row = GetRealizedRows(tableView).First();
                                var header = RequireColumnHeader(tableView, 0);
                                switch (surface)
                                {
                                    case "Row": target = row; break;
                                    case "Cell": target = GetRowCell(row, 0); break;
                                    case "Column": target = header; break;
                                    case "Section": target = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First(); break;
                                    case "Dropdown": target = (DropDownButton)header.FindVisualChildByName("PART_HeaderFlyoutButton"); break;
                                }
                                Verify.IsNotNull(target, surface);
                                target.IsTabStop = true;
                                Verify.IsTrue(target.Focus(FocusState.Keyboard), surface);
                            });
                            IdleSynchronizer.Wait();

                            RunOnUIThread.Execute(() =>
                            {
                                var context = $"{theme}, {testCase.Density}, {lines}, {surface}";
                                Verify.AreEqual(FocusState.Keyboard, target.FocusState, context);
                                Verify.IsTrue(target.UseSystemFocusVisuals, context);
                                var expected = new Thickness(0);
                                if (surface == "Row" || surface == "Cell")
                                {
                                    expected = new Thickness(0, -1, 0, -1);
                                    if (surface == "Cell")
                                    {
                                        var row = GetRealizedRows(tableView).First();
                                        var chrome = (FrameworkElement)row.FindVisualChildByName("PART_RowChrome");
                                        var bounds = chrome.TransformToVisual(target).TransformBounds(
                                            new Rect(0, 0, chrome.ActualWidth, chrome.ActualHeight));
                                        expected.Top += bounds.Top;
                                        expected.Bottom += target.ActualHeight - bounds.Bottom;
                                    }
                                }
                                else if (surface == "Dropdown")
                                {
                                    expected = new Thickness(highContrast ? 0 : -1);
                                }
                                Verify.AreEqual(expected, target.FocusVisualMargin, context);
                                Verify.AreEqual(testCase.RowMinHeight, GetRealizedRows(tableView).First().ActualHeight,
                                    $"Focus margins must not grow row layout: {context}");
                            });
                        }
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyDensitySelectionOutlineSpacingFromNativeFocusVisuals()
        {
            bool highContrast = false;
            RunOnUIThread.Execute(() => highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast);
            foreach (var testCase in DensityCases)
            {
                TableView tableView = null;
                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.IsTextScaleFactorEnabled = false;
                    tableView.Density = testCase.Density;
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();

                foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        tableView.RequestedTheme = theme;
                        tableView.FlowDirection = theme == ElementTheme.Dark ? FlowDirection.RightToLeft : FlowDirection.LeftToRight;
                    });
                    IdleSynchronizer.Wait();

                    foreach (var focusCell in new[] { false, true })
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            var row = GetRealizedRows(tableView).First();
                            FrameworkElement target = focusCell ? GetRowCell(row, 0) : row;
                            target.IsTabStop = true;
                            Verify.IsTrue(target.Focus(FocusState.Keyboard), "The row or cell must take keyboard focus.");
                            Verify.IsTrue(VisualStateManager.GoToState(row, highContrast ? "HighContrastSelected" : "Selected", false));
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var row = GetRealizedRows(tableView).First();
                            var outline = (Border)row.FindVisualChildByName("PART_SelectionIndicator");
                            FrameworkElement target = focusCell ? GetRowCell(row, 0) : row;
                            Verify.AreEqual(FocusState.Keyboard, target.FocusState);
                            Verify.IsTrue(target.UseSystemFocusVisuals, "Keep native focus rendering for rows and cells.");
                            Verify.AreEqual(new Thickness(0, -1, 0, -1),
                                row.FocusVisualMargin, "All themes retain the original vertical focus outset.");
                            Verify.AreEqual(new Thickness(3, 2, 3, 2), outline.Margin);
                            var chrome = (FrameworkElement)row.FindVisualChildByName("PART_RowChrome");
                            Verify.IsTrue(Control.GetIsTemplateFocusTarget(chrome),
                                "Row focus must target the visual area outside the separator.");
                            var chromeBounds = chrome.TransformToVisual(row).TransformBounds(
                                new Rect(0, 0, chrome.ActualWidth, chrome.ActualHeight));
                            var targetBounds = focusCell
                                ? target.TransformToVisual(row).TransformBounds(
                                    new Rect(0, 0, target.ActualWidth, target.ActualHeight))
                                : chromeBounds;
                            var outlineBounds = outline.TransformToVisual(row).TransformBounds(
                                new Rect(0, 0, outline.ActualWidth, outline.ActualHeight));
                            var margin = target.FocusVisualMargin;
                            var primary = target.FocusVisualPrimaryThickness;
                            var secondary = target.FocusVisualSecondaryThickness;
                            Verify.IsLessThanOrEqual(Math.Abs(outlineBounds.Top -
                                (targetBounds.Top + margin.Top + primary.Top + secondary.Top)), 0.001,
                                "Selection must abut the inner focus stroke at the top in every theme.");
                            Verify.IsLessThanOrEqual(Math.Abs(
                                (targetBounds.Bottom - margin.Bottom - primary.Bottom - secondary.Bottom)
                                - outlineBounds.Bottom), 0.001,
                                "Row and cell focus must keep the same requested geometry above the separator.");
                            if (!focusCell)
                            {
                                Verify.IsLessThanOrEqual(Math.Abs(outlineBounds.Left -
                                    (targetBounds.Left + margin.Left + primary.Left + secondary.Left)), 0.001,
                                    "Full-row focus must abut selection horizontally in every theme.");
                                Verify.IsLessThanOrEqual(Math.Abs(
                                    (targetBounds.Right - margin.Right - primary.Right - secondary.Right)
                                    - outlineBounds.Right), 0.001,
                                    "Full-row focus must keep the same requested geometry on the right.");
                            }
                            Verify.AreEqual(chrome.ActualHeight - 6, outline.ActualHeight
                                - outline.BorderThickness.Top - outline.BorderThickness.Bottom,
                                "The selection outline keeps its existing content area without growing the row.");
                            Verify.AreEqual(1.0, outline.Opacity, "The selection indicator must remain enabled while focused.");
                            Verify.AreEqual(testCase.RowMinHeight, row.ActualHeight, "Focus and selection must not grow the row.");
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyHorizontalSeparatorsStayOutsideRowChrome()
        {
            foreach (var grouped in new[] { false, true })
            {
                TableView tableView = null;
                RunOnUIThread.Execute(() =>
                {
                    tableView = grouped ? CreateGroupedThemingTable() : CreateThemingTable();
                    tableView.IsTextScaleFactorEnabled = false;
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();

                foreach (var testCase in DensityCases)
                {
                    foreach (var mode in new[] { TableViewGridLinesVisibility.All, TableViewGridLinesVisibility.None,
                        TableViewGridLinesVisibility.Horizontal, TableViewGridLinesVisibility.Vertical })
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            tableView.Density = testCase.Density;
                            tableView.GridLinesVisibility = mode;
                            tableView.UpdateLayout();
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var row = GetRealizedRows(tableView).First();
                            var chrome = (FrameworkElement)row.FindVisualChildByName("PART_RowChrome");
                            var background = (Border)row.FindVisualChildByName("PART_RootBorder");
                            var outline = (Border)row.FindVisualChildByName("PART_SelectionIndicator");
                            var line = (Border)row.FindVisualChildByName("PART_GridLineBorder");
                            var horizontal = mode == TableViewGridLinesVisibility.All ||
                                mode == TableViewGridLinesVisibility.Horizontal;
                            Verify.AreEqual(horizontal ? 1.0 : 0.0, line.BorderThickness.Bottom);
                            Verify.AreEqual(new Thickness(0, 0, 0, line.BorderThickness.Bottom), chrome.Margin);
                            Verify.AreEqual(new Thickness(3, 2, 3, 2), background.Margin,
                                "Preserve the approved interaction-backplate inset.");
                            Verify.AreEqual(chrome.ActualHeight - 4, background.ActualHeight,
                                "Backplates retain their symmetric 2px vertical inset inside divider-free chrome.");
                            var chromeOrigin = chrome.TransformToVisual(row).TransformPoint(new Point());
                            Verify.AreEqual(new Point(chromeOrigin.X + 3, chromeOrigin.Y + 2),
                                background.TransformToVisual(row).TransformPoint(new Point()),
                                "Optical text alignment must not move the existing backplate.");
                            Verify.AreEqual(testCase.RowMinHeight, row.ActualHeight,
                                "Separators must not add height to any density.");
                            Verify.AreEqual(row.ActualHeight, GetRowCell(row, 0).ActualHeight,
                                "Cell content and hit targets retain the full density slot.");
                            var scale = tableView.XamlRoot.RasterizationScale;
                            var separatorHeight = Math.Round(line.BorderThickness.Bottom * scale,
                                MidpointRounding.AwayFromZero) / scale;
                            Verify.IsLessThanOrEqual(Math.Abs(row.ActualHeight - chrome.ActualHeight -
                                separatorHeight), 0.001, "The separator must sit outside the row visuals.");
                            Verify.AreEqual(chrome.ActualHeight - 4, outline.ActualHeight,
                                "Selection keeps its symmetric 2px inset within the row visuals.");
                            Verify.IsTrue(Control.GetIsTemplateFocusTarget(chrome));
                            var text = (TextBlock)GetCellContent(GetRowCell(row, 0));
                            Verify.AreEqual(GetAlignedContentPadding(row, testCase.TextPadding), text.Padding);
                            var chromeTop = chrome.TransformToVisual(row).TransformPoint(new Point()).Y;
                            var textTop = text.TransformToVisual(row).TransformPoint(new Point()).Y;
                            var lineCenter = textTop + (text.ActualHeight + text.Padding.Top - text.Padding.Bottom) / 2;
                            Verify.IsLessThanOrEqual(Math.Abs(lineCenter - chromeTop - chrome.ActualHeight / 2 + 1),
                                1.0 / scale, "Only the display text is optically lifted inside the unchanged row chrome.");
                            Verify.AreEqual(testCase.CellPadding.Top + testCase.CellPadding.Bottom,
                                text.Padding.Top + text.Padding.Bottom, "Alignment must not grow the text's padding.");
                            if (grouped)
                            {
                                var header = FindVisualChildrenByType<TableViewGroupHeader>(tableView).First();
                                var headerChrome = (FrameworkElement)header.FindVisualChildByName("PART_RowChrome");
                                var headerLine = (Border)header.FindVisualChildByName("PART_GridLineBorder");
                                Verify.AreEqual(mode == TableViewGridLinesVisibility.None ? 0.0 : 1.0,
                                    headerLine.BorderThickness.Bottom,
                                    "Sections keep a bottom separator even in Vertical mode.");
                                Verify.AreEqual(new Thickness(0, header.BorderThickness.Top, 0, headerLine.BorderThickness.Bottom),
                                    headerChrome.Margin);
                                Verify.AreEqual(testCase.RowMinHeight, header.ActualHeight);
                                Verify.IsTrue(Control.GetIsTemplateFocusTarget(headerChrome));
                                var contentHost = (Border)header.FindVisualChildByName("PART_ContentHost");
                                Verify.AreEqual(new Thickness(header.Padding.Left, 0, header.Padding.Right, 0), contentHost.Padding);
                                var presenter = (ContentPresenter)header.FindVisualChildByName("PART_ContentPresenter");
                                var sectionTextPadding = new Thickness(0, testCase.TextPadding.Top, 0, testCase.TextPadding.Bottom);
                                Verify.AreEqual(GetAlignedContentPadding(header, sectionTextPadding), presenter.Padding);
                                var contentCenter = presenter.TransformToVisual(header).TransformPoint(new Point()).Y
                                    + (presenter.ActualHeight + presenter.Padding.Top - presenter.Padding.Bottom) / 2;
                                var headerChromeCenter = headerChrome.TransformToVisual(header).TransformPoint(new Point()).Y
                                    + headerChrome.ActualHeight / 2;
                                Verify.IsLessThanOrEqual(Math.Abs(contentCenter - headerChromeCenter + 1),
                                    1.0 / scale, "Section text must match the cells' vertical alignment.");
                                var gutter = (Border)header.FindVisualChildByName("PART_ExpanderGutter");
                                Verify.AreEqual(GetAlignedContentPadding(header,
                                    new Thickness(0, header.Padding.Top, 0, header.Padding.Bottom)), gutter.Padding);
                                var icon = (FrameworkElement)header.FindVisualChildByName("PART_ExpanderIcon");
                                var iconCenter = icon.TransformToVisual(header).TransformPoint(new Point()).Y + icon.ActualHeight / 2;
                                Verify.IsLessThanOrEqual(Math.Abs(iconCenter - headerChromeCenter), 1.0 / scale,
                                    "Text alignment must not move the section chevron.");
                            }
                        });
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies header cells are rounded only while keyboard-focused, so their system focus visual is rounded without clipping.")]
        public void VerifyHeaderCellCornerRadiusMatchesControlCornerRadius()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            VerifyRoundedOnlyWhileKeyboardFocused(tableView,
                () => GetHeaderCell(tableView, 0) as TableViewColumnHeader, () => GetHeaderCell(tableView, 1), "header cell");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style's CornerRadius is kept while the header is keyboard-focused.")]
        public void VerifyHeaderStyleCornerRadiusSurvivesKeyboardFocus()
        {
            TableView tableView = null;
            var styleRadius = new CornerRadius(7);

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                var style = new Style(typeof(TableViewColumnHeader));
                style.Setters.Add(new Setter(Control.CornerRadiusProperty, styleRadius));
                tableView.ColumnHeaderStyle = style;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                Verify.AreNotEqual(styleRadius, tableView.CornerRadius, "Precondition: the style radius differs from the table's.");
                Verify.AreEqual(styleRadius, header.CornerRadius, "Precondition: the style radius reaches the header.");
                Verify.IsTrue(header.Focus(FocusState.Keyboard), "Precondition: the header takes keyboard focus.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                Verify.AreEqual(FocusState.Keyboard, header.FocusState, "Precondition: the header is keyboard-focused.");
                Verify.AreEqual(styleRadius, header.CornerRadius, "A keyboard-focused header must keep its Style's CornerRadius.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies data cells are rounded only while keyboard-focused, so their system focus visual is rounded without clipping.")]
        public void VerifyCellCornerRadiusMatchesControlCornerRadius()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            VerifyRoundedOnlyWhileKeyboardFocused(tableView,
                () => GetRowCell(FindVisualChildrenByType<TableViewRow>(tableView).First(), 0),
                () => GetHeaderCell(tableView, 0), "cell");
        }

        // A rounded Grid clips its children and hit-testing, so it may round only while keyboard-focused.
        private static CornerRadius RadiusOf(FrameworkElement element)
            => element is Control control ? control.CornerRadius : ((Grid)element).CornerRadius;

        private static void VerifyRoundedOnlyWhileKeyboardFocused(
            TableView tableView, Func<FrameworkElement> getTarget, Func<UIElement> getOther, string what)
        {
            var expected = new CornerRadius(0);

            RunOnUIThread.Execute(() =>
            {
                var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                Verify.IsNotNull(row, "The TableView should realize at least one row.");
                expected = row.CornerRadius;
                Verify.AreNotEqual(new CornerRadius(0), expected, "The row style should round row focus.");

                var target = getTarget();
                Verify.IsNotNull(target, $"The {what} should exist.");
                Verify.IsTrue(target.UseSystemFocusVisuals, $"The {what} should use the system focus visual.");
                Verify.AreEqual(new CornerRadius(0), RadiusOf(target),
                    $"An unfocused {what} must stay square: a rounded root clips its children.");
                target.IsTabStop = true;
                Verify.IsTrue(target.Focus(FocusState.Keyboard), $"Precondition: the {what} takes keyboard focus.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var target = getTarget();
                Verify.IsTrue(ReferenceEquals(target, FocusManager.GetFocusedElement(target.XamlRoot)),
                    $"Precondition: keyboard focus stays on the {what}.");
                Verify.AreEqual(FocusState.Keyboard, target.FocusState, $"Precondition: the {what} is keyboard-focused.");
                Verify.AreEqual(expected, RadiusOf(target),
                    $"A keyboard-focused {what} must be rounded like the rows' focus rect.");
                Verify.IsTrue(getOther().Focus(FocusState.Programmatic), "Precondition: focus moves elsewhere.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var target = getTarget();
                Verify.AreEqual(new CornerRadius(0), RadiusOf(target), $"The {what} must drop its radius when it loses focus.");
                target.IsTabStop = true;
                Verify.IsTrue(target.Focus(FocusState.Pointer), $"Precondition: the {what} takes pointer focus.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(new CornerRadius(0), RadiusOf(getTarget()),
                    $"Pointer focus shows no focus rect, so the {what} must stay square.");
                Verify.IsTrue(getOther().Focus(FocusState.Programmatic), "Precondition: focus moves elsewhere.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var target = getTarget();
                target.IsTabStop = true;
                Verify.IsTrue(target.Focus(FocusState.Programmatic), $"Precondition: the {what} takes programmatic focus.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // XAML coerces programmatic focus to Keyboard or Pointer from the content root's last
                // input device, which earlier tests or the harness may have left as keyboard; the
                // focus rect, and so the radius, follows the coerced state.
                var target = getTarget();
                var state = target.FocusState;
                Log.Comment($"Programmatic focus on the {what} was coerced to {state}.");
                Verify.IsTrue(state == FocusState.Keyboard || state == FocusState.Pointer,
                    $"Precondition: programmatic focus on the {what} is coerced to Keyboard or Pointer.");
                Verify.AreEqual(state == FocusState.Keyboard ? expected : new CornerRadius(0), RadiusOf(target),
                    $"Programmatic focus shows the focus rect only after keyboard input, so the {what} is rounded only then.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies group headers carry ControlCornerRadius so their system focus visual is rounded.")]
        public void VerifyGroupHeaderCornerRadiusMatchesRowCornerRadius()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                var groupHeader = FindVisualChildrenByType<TableViewGroupHeader>(tableView).FirstOrDefault();
                Verify.IsNotNull(row, "A grouped TableView should realize at least one row.");
                Verify.IsNotNull(groupHeader, "A grouped TableView should realize at least one group header.");
                Verify.IsTrue(groupHeader.UseSystemFocusVisuals, "Group headers should use the system focus visual.");
                Verify.AreNotEqual(new CornerRadius(0), row.CornerRadius, "The row style should round row focus.");
                Verify.AreEqual(row.CornerRadius, groupHeader.CornerRadius,
                    "Group headers must carry ControlCornerRadius so the keyboard focus rect is rounded like the rows'.");
            });
        }

        // Distinct from the tests above, which set density before load. Without this, density applies
        // only to rows realized AFTER the change, so a table that has already rendered shows a mix of
        // densities as the user scrolls.
        [TestMethod]
        public void VerifyDensityChangeAfterLoadUpdatesRealizedRows()
        {
            TableView tableView = null;
            TableViewRow realizedRow = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                Verify.AreEqual(TableViewDensity.Standard, tableView.Density,
                    "This test starts from the Standard default.");

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                realizedRow = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                Verify.IsNotNull(realizedRow);
                Verify.AreEqual(40.0, realizedRow.MinHeight, "The row should start at the Standard metric.");
            });

            foreach (var testCase in new[] { DensityCases[0], DensityCases[2], DensityCases[1] })
            {
                RunOnUIThread.Execute(() => tableView.Density = testCase.Density);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var rowAfter = FindVisualChildrenByType<TableViewRow>(tableView).First();
                    Verify.AreEqual(realizedRow, rowAfter,
                        "A density change must update the existing row rather than force a rebuild.");
                    Verify.AreEqual(testCase.RowMinHeight, rowAfter.MinHeight);
                    var cellText = RequireFirstCellTextBlock(tableView);
                    Verify.AreEqual(GetAlignedContentPadding(rowAfter, testCase.TextPadding), cellText.Padding);
                    Verify.AreEqual(testCase.FontSize, cellText.FontSize, "Realized text must follow live density changes.");
                    Verify.AreEqual(testCase.FontSize, GetHeaderPresenter(tableView, 0).FontSize);
                });
            }
        }

        // The only test that can tell the difference between the resource and the C++ fallback constant:
        // every other density test passes identically either way, because the two agree
        // (TableView.cpp DensityRowMinHeightFallback mirrors TableView_themeresources.xaml).
        //
        // Also guards the cache - override before load, since the resolved value is cached per instance.
        [TestMethod]
        public void VerifyDensityResourceOverrideWins()
        {
            const double overriddenMinHeight = 71.0;
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TableViewRowMinHeightCompact", overriddenMinHeight);

                tableView = CreateThemingTable();
                tableView.Density = TableViewDensity.Compact;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
                    if (row == null)
                    {
                        Verify.Fail("The TableView should realize at least one row.");
                        return;
                    }

                    Verify.AreEqual(overriddenMinHeight, row.MinHeight,
                        "An app-scope TableViewRowMinHeightCompact override must win. A value of 32 here means the metric came " +
                        "from the C++ fallback constant and the resource key is decorative.");
                });
            }
            finally
            {
                // Application.Resources is UI-thread affine; a finally block runs on the test thread.
                RunOnUIThread.Execute(() => ClearApplicationResource("TableViewRowMinHeightCompact"));
            }
        }

        [TestMethod]
        public void VerifyDensityFontSizePresets()
        {
            RunOnUIThread.Execute(() =>
            {
                var resources = new TabularControlsResources();
                foreach (var theme in new[] { "Light", "Default", "HighContrast" })
                {
                    var dictionary = (ResourceDictionary)resources.ThemeDictionaries[theme];
                    foreach (var key in new[] { "TableViewCellFontSize", "TableViewHeaderFontSize",
                        "TableViewCellFontSizeExtraCompact", "TableViewHeaderFontSizeExtraCompact" })
                    {
                        Verify.AreEqual(14.0, dictionary[key], $"{theme}: all built-in density presets must ship at 14px.");
                    }
                }
            });
            foreach (var testCase in DensityCases)
            {
                TableView tableView = null;

                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.Density = testCase.Density;
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var cellText = RequireFirstCellTextBlock(tableView);
                    if (cellText == null)
                    {
                        return;
                    }

                    Verify.AreEqual(testCase.FontSize, cellText.FontSize,
                        $"Built-in cell typography must follow Density={testCase.Density}.");

                    var headerPresenter = GetHeaderPresenter(tableView, 0);
                    Verify.AreEqual(testCase.FontSize, headerPresenter.FontSize,
                        $"Built-in header typography must follow Density={testCase.Density}.");
                });
            }
        }

        [TestMethod]
        public void VerifyDensityFontResourceOverridesWin()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                tableView.Resources["TableViewCellFontSizeExtraCompact"] = 13.0;
                tableView.Resources["TableViewHeaderFontSizeExtraCompact"] = 15.0;
                tableView.Density = TableViewDensity.ExtraCompact;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(13.0, RequireFirstCellTextBlock(tableView).FontSize);
                Verify.AreEqual(15.0, GetHeaderPresenter(tableView, 0).FontSize);
            });
        }

        [TestMethod]
        public void VerifyDensityPreservesColumnHeaderFontStyles()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                var tableBase = new Style(typeof(Control));
                tableBase.Setters.Add(new Setter(Control.FontSizeProperty, 17.0));
                tableView.ColumnHeaderStyle = new Style(typeof(TableViewColumnHeader)) { BasedOn = tableBase };
                var columnBase = new Style(typeof(ContentControl));
                columnBase.Setters.Add(new Setter(Control.FontSizeProperty, 18.0));
                tableView.Columns[1].HeaderStyle = new Style(typeof(TableViewColumnHeader)) { BasedOn = columnBase };
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            foreach (var testCase in DensityCases)
            {
                RunOnUIThread.Execute(() => tableView.Density = testCase.Density);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(17.0, GetHeaderPresenter(tableView, 0).FontSize);
                    Verify.AreEqual(18.0, GetHeaderPresenter(tableView, 1).FontSize);
                });
            }
        }

        [TestMethod]
        public void VerifyDensityPreservesImplicitColumnHeaderFontStyle()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                var style = new Style(typeof(TableViewColumnHeader));
                style.Setters.Add(new Setter(Control.FontSizeProperty, 18.0));
                tableView.Resources[typeof(TableViewColumnHeader)] = style;
                tableView.Density = TableViewDensity.ExtraCompact;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
                Verify.AreEqual(18.0, GetHeaderPresenter(tableView, 0).FontSize,
                    "The density default must not override an app's implicit font style."));
        }

        [TestMethod]
        public void VerifySelectedRowOutlineSpansTheRowWithoutChangingCellLayout()
        {
            TableView tableView = null;
            TableViewRow row = null;
            double cellWidth = 0;
            double cellHeight = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.All;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                row = GetRealizedRows(tableView).First();
                var cell = GetRowCell(row, 0);
                cellWidth = cell.ActualWidth;
                cellHeight = cell.ActualHeight;
            });

            foreach (var state in new[] { "Selected", "SelectedPointerOver", "SelectedPressed", "SelectedDisabled", "Normal" })
            {
                RunOnUIThread.Execute(() =>
                    Verify.IsTrue(VisualStateManager.GoToState(row, state, false), $"The row must define {state}."));
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var outline = row.FindVisualChildByName("PART_SelectionIndicator") as Border;
                    Verify.IsNotNull(outline, "The selection indicator must be a whole-row Border.");
                    Verify.AreEqual(new Thickness(1), outline.BorderThickness, "The selection outline is 1px.");
                    Verify.AreEqual(row.CornerRadius, outline.CornerRadius, "All selection corners must follow the row radius.");
                    Verify.AreEqual(new Thickness(3, 2, 3, 2), outline.Margin, "Selection must leave room for both native focus strokes.");
                    Verify.AreEqual(row.ActualWidth - 6, outline.ActualWidth, "Selection must span every column inside the focus visual.");
                    var chrome = (FrameworkElement)row.FindVisualChildByName("PART_RowChrome");
                    Verify.AreEqual(chrome.ActualHeight - 4, outline.ActualHeight,
                        "Selection uses a symmetric 2px inset outside the separator.");
                    Verify.IsFalse(outline.IsHitTestVisible, "Selection must not intercept cell input.");
                    Verify.AreEqual(state == "Normal" ? 0.0 : 1.0, outline.Opacity, $"Outline visibility in {state}.");
                    var cell = GetRowCell(row, 0);
                    Verify.AreEqual(cellWidth, cell.ActualWidth, $"{state} must not change cell width.");
                    Verify.AreEqual(cellHeight, cell.ActualHeight, $"{state} must not change cell height.");
                    if (state == "SelectedDisabled")
                    {
                        Verify.AreEqual(
                            RequireThemeColor(row.ActualTheme == ElementTheme.Dark ? "Default" : "Light",
                                "AccentFillColorDisabled"),
                            (outline.BorderBrush as SolidColorBrush)?.Color ?? default,
                            "Disabled selection must use the disabled outline brush.");
                    }
                });
            }
        }

        [TestMethod]
        public void VerifySelectedRowBackplateChangesWithoutChangingOutline()
        {
            TableView tableView = null;
            TableViewRow row = null;
            TabularControlsResources resources = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                resources = new TabularControlsResources();
                foreach (var themeName in new[] { "Light", "Default" })
                {
                    Verify.AreEqual(RequireThemeColor(themeName, "SubtleFillColorSecondary"),
                        RequireBrush(resources, themeName, "TabularSurfaceRowBackgroundSelectedDisabledBrush").Color,
                        $"{themeName}: disabled selection must retain the subtle selected fill.");
                }
                Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorWindowColorBrush"),
                    RequireBrush(resources, "HighContrast", "TabularSurfaceRowBackgroundSelectedDisabledBrush")),
                    "HighContrast must preserve the window background behind gray disabled text.");
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => row = GetRealizedRows(tableView).First());

            foreach (var testCase in new[]
            {
                (Theme: ElementTheme.Light, Density: TableViewDensity.Standard),
                (Theme: ElementTheme.Dark, Density: TableViewDensity.Compact),
                (Theme: ElementTheme.Light, Density: TableViewDensity.ExtraCompact)
            })
            {
                var theme = testCase.Theme;
                RunOnUIThread.Execute(() =>
                {
                    tableView.RequestedTheme = theme;
                    tableView.Density = testCase.Density;
                });
                IdleSynchronizer.Wait();

                foreach (var state in new[] { "Selected", "SelectedPointerOver", "SelectedPressed",
                    "PointerOver", "Selected", "Pressed", "Disabled", "SelectedDisabled", "Selected", "Normal" })
                {
                    RunOnUIThread.Execute(() =>
                        Verify.IsTrue(VisualStateManager.GoToState(row, state, false), $"The row must define {state}."));
                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        Verify.AreEqual(theme, row.ActualTheme, "The realized row must follow the requested theme.");
                        var themeName = theme == ElementTheme.Dark ? "Default" : "Light";
                        var backplate = row.FindVisualChildByName("PART_RootBorder") as Border;
                        var outline = row.FindVisualChildByName("PART_SelectionIndicator") as Border;
                        Verify.IsNotNull(backplate);
                        Verify.IsNotNull(outline);
                        var selected = state.StartsWith("Selected", StringComparison.Ordinal);
                        Verify.AreEqual(outline.ActualWidth, backplate.ActualWidth,
                            $"{theme}/{testCase.Density}/{state}: backplate width must match its visual bounds.");
                        Verify.AreEqual(outline.ActualHeight, backplate.ActualHeight,
                            $"{theme}/{testCase.Density}/{state}: backplate height must match its visual bounds.");
                        Verify.AreEqual(outline.TransformToVisual(row).TransformPoint(new Point()),
                            backplate.TransformToVisual(row).TransformPoint(new Point()),
                            $"{theme}/{testCase.Density}/{state}: backplate must not extend outside its visual bounds.");
                        Verify.AreEqual(selected ? outline.BorderThickness : new Thickness(0), backplate.BorderThickness,
                            "Only selected fills reserve the stroke; unselected hover/pressed fills reach the outline's outer edge.");
                        Verify.AreEqual(BackgroundSizing.InnerBorderEdge, backplate.BackgroundSizing);
                        Verify.AreEqual(outline.CornerRadius, backplate.CornerRadius);
                        Verify.IsNull(backplate.BorderBrush, "The backplate must not paint a second selection stroke.");
                        Verify.AreEqual(selected ? 1.0 : 0.0, outline.Opacity,
                            "Unselected hover and pressed states must not show a selection outline.");
                        var background = backplate.Background as SolidColorBrush;
                        var border = outline.BorderBrush as SolidColorBrush;
                        Verify.IsNotNull(background);
                        Verify.IsNotNull(border);
                        var backgroundKey = state == "Normal"
                            ? "TabularSurfaceRowBackgroundBrush"
                            : $"TabularSurfaceRowBackground{state}Brush";
                        Verify.AreEqual(
                            RequireBrush(resources, themeName, backgroundKey).Color,
                            background.Color,
                            $"{theme}/{state} must use its backplate brush.");
                        Verify.AreEqual(
                            state == "SelectedDisabled" ? RequireThemeColor(themeName, "AccentFillColorDisabled") :
                                RequireBrush(resources, themeName, "TabularSurfaceSelectionIndicatorBrush").Color,
                            border.Color,
                            $"{theme}/{state}: hover and press must preserve the selection outline; disabled has its own brush.");
                        if (state == "Disabled" || state == "SelectedDisabled")
                        {
                            var foreground = (row.FindVisualChildByName("PART_CellForegroundPresenter") as ContentPresenter)?.Foreground as SolidColorBrush;
                            Verify.IsNotNull(foreground);
                            Verify.AreEqual(RequireThemeColor(themeName, "TextFillColorDisabled"), foreground.Color,
                                $"{theme}/{state} must preserve disabled text.");
                        }
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyHeaderFillCornersFollowVisibleEdges()
        {
            TableView tableView = null;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role", "Status" });
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            void VerifyCorners()
            {
                var headers = FindVisualChildrenByType<TableViewColumnHeader>(tableView)
                    .Where(header => header.Visibility == Visibility.Visible && header.Column.ActualWidth > 0).ToList();
                Verify.IsGreaterThan(headers.Count, 0, "At least one header must be visible.");
                for (int i = 0; i < headers.Count; ++i)
                {
                    var header = headers[i];
                    var fill = header.FindVisualChildByName("PART_Background") as Border;
                    var separator = header.FindVisualChildByName("PART_GridLine") as Border;
                    Verify.IsNotNull(fill, "The header fill must be separate from content and resize hit targets.");
                    var leading = i == 0;
                    var trailing = i == headers.Count - 1 && separator.Visibility == Visibility.Collapsed;
                    var expected = new CornerRadius(
                        leading ? 4 : 0, trailing ? 4 : 0, 0, 0);
                    Verify.AreEqual(expected, fill.CornerRadius, $"Header {i}: gridline-adjacent corners must stay square.");
                    Verify.IsFalse(fill.IsHitTestVisible, "The decorative fill must not intercept resizing.");
                }
            }

            RunOnUIThread.Execute(() =>
            {
                VerifyCorners();
                tableView.Columns[0].Visibility = Visibility.Collapsed;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                VerifyCorners();
                tableView.FlowDirection = FlowDirection.RightToLeft;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                VerifyCorners();
                tableView.Columns[1].Visibility = Visibility.Collapsed;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(VerifyCorners);
        }

        [TestMethod]
        public void VerifyHeaderCornerGeometryFollowsFlowDirection()
        {
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            {
                foreach (var hasFlyout in new[] { false, true })
                {
                    TableView tableView = null;
                    Grid layoutRoot = null;
                    Border firstFill = null;
                    Border lastFill = null;
                    DropDownButton button = null;
                    RunOnUIThread.Execute(() =>
                    {
                        tableView = CreateThemingTable();
                        tableView.FlowDirection = flow;
                        tableView.Resources["TableViewHeaderLeftCornerRadius"] = new CornerRadius(8, 0, 0, 0);
                        tableView.Resources["TableViewHeaderRightCornerRadius"] = new CornerRadius(0, 8, 0, 0);
                        foreach (var column in tableView.Columns)
                        {
                            column.Width = new GridLength(1, GridUnitType.Star);
                        }
                        if (hasFlyout)
                        {
                            tableView.Columns[1].HeaderFlyout = new MenuFlyout();
                        }
                        // An LTR ancestor makes the table's RTL transform observable.
                        layoutRoot = new Grid
                        {
                            Width = tableView.Width,
                            Height = tableView.Height,
                            FlowDirection = FlowDirection.LeftToRight
                        };
                        layoutRoot.Children.Add(tableView);
                        LoadContent(layoutRoot);
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var first = RequireColumnHeader(tableView, 0);
                        var last = RequireColumnHeader(tableView, 1);
                        firstFill = (Border)first.FindVisualChildByName("PART_Background");
                        lastFill = (Border)last.FindVisualChildByName("PART_Background");
                        Verify.AreEqual(Visibility.Collapsed,
                            ((Border)last.FindVisualChildByName("PART_GridLine")).Visibility,
                            "Star columns must fill the viewport so the terminal header is an exposed edge.");
                        if (hasFlyout)
                        {
                            button = (DropDownButton)last.FindVisualChildByName("PART_HeaderFlyoutButton");
                        }
                    });

                    foreach (var state in hasFlyout ? new[] { "FlyoutClosed", "FlyoutOpen" } : new[] { "Normal" })
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            if (button != null)
                            {
                                Verify.IsTrue(VisualStateManager.GoToState(button, state, false));
                            }
                            Content.UpdateLayout();
                            var context = $"{flow}, flyout={hasFlyout}, {state}";
                            void VerifyCorners(Border plate, CornerRadius expected)
                            {
                                Verify.AreEqual(expected, plate.CornerRadius, context);
                                Verify.IsGreaterThan(plate.ActualWidth, 0.0, context);
                                var transform = plate.TransformToVisual(layoutRoot);
                                var left = transform.TransformPoint(new Point());
                                var right = transform.TransformPoint(new Point(plate.ActualWidth, 0));
                                Verify.IsTrue(flow == FlowDirection.RightToLeft ? right.X < left.X : right.X > left.X,
                                    $"Logical corners must mirror into physical coordinates: {context}.");
                            }
                            VerifyCorners(firstFill, new CornerRadius(8, 0, 0, 0));
                            VerifyCorners(lastFill, new CornerRadius(0, hasFlyout ? 0 : 8, 0, 0));
                            if (button != null)
                            {
                                VerifyCorners((Border)button.FindVisualChildByName(state == "FlyoutOpen" ? "OpenChrome" : "Chrome"),
                                    new CornerRadius(0, 8, 0, 0));
                            }
                        });
                        IdleSynchronizer.Wait();
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyHeaderFontWeightDefaultIsNormal()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((ushort)400, GetHeaderPresenter(tableView, 0).FontWeight.Weight,
                    "TableViewHeaderFontWeight defaults to Normal.");
            });
        }

        // Override before load: the resolved weight is cached per instance.
        [TestMethod]
        public void VerifyHeaderFontWeightResourceOverrideApplies()
        {
            TableView appScoped = null;
            TableView elementScoped = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TableViewHeaderFontWeight", Microsoft.UI.Text.FontWeights.SemiBold);

                appScoped = CreateThemingTable();
                LoadContent(appScoped);
            });

            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual((ushort)600, GetHeaderPresenter(appScoped, 0).FontWeight.Weight,
                        "An app-scope TableViewHeaderFontWeight override must win over the Normal default.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TableViewHeaderFontWeight"));
            }

            RunOnUIThread.Execute(() =>
            {
                elementScoped = CreateThemingTable();
                elementScoped.Resources["TableViewHeaderFontWeight"] = Microsoft.UI.Text.FontWeights.Bold;
                LoadContent(elementScoped);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual((ushort)700, GetHeaderPresenter(elementScoped, 0).FontWeight.Weight,
                    "A TableViewHeaderFontWeight in the TableView's own Resources must win.");
            });
        }

        // FontWeight comes back as a fresh box per lookup, so the own-entry check must compare it by
        // value; otherwise the APP-theme dictionary's entry reads as an own entry and wins. Both themes
        // run so one of them always differs from the app theme.
        [TestMethod]
        public void VerifyHeaderFontWeightThemeDictionaryOverrideFollowsElementTheme()
        {
            var cases = new[]
            {
                (Theme: ElementTheme.Light, Weight: (ushort)700),
                (Theme: ElementTheme.Dark, Weight: (ushort)300),
            };

            foreach (var testCase in cases)
            {
                TableView tableView = null;

                RunOnUIThread.Execute(() =>
                {
                    EnsureTabularControlsResources();
                    tableView = CreateThemingTable();
                    tableView.RequestedTheme = testCase.Theme;
                    tableView.Resources.ThemeDictionaries["Light"] = new ResourceDictionary
                    {
                        ["TableViewHeaderFontWeight"] = Microsoft.UI.Text.FontWeights.Bold,
                    };
                    tableView.Resources.ThemeDictionaries["Dark"] = new ResourceDictionary
                    {
                        ["TableViewHeaderFontWeight"] = Microsoft.UI.Text.FontWeights.Light,
                    };
                    LoadContent(tableView);
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(testCase.Theme, tableView.ActualTheme, "Precondition: RequestedTheme picks the table's theme.");
                    Verify.AreEqual(testCase.Weight, GetHeaderPresenter(tableView, 0).FontWeight.Weight,
                        $"Under {testCase.Theme}, the TableView's ThemeDictionaries[\"{testCase.Theme}\"] TableViewHeaderFontWeight must win.");
                });
            }
        }

        #endregion

        #region 13.4 Column header style

        [TestMethod]
        public void VerifyStrongColumnHeaderStylePreservesTemplateAndFollowsTheme()
        {
            TableView strongTable = null;
            TableView defaultTable = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();
                var style = resources["StrongTableViewColumnHeaderStyle"] as Style;
                Verify.IsNotNull(style, "TabularControlsResources must expose the named strong header variant.");
                Verify.AreEqual(typeof(TableViewColumnHeader), style.TargetType, "The variant must target column headers.");
                strongTable = CreateThemingTable();
                strongTable.ColumnHeaderStyle = style;
                defaultTable = CreateThemingTable();
                host = new StackPanel { RequestedTheme = ElementTheme.Light };
                host.Children.Add(defaultTable);
                host.Children.Add(strongTable);
                LoadContent(host);
                Verify.IsTrue(defaultTable.SortByColumn(defaultTable.Columns[0], SortDirection.Ascending));
                Verify.IsTrue(strongTable.SortByColumn(strongTable.Columns[0], SortDirection.Descending));
            });
            IdleSynchronizer.Wait();

            void VerifySortColor(TableView table, Color expected)
            {
                var header = RequireColumnHeader(table, 0);
                Verify.AreEqual(expected, ((SolidColorBrush)GetHeaderPresenter(table, 0).Foreground).Color,
                    "The header text must use the resolved foreground.");
                var indicator = (SortIndicator)header.FindVisualChildByName("TableViewSortIndicator");
                Verify.IsNotNull(indicator, "The sorted header must host its indicator.");
                Verify.AreEqual(expected, ((SolidColorBrush)indicator.Foreground).Color,
                    "The sort indicator must match the resolved header text color.");
                var path = (Microsoft.UI.Xaml.Shapes.Path)indicator.FindVisualChildByName("GlyphIcon");
                Verify.AreEqual(expected, ((SolidColorBrush)path.Fill).Color,
                    "The vector itself must match the resolved header text color.");
            }

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                RunOnUIThread.Execute(() => host.RequestedTheme = theme);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => host.UpdateLayout());
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var themeName = theme == ElementTheme.Light ? "Light" : "Default";
                    var header = RequireColumnHeader(strongTable, 0);
                    var presenter = GetHeaderPresenter(strongTable, 0);
                    Verify.AreEqual((ushort)600, presenter.FontWeight.Weight, "The strong header must be semibold.");
                    Verify.AreEqual(RequireThemeColor(themeName, "TextFillColorPrimary"),
                        (presenter.Foreground as SolidColorBrush)?.Color ?? default,
                        "Strong headers must re-resolve primary text under the element's theme.");
                    Verify.AreEqual((ushort)400, GetHeaderPresenter(defaultTable, 0).FontWeight.Weight,
                        "Using the named style must not change another table's default weight.");
                    Verify.AreEqual(RequireThemeColor(themeName, "TextFillColorSecondary"),
                        (GetHeaderPresenter(defaultTable, 0).Foreground as SolidColorBrush)?.Color ?? default,
                        "Using the named style must not change another table's default foreground.");
                    VerifySortColor(strongTable, RequireThemeColor(themeName, "TextFillColorPrimary"));
                    VerifySortColor(defaultTable, RequireThemeColor(themeName, "TextFillColorSecondary"));
                    Verify.IsNotNull(header.Template, "The strong variant must keep the stock template.");
                    Verify.IsNotNull(header.FindVisualChildByName("PART_ResizeGripperHost"), "The resize host must remain.");
                    Verify.IsNotNull(header.FindVisualChildByName("PART_SortIndicatorHost"), "The sort host must remain.");
                    Verify.AreEqual(GetHeaderPresenter(defaultTable, 0).Padding, presenter.Padding,
                        "The variant must not change density padding.");
                    Verify.IsTrue(header.UseSystemFocusVisuals, "The variant must retain system focus visuals.");

                    foreach (var state in new[] { "PointerOver", "Pressed" })
                    {
                        Verify.IsTrue(VisualStateManager.GoToState(header, state, false));
                        Verify.AreEqual(RequireThemeColor(themeName, "TextFillColorPrimary"),
                            (presenter.Foreground as SolidColorBrush)?.Color ?? default,
                            $"The strong variant must retain primary text in {state}.");
                    }
                    Verify.IsTrue(VisualStateManager.GoToState(header, "Normal", false));
                });

                foreach (var strong in new[] { true, false })
                {
                    RunOnUIThread.Execute(() =>
                        defaultTable.ColumnHeaderStyle = strong ? strongTable.ColumnHeaderStyle : null);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var themeName = theme == ElementTheme.Light ? "Light" : "Default";
                        VerifySortColor(defaultTable, RequireThemeColor(themeName,
                            strong ? "TextFillColorPrimary" : "TextFillColorSecondary"));
                    });
                }
            }

            var customColor = Color.FromArgb(0xFF, 0x33, 0x66, 0x99);
            RunOnUIThread.Execute(() =>
                RequireColumnHeader(defaultTable, 0).Foreground = new SolidColorBrush(customColor));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifySortColor(defaultTable, customColor));
        }

        private static TableViewColumnHeader RequireColumnHeader(TableView tableView, int index)
        {
            var header = GetHeaderCell(tableView, index) as TableViewColumnHeader;
            Verify.IsNotNull(header, $"Header {index} should be a TableViewColumnHeader.");
            return header;
        }

        private static string CurrentState(TableViewColumnHeader header, string groupName)
        {
            var root = VisualTreeHelper.GetChild(header, 0) as FrameworkElement;
            Verify.IsNotNull(root, "The header template should have applied.");
            var group = VisualStateManager.GetVisualStateGroups(root).FirstOrDefault(g => g.Name == groupName);
            Verify.IsNotNull(group, $"The header template should declare {groupName}.");
            return group.CurrentState?.Name;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies TableView.ColumnHeaderStyle reaches every header, TableViewColumn.HeaderStyle wins for its column, and a style Padding/MinHeight beats the density values.")]
        public void VerifyColumnHeaderStyleApplies()
        {
            TableView tableView = null;
            SolidColorBrush tableBrush = null;
            SolidColorBrush columnBrush = null;
            var stylePadding = new Thickness(20, 1, 20, 1);

            RunOnUIThread.Execute(() =>
            {
                tableBrush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                columnBrush = new SolidColorBrush(Microsoft.UI.Colors.Green);
                tableView = CreateThemingTable();
                var tableStyle = new Style(typeof(TableViewColumnHeader));
                tableStyle.Setters.Add(new Setter(Control.BackgroundProperty, tableBrush));
                tableStyle.Setters.Add(new Setter(Control.FontWeightProperty, Microsoft.UI.Text.FontWeights.Bold));
                tableStyle.Setters.Add(new Setter(Control.PaddingProperty, stylePadding));
                tableStyle.Setters.Add(new Setter(FrameworkElement.MinHeightProperty, 60.0));
                tableView.ColumnHeaderStyle = tableStyle;

                var columnStyle = new Style(typeof(TableViewColumnHeader));
                columnStyle.Setters.Add(new Setter(Control.BackgroundProperty, columnBrush));
                tableView.Columns[1].HeaderStyle = columnStyle;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var first = RequireColumnHeader(tableView, 0);
                Verify.IsTrue(ReferenceEquals(tableBrush, first.Background), "ColumnHeaderStyle's Background setter must reach the header.");
                Verify.AreEqual((ushort)700, GetHeaderPresenter(tableView, 0).FontWeight.Weight,
                    "ColumnHeaderStyle's FontWeight setter must reach the header text.");
                Verify.AreEqual(stylePadding, first.Padding, "A style Padding must win over the density padding.");
                Verify.AreEqual(60.0, first.MinHeight, "A style MinHeight must win over the density row height.");

                var second = RequireColumnHeader(tableView, 1);
                Verify.IsTrue(ReferenceEquals(tableView.Columns[1], second.Column), "A header's Column must be the column it represents.");
                Verify.IsTrue(ReferenceEquals(columnBrush, second.Background), "TableViewColumn.HeaderStyle must win over ColumnHeaderStyle.");
                Verify.AreEqual((ushort)400, GetHeaderPresenter(tableView, 1).FontWeight.Weight,
                    "A column style without a FontWeight setter keeps the default Normal weight.");
                Verify.AreEqual(40.0, second.MinHeight, "Without a style MinHeight the header follows the density row height.");

                tableView.ColumnHeaderStyle = null;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(ReferenceEquals(tableBrush, RequireColumnHeader(tableView, 0).Background),
                    "Clearing ColumnHeaderStyle must restyle the realized headers.");
                Verify.AreEqual((ushort)400, GetHeaderPresenter(tableView, 0).FontWeight.Weight,
                    "Clearing ColumnHeaderStyle must restore the default header weight.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies header Styles that only inherit setters through BasedOn a ContentControl or Control Style apply, and their BasedOn Padding beats the density padding.")]
        public void VerifyBasedOnOnlyColumnHeaderStyleApplies()
        {
            TableView tableView = null;
            SolidColorBrush tableBrush = null;
            SolidColorBrush columnBrush = null;
            var stylePadding = new Thickness(20, 1, 20, 1);
            bool loaded = false;

            // A checked build can stop on a framework assert while loading; fail instead of stalling.
            using var watchdog = new global::System.Threading.Timer(_ =>
            {
                if (!global::System.Threading.Volatile.Read(ref loaded))
                {
                    Environment.FailFast("VerifyBasedOnOnlyColumnHeaderStyleApplies: the TableView did not load within 60 seconds.");
                }
            }, null, TimeSpan.FromSeconds(60), global::System.Threading.Timeout.InfiniteTimeSpan);

            RunOnUIThread.Execute(() =>
            {
                tableBrush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                columnBrush = new SolidColorBrush(Microsoft.UI.Colors.Green);
                var tableBase = new Style(typeof(ContentControl));
                tableBase.Setters.Add(new Setter(Control.BackgroundProperty, tableBrush));
                tableBase.Setters.Add(new Setter(Control.PaddingProperty, stylePadding));
                var columnBase = new Style(typeof(Control));
                columnBase.Setters.Add(new Setter(Control.BackgroundProperty, columnBrush));

                tableView = CreateThemingTable();
                tableView.ColumnHeaderStyle = new Style(typeof(TableViewColumnHeader)) { BasedOn = tableBase };
                tableView.Columns[1].HeaderStyle = new Style(typeof(TableViewColumnHeader)) { BasedOn = columnBase };
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            global::System.Threading.Volatile.Write(ref loaded, true);

            RunOnUIThread.Execute(() => tableView.Density = TableViewDensity.Compact);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var first = RequireColumnHeader(tableView, 0);
                Verify.IsTrue(ReferenceEquals(tableBrush, first.Background), "A BasedOn Background setter must reach the header.");
                Verify.AreEqual(stylePadding, first.Padding, "A BasedOn Padding must win over the density padding.");

                var second = RequireColumnHeader(tableView, 1);
                Verify.IsTrue(ReferenceEquals(columnBrush, second.Background), "TableViewColumn.HeaderStyle's BasedOn Background must win over ColumnHeaderStyle.");
                Verify.AreEqual(32.0, second.MinHeight, "Without a style MinHeight the header follows the Compact row height.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a UIElement column Header moves to the new header when the header band is rebuilt.")]
        public void VerifyUIElementHeaderSurvivesHeaderRebuild()
        {
            TableView tableView = null;
            Border headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                headerElement = new Border { Width = 20, Height = 12, Background = new SolidColorBrush(Microsoft.UI.Colors.Blue) };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => tableView.ColumnHeaderStyle = new Style(typeof(TableViewColumnHeader)));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => tableView.Density = TableViewDensity.Compact);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                Verify.IsTrue(ReferenceEquals(headerElement, header.Content), "The rebuilt header must show the column's UIElement Header.");

                DependencyObject current = headerElement;
                while (current != null && !ReferenceEquals(current, header))
                {
                    current = VisualTreeHelper.GetParent(current);
                }
                Verify.IsNotNull(current, "The UIElement Header must be parented under the rebuilt header.");
                Verify.AreEqual(20.0, headerElement.ActualWidth, "The UIElement Header must be laid out in the rebuilt header.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a UIElement column Header moves to the new header band when the TableView is re-templated.")]
        public void VerifyUIElementHeaderSurvivesRetemplate()
        {
            TableView tableView = null;
            Border headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                headerElement = new Border { Width = 20, Height = 12, Background = new SolidColorBrush(Microsoft.UI.Colors.Blue) };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                tableView.Template = (ControlTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    @"<ControlTemplate
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:tabular='using:Microsoft.UI.Xaml.Controls.Tabular'
                        TargetType='tabular:TableView'>
                        <Grid>
                            <Grid.RowDefinitions>
                                <RowDefinition Height='Auto' />
                                <RowDefinition Height='*' />
                            </Grid.RowDefinitions>
                            <Border x:Name='PART_HeaderRow'>
                                <tabular:TableViewCellsPanel x:Name='PART_HeaderHost' HorizontalAlignment='Left' />
                            </Border>
                            <ItemsRepeater x:Name='PART_RowsRepeater' Grid.Row='1'>
                                <ItemsRepeater.Layout>
                                    <StackLayout Orientation='Vertical' />
                                </ItemsRepeater.Layout>
                            </ItemsRepeater>
                        </Grid>
                    </ControlTemplate>");
                tableView.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyHostedUnderHeader(tableView, 0, headerElement, "after re-templating"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column with a UIElement Header can move from one TableView to another.")]
        public void VerifyUIElementHeaderSurvivesMoveToAnotherTableView()
        {
            TableView source = null;
            TableView target = null;
            StackPanel host = null;
            Border headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                source = CreateThemingTable();
                headerElement = new Border { Width = 20, Height = 12, Background = new SolidColorBrush(Microsoft.UI.Colors.Blue) };
                source.Columns[0].Header = headerElement;
                host = new StackPanel();
                host.Children.Add(source);
                LoadContent(host);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var column = source.Columns[0];
                source.Columns.RemoveAt(0);
                target = new TableView();
                target.Columns.Add(column);
                // Templating the target rebuilds its headers before the source's queued rebuild runs.
                host.Children.Add(target);
                host.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyHostedUnderHeader(target, 0, headerElement, "after the move"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus inside a UIElement column Header returns to that header after the header band is rebuilt.")]
        public void VerifyFocusInUIElementHeaderIsRestoredAfterRebuild()
        {
            TableView tableView = null;
            CheckBox headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                headerElement = new CheckBox { MinWidth = 0 };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(headerElement.Focus(FocusState.Keyboard), "Precondition: the header CheckBox takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => tableView.Density = TableViewDensity.Compact);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                DependencyObject current = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                Log.Comment($"Focused after rebuild: {current}");
                while (current != null && !ReferenceEquals(current, header))
                {
                    current = VisualTreeHelper.GetParent(current);
                }
                Verify.IsNotNull(current, "Focus must return to the rebuilt header of the column that held it.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a replaced UIElement column Header can be re-parented right away, before the header band rebuilds.")]
        public void VerifyReplacedUIElementHeaderCanBeReparentedImmediately()
        {
            TableView tableView = null;
            StackPanel root = null;
            Border headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                headerElement = new Border { Width = 20, Height = 12, Background = new SolidColorBrush(Microsoft.UI.Colors.Blue) };
                tableView.Columns[0].Header = headerElement;
                root = new StackPanel();
                root.Children.Add(tableView);
                LoadContent(root);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                tableView.Columns[0].Header = "Name";
                var other = new StackPanel();
                root.Children.Add(other);
                other.Children.Add(headerElement);
                Verify.IsTrue(ReferenceEquals(other, VisualTreeHelper.GetParent(headerElement)), "The old Header must move to the new panel.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.AreEqual("Name", (RequireColumnHeader(tableView, 0).Content as TextBlock)?.Text,
                "The header band must show the new Header."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus inside a UIElement column Header follows the column when it moves within the same TableView.")]
        public void VerifyFocusInUIElementHeaderFollowsColumnMove()
        {
            TableView tableView = null;
            CheckBox headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                headerElement = new CheckBox { MinWidth = 0 };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(headerElement.Focus(FocusState.Keyboard), "Precondition: the header CheckBox takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];
                tableView.Columns.RemoveAt(0);
                tableView.Columns.Insert(1, column);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 1);
                Verify.IsTrue(ReferenceEquals(headerElement, header.Content), "Precondition: the moved column's header hosts its Header.");
                DependencyObject current = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                Log.Comment($"Focused after the move: {current}");
                while (current != null && !ReferenceEquals(current, header))
                {
                    current = VisualTreeHelper.GetParent(current);
                }
                Verify.IsNotNull(current, "Focus must follow the moved column to its rebuilt header.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing a column whose UIElement Header holds focus moves focus to the nearest remaining header, never onto the removed one.")]
        public void VerifyFocusInRemovedColumnHeaderMovesToNearestHeader()
        {
            TableView tableView = null;
            CheckBox headerElement = null;
            var removedHeaderFocused = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                headerElement = new CheckBox { MinWidth = 0 };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(headerElement.Focus(FocusState.Keyboard), "Precondition: the header CheckBox takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var removedHeader = RequireColumnHeader(tableView, 0);
                EventHandler<GettingFocusEventArgs> handler = (s, args) =>
                {
                    if (ReferenceEquals(args.NewFocusedElement, removedHeader))
                    {
                        removedHeaderFocused = true;
                    }
                };
                FocusManager.GettingFocus += handler;
                try
                {
                    tableView.Columns.RemoveAt(0);
                }
                finally
                {
                    FocusManager.GettingFocus -= handler;
                }
                Verify.IsFalse(removedHeaderFocused, "Focus must not move onto the removed column's header.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                DependencyObject focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                Log.Comment($"Focused after the removal: {focused}");
                Verify.IsTrue(focused != null && IsUnder(focused, header), "Focus must move to the nearest remaining header.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing the Columns vector while a header holds keyboard focus keeps focus on a header of the table.")]
        public void VerifyHeaderFocusStaysInHeaderBandWhenColumnsVectorReplaced()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(RequireColumnHeader(tableView, 1).Focus(FocusState.Keyboard), "Precondition: the second header takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // A vector no table is subscribed to, holding columns no table owns.
                var donor = new TableView();
                var replacement = donor.GetValue(TableView.ColumnsProperty);
                donor.SetValue(TableView.ColumnsProperty, new TableView().GetValue(TableView.ColumnsProperty));
                var columns = (IList<TableViewColumn>)replacement;
                columns.Add(MakeTextColumn("Name"));
                columns.Add(MakeTextColumn("Role"));

                tableView.SetValue(TableView.ColumnsProperty, replacement);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                Log.Comment($"Focused after the replacement: {focused}");
                var header = FindVisualChildrenByType<TableViewColumnHeader>(tableView).FirstOrDefault(h => focused != null && IsUnder(focused, h));
                Verify.IsNotNull(header, "Focus must stay on a header of the table.");
                Verify.AreEqual(FocusState.Keyboard, (focused as UIElement)?.FocusState ?? FocusState.Unfocused, "Focus keeps its keyboard state.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus in the header of a column that is moved or re-added with no other header to hold focus returns to that column's header.")]
        [TestProperty("data:Change", "{MoveOnlyColumn, ClearAndReAddLast}")]
        public void VerifyHeaderFocusReturnsWhenNoOtherHeaderCanHoldIt()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            var focusedIndex = change == "ClearAndReAddLast" ? 1 : 0;
            TableView tableView = null;
            CheckBox headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = change == "MoveOnlyColumn" ? CreateTableView(headers: new[] { "Name" }) : CreateThemingTable();
                headerElement = new CheckBox { MinWidth = 0 };
                tableView.Columns[focusedIndex].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(headerElement.Focus(FocusState.Keyboard), "Precondition: the header CheckBox takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var columns = tableView.Columns.ToList();
                if (change == "MoveOnlyColumn")
                {
                    tableView.Columns.RemoveAt(0);
                    tableView.Columns.Insert(0, columns[0]);
                }
                else
                {
                    tableView.Columns.Clear();
                    foreach (var column in columns)
                    {
                        tableView.Columns.Add(column);
                    }
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, focusedIndex);
                var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                Log.Comment($"{change}: focused {focused}");
                Verify.IsTrue(focused != null && IsUnder(focused, header), $"{change}: focus must return to the column's header.");
                Verify.AreEqual(FocusState.Keyboard, ((UIElement)focused).FocusState, $"{change}: keyboard focus must stay keyboard focus.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus in the header of the only column moves into the body when that column is removed.")]
        public void VerifyFocusInOnlyRemovedColumnHeaderMovesToBody()
        {
            TableView tableView = null;
            CheckBox headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                headerElement = new CheckBox { MinWidth = 0 };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(headerElement.Focus(FocusState.Keyboard), "Precondition: the header CheckBox takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => tableView.Columns.RemoveAt(0));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                Log.Comment($"Focused after the removal: {focused}");
                Verify.IsTrue(focused != null && IsUnder(focused, GetRowsRepeater(tableView)), "Focus must move into the table body.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies focus in a header stays in the header band when the columns are replaced by new instances.")]
        [TestProperty("data:Change", "{ClearAndAddNew, ReplaceOnlyColumnWithoutRows}")]
        public void VerifyHeaderFocusStaysInHeaderBandWhenColumnsReplaced()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            TableView tableView = null;
            CheckBox headerElement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = change == "ReplaceOnlyColumnWithoutRows"
                    ? CreateTableView(new List<Person>(), headers: new[] { "Name" })
                    : CreateThemingTable();
                headerElement = new CheckBox { MinWidth = 0 };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(headerElement.Focus(FocusState.Keyboard), "Precondition: the header CheckBox takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                if (change == "ClearAndAddNew")
                {
                    tableView.Columns.Clear();
                    tableView.Columns.Add(MakeTextColumn("Name"));
                    tableView.Columns.Add(MakeTextColumn("Role"));
                }
                else
                {
                    tableView.Columns[0] = MakeTextColumn("Name");
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                Log.Comment($"{change}: focused {focused}");
                Verify.IsTrue(focused != null && IsUnder(focused, RequireColumnHeader(tableView, 0)), $"{change}: focus must move to the first header of the new columns.");
                Verify.AreEqual(FocusState.Keyboard, ((UIElement)focused).FocusState, $"{change}: keyboard focus must stay keyboard focus.");
            });
        }


        [TestMethod]
        [TestProperty("Description", "Verifies changing a string Header keeps the header's content and tooltip until the header band rebuilds.")]
        public void VerifyStringHeaderChangeKeepsHeaderContentUntilRebuild()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                tableView.Columns[0].HeaderToolTip = "Tip";
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                Verify.IsNotNull(ToolTipService.GetToolTip(header), "Precondition: the header has a tooltip.");
                tableView.Columns[0].Header = "Renamed";
                Verify.IsNotNull(header.Content, "A string Header change must not blank the header before the rebuild.");
                Verify.IsNotNull(ToolTipService.GetToolTip(header), "A string Header change must not drop the tooltip before the rebuild.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.AreEqual("Renamed", (RequireColumnHeader(tableView, 0).Content as TextBlock)?.Text,
                "The header band must show the new Header."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header rebuild started from app code while another rebuild releases headers keeps UIElement Headers hosted.")]
        public void VerifyReentrantHeaderRebuildKeepsUIElementHeader()
        {
            TableView tableView = null;
            CheckBox headerElement = null;
            bool reentered = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                headerElement = new CheckBox { MinWidth = 0 };
                tableView.Columns[0].Header = headerElement;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(headerElement.Focus(FocusState.Keyboard), "Precondition: the header CheckBox takes focus."));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                EventHandler<GettingFocusEventArgs> handler = (s, args) =>
                {
                    if (!reentered && ReferenceEquals(args.OldFocusedElement, headerElement))
                    {
                        reentered = true;
                        // Rebuilds the header band synchronously, from inside the outer rebuild.
                        tableView.CanUserResizeColumns = !tableView.CanUserResizeColumns;
                    }
                };
                FocusManager.GettingFocus += handler;
                try
                {
                    tableView.Density = TableViewDensity.Compact;
                }
                finally
                {
                    FocusManager.GettingFocus -= handler;
                }
                Verify.IsTrue(reentered, "Precondition: releasing the focused Header re-entered the rebuild.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                Verify.IsTrue(ReferenceEquals(headerElement, header.Content), "The header must show the column's UIElement Header.");
                Verify.IsTrue(IsUnder(headerElement, header), "The UIElement Header must be parented under the rebuilt header.");
            });
        }

        private static bool IsUnder(DependencyObject element, DependencyObject ancestor)
        {
            for (var current = element; current != null; current = VisualTreeHelper.GetParent(current))
            {
                if (ReferenceEquals(current, ancestor))
                {
                    return true;
                }
            }
            return false;
        }

        private static void VerifyHostedUnderHeader(TableView tableView, int index, FrameworkElement headerElement, string when)
        {
            var header = RequireColumnHeader(tableView, index);
            Verify.IsTrue(ReferenceEquals(headerElement, header.Content), $"The header must show the column's UIElement Header {when}.");

            DependencyObject current = headerElement;
            while (current != null && !ReferenceEquals(current, header))
            {
                current = VisualTreeHelper.GetParent(current);
            }
            Verify.IsNotNull(current, $"The UIElement Header must be parented under the header {when}.");
            Verify.AreEqual(20.0, headerElement.ActualWidth, $"The UIElement Header must be laid out {when}.");
        }

        [TestMethod]
        public void VerifySortableHighContrastHeadersUseButtonColors()
        {
            bool highContrast = false;
            RunOnUIThread.Execute(() => highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast);
            Log.Comment($"System High Contrast: {highContrast}");

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                foreach (var strong in new[] { false, true })
                {
                    TableView tableView = null;
                    RunOnUIThread.Execute(() =>
                    {
                        tableView = CreateThemingTable();
                        tableView.RequestedTheme = theme;
                        if (strong)
                        {
                            tableView.ColumnHeaderStyle = (Style)new TabularControlsResources()["StrongTableViewColumnHeaderStyle"];
                        }
                        LoadContent(tableView);
                    });
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending)));
                    IdleSynchronizer.Wait();

                    foreach (var scenario in new[] { "Sortable", "TableOff", "TableOn", "ColumnOff", "ColumnOn", "Disabled", "Enabled" })
                    {
                        if (scenario == "Disabled" || scenario == "Enabled")
                        {
                            TableViewColumnHeader header = null;
                            RunOnUIThread.Execute(() => header = RequireColumnHeader(tableView, 0));
                            SetIsEnabledAndWait(tableView, scenario == "Enabled", header);
                        }
                        else
                        {
                            RunOnUIThread.Execute(() =>
                            {
                                switch (scenario)
                                {
                                    case "TableOff": tableView.CanUserSortColumns = false; break;
                                    case "TableOn": tableView.CanUserSortColumns = true; break;
                                    case "ColumnOff": tableView.Columns[0].CanSort = false; break;
                                    case "ColumnOn": tableView.Columns[0].CanSort = true; break;
                                }
                            });
                        }
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var header = RequireColumnHeader(tableView, 0);
                            var presenter = GetHeaderPresenter(tableView, 0);
                            var background = (Border)header.FindVisualChildByName("PART_Background");
                            var glyph = (Control)header.FindVisualChildByName("TableViewSortIndicator");
                            var sortable = tableView.CanUserSortColumns && tableView.Columns[0].CanSort && tableView.IsEnabled;
                            var context = $"{theme}, strong={strong}, {scenario}";
                            Verify.IsTrue(header.IsLoaded, $"Header is loaded: {context}");
                            Verify.AreEqual(tableView.IsEnabled, header.IsEnabled, $"Header inherits enabled state: {context}");
                            Verify.AreEqual(highContrast && sortable ? "HighContrastSortable" : "HighContrastNormal",
                                CurrentState(header, "HighContrastStates"), context);
                            if (highContrast && sortable)
                            {
                                Verify.AreEqual(((SolidColorBrush)Application.Current.Resources["SystemColorButtonFaceColorBrush"]).Color,
                                    ((SolidColorBrush)background.Background).Color, $"ButtonFace background: {context}");
                                Verify.AreEqual(((SolidColorBrush)Application.Current.Resources["SystemColorButtonTextColorBrush"]).Color,
                                    ((SolidColorBrush)presenter.Foreground).Color, $"ButtonText foreground: {context}");
                                Verify.AreEqual(ElementHighContrastAdjustment.None, presenter.HighContrastAdjustment, context);
                            }
                            else
                            {
                                Verify.AreEqual(header.Background, background.Background, $"Restore the resting background: {context}");
                                if (tableView.IsEnabled)
                                {
                                    Verify.AreEqual(header.Foreground, presenter.Foreground, $"Restore the resting foreground: {context}");
                                }
                            }
                            if (sortable)
                            {
                                Verify.IsNotNull(glyph, $"The sorted, sortable column hosts its sort glyph: {context}");
                            }
                            if (glyph != null)
                            {
                                Verify.AreEqual(((SolidColorBrush)presenter.Foreground).Color, ((SolidColorBrush)glyph.Foreground).Color,
                                    $"Sort glyph follows header text: {context}");
                            }
                            if (highContrast && sortable)
                            {
                                foreach (var state in new[] { "PointerOver", "Pressed" })
                                {
                                    Verify.IsTrue(VisualStateManager.GoToState(header, state, false));
                                    Verify.IsTrue(VisualStateManager.GoToState(header, "HighContrast" + state, false));
                                    var backgroundKey = "SystemColorHighlightColorBrush";
                                    var foregroundKey = "SystemColorHighlightTextColorBrush";
                                    Verify.AreEqual(((SolidColorBrush)Application.Current.Resources[backgroundKey]).Color,
                                        ((SolidColorBrush)background.Background).Color, $"Header background: {state}, {context}");
                                    Verify.AreEqual(((SolidColorBrush)Application.Current.Resources[foregroundKey]).Color,
                                        ((SolidColorBrush)presenter.Foreground).Color, $"Header foreground: {state}, {context}");
                                    Verify.IsTrue(VisualStateManager.GoToState(header, "Normal", false));
                                    Verify.IsTrue(VisualStateManager.GoToState(header, "HighContrastSortable", false));
                                    Verify.AreEqual(((SolidColorBrush)Application.Current.Resources["SystemColorButtonFaceColorBrush"]).Color,
                                        ((SolidColorBrush)background.Background).Color, $"Restore ButtonFace after {state}: {context}");
                                    Verify.AreEqual(((SolidColorBrush)Application.Current.Resources["SystemColorButtonTextColorBrush"]).Color,
                                        ((SolidColorBrush)presenter.Foreground).Color, $"Restore ButtonText after {state}: {context}");
                                }
                            }
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyHeaderPlateBrushesKeepContinuousFillAndAllowOutlineOnlyOverrides()
        {
            var fill = Color.FromArgb(0xFF, 0x20, 0x40, 0x60);
            var outline = Color.FromArgb(0xFF, 0x12, 0x9E, 0x5A);
            var replacementOutline = Color.FromArgb(0xFF, 0xA0, 0x30, 0x20);

            foreach (var theme in new[] { "HighContrast", "Light", "Default" })
            {
                foreach (var density in DensityCases)
                {
                    foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
                    {
                        TableView tableView = null;
                        var brushes = new Dictionary<string, SolidColorBrush>();
                        RunOnUIThread.Execute(() =>
                        {
                            var resources = new TabularControlsResources();
                            var dictionary = (ResourceDictionary)resources.ThemeDictionaries[theme];
                            tableView = CreateGroupedThemingTable();
                            tableView.RequestedTheme = theme == "Light" ? ElementTheme.Light : ElementTheme.Dark;
                            tableView.Density = density.Density;
                            tableView.FlowDirection = flow;
                            tableView.HighContrastAdjustment = ElementHighContrastAdjustment.None;
                            foreach (var entry in dictionary)
                            {
                                if (entry.Key is string key && key.StartsWith("TabularSurface", StringComparison.Ordinal)
                                    && entry.Value is SolidColorBrush brush)
                                {
                                    brushes[key] = new SolidColorBrush(brush.Color) { Opacity = brush.Opacity };
                                    tableView.Resources[key] = brushes[key];
                                }
                            }
                            var thickness = (Thickness)dictionary["TableViewHeaderInteractionBorderThickness"];
                            Verify.AreEqual(new Thickness(theme == "HighContrast" ? 1 : 0), thickness);
                            tableView.Resources["TableViewHeaderInteractionBorderThickness"] = thickness;
                            LoadContent(tableView);
                        });
                        IdleSynchronizer.Wait();

                        foreach (var section in new[] { false, true })
                        {
                            foreach (var state in new[] { "Normal", "Sortable", "PointerOver", "Pressed", "Disabled" })
                            {
                                if (state == "Sortable" && (section || theme != "HighContrast"))
                                {
                                    continue;
                                }

                                Control header = null;
                                Border plate = null;
                                double plateWidth = 0;
                                CornerRadius cornerRadius = default;
                                FrameworkElement content = null;
                                Rect contentBounds = default;
                                Thickness thickness = default;
                                string suffix = state == "Normal" || state == "Disabled" ? "" : state;
                                string resourceSuffix = !section && state == "PointerOver" && theme != "HighContrast"
                                    ? "" : suffix;
                                string prefix = section ? "TabularSurfaceGroupHeader" : "TabularSurfaceHeader";
                                string backgroundKey = prefix + "Background" + resourceSuffix + "Brush";
                                string borderKey = prefix + (section ? "InteractionBorder" : "Border") + resourceSuffix + "Brush";
                                var background = brushes[backgroundKey];
                                var border = brushes[borderKey];
                                Color originalBackground = default;
                                Color originalBorder = default;
                                string context = $"{theme}, {density.Density}, {flow}, section={section}, {state}";
                                RunOnUIThread.Execute(() =>
                                {
                                    originalBackground = background.Color;
                                    originalBorder = border.Color;
                                    header = section
                                        ? (Control)FindVisualChildrenByType<TableViewGroupHeader>(tableView).First()
                                        : RequireColumnHeader(tableView, 0);
                                    Verify.IsTrue(VisualStateManager.GoToState(header, state == "Sortable" ? "Normal" : state, false));
                                    if (!section)
                                    {
                                        string hcState = theme == "HighContrast" && suffix.Length > 0
                                            ? "HighContrast" + suffix : "HighContrastNormal";
                                        Verify.IsTrue(VisualStateManager.GoToState(header, hcState, false));
                                    }
                                    plate = (Border)header.FindVisualChildByName(section ? "PART_Root" : "PART_Background");
                                    // An isolated RTL snapshot omits the ancestor mirror transform.
                                    plate.FlowDirection = FlowDirection.LeftToRight;
                                    plateWidth = plate.ActualWidth;
                                    cornerRadius = plate.CornerRadius;
                                    content = (FrameworkElement)header.FindVisualChildByName("PART_ContentPresenter");
                                    contentBounds = content.TransformToVisual(header).TransformBounds(
                                        new Rect(0, 0, content.ActualWidth, content.ActualHeight));
                                    thickness = plate.BorderThickness;
                                    Verify.AreEqual(originalBackground, ((SolidColorBrush)plate.Background).Color, context);
                                    Verify.AreEqual(originalBorder, ((SolidColorBrush)plate.BorderBrush).Color, context);
                                    if (theme == "HighContrast")
                                    {
                                        Verify.AreEqual(originalBackground, originalBorder,
                                            $"Default HC stroke must blend into the existing fill: {context}");
                                    }
                                });
                                IdleSynchronizer.Wait();

                                var withBorder = RenderElementPixels(plate);
                                if (theme == "HighContrast")
                                {
                                    // The stroke changes corner antialiasing, but must not introduce an interior seam.
                                    double radius = Math.Max(Math.Max(cornerRadius.TopLeft, cornerRadius.TopRight),
                                        Math.Max(cornerRadius.BottomLeft, cornerRadius.BottomRight));
                                    int corner = (int)Math.Ceiling((radius + 1) * withBorder.Width / plateWidth);
                                    bool continuous = true;
                                    string mismatch = "";
                                    for (int y = 1; y < withBorder.Height - 1 && continuous; y++)
                                    {
                                        for (int x = 1; x < withBorder.Width - 1; x++)
                                        {
                                            if ((x < corner || x >= withBorder.Width - corner)
                                                && (y < corner || y >= withBorder.Height - corner))
                                            {
                                                continue;
                                            }
                                            if (!ReadRenderedPixel(withBorder, x, y).Equals(originalBackground))
                                            {
                                                continuous = false;
                                                mismatch = $"pixel ({x},{y})={ReadRenderedPixel(withBorder, x, y)}, expected={originalBackground}";
                                                break;
                                            }
                                        }
                                    }
                                    Verify.IsTrue(continuous, $"The HC fill and stroke must form one continuous plate: {context}; {mismatch}");
                                }
                                else
                                {
                                    RunOnUIThread.Execute(() => plate.BorderThickness = new Thickness(0));
                                    IdleSynchronizer.Wait();
                                    var originalFillOnly = RenderElementPixels(plate);
                                    Verify.IsTrue(originalFillOnly.Pixels.SequenceEqual(withBorder.Pixels),
                                        $"Light/Dark plate pixels must retain the former fill-only geometry: {context}");
                                }

                                RunOnUIThread.Execute(() =>
                                {
                                    plate.BorderThickness = new Thickness(1);
                                    background.Color = fill;
                                    border.Color = outline;
                                });
                                IdleSynchronizer.Wait();
                                var custom = RenderElementPixels(plate);
                                Verify.AreEqual(fill, ReadRenderedPixel(custom, custom.Width / 2, custom.Height / 2), context);
                                Verify.AreEqual(outline, ReadRenderedPixel(custom, custom.Width / 2, 1), context);

                                RunOnUIThread.Execute(() => background.Color = Microsoft.UI.Colors.Transparent);
                                IdleSynchronizer.Wait();
                                var outlineOnly = RenderElementPixels(plate);
                                Verify.AreEqual((byte)0, ReadRenderedPixel(outlineOnly, outlineOnly.Width / 2, outlineOnly.Height / 2).A, context);
                                Verify.AreEqual(outline, ReadRenderedPixel(outlineOnly, outlineOnly.Width / 2, 1),
                                    $"Making only the background transparent must retain the outline: {context}");

                                RunOnUIThread.Execute(() => border.Color = replacementOutline);
                                IdleSynchronizer.Wait();
                                var recolored = RenderElementPixels(plate);
                                Verify.AreEqual(replacementOutline, ReadRenderedPixel(recolored, recolored.Width / 2, 1), context);
                                RunOnUIThread.Execute(() =>
                                {
                                    Verify.AreEqual(contentBounds, content.TransformToVisual(header).TransformBounds(
                                        new Rect(0, 0, content.ActualWidth, content.ActualHeight)),
                                        $"The interaction stroke must not move content: {context}");
                                    background.Color = originalBackground;
                                    border.Color = originalBorder;
                                    plate.BorderThickness = thickness;
                                });
                            }
                        }
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyHeaderDropdownDoesNotCoverColumnSeparator()
        {
            var separatorColor = Color.FromArgb(0xFF, 0x12, 0x9E, 0x5A);
            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
                {
                    foreach (var density in DensityCases)
                    {
                        TableView tableView = null;
                        Grid snapshotRoot = null;
                        TableViewColumnHeader header = null;
                        DropDownButton button = null;
                        Border separator = null;
                        Rect bounds = default;
                        double snapshotWidth = 0;
                        double snapshotHeight = 0;
                        RunOnUIThread.Execute(() =>
                        {
                            tableView = CreateThemingTable();
                            tableView.Columns.Add(MakeTextColumn("Status", bindingPath: "Role"));
                            tableView.Columns[1].HeaderFlyout = new MenuFlyout();
                            tableView.RequestedTheme = theme;
                            tableView.FlowDirection = flow;
                            tableView.Density = density.Density;
                            tableView.HighContrastAdjustment = ElementHighContrastAdjustment.None;
                            tableView.Resources["TabularSurfaceHeaderSeparatorBrush"] = new SolidColorBrush(separatorColor);
                            // Include the header's RTL mirror transform in the captured subtree.
                            snapshotRoot = new Grid { Width = tableView.Width, Height = tableView.Height, FlowDirection = FlowDirection.LeftToRight };
                            snapshotRoot.Children.Add(tableView);
                            LoadContent(snapshotRoot);
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            header = RequireColumnHeader(tableView, 1);
                            button = (DropDownButton)header.FindVisualChildByName("PART_HeaderFlyoutButton");
                            separator = (Border)header.FindVisualChildByName("PART_GridLine");
                            Verify.AreEqual(Visibility.Visible, button.Visibility);
                            Verify.IsFalse(separator.IsHitTestVisible, "The overlay must not intercept dropdown or resize input.");

                            // Force opaque HC-like chrome in every theme, including when the OS is not in HC.
                            ((Border)button.FindVisualChildByName("Chrome")).Background = new SolidColorBrush(Microsoft.UI.Colors.Black);
                            ((Border)button.FindVisualChildByName("OpenChrome")).Background = new SolidColorBrush(Microsoft.UI.Colors.Black);
                        });

                        foreach (var state in new[] { "FlyoutClosed", "FlyoutOpen" })
                        {
                            RunOnUIThread.Execute(() =>
                            {
                                Verify.IsTrue(VisualStateManager.GoToState(button, state, false));
                                Content.UpdateLayout();
                            });
                            IdleSynchronizer.Wait();
                            RunOnUIThread.Execute(() =>
                            {
                                bounds = separator.TransformToVisual(snapshotRoot).TransformBounds(
                                    new Rect(0, 0, separator.ActualWidth, separator.ActualHeight));
                                snapshotWidth = snapshotRoot.ActualWidth;
                                snapshotHeight = snapshotRoot.ActualHeight;
                            });

                            var image = RenderElementPixels(snapshotRoot);
                            int centerX = (int)((bounds.X + 0.5) * image.Width / snapshotWidth);
                            int y = (int)((bounds.Y + bounds.Height / 2) * image.Height / snapshotHeight);
                            bool found = false;
                            for (int x = Math.Max(0, centerX - 2); x <= Math.Min(image.Width - 1, centerX + 2); x++)
                            {
                                int offset = (y * image.Width + x) * 4;
                                found |= image.Pixels[offset] == separatorColor.B
                                    && image.Pixels[offset + 1] == separatorColor.G
                                    && image.Pixels[offset + 2] == separatorColor.R
                                    && image.Pixels[offset + 3] == separatorColor.A;
                            }
                            Verify.IsTrue(found,
                                $"The always-present column separator must remain above opaque dropdown chrome: {theme}, {flow}, {density.Density}, {state}.");
                        }
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyHeaderDropdownRestoresInheritedForegroundAfterHighContrastStyle()
        {
            bool highContrast = false;
            RunOnUIThread.Execute(() => highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast);

            foreach (var strong in new[] { false, true })
            {
                TableView tableView = null;
                DropDownButton button = null;
                FontIcon chevron = null;
                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.Columns[0].HeaderFlyout = new MenuFlyout();
                    if (strong)
                    {
                        tableView.ColumnHeaderStyle = (Style)new TabularControlsResources()["StrongTableViewColumnHeaderStyle"];
                    }
                    LoadContent(tableView);
                });
                IdleSynchronizer.Wait();

                foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
                {
                    RunOnUIThread.Execute(() => tableView.RequestedTheme = theme);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var header = RequireColumnHeader(tableView, 0);
                        button = (DropDownButton)header.FindVisualChildByName("PART_HeaderFlyoutButton");
                        chevron = (FontIcon)button.FindVisualChildByName("Chevron");
                        Verify.IsNotNull(chevron.Foreground, "Theme changes must not leave the chevron without a foreground.");
                        var expected = highContrast
                            ? ((SolidColorBrush)Application.Current.Resources["SystemColorButtonTextColorBrush"]).Color
                            : ((SolidColorBrush)header.Foreground).Color;
                        Verify.AreEqual(expected, ((SolidColorBrush)chevron.Foreground).Color,
                            $"The existing chevron must follow the resolved theme: {theme}, strong={strong}.");
                    });
                }

                // Exercise the real template's HC -> normal style replacement without changing the OS setting.
                foreach (var styleTheme in new[] { "HighContrast", "Light", "HighContrast", "Default", "Light" })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var root = (Grid)VisualTreeHelper.GetParent(button.FindVisualChildByName("Chrome"));
                        var dictionary = (ResourceDictionary)root.Resources.ThemeDictionaries[styleTheme];
                        chevron.Style = (Style)dictionary["HeaderFlyoutChevronStyle"];
                    });
                    IdleSynchronizer.Wait();

                    foreach (var color in new[] { Color.FromArgb(0xFF, 0x80, 0x60, 0x40), Color.FromArgb(0xFF, 0x40, 0x60, 0x80) })
                    {
                        RunOnUIThread.Execute(() => RequireColumnHeader(tableView, 0).Foreground = new SolidColorBrush(color));
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var expected = styleTheme == "HighContrast"
                                ? ((SolidColorBrush)Application.Current.Resources["SystemColorButtonTextColorBrush"]).Color
                                : color;
                            Verify.IsNotNull(chevron.Foreground, $"The chevron must have a foreground after {styleTheme} style restoration.");
                            Verify.AreEqual(expected, ((SolidColorBrush)chevron.Foreground).Color,
                                $"Only HC overrides inherited foreground: {styleTheme}, strong={strong}.");
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyHeaderDropdownForegroundIsIndependentOfHeaderInteraction()
        {
            bool highContrast = false;
            var hover = Color.FromArgb(0xFF, 0x20, 0x40, 0x60);
            var pressed = Color.FromArgb(0xFF, 0x60, 0x40, 0x20);
            var disabled = Color.FromArgb(0xFF, 0x40, 0x60, 0x20);
            RunOnUIThread.Execute(() =>
            {
                highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceHeaderForegroundPointerOverBrush", new SolidColorBrush(hover));
                SetApplicationResource("TabularSurfaceHeaderForegroundPressedBrush", new SolidColorBrush(pressed));
                SetApplicationResource("TabularSurfaceRowForegroundDisabledBrush", new SolidColorBrush(disabled));
            });

            try
            {
                foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
                {
                    foreach (var appearance in new[] { "Regular", "Strong", "Custom", "SortingOff", "NonSortable" })
                    {
                        TableView tableView = null;
                        TableViewColumnHeader header = null;
                        DropDownButton button = null;
                        FontIcon chevron = null;
                        Color foreground = default;
                        Color background = default;
                        RunOnUIThread.Execute(() =>
                        {
                            tableView = CreateThemingTable();
                            tableView.RequestedTheme = theme;
                            tableView.Columns[0].HeaderFlyout = new MenuFlyout();
                            if (appearance == "SortingOff")
                            {
                                tableView.CanUserSortColumns = false;
                            }
                            if (appearance == "NonSortable")
                            {
                                tableView.Columns[0].CanSort = false;
                            }
                            if (appearance == "Strong")
                            {
                                tableView.ColumnHeaderStyle = (Style)new TabularControlsResources()["StrongTableViewColumnHeaderStyle"];
                            }
                            LoadContent(tableView);
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            header = RequireColumnHeader(tableView, 0);
                            button = (DropDownButton)header.FindVisualChildByName("PART_HeaderFlyoutButton");
                            chevron = (FontIcon)button.FindVisualChildByName("Chevron");
                            Verify.IsNotNull(chevron.Foreground, "The dropdown chevron must resolve its resting foreground.");
                            if (appearance == "Custom")
                            {
                                header.Foreground = new SolidColorBrush(Color.FromArgb(0xFF, 0x80, 0x60, 0x40));
                            }
                            foreground = highContrast
                                ? ((SolidColorBrush)Application.Current.Resources["SystemColorButtonTextColorBrush"]).Color
                                : ((SolidColorBrush)header.Foreground).Color;
                            background = ((SolidColorBrush)((Border)button.FindVisualChildByName("Chrome")).Background).Color;
                            if (highContrast)
                            {
                                Verify.AreEqual(((SolidColorBrush)Application.Current.Resources["SystemColorButtonFaceColorBrush"]).Color,
                                    background, $"The HC dropdown uses ButtonFace independently of sorting ({theme}, {appearance}).");
                            }
                        });
                        IdleSynchronizer.Wait();

                        foreach (var state in new[] { "PointerOver", "Pressed" })
                        {
                            var interaction = state == "PointerOver" ? hover : pressed;
                            RunOnUIThread.Execute(() =>
                            {
                                Verify.AreEqual(foreground, ((SolidColorBrush)chevron.Foreground).Color);
                                Verify.IsTrue(VisualStateManager.GoToState(header, state, false));
                                Verify.IsTrue(VisualStateManager.GoToState(header, "HighContrast" + state, false));
                            });
                            IdleSynchronizer.Wait();
                            RunOnUIThread.Execute(() =>
                            {
                                var context = $"{theme}, {appearance}, {state}";
                                Verify.AreEqual(interaction, ((SolidColorBrush)GetHeaderPresenter(tableView, 0).Foreground).Color,
                                    $"The header uses its High Contrast interaction foreground ({context}).");
                                Verify.AreEqual(foreground, ((SolidColorBrush)chevron.Foreground).Color,
                                    $"Header interaction must not recolor the unhovered dropdown ({context}).");
                                Verify.AreEqual(background,
                                    ((SolidColorBrush)((Border)button.FindVisualChildByName("Chrome")).Background).Color,
                                    $"Header interaction must not change the dropdown background ({context}).");
                                Verify.IsTrue(VisualStateManager.GoToState(button, state, false));
                            });
                            IdleSynchronizer.Wait();
                            RunOnUIThread.Execute(() =>
                            {
                                Verify.AreEqual(interaction, ((SolidColorBrush)chevron.Foreground).Color,
                                    "The dropdown must still use its interaction foreground when it is itself hovered or pressed.");
                                Verify.IsTrue(VisualStateManager.GoToState(button, "Normal", false));
                                Verify.IsTrue(VisualStateManager.GoToState(header, "Normal", false));
                                Verify.IsTrue(VisualStateManager.GoToState(header, "HighContrastNormal", false));
                            });
                            IdleSynchronizer.Wait();
                        }

                        SetIsEnabledAndWait(tableView, false, header, button);
                        RunOnUIThread.Execute(() =>
                        {
                            var context = $"{theme}, {appearance}";
                            Verify.IsFalse(button.IsEnabled, $"The disabled header must disable its dropdown ({context}).");
                            Verify.AreEqual(disabled, ((SolidColorBrush)chevron.Foreground).Color,
                                $"Disabling the header must still dim its dropdown independently ({context}).");
                        });
                        SetIsEnabledAndWait(tableView, true, header, button);
                        RunOnUIThread.Execute(() =>
                        {
                            Verify.IsTrue(button.IsEnabled);
                            Verify.AreEqual(foreground, ((SolidColorBrush)chevron.Foreground).Color,
                                "Re-enabling the header restores its dropdown's resting foreground.");
                        });
                    }
                }
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    ClearApplicationResource("TabularSurfaceHeaderForegroundPointerOverBrush");
                    ClearApplicationResource("TabularSurfaceHeaderForegroundPressedBrush");
                    ClearApplicationResource("TabularSurfaceRowForegroundDisabledBrush");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a disabled TableView puts its headers in Disabled, dimming the header text and sort glyph.")]
        public void VerifyDisabledHeaderDimsText()
        {
            var disabled = Color.FromArgb(0xFF, 0x20, 0x40, 0x60);
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceRowForegroundDisabledBrush", new SolidColorBrush(disabled));
                tableView = CreateThemingTable();
                tableView.Columns[0].CanSort = true;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the column sorts.");
                    tableView.IsEnabled = false;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var header = RequireColumnHeader(tableView, 0);
                    Verify.AreEqual("Disabled", CurrentState(header, "CommonStates"), "A disabled table's header must be Disabled.");
                    Verify.AreEqual(disabled, (GetHeaderPresenter(tableView, 0).Foreground as SolidColorBrush)?.Color ?? default,
                        "Disabled must dim the header text.");
                    var glyph = header.FindVisualChildByName("TableViewSortIndicator") as Control;
                    Verify.IsNotNull(glyph, "A sorted column must host the sort glyph.");
                    Verify.AreEqual(disabled, (glyph?.Foreground as SolidColorBrush)?.Color ?? default, "Disabled must dim the sort glyph.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceRowForegroundDisabledBrush"));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies disabling a header in its High Contrast hover state keeps the disabled text and glyph brush, and re-enabling drops it.")]
        public void VerifyDisablingHighContrastHoveredHeaderKeepsDisabledForeground()
        {
            var disabled = Color.FromArgb(0xFF, 0x20, 0x40, 0x60);
            TableView tableView = null;
            TableViewColumnHeader header = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                SetApplicationResource("TabularSurfaceRowForegroundDisabledBrush", new SolidColorBrush(disabled));
                tableView = CreateThemingTable();
                tableView.Columns[0].CanSort = true;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            try
            {
                RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the column sorts."));
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    header = RequireColumnHeader(tableView, 0);
                    // The hovered High Contrast states, entered directly: a test cannot switch the system theme.
                    Verify.IsTrue(VisualStateManager.GoToState(header, "PointerOver", false), "PointerOver must exist.");
                    Verify.IsTrue(VisualStateManager.GoToState(header, "HighContrastPointerOver", false), "HighContrastPointerOver must exist.");
                    header.IsEnabled = false;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual("Disabled", CurrentState(header, "CommonStates"), "Precondition: the disabled header is Disabled.");
                    Verify.AreEqual("HighContrastNormal", CurrentState(header, "HighContrastStates"), "Precondition: the High Contrast hover state is cleared.");
                    Verify.AreEqual(disabled, (GetHeaderPresenter(tableView, 0).Foreground as SolidColorBrush)?.Color ?? default,
                        "Leaving the High Contrast hover state must not drop the disabled text brush.");
                    var glyph = header.FindVisualChildByName("TableViewSortIndicator") as Control;
                    Verify.IsNotNull(glyph, "A sorted column must host the sort glyph.");
                    Verify.AreEqual(disabled, (glyph?.Foreground as SolidColorBrush)?.Color ?? default,
                        "Leaving the High Contrast hover state must not drop the disabled glyph brush.");
                    header.IsEnabled = true;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual("Normal", CurrentState(header, "CommonStates"), "Precondition: the re-enabled header is Normal.");
                    Verify.AreNotEqual(disabled, (GetHeaderPresenter(tableView, 0).Foreground as SolidColorBrush)?.Color ?? default,
                        "Re-enabling must drop the disabled text brush.");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => ClearApplicationResource("TabularSurfaceRowForegroundDisabledBrush"));
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a disabled header's sort glyph follows the header text when its theme brush re-resolves without a visual state change.")]
        public void VerifyDisabledSortGlyphFollowsTextAcrossThemeChange()
        {
            TableView tableView = null;
            TableViewColumnHeader header = null;
            Color before = default;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                tableView.RequestedTheme = ElementTheme.Light;
                tableView.Columns[0].CanSort = true;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the column sorts.");
                tableView.IsEnabled = false;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                header = RequireColumnHeader(tableView, 0);
                Verify.AreEqual("Disabled", CurrentState(header, "CommonStates"), "Precondition: a disabled table's header is Disabled.");
                before = (GetHeaderPresenter(tableView, 0).Foreground as SolidColorBrush)?.Color ?? default;

                // Re-resolves the header's ThemeResources without re-running its visual states, as a
                // High Contrast toggle does.
                header.RequestedTheme = ElementTheme.Dark;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var text = (GetHeaderPresenter(tableView, 0).Foreground as SolidColorBrush)?.Color ?? default;
                Verify.AreNotEqual(before, text, "Precondition: the disabled text brush differs between Light and Dark.");
                var glyph = header.FindVisualChildByName("TableViewSortIndicator") as Control;
                Verify.IsNotNull(glyph, "A sorted column must host the sort glyph.");
                Verify.AreEqual(text, (glyph?.Foreground as SolidColorBrush)?.Color ?? default,
                    "The disabled sort glyph must keep the header text's brush after it re-resolves.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style with an incompatible TargetType is ignored instead of breaking the header band.")]
        public void VerifyIncompatibleColumnHeaderStyleIsIgnored()
        {
            TableView tableView = null;
            SolidColorBrush brush = null;

            RunOnUIThread.Execute(() =>
            {
                brush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                var buttonStyle = new Style(typeof(Button));
                buttonStyle.Setters.Add(new Setter(Control.BackgroundProperty, brush));
                tableView = CreateThemingTable();
                tableView.ColumnHeaderStyle = buttonStyle;
                tableView.Columns[1].HeaderStyle = buttonStyle;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var index in new[] { 0, 1 })
                {
                    var header = RequireColumnHeader(tableView, index);
                    Verify.IsNull(header.Style, $"Header {index} must not take a Style targeting another type.");
                    Verify.IsFalse(ReferenceEquals(brush, header.Background), $"Header {index} must keep its default look.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style whose BasedOn targets an incompatible type is ignored instead of breaking the header band.")]
        public void VerifyColumnHeaderStyleWithIncompatibleBasedOnIsIgnored()
        {
            TableView tableView = null;
            SolidColorBrush brush = null;

            RunOnUIThread.Execute(() =>
            {
                brush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                var headerStyle = new Style(typeof(TableViewColumnHeader)) { BasedOn = new Style(typeof(Button)) };
                headerStyle.Setters.Add(new Setter(Control.BackgroundProperty, brush));
                tableView = CreateThemingTable();
                tableView.ColumnHeaderStyle = headerStyle;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var index in new[] { 0, 1 })
                {
                    var header = RequireColumnHeader(tableView, index);
                    Verify.IsNull(header.Style, $"Header {index} must not take a Style based on another type.");
                    Verify.IsFalse(ReferenceEquals(brush, header.Background), $"Header {index} must keep its default look.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style whose BasedOn chain loops is ignored instead of breaking the header band.")]
        public void VerifyColumnHeaderStyleWithBasedOnCycleIsIgnored()
        {
            TableView tableView = null;
            SolidColorBrush brush = null;

            RunOnUIThread.Execute(() =>
            {
                brush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                var first = new Style(typeof(TableViewColumnHeader));
                var second = new Style(typeof(TableViewColumnHeader)) { BasedOn = first };
                first.BasedOn = second;
                first.Setters.Add(new Setter(Control.BackgroundProperty, brush));
                tableView = CreateThemingTable();
                tableView.ColumnHeaderStyle = first;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var index in new[] { 0, 1 })
                {
                    var header = RequireColumnHeader(tableView, index);
                    Verify.IsNull(header.Style, $"Header {index} must not take a Style whose BasedOn chain loops.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style without a TargetType, alone or as a BasedOn, is ignored instead of breaking the header band.")]
        public void VerifyUntypedColumnHeaderStyleIsIgnored()
        {
            TableView tableView = null;
            SolidColorBrush tableBrush = null;
            SolidColorBrush columnBrush = null;

            RunOnUIThread.Execute(() =>
            {
                tableBrush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                columnBrush = new SolidColorBrush(Microsoft.UI.Colors.Green);
                var untyped = new Style();
                untyped.Setters.Add(new Setter(Control.BackgroundProperty, tableBrush));
                var untypedBase = new Style();
                untypedBase.Setters.Add(new Setter(Control.BackgroundProperty, columnBrush));
                tableView = CreateThemingTable();
                tableView.ColumnHeaderStyle = untyped;
                tableView.Columns[1].HeaderStyle = new Style(typeof(TableViewColumnHeader)) { BasedOn = untypedBase };
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var index in new[] { 0, 1 })
                {
                    var header = RequireColumnHeader(tableView, index);
                    Verify.IsNull(header.Style, $"Header {index} must not take a Style chain without a TargetType.");
                    Verify.IsFalse(ReferenceEquals(tableBrush, header.Background) || ReferenceEquals(columnBrush, header.Background),
                        $"Header {index} must keep its default look.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies header Styles targeting TableViewColumnHeader's framework base types are applied.")]
        public void VerifyColumnHeaderStyleTargetingBaseTypesApplies()
        {
            TableView tableView = null;
            Style tableStyle = null;
            Style columnStyle = null;
            SolidColorBrush tableBrush = null;
            SolidColorBrush columnBrush = null;

            RunOnUIThread.Execute(() =>
            {
                tableBrush = new SolidColorBrush(Microsoft.UI.Colors.Red);
                columnBrush = new SolidColorBrush(Microsoft.UI.Colors.Green);
                // Framework types: their TargetType names come back as freshly allocated strings.
                tableStyle = new Style(typeof(ContentControl));
                tableStyle.Setters.Add(new Setter(Control.BackgroundProperty, tableBrush));
                columnStyle = new Style(typeof(Control));
                columnStyle.Setters.Add(new Setter(Control.BackgroundProperty, columnBrush));
                tableView = CreateThemingTable();
                tableView.ColumnHeaderStyle = tableStyle;
                tableView.Columns[1].HeaderStyle = columnStyle;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var first = RequireColumnHeader(tableView, 0);
                Verify.IsTrue(ReferenceEquals(tableStyle, first.Style), "A ContentControl Style must apply to the header.");
                Verify.IsTrue(ReferenceEquals(tableBrush, first.Background), "The ContentControl Style's setter must reach the header.");
                var second = RequireColumnHeader(tableView, 1);
                Verify.IsTrue(ReferenceEquals(columnStyle, second.Style), "A Control Style must apply to the header.");
                Verify.IsTrue(ReferenceEquals(columnBrush, second.Background), "The Control Style's setter must reach the header.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a disabled header keeps one glyph Foreground binding across visual state updates instead of rebinding each time.")]
        public void VerifyDisabledSortGlyphBindingIsNotRecreated()
        {
            TableView tableView = null;
            Control glyph = null;
            BindingExpression binding = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                tableView.Columns[0].CanSort = true;
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the column sorts.");
                tableView.IsEnabled = false;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                glyph = RequireColumnHeader(tableView, 0).FindVisualChildByName("TableViewSortIndicator") as Control;
                Verify.IsNotNull(glyph, "Precondition: a sorted column hosts the sort glyph.");
                binding = glyph.GetBindingExpression(Control.ForegroundProperty);
                Verify.IsNotNull(binding, "Precondition: the disabled glyph is bound to the header text.");
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending), "Precondition: the sort direction changes.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(ReferenceEquals(glyph, RequireColumnHeader(tableView, 0).FindVisualChildByName("TableViewSortIndicator")),
                    "Precondition: the sort change updates the realized header in place.");
                Verify.IsTrue(ReferenceEquals(binding, glyph.GetBindingExpression(Control.ForegroundProperty)),
                    "A state update must keep the glyph's existing binding.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the default header template provides its parts and hosts the sort glyph and resize gripper in them.")]
        public void VerifyColumnHeaderTemplateParts()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                var presenter = header.FindVisualChildByName("PART_ContentPresenter") as ContentPresenter;
                Verify.IsNotNull(presenter, "PART_ContentPresenter should exist.");
                Verify.IsTrue(ReferenceEquals(presenter, GetHeaderPresenter(tableView, 0)), "The content presenter should be the header's first ContentPresenter.");
                Verify.IsNotNull(header.FindVisualChildByName("PART_GridLine") as Border, "PART_GridLine should exist.");

                var indicatorHost = header.FindVisualChildByName("PART_SortIndicatorHost") as Border;
                Verify.IsNotNull(indicatorHost, "PART_SortIndicatorHost should exist.");
                Verify.IsTrue(indicatorHost.Child is SortIndicator, "A sortable header should host its sort glyph in PART_SortIndicatorHost.");

                var gripperHost = header.FindVisualChildByName("PART_ResizeGripperHost") as Border;
                Verify.IsNotNull(gripperHost, "PART_ResizeGripperHost should exist.");
                Verify.IsTrue(gripperHost.Child is ResizeGripper, "A resizable header should host its gripper in PART_ResizeGripperHost.");
            });
        }

        [TestMethod]
        public void VerifySortIndicatorGeometryAndHeaderPlacement()
        {
            foreach (var density in new[] { TableViewDensity.Standard, TableViewDensity.Compact, TableViewDensity.ExtraCompact })
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            {
                TableView tableView = null;
                Grid root = null;
                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateThemingTable();
                    tableView.Density = density;
                    tableView.FlowDirection = flow;
                    tableView.Columns[0].Width = new GridLength(200);
                    root = new Grid { FlowDirection = FlowDirection.LeftToRight };
                    root.Children.Add(tableView);
                    LoadContent(root);
                    tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending);
                });
                IdleSynchronizer.Wait();

                void VerifyPlacement(bool customPadding, SortDirection direction)
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var context = $"{density}/{flow}/{direction}, custom padding={customPadding}";
                        var header = RequireColumnHeader(tableView, 0);
                        var presenter = (ContentPresenter)header.FindVisualChildByName("PART_ContentPresenter");
                        var host = (Border)header.FindVisualChildByName("PART_SortIndicatorHost");
                        var indicator = (SortIndicator)host.Child;
                        var glyph = indicator.FindVisualChildByName("GlyphIcon") as Microsoft.UI.Xaml.Shapes.Path;
                        Verify.IsNotNull(glyph, $"The icon must be a vector Path ({context}).");
                        var geometry = glyph.Data as PathGeometry;
                        Verify.IsNotNull(geometry, $"The direction must supply vector geometry ({context}).");
                        Verify.AreEqual(4, geometry.Figures.Count, $"Keep the arrow and three sort lines ({context}).");
                        Verify.AreEqual(direction == SortDirection.Ascending ? 14.5 : 1.5,
                            geometry.Figures[0].StartPoint.Y, $"Use the corresponding exported SVG ({context}).");

                        var bounds = indicator.TransformToVisual(header).TransformBounds(
                            new Rect(0, 0, indicator.ActualWidth, indicator.ActualHeight));
                        Verify.IsLessThan(Math.Abs(bounds.Width - 16), 0.1, $"Preserve the 16px icon viewport ({context}).");
                        Verify.IsLessThan(Math.Abs(bounds.Height - 16), 0.1, $"Preserve the 16px icon viewport ({context}).");
                        var physicalBounds = indicator.TransformToVisual(root).TransformBounds(
                            new Rect(0, 0, indicator.ActualWidth, indicator.ActualHeight));
                        var headerBounds = header.TransformToVisual(root).TransformBounds(
                            new Rect(0, 0, header.ActualWidth, header.ActualHeight));
                        var trailingInset = flow == FlowDirection.LeftToRight
                            ? headerBounds.Right - physicalBounds.Right : physicalBounds.Left - headerBounds.Left;
                        Verify.IsLessThan(Math.Abs(header.Padding.Right + 4 - trailingInset), 0.1,
                            $"The icon must follow header padding plus 4px at the trailing edge ({context}).");
                        var centerTolerance = 0.5 / header.XamlRoot.RasterizationScale + 0.01;
                        Verify.IsLessThan(Math.Abs((header.ActualHeight + header.Padding.Top - header.Padding.Bottom) / 2
                            - bounds.Top - bounds.Height / 2), centerTolerance, $"Center within the padded header ({context}).");
                        Verify.IsLessThan(Math.Abs(presenter.Margin.Right - 24), 0.1,
                            $"Reserve the icon and both 4px host paddings ({context}).");
                        var text = (TextBlock)presenter.Content;
                        var textBounds = text.TransformToVisual(root).TransformBounds(
                            new Rect(0, 0, text.ActualWidth, text.ActualHeight));
                        var gap = flow == FlowDirection.LeftToRight
                            ? physicalBounds.Left - textBounds.Right : textBounds.Left - physicalBounds.Right;
                        Verify.IsTrue(gap >= 3.9, $"Text must stay at least 4px away from the icon ({context}).");
                        Verify.IsFalse(indicator.IsHitTestVisible, $"The icon must not intercept header input ({context}).");
                        Verify.AreEqual(AccessibilityView.Raw, AutomationProperties.GetAccessibilityView(indicator),
                            $"The header peer, not the decorative icon, announces sorting ({context}).");
                    });
                }

                VerifyPlacement(false, SortDirection.Ascending);
                RunOnUIThread.Execute(() =>
                {
                    RequireColumnHeader(tableView, 0).Padding = new Thickness(20, 3, 28, 7);
                    tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending);
                    tableView.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                VerifyPlacement(true, SortDirection.Descending);

                RunOnUIThread.Execute(() => tableView.ClearSort());
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var header = RequireColumnHeader(tableView, 0);
                    var host = (Border)header.FindVisualChildByName("PART_SortIndicatorHost");
                    Verify.AreEqual(Visibility.Collapsed, host.Visibility, "Clearing sort hides the icon.");
                    Verify.AreEqual(24.0, ((ContentPresenter)header.FindVisualChildByName("PART_ContentPresenter")).Margin.Right,
                        "Unsorted headers must keep the same text reservation.");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the unsorted affordance uses an 83ms opacity-only fade, survives reversals, and never hides an actual sort.")]
        public void VerifyUnsortedHeaderAffordanceFadeTransitions()
        {
            TableView tableView = null;
            TableViewColumnHeader header = null;
            Border host = null;

            void VerifySettledState(string state, Visibility visibility, double opacity)
            {
                var stopwatch = global::System.Diagnostics.Stopwatch.StartNew();
                bool settled = false;
                do
                {
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() => settled = CurrentState(header, "SortStates") == state &&
                        host.Visibility == visibility && host.Opacity == opacity);
                    if (!settled)
                    {
                        Thread.Sleep(50);
                    }
                }
                while (!settled && stopwatch.Elapsed < TimeSpan.FromSeconds(10));

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(state, CurrentState(header, "SortStates"), "The requested sort state must settle.");
                    Verify.AreEqual(visibility, host.Visibility, "The affordance must reach its final visibility.");
                    Verify.AreEqual(opacity, host.Opacity, "The affordance must reach its final opacity.");
                });
            }

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                header = RequireColumnHeader(tableView, 0);
                host = (Border)header.FindVisualChildByName("PART_SortIndicatorHost");
                var root = (FrameworkElement)VisualTreeHelper.GetChild(header, 0);
                var group = VisualStateManager.GetVisualStateGroups(root).Single(g => g.Name == "SortStates");
                Verify.AreEqual(2, group.Transitions.Count, "Only unsorted affordance entry/exit should animate.");
                foreach (var transition in group.Transitions)
                {
                    bool entering = transition.To == "UnsortedSortAvailable";
                    Verify.AreEqual(entering ? "Unsorted" : "UnsortedSortAvailable", transition.From);
                    Verify.AreEqual(entering ? "UnsortedSortAvailable" : "Unsorted", transition.To);
                    Verify.AreEqual(83.0, transition.GeneratedDuration.TimeSpan.TotalMilliseconds);
                    Verify.IsNull(transition.GeneratedEasingFunction, "The generated opacity fade must be linear.");
                    var timelines = transition.Storyboard.Children;
                    Verify.AreEqual(1, timelines.Count, "Only hold visibility explicitly; VSM generates the opacity fade.");
                    var visibility = (ObjectAnimationUsingKeyFrames)timelines[0];
                    Verify.AreEqual("PART_SortIndicatorHost", Storyboard.GetTargetName(visibility));
                    Verify.AreEqual("Visibility", Storyboard.GetTargetProperty(visibility));
                    Verify.AreEqual(83.0, visibility.Duration.TimeSpan.TotalMilliseconds);
                    Verify.AreEqual(1, visibility.KeyFrames.Count);
                    Verify.AreEqual(TimeSpan.Zero, visibility.KeyFrames[0].KeyTime.TimeSpan);
                    Verify.AreEqual("Visible", visibility.KeyFrames[0].Value.ToString(),
                        "Keep the glyph visible until fade-out has completed.");
                }
                Verify.AreEqual(SortIndicatorDirection.Ascending, ((SortIndicator)host.Child).Direction,
                    "The hidden glyph must remain rendered for the host's fade-out.");
                Verify.IsTrue(VisualStateManager.GoToState(header, "UnsortedSortAvailable", false));
                Verify.IsTrue(VisualStateManager.GoToState(header, "Unsorted", true));
            });
            VerifySettledState("Unsorted", Visibility.Collapsed, 0);

            RunOnUIThread.Execute(() =>
                Verify.IsTrue(VisualStateManager.GoToState(header, "UnsortedSortAvailable", true)));
            VerifySettledState("UnsortedSortAvailable", Visibility.Visible, 1);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(VisualStateManager.GoToState(header, "Unsorted", true));
                Verify.IsTrue(VisualStateManager.GoToState(header, "UnsortedSortAvailable", true));
                Verify.IsTrue(VisualStateManager.GoToState(header, "Unsorted", true));
            });
            VerifySettledState("Unsorted", Visibility.Collapsed, 0);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortIndicatorDirection.Ascending, ((SortIndicator)host.Child).Direction);
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection);
                Verify.IsTrue(VisualStateManager.GoToState(header, "UnsortedSortAvailable", false));
                Verify.IsTrue(VisualStateManager.GoToState(header, "Unsorted", true));
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending));
                Verify.AreEqual(Visibility.Visible, host.Visibility, "Sorting must immediately replace a pending fade.");
                Verify.AreEqual(1.0, host.Opacity, "An actual sorted indicator must be fully visible immediately.");
            });
            VerifySettledState("SortDescending", Visibility.Visible, 1);
            RunOnUIThread.Execute(() =>
                Verify.AreEqual(SortIndicatorDirection.Descending, ((SortIndicator)host.Child).Direction));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies keyboard focus previews sorting without changing sort state, geometry, or automation help.")]
        public void VerifyUnsortedHeaderFocusShowsSortAffordance()
        {
            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                foreach (var density in DensityCases)
                {
                    foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
                    {
                        TableView tableView = null;
                        TableViewColumnHeader header = null;
                        string help = null;
                        double width = 0;
                        Thickness margin = default;
                        RunOnUIThread.Execute(() =>
                        {
                            tableView = CreateThemingTable();
                            tableView.RequestedTheme = theme;
                            tableView.Density = density.Density;
                            tableView.FlowDirection = flow;
                            tableView.Columns[1].CanSort = false;
                            LoadContent(tableView);
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            header = RequireColumnHeader(tableView, 0);
                            help = FrameworkElementAutomationPeer.CreatePeerForElement(header).GetHelpText();
                            width = tableView.Columns[0].ActualWidth;
                            margin = GetHeaderPresenter(tableView, 0).Margin;
                            Verify.IsTrue(header.Focus(FocusState.Keyboard), "The sortable header takes keyboard focus.");
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var context = $"{theme}, {density.Density}, {flow}";
                            Verify.AreEqual(FocusState.Keyboard, header.FocusState, context);
                            Verify.AreEqual("UnsortedSortAvailable", CurrentState(header, "SortStates"), context);
                            var host = (Border)header.FindVisualChildByName("PART_SortIndicatorHost");
                            Verify.AreEqual(Visibility.Visible, host.Visibility, context);
                            Verify.AreEqual(1.0, host.Opacity, $"Keyboard-focus feedback must be immediate: {context}");
                            Verify.AreEqual(SortIndicatorDirection.Ascending, ((SortIndicator)host.Child).Direction,
                                $"The unsorted affordance uses the ascending glyph: {context}");
                            Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, $"Focus must not sort: {context}");
                            Verify.AreEqual(help, FrameworkElementAutomationPeer.CreatePeerForElement(header).GetHelpText(),
                                $"Focus must not announce an actual sort: {context}");
                            Verify.AreEqual(width, tableView.Columns[0].ActualWidth, $"Focus must not resize columns: {context}");
                            Verify.AreEqual(margin, GetHeaderPresenter(tableView, 0).Margin, $"Focus must not move text: {context}");
                            tableView.CanUserSortColumns = false;
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            header = RequireColumnHeader(tableView, 0);
                            Verify.AreEqual("Unsorted", CurrentState(header, "SortStates"),
                                "Disabling sorting hides the unsorted affordance even with focus.");
                            Verify.AreEqual(Visibility.Collapsed,
                                ((Border)header.FindVisualChildByName("PART_SortIndicatorHost")).Visibility);
                            tableView.CanUserSortColumns = true;
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            header = RequireColumnHeader(tableView, 0);
                            Verify.IsTrue(header.Focus(FocusState.Keyboard));
                            tableView.IsEnabled = false;
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            Verify.AreEqual("Unsorted", CurrentState(header, "SortStates"),
                                "A disabled header must not advertise sorting.");
                            tableView.IsEnabled = true;
                            Verify.IsTrue(header.Focus(FocusState.Keyboard));
                            Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending));
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            Verify.AreEqual("SortDescending", CurrentState(header, "SortStates"),
                                "An actual descending sort must not be replaced by the ascending preview.");
                            var host = (Border)header.FindVisualChildByName("PART_SortIndicatorHost");
                            Verify.AreEqual(SortIndicatorDirection.Descending, ((SortIndicator)host.Child).Direction);
                            Verify.IsTrue(tableView.ClearSort());
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            Verify.AreEqual("UnsortedSortAvailable", CurrentState(header, "SortStates"),
                                "Clearing sorting while focused restores the affordance.");
                            Verify.IsTrue(RequireColumnHeader(tableView, 1).Focus(FocusState.Keyboard));
                        });
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            Verify.AreEqual("Unsorted", CurrentState(header, "SortStates"),
                                "Leaving the header hides its unsorted affordance.");
                            Verify.AreEqual(Visibility.Collapsed,
                                ((Border)header.FindVisualChildByName("PART_SortIndicatorHost")).Visibility);
                            Verify.AreEqual("Unsorted", CurrentState(RequireColumnHeader(tableView, 1), "SortStates"),
                                "A focused non-sortable header does not advertise sorting.");
                        });
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies resting SortStates follow the column's actual sort direction.")]
        public void VerifyColumnHeaderSortStates()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            void VerifyState(string expected, Visibility hostVisibility, string context)
            {
                RunOnUIThread.Execute(() =>
                {
                    var header = RequireColumnHeader(tableView, 0);
                    Verify.AreEqual(expected, CurrentState(header, "SortStates"), $"SortStates ({context}).");
                    var host = (UIElement)header.FindVisualChildByName("PART_SortIndicatorHost");
                    Verify.AreEqual(hostVisibility, host.Visibility, $"Sort glyph visibility ({context}).");
                    Verify.AreEqual("Unsorted", CurrentState(RequireColumnHeader(tableView, 1), "SortStates"),
                        $"The other column stays Unsorted ({context}).");
                });
            }

            VerifyState("Unsorted", Visibility.Collapsed, "initial");

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending)));
            IdleSynchronizer.Wait();
            VerifyState("SortAscending", Visibility.Visible, "ascending");

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending)));
            IdleSynchronizer.Wait();
            VerifyState("SortDescending", Visibility.Visible, "descending");

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.ClearSort()));
            IdleSynchronizer.Wait();
            VerifyState("Unsorted", Visibility.Collapsed, "cleared");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies header typography comes from the header Style, not from local values written by the control.")]
        public void VerifyColumnHeaderTypographyHasNoLocalValues()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateThemingTable();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = RequireColumnHeader(tableView, 0);
                var presenter = GetHeaderPresenter(tableView, 0);
                var text = presenter.Content as TextBlock;
                Verify.IsNotNull(text, "A plain string header should render through a TextBlock.");

                foreach (var (element, fontSize, fontWeight, name) in new (DependencyObject, DependencyProperty, DependencyProperty, string)[]
                {
                    (header, Control.FontSizeProperty, Control.FontWeightProperty, "header"),
                    (presenter, ContentPresenter.FontSizeProperty, ContentPresenter.FontWeightProperty, "content presenter"),
                    (text, TextBlock.FontSizeProperty, TextBlock.FontWeightProperty, "header text"),
                })
                {
                    Verify.AreEqual(DependencyProperty.UnsetValue, element.ReadLocalValue(fontSize), $"The {name} must not carry a local FontSize.");
                    Verify.AreEqual(DependencyProperty.UnsetValue, element.ReadLocalValue(fontWeight), $"The {name} must not carry a local FontWeight.");
                }

                Verify.AreEqual((ushort)400, text.FontWeight.Weight, "The default header style keeps Normal weight.");
                Verify.AreEqual(14.0, text.FontSize, "The default header style keeps TableViewHeaderFontSize.");
            });
        }

        #endregion

        #region 13.6 Row banding under an element theme

        [TestMethod]
        public void VerifyBandedRowPlateGeometryMatchesHover()
        {
            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            foreach (var density in DensityCases)
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            foreach (var lines in new[] { TableViewGridLinesVisibility.None, TableViewGridLinesVisibility.All })
            {
                TableView table = null;
                TableViewRow row = null;
                Border plate = null;
                Rect resting = default;
                Rect content = default;
                double height = 0;
                CornerRadius corners = default;
                RunOnUIThread.Execute(() =>
                {
                    table = CreateThemingTable();
                    table.RequestedTheme = theme;
                    table.Density = density.Density;
                    table.FlowDirection = flow;
                    table.GridLinesVisibility = lines;
                    table.AlternatingRowBackground = new SolidColorBrush(Microsoft.UI.Colors.Red);
                    LoadContent(table);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    row = GetProjectedRow(table, 1);
                    plate = (Border)row.FindVisualChildByName("PART_RootBorder");
                    Verify.IsTrue(VisualStateManager.GoToState(row, "Normal", false));
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    resting = plate.TransformToVisual(row).TransformBounds(new Rect(0, 0, plate.ActualWidth, plate.ActualHeight));
                    var cell = GetRowCell(row, 0);
                    content = cell.TransformToVisual(row).TransformBounds(new Rect(0, 0, cell.ActualWidth, cell.ActualHeight));
                    height = row.ActualHeight;
                    corners = plate.CornerRadius;
                    Verify.AreEqual(new Thickness(3, 2, 3, 2), plate.Margin);
                    Verify.AreEqual(Microsoft.UI.Colors.Red, ((SolidColorBrush)plate.Background).Color);
                });
                foreach (var state in new[] { "PointerOver", "Pressed", "Normal" })
                {
                    RunOnUIThread.Execute(() => Verify.IsTrue(VisualStateManager.GoToState(row, state, false)));
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var context = $"{theme}, {density.Density}, {flow}, {lines}, {state}";
                        Verify.AreEqual(resting, plate.TransformToVisual(row).TransformBounds(
                            new Rect(0, 0, plate.ActualWidth, plate.ActualHeight)), context);
                        Verify.AreEqual(corners, plate.CornerRadius, context);
                        Verify.AreEqual(height, row.ActualHeight, context);
                        var cell = GetRowCell(row, 0);
                        Verify.AreEqual(content, cell.TransformToVisual(row).TransformBounds(
                            new Rect(0, 0, cell.ActualWidth, cell.ActualHeight)), context);
                        if (state == "Normal")
                        {
                            Verify.AreEqual(Microsoft.UI.Colors.Red, ((SolidColorBrush)plate.Background).Color);
                        }
                        else
                        {
                            Verify.AreNotEqual(Microsoft.UI.Colors.Red, ((SolidColorBrush)plate.Background).Color,
                                "Interaction must replace the band color, not change plate geometry.");
                        }
                    });
                }
            }
        }

        [TestMethod]
        public void VerifyBandedTableStyleBrushesMatchSubtleTertiaryAndHighContrast()
        {
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                var resources = new TabularControlsResources();
                foreach (var themeName in new[] { "Light", "Default" })
                {
                    Verify.AreEqual(RequireThemeColor(themeName, "SubtleFillColorTertiary"),
                        RequireBrush(resources, themeName, "TabularSurfaceRowBackgroundAlternatingBrush").Color,
                        $"{themeName}: banding must match Figma's Subtle/Tertiary token.");
                }
                Verify.IsTrue(ReferenceEquals(RequireApplicationBrush("SystemColorWindowColorBrush"),
                    RequireBrush(resources, "HighContrast", "TabularSurfaceRowBackgroundAlternatingBrush")),
                    "High Contrast banding must preserve the window-text/window-background pairing.");
            });
        }

        [TestMethod]
        public void VerifyBandedTableStylePreservesTemplateAndFollowsTheme()
        {
            TableView bandedTable = null;
            TableView defaultTable = null;
            StackPanel host = null;
            TabularControlsResources resources = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                resources = new TabularControlsResources();
                var style = resources["BandedTableViewStyle"] as Style;
                Verify.IsNotNull(style, "TabularControlsResources must expose the named banded variant.");
                Verify.AreEqual(typeof(TableView), style.TargetType);
                Verify.AreEqual(1, style.Setters.Count, "Banding must not change unrelated table settings.");
                bandedTable = CreateThemingTable();
                bandedTable.Style = style;
                bandedTable.GridLinesVisibility = TableViewGridLinesVisibility.Vertical;
                bandedTable.ColumnHeaderStyle = (Style)resources["StrongTableViewColumnHeaderStyle"];
                defaultTable = CreateThemingTable();
                host = new StackPanel { RequestedTheme = ElementTheme.Light };
                host.Children.Add(defaultTable);
                host.Children.Add(bandedTable);
                LoadContent(host);
            });
            IdleSynchronizer.Wait();

            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Light })
            {
                RunOnUIThread.Execute(() => host.RequestedTheme = theme);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => host.UpdateLayout());
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var themeName = theme == ElementTheme.Dark ? "Default" : "Light";
                    var expected = RequireBrush(resources, themeName, "TabularSurfaceRowBackgroundAlternatingBrush").Color;
                    var oddRow = GetProjectedRow(bandedTable, 1);
                    Verify.IsNotNull(oddRow);
                    Verify.AreEqual(theme, oddRow.ActualTheme);
                    var band = oddRow.Background as SolidColorBrush;
                    Verify.IsNotNull(band);
                    Verify.AreEqual(expected, band.Color, "Every second row must paint the element theme's banding brush.");
                    Verify.IsNull(GetProjectedRow(bandedTable, 0).ReadLocalValue(Control.BackgroundProperty) as Brush,
                        "The named style must not add a base backplate to even rows.");
                    Verify.IsNull(defaultTable.AlternatingRowBackground, "The default table must remain unbanded.");
                    Verify.IsNull(GetProjectedRow(defaultTable, 1).ReadLocalValue(Control.BackgroundProperty) as Brush,
                        "The named style must not affect other table instances.");
                    Verify.IsNotNull(bandedTable.Template, "The named style must retain the default template.");
                    Verify.AreEqual(TableViewGridLinesVisibility.Vertical, bandedTable.GridLinesVisibility,
                        "Gridline visibility must remain independent.");
                    Verify.AreEqual((ushort)600, GetHeaderPresenter(bandedTable, 0).FontWeight.Weight,
                        "Banding must compose with the Strong header style.");
                });

                foreach (var state in new[] { "PointerOver", "Pressed", "Selected", "SelectedPointerOver", "SelectedPressed", "Disabled", "Normal" })
                {
                    RunOnUIThread.Execute(() =>
                        Verify.IsTrue(VisualStateManager.GoToState(GetProjectedRow(bandedTable, 1), state, false)));
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() =>
                    {
                        var themeName = theme == ElementTheme.Dark ? "Default" : "Light";
                        var key = state == "Normal"
                            ? "TabularSurfaceRowBackgroundAlternatingBrush"
                            : $"TabularSurfaceRowBackground{state}Brush";
                        var backplate = GetProjectedRow(bandedTable, 1).FindVisualChildByName("PART_RootBorder") as Border;
                        Verify.IsNotNull(backplate);
                        var fill = backplate.Background as SolidColorBrush;
                        Verify.IsNotNull(fill);
                        Verify.AreEqual(RequireBrush(resources, themeName, key).Color, fill.Color,
                            $"{theme}/{state}: interaction fills must override banding, and Normal must restore it.");
                    });
                }
            }

            RunOnUIThread.Execute(() => bandedTable.Style = null);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(bandedTable.AlternatingRowBackground, "Removing the style must restore the unbanded default.");
                Verify.IsNull(GetProjectedRow(bandedTable, 1).ReadLocalValue(Control.BackgroundProperty) as Brush,
                    "Removing the style must clear the realized row's banding.");
                Verify.IsNotNull(bandedTable.Template, "Removing the variant must retain the default template.");
            });
        }

        // The subtree runs opposite to the app theme so an Application-theme resolution is detectable.
        [TestMethod]
        public void VerifyAlternatingRowBackgroundFollowsElementThemeOppositeToApp()
        {
            const string appDictionaryXaml =
                @"<ResourceDictionary xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                                      xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
                    <ResourceDictionary.ThemeDictionaries>
                      <ResourceDictionary x:Key=""Light"">
                        <SolidColorBrush x:Key=""TvBandLiteral"" Color=""#FF112233""/>
                        <SolidColorBrush x:Key=""TvBandNestedThemeColor"" Color=""{ThemeResource SubtleFillColorSecondary}""/>
                      </ResourceDictionary>
                      <ResourceDictionary x:Key=""Dark"">
                        <SolidColorBrush x:Key=""TvBandLiteral"" Color=""#FF445566""/>
                        <SolidColorBrush x:Key=""TvBandNestedThemeColor"" Color=""{ThemeResource SubtleFillColorSecondary}""/>
                      </ResourceDictionary>
                    </ResourceDictionary.ThemeDictionaries>
                  </ResourceDictionary>";

            ResourceDictionary appDictionary = null;
            StackPanel root = null;
            ElementTheme elementTheme = ElementTheme.Default;
            ElementTheme appTheme = ElementTheme.Default;
            var cases = new[] { "Direct", "Setter", "Literal", "NestedThemeColor" };

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                appTheme = Application.Current.RequestedTheme == ApplicationTheme.Dark ? ElementTheme.Dark : ElementTheme.Light;
                elementTheme = appTheme == ElementTheme.Dark ? ElementTheme.Light : ElementTheme.Dark;
                Log.Comment($"Application theme {appTheme}; window-root theme {elementTheme}.");

                appDictionary = (ResourceDictionary)Microsoft.UI.Xaml.Markup.XamlReader.Load(appDictionaryXaml);
                Application.Current.Resources.MergedDictionaries.Add(appDictionary);

                root = (StackPanel)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    $@"<StackPanel xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                                  xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml""
                                  xmlns:tv=""using:Microsoft.UI.Xaml.Controls.Tabular""
                                  RequestedTheme=""{elementTheme}"">
                        <StackPanel.Resources>
                          <Style x:Key=""BandStyle"" TargetType=""tv:TableView"">
                            <Setter Property=""AlternatingRowBackground"" Value=""{{ThemeResource SubtleFillColorSecondaryBrush}}""/>
                          </Style>
                        </StackPanel.Resources>
                        <Border x:Name=""Reference"" Width=""10"" Height=""10"" Background=""{{ThemeResource SubtleFillColorSecondaryBrush}}""/>
                        <tv:TableView x:Name=""Direct"" Width=""300"" Height=""120"" AlternatingRowBackground=""{{ThemeResource SubtleFillColorSecondaryBrush}}""/>
                        <tv:TableView x:Name=""Setter"" Width=""300"" Height=""120"" Style=""{{StaticResource BandStyle}}""/>
                        <tv:TableView x:Name=""Literal"" Width=""300"" Height=""120"" AlternatingRowBackground=""{{ThemeResource TvBandLiteral}}""/>
                        <tv:TableView x:Name=""NestedThemeColor"" Width=""300"" Height=""120"" AlternatingRowBackground=""{{ThemeResource TvBandNestedThemeColor}}""/>
                      </StackPanel>");

                foreach (var name in cases)
                {
                    var tableView = (TableView)root.FindName(name);
                    tableView.Columns.Add(MakeTextColumn("Name"));
                    tableView.ItemsSource = MakeItems();
                }

                LoadContent(root);
            });

            try
            {
                foreach (var theme in new[] { elementTheme, appTheme })
                {
                    RunOnUIThread.Execute(() => root.RequestedTheme = theme);
                    IdleSynchronizer.Wait();
                    RunOnUIThread.Execute(() => root.UpdateLayout());
                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        var expected = ((SolidColorBrush)((Border)root.FindName("Reference")).Background).Color;
                        var expectedLiteral = theme == ElementTheme.Dark
                            ? Color.FromArgb(0xFF, 0x44, 0x55, 0x66)
                            : Color.FromArgb(0xFF, 0x11, 0x22, 0x33);

                        foreach (var name in cases)
                        {
                            var tableView = (TableView)root.FindName(name);
                            Verify.AreEqual(theme, tableView.ActualTheme, $"Precondition: {name} runs under {theme}.");
                            var oddRow = GetProjectedRow(tableView, 1);
                            var actual = (oddRow?.Background as SolidColorBrush)?.Color;
                            var want = name == "Literal" ? expectedLiteral : expected;
                            Log.Comment($"{theme} {name}: band {actual}, expected {want}.");
                            Verify.AreEqual(want, actual, $"Under {theme}, the {name} banding pattern must paint the {theme} color on odd rows.");
                            Verify.IsNull(GetProjectedRow(tableView, 0)?.ReadLocalValue(Control.BackgroundProperty) as Brush,
                                $"Even rows stay unbanded without RowBackground ({name}).");
                        }
                    });
                }
            }
            finally
            {
                RunOnUIThread.Execute(() => Application.Current.Resources.MergedDictionaries.Remove(appDictionary));
            }
        }

        #endregion
    }

    internal static class TableViewThemingTestHelpers
    {
        internal static Color ReadRenderedPixel((int Width, int Height, byte[] Pixels) image, int x, int y)
        {
            int offset = (y * image.Width + x) * 4;
            return Color.FromArgb(image.Pixels[offset + 3], image.Pixels[offset + 2],
                image.Pixels[offset + 1], image.Pixels[offset]);
        }

        internal static (int Width, int Height, byte[] Pixels) RenderElementPixels(FrameworkElement element)
        {
            RenderTargetBitmap bitmap = null;
            using var completed = new ManualResetEvent(false);
            var status = AsyncStatus.Started;
            RunOnUIThread.Execute(() =>
            {
                bitmap = new RenderTargetBitmap();
                // Oversample so a 1px rule contains fully covered pixels at fractional column widths.
                bitmap.RenderAsync(element, (int)Math.Ceiling(element.ActualWidth * 3),
                    (int)Math.Ceiling(element.ActualHeight * 3)).Completed = (operation, result) =>
                {
                    status = result;
                    completed.Set();
                };
            });
            Verify.IsTrue(completed.WaitOne(TimeSpan.FromSeconds(10)), "Rendering the header must complete.");
            Verify.AreEqual(AsyncStatus.Completed, status, "Rendering the header must succeed.");

            int width = 0;
            int height = 0;
            byte[] pixels = null;
            completed.Reset();
            RunOnUIThread.Execute(() =>
            {
                width = bitmap.PixelWidth;
                height = bitmap.PixelHeight;
                bitmap.GetPixelsAsync().Completed = (operation, result) =>
                {
                    status = result;
                    if (result == AsyncStatus.Completed)
                    {
                        pixels = operation.GetResults().ToArray();
                    }
                    completed.Set();
                };
            });
            Verify.IsTrue(completed.WaitOne(TimeSpan.FromSeconds(10)), "Reading the rendered pixels must complete.");
            Verify.AreEqual(AsyncStatus.Completed, status, "Reading the rendered pixels must succeed.");
            Verify.IsNotNull(pixels);
            Verify.IsTrue(width > 0 && height > 0 && pixels.Length == width * height * 4);
            return (width, height, pixels);
        }

        internal static Thickness GetAlignedContentPadding(FrameworkElement row, Thickness padding)
        {
            var chrome = (FrameworkElement)row.FindVisualChildByName("PART_RowChrome");
            var offset = chrome.TransformToVisual(row).TransformPoint(new Point()).Y
                + (chrome.ActualHeight - row.ActualHeight) / 2;
            return OffsetContentPadding(padding, offset);
        }

        internal static Thickness OffsetContentPadding(Thickness padding, double offset)
        {
            offset = Math.Max(-padding.Top, Math.Min(padding.Bottom, offset));
            return new Thickness(padding.Left, padding.Top + offset, padding.Right, padding.Bottom - offset);
        }

        internal struct DensityCase
        {
            internal TableViewDensity Density;
            internal double RowMinHeight;
            internal Thickness CellPadding;
            internal Thickness TextPadding;
            internal double FontSize;
            internal string ResourceSuffix => Density == TableViewDensity.Standard ? "" : Density.ToString();
        }

        // The literal presets from TableView_themeresources.xaml, not from the code constants. They
        // agree today; VerifyDensityResourceOverrideWins is what proves the resource is the real source.
        internal static readonly DensityCase[] DensityCases = new[]
        {
            new DensityCase { Density = TableViewDensity.Compact, RowMinHeight = 32.0, CellPadding = new Thickness(12, 6, 12, 6), TextPadding = new Thickness(12, 5, 12, 7), FontSize = 14.0 },
            new DensityCase { Density = TableViewDensity.Standard, RowMinHeight = 40.0, CellPadding = new Thickness(12, 10, 12, 10), TextPadding = new Thickness(12, 9, 12, 11), FontSize = 14.0 },
            new DensityCase { Density = TableViewDensity.ExtraCompact, RowMinHeight = 24.0, CellPadding = new Thickness(12, 2, 12, 2), TextPadding = new Thickness(12, 1, 12, 3), FontSize = 14.0 },
        };

        internal static TableView CreateThemingTable()
            => CreateTableView(headers: new[] { "Name", "Role" });

        internal static TableView CreateGroupedThemingTable()
        {
            var tableView = CreateThemingTable();
            tableView.Height = 400;
            tableView.ItemsSource = TableViewSource.From(MakeItems()).GroupBy(item => (object)((Person)item).Role);
            return tableView;
        }

        // Every key in a dictionary, including its own merged dictionaries. A key nested in a merged
        // dictionary resolves exactly like a top-level one, so parity has to account for both.
        internal static HashSet<string> CollectKeys(ResourceDictionary dictionary)
        {
            var keys = new HashSet<string>(StringComparer.Ordinal);
            CollectKeysCore(dictionary, keys);
            return keys;
        }

        private static void CollectKeysCore(ResourceDictionary dictionary, HashSet<string> keys)
        {
            foreach (var key in dictionary.Keys)
            {
                if (key is string name)
                {
                    keys.Add(name);
                }
            }

            foreach (var merged in dictionary.MergedDictionaries)
            {
                CollectKeysCore(merged, keys);
            }
        }

        // SystemColor* resources are framework-provided and do not appear when enumerating
        // Application.Current.Resources.Keys, so they have to be asked for by name. This is the full
        // set the high-contrast palette draws on.
        internal static readonly string[] SystemColorNames = new[]
        {
            "SystemColorWindowColor",
            "SystemColorWindowTextColor",
            "SystemColorButtonFaceColor",
            "SystemColorButtonTextColor",
            "SystemColorHighlightColor",
            "SystemColorHighlightTextColor",
            "SystemColorHotlightColor",
            "SystemColorGrayTextColor",
            "SystemColorCaptionTextColor",
            "SystemColorActiveCaptionColor",
            "SystemColorBackgroundColor",
        };

        internal static HashSet<Color> CollectSystemColors()
        {
            var colors = new HashSet<Color>();

            foreach (var name in SystemColorNames)
            {
                if (!Application.Current.Resources.ContainsKey(name))
                {
                    continue;
                }

                var value = Application.Current.Resources[name];
                if (value is Color color)
                {
                    colors.Add(color);
                }
                else if (value is SolidColorBrush brush)
                {
                    colors.Add(brush.Color);
                }

                var brushName = name + "Brush";
                if (Application.Current.Resources.ContainsKey(brushName) &&
                    Application.Current.Resources[brushName] is SolidColorBrush namedBrush)
                {
                    colors.Add(namedBrush.Color);
                }
            }

            return colors;
        }

        internal static SolidColorBrush RequireApplicationBrush(string key)
        {
            if (!Application.Current.Resources.ContainsKey(key) ||
                !(Application.Current.Resources[key] is SolidColorBrush brush))
            {
                Verify.Fail($"'{key}' must resolve to a SolidColorBrush from the application resources.");
                return null;
            }

            return brush;
        }

        internal static Color RequireApplicationColor(string key)
        {
            if (!Application.Current.Resources.ContainsKey(key) ||
                !(Application.Current.Resources[key] is Color color))
            {
                Verify.Fail($"'{key}' must resolve to a Color from the application resources.");
                return default;
            }

            return color;
        }

        internal static Color RequireThemeColor(string themeName, string key)
        {
            var pending = new Stack<ResourceDictionary>();
            pending.Push(Application.Current.Resources);
            while (pending.Count > 0)
            {
                var dictionary = pending.Pop();
                if (dictionary.ThemeDictionaries.TryGetValue(themeName, out var theme) &&
                    theme is ResourceDictionary themeDictionary &&
                    themeDictionary.TryGetValue(key, out var value) &&
                    value is Color color)
                {
                    return color;
                }

                foreach (var merged in dictionary.MergedDictionaries)
                {
                    pending.Push(merged);
                }
            }

            Verify.Fail($"'{key}' must resolve to a Color in an application '{themeName}' theme dictionary.");
            return default;
        }

        internal static SolidColorBrush RequireBrush(ResourceDictionary resources, string themeName, string key)
        {
            var themeDictionary = resources.ThemeDictionaries[themeName] as ResourceDictionary;
            if (themeDictionary == null)
            {
                Verify.Fail($"TabularControlsResources must declare a '{themeName}' theme dictionary.");
                return null;
            }

            if (!themeDictionary.ContainsKey(key))
            {
                Verify.Fail($"'{key}' must be defined in the '{themeName}' theme dictionary.");
                return null;
            }

            var brush = themeDictionary[key] as SolidColorBrush;
            if (brush == null)
            {
                Verify.Fail($"'{key}' in '{themeName}' must be a SolidColorBrush.");
                return null;
            }

            return brush;
        }

        // Written directly into Application.Current.Resources, which standard XAML precedence looks up
        // before its own theme dictionaries and merged dictionaries - so this is the app-scope override
        // an app author writes.
        internal static void SetApplicationResource(string key, object value)
        {
            Application.Current.Resources[key] = value;
        }

        internal static void ClearApplicationResource(string key)
        {
            if (Application.Current.Resources.ContainsKey(key))
            {
                Application.Current.Resources.Remove(key);
            }
        }

        // The TextBlock TableViewTextColumn generates, which is where cell padding and font size land.
        internal static TextBlock RequireFirstCellTextBlock(TableView tableView)
        {
            var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
            if (row == null)
            {
                Verify.Fail("The TableView should realize at least one row.");
                return null;
            }

            return GetCellTextBlock(GetRowCell(row, 0));
        }

        internal static Color RequireCellSeparatorColor(TableView tableView, string context)
        {
            var row = FindVisualChildrenByType<TableViewRow>(tableView).FirstOrDefault();
            if (row == null)
            {
                Verify.Fail($"The TableView should realize at least one row ({context}).");
                return default;
            }

            var wrapper = GetRowCell(row, 0);
            var brush = wrapper.BorderBrush as SolidColorBrush;
            if (brush == null)
            {
                Verify.Fail($"A cell wrapper should carry a SolidColorBrush vertical separator ({context}).");
                return default;
            }

            return brush.Color;
        }
    }
}
