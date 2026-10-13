// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Linq;
using System.Threading;
using Microsoft.UI.Input;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using Windows.Foundation;
using Windows.Graphics;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TitleBarTests : ApiTestBase
    {
        private const string HostWindowTitle = "TitleBar API test window";

        // Scenario: create a TitleBar in code without putting it in a window.
        // Expected: every public property has its documented default (empty Title/Subtitle, no icon/headers/content,
        //           back button hidden but enabled, pane toggle hidden, AutoRefreshDragRegions off, TemplateSettings
        //           present with no IconElement), and IsDragRegion is unset (null) on an arbitrary element.
        // A failure means: a default value changed, so existing apps that rely on the defaults would look or behave
        //                  differently.
        [TestMethod]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var titleBar = new TitleBar();

                Verify.AreEqual(string.Empty, titleBar.Title);
                Verify.AreEqual(string.Empty, titleBar.Subtitle);
                Verify.IsNull(titleBar.IconSource);
                Verify.IsNull(titleBar.LeftHeader);
                Verify.IsNull(titleBar.Content);
                Verify.IsNull(titleBar.RightHeader);
                Verify.IsFalse(titleBar.IsBackButtonVisible);
                Verify.IsTrue(titleBar.IsBackButtonEnabled);
                Verify.IsFalse(titleBar.IsPaneToggleButtonVisible);
                Verify.IsFalse(titleBar.AutoRefreshDragRegions);
                Verify.IsNotNull(titleBar.TemplateSettings);
                Verify.IsNull(titleBar.TemplateSettings.IconElement);
                Verify.IsNull(TitleBar.GetIsDragRegion(new Border()));
            });
        }

        // Scenario: ask UI Automation for the TitleBar's automation peer and read its control type, class name and
        //           name while changing Title and AutomationProperties.Name.
        // Expected: a TitleBarAutomationPeer with control type TitleBar and class name
        //           Microsoft.UI.Xaml.Controls.TitleBar; its name follows Title, an explicit AutomationProperties.Name
        //           wins over Title, and it is empty when both are cleared.
        // A failure means: screen readers would announce the TitleBar with the wrong role or name.
        [TestMethod]
        public void VerifyAutomationPeer()
        {
            RunOnUIThread.Execute(() =>
            {
                var titleBar = new TitleBar() { Title = "Peer title" };

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(titleBar);
                Verify.IsTrue(peer is TitleBarAutomationPeer, "TitleBar should create a TitleBarAutomationPeer");
                Verify.AreEqual(AutomationControlType.TitleBar, peer.GetAutomationControlType());
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TitleBar", peer.GetClassName());

                // With no explicit automation name, the peer falls back to the current Title.
                Verify.AreEqual("Peer title", peer.GetName());
                titleBar.Title = "Changed title";
                Verify.AreEqual("Changed title", peer.GetName());

                // An explicit AutomationProperties.Name wins over Title.
                AutomationProperties.SetName(titleBar, "Explicit name");
                Verify.AreEqual("Explicit name", peer.GetName());

                titleBar.ClearValue(AutomationProperties.NameProperty);
                titleBar.Title = string.Empty;
                Verify.AreEqual(string.Empty, peer.GetName());
            });
        }

        // Scenario: configure a TitleBar that is never added to a window, call RecomputeDragRegions(), and use
        //           IsDragRegion on an element outside any TitleBar.
        // Expected: the icon element is still built from IconSource, nothing throws, the main test window's title is
        //           untouched, and IsDragRegion just stores and returns its value.
        // A failure means: a TitleBar that is not hosted yet (e.g. created in code before being shown) crashes or
        //                  modifies an unrelated window.
        [TestMethod]
        public void VerifyTitleBarWithoutWindowHasNoWindowSideEffects()
        {
            RunOnUIThread.Execute(() =>
            {
                var mainAppWindow = MUXControlsTestApp.App.CurrentWindow.AppWindow;
                string mainWindowTitle = mainAppWindow.Title;

                var titleBar = new TitleBar()
                {
                    Title = "Unhosted title",
                    IsBackButtonVisible = true,
                    IconSource = new SymbolIconSource() { Symbol = Symbol.Home },
                };

                // Without a template or XamlRoot the TitleBar still materializes its icon element,
                // but must not touch any window and must tolerate a manual drag-region refresh.
                titleBar.RecomputeDragRegions();

                var iconElement = titleBar.TemplateSettings.IconElement as SymbolIcon;
                Verify.IsNotNull(iconElement, "IconElement should be a SymbolIcon created from the SymbolIconSource");
                Verify.AreEqual(Symbol.Home, iconElement.Symbol);
                Verify.AreEqual(mainWindowTitle, mainAppWindow.Title, "An unhosted TitleBar must not change any window title");

                titleBar.IconSource = null;
                Verify.IsNull(titleBar.TemplateSettings.IconElement);

                // IsDragRegion on an element outside any TitleBar round-trips and is otherwise inert.
                var looseElement = new Border();
                TitleBar.SetIsDragRegion(looseElement, false);
                Verify.AreEqual((bool?)false, TitleBar.GetIsDragRegion(looseElement));
                TitleBar.SetIsDragRegion(looseElement, null);
                Verify.IsNull(TitleBar.GetIsDragRegion(looseElement));
            });
        }

        // Scenario: host a TitleBar in a window, then set and clear Title and Subtitle.
        // Expected: the title/subtitle text parts are collapsed when empty, become visible with the right text when
        //           set, and collapse again when cleared (the TitleText*/SubtitleText* visual states follow).
        // A failure means: the title or subtitle text is shown when empty, hidden when set, or shows stale text.
        [TestMethod]
        public void VerifyTitleAndSubtitleVisibility()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()))
            {
                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    Verify.AreEqual("TitleTextCollapsed", GetCurrentState(titleBar, "TitleTextGroup"));
                    Verify.AreEqual("SubtitleTextCollapsed", GetCurrentState(titleBar, "SubtitleTextGroup"));

                    titleBar.Title = "Title text";
                    titleBar.Subtitle = "Subtitle text";

                    Verify.AreEqual("TitleTextVisible", GetCurrentState(titleBar, "TitleTextGroup"));
                    Verify.AreEqual("SubtitleTextVisible", GetCurrentState(titleBar, "SubtitleTextGroup"));
                    var titleText = (TextBlock)GetTemplatePart(titleBar, "PART_TitleText");
                    var subtitleText = (TextBlock)GetTemplatePart(titleBar, "PART_SubtitleText");
                    Verify.AreEqual(Visibility.Visible, titleText.Visibility);
                    Verify.AreEqual("Title text", titleText.Text);
                    Verify.AreEqual(Visibility.Visible, subtitleText.Visibility);
                    Verify.AreEqual("Subtitle text", subtitleText.Text);

                    titleBar.Title = string.Empty;
                    titleBar.Subtitle = string.Empty;

                    Verify.AreEqual("TitleTextCollapsed", GetCurrentState(titleBar, "TitleTextGroup"));
                    Verify.AreEqual("SubtitleTextCollapsed", GetCurrentState(titleBar, "SubtitleTextGroup"));
                    Verify.AreEqual(Visibility.Collapsed, titleText.Visibility);
                    Verify.AreEqual(Visibility.Collapsed, subtitleText.Visibility);
                });
            }
        }

        // Scenario: host a TitleBar whose Title is set before load, change Title, clear it, and repeat after the app
        //           changes the window title itself.
        // Expected: the window (AppWindow) title follows TitleBar.Title, and clearing Title restores the window title
        //           that was there before the TitleBar replaced it.
        // A failure means: the taskbar/Alt+Tab title is not synced with the TitleBar, or the original window title is
        //                  lost after clearing Title.
        [TestMethod]
        public void VerifyTitleUpdatesWindowTitleAndRestoresDefault()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar() { Title = "Initial title" }))
            {
                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    var appWindow = host.Window.AppWindow;

                    // A Title set before the template is applied is pushed to the window when loaded.
                    Verify.AreEqual("Initial title", appWindow.Title);

                    titleBar.Title = "Updated title";
                    Verify.AreEqual("Updated title", appWindow.Title);

                    // Clearing Title restores the window title the TitleBar originally replaced.
                    titleBar.Title = string.Empty;
                    Verify.AreEqual(HostWindowTitle, appWindow.Title);

                    // After a reset, the next Title replaces (and later restores) whatever the window title is then.
                    appWindow.Title = "App title";
                    titleBar.Title = "Second title";
                    Verify.AreEqual("Second title", appWindow.Title);
                    titleBar.Title = string.Empty;
                    Verify.AreEqual("App title", appWindow.Title);
                });
            }
        }

        // Scenario: TitleBar.Title sets the window title, then the app sets AppWindow.Title directly, then
        //           TitleBar.Title is cleared.
        // Expected: the app's own window title is kept (the TitleBar does not overwrite a title it no longer owns) and
        //           the title text collapses.
        // A failure means: clearing TitleBar.Title clobbers a window title the app set on purpose.
        [TestMethod]
        public void VerifyClearingTitleKeepsExternallySetWindowTitle()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()))
            {
                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    var appWindow = host.Window.AppWindow;

                    titleBar.Title = "TitleBar title";
                    Verify.AreEqual("TitleBar title", appWindow.Title);

                    // The app takes ownership of the window title; clearing TitleBar.Title must not overwrite it.
                    appWindow.Title = "App owned title";
                    titleBar.Title = string.Empty;
                    Verify.AreEqual("App owned title", appWindow.Title);
                    Verify.AreEqual("TitleTextCollapsed", GetCurrentState(titleBar, "TitleTextGroup"));
                });
            }
        }

        // Scenario: toggle IsBackButtonVisible and IsPaneToggleButtonVisible through all four combinations, then
        //           disable the back button.
        // Expected: the buttons are not created until first shown; each appears/collapses with its visual state; the
        //           left-header spacing switches between the default and negative-inset values the template defines;
        //           IsBackButtonEnabled=false disables the back button.
        // A failure means: the buttons show at the wrong time, the spacing next to them is wrong, or a disabled back
        //                  button can still be clicked.
        [TestMethod]
        public void VerifyBackAndPaneToggleButtonVisibilityAndSpacing()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()))
            {
                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    double defaultSpacing = GetDoubleResource("TitleBarLeftHeaderPaddingWidth");
                    double negativeInsetSpacing = GetDoubleResource("TitleBarHeaderNegativeInsetPaddingWidth");
                    Verify.AreNotEqual(defaultSpacing, negativeInsetSpacing);

                    // The buttons are deferred and must not be realized until they are first shown.
                    Verify.IsNull(titleBar.FindVisualChildByName("PART_BackButton"));
                    Verify.IsNull(titleBar.FindVisualChildByName("PART_PaneToggleButton"));
                    VerifyLeftHeaderSpacing(titleBar, "DefaultSpacing", defaultSpacing);

                    titleBar.IsBackButtonVisible = true;
                    var backButton = (Button)GetTemplatePart(titleBar, "PART_BackButton");
                    Verify.AreEqual(Visibility.Visible, backButton.Visibility);
                    Verify.AreEqual("BackButtonVisible", GetCurrentState(titleBar, "BackButtonGroup"));
                    VerifyLeftHeaderSpacing(titleBar, "NegativeInsetSpacing", negativeInsetSpacing);

                    titleBar.IsPaneToggleButtonVisible = true;
                    var paneToggleButton = (Button)GetTemplatePart(titleBar, "PART_PaneToggleButton");
                    Verify.AreEqual(Visibility.Visible, paneToggleButton.Visibility);
                    Verify.AreEqual("PaneToggleButtonVisible", GetCurrentState(titleBar, "PaneToggleButtonGroup"));
                    VerifyLeftHeaderSpacing(titleBar, "DefaultSpacing", defaultSpacing);

                    titleBar.IsBackButtonVisible = false;
                    Verify.AreEqual(Visibility.Collapsed, backButton.Visibility);
                    Verify.AreEqual("BackButtonCollapsed", GetCurrentState(titleBar, "BackButtonGroup"));
                    VerifyLeftHeaderSpacing(titleBar, "NegativeInsetSpacing", negativeInsetSpacing);

                    titleBar.IsPaneToggleButtonVisible = false;
                    Verify.AreEqual(Visibility.Collapsed, paneToggleButton.Visibility);
                    Verify.AreEqual("PaneToggleButtonCollapsed", GetCurrentState(titleBar, "PaneToggleButtonGroup"));
                    VerifyLeftHeaderSpacing(titleBar, "DefaultSpacing", defaultSpacing);

                    titleBar.IsBackButtonEnabled = false;
                    titleBar.IsBackButtonVisible = true;
                    Verify.IsFalse(backButton.IsEnabled, "IsBackButtonEnabled=false should disable the back button");
                    titleBar.IsBackButtonEnabled = true;
                    Verify.IsTrue(backButton.IsEnabled);
                });
            }
        }

        // Scenario: show both buttons and read their automation names and tooltips.
        // Expected: both buttons have non-empty, different automation names and tooltips, and the pane toggle tooltip
        //           matches its name. Text is not compared to English because it is localized.
        // A failure means: the buttons are unnamed or ambiguous for screen-reader and keyboard users.
        [TestMethod]
        public void VerifyBackAndPaneToggleButtonAccessibility()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar() { IsBackButtonVisible = true, IsPaneToggleButtonVisible = true }))
            {
                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    var backButton = (Button)GetTemplatePart(titleBar, "PART_BackButton");
                    var paneToggleButton = (Button)GetTemplatePart(titleBar, "PART_PaneToggleButton");

                    // Names and tooltips are localized; verify they are populated rather than matching English text.
                    string backName = AutomationProperties.GetName(backButton);
                    Verify.IsFalse(string.IsNullOrEmpty(backName), "Back button should have an automation name");
                    var backToolTip = ToolTipService.GetToolTip(backButton) as ToolTip;
                    Verify.IsNotNull(backToolTip, "Back button should have a ToolTip");
                    Verify.IsFalse(string.IsNullOrEmpty(backToolTip.Content as string), "Back button ToolTip should have text");

                    string paneToggleName = AutomationProperties.GetName(paneToggleButton);
                    Verify.IsFalse(string.IsNullOrEmpty(paneToggleName), "Pane toggle button should have an automation name");
                    Verify.AreNotEqual(backName, paneToggleName);
                    var paneToggleToolTip = ToolTipService.GetToolTip(paneToggleButton) as ToolTip;
                    Verify.IsNotNull(paneToggleToolTip, "Pane toggle button should have a ToolTip");
                    Verify.AreEqual(paneToggleName, paneToggleToolTip.Content as string);
                });
            }
        }

        // Scenario: invoke the back and pane-toggle buttons through UI Automation, then remove the handlers and invoke
        //           them again.
        // Expected: each button raises only its own event, exactly once, with the TitleBar as sender (BackRequested
        //           args are null); removed handlers are not called again.
        // A failure means: apps are not told (or are told twice or wrongly) about back/pane-toggle presses, or handler
        //                  removal does not work.
        [TestMethod]
        public void VerifyBackRequestedAndPaneToggleRequestedEvents()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar() { IsBackButtonVisible = true, IsPaneToggleButtonVisible = true }))
            {
                int backRequestedCount = 0;
                int paneToggleRequestedCount = 0;
                object backSender = null;
                object backArgs = "unset";
                TypedEventHandler<TitleBar, object> backHandler = (sender, args) =>
                {
                    backRequestedCount++;
                    backSender = sender;
                    backArgs = args;
                };
                TypedEventHandler<TitleBar, object> paneToggleHandler = (sender, args) => paneToggleRequestedCount++;

                RunOnUIThread.Execute(() =>
                {
                    host.TitleBar.BackRequested += backHandler;
                    host.TitleBar.PaneToggleRequested += paneToggleHandler;
                    Invoke((Button)GetTemplatePart(host.TitleBar, "PART_BackButton"));
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(1, backRequestedCount);
                    Verify.AreEqual(0, paneToggleRequestedCount, "Back button must not raise PaneToggleRequested");
                    Verify.AreSame(host.TitleBar, backSender);
                    Verify.IsNull(backArgs);

                    Invoke((Button)GetTemplatePart(host.TitleBar, "PART_PaneToggleButton"));
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(1, paneToggleRequestedCount);
                    Verify.AreEqual(1, backRequestedCount, "Pane toggle button must not raise BackRequested");

                    // Removed handlers are no longer called.
                    host.TitleBar.BackRequested -= backHandler;
                    host.TitleBar.PaneToggleRequested -= paneToggleHandler;
                    Invoke((Button)GetTemplatePart(host.TitleBar, "PART_BackButton"));
                    Invoke((Button)GetTemplatePart(host.TitleBar, "PART_PaneToggleButton"));
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(1, backRequestedCount);
                    Verify.AreEqual(1, paneToggleRequestedCount);
                });
            }
        }

        // Scenario: set IconSource on a hosted TitleBar, replace it, then clear it.
        // Expected: TemplateSettings.IconElement is created from (and follows) IconSource, the icon becomes visible,
        //           the window's Icon non-client region matches the icon's bounds, and clearing IconSource hides the
        //           icon and removes the Icon region.
        // A failure means: the icon is missing or stale, or the system icon area (system-menu click target) is wrong
        //                  or left behind.
        [TestMethod]
        public void VerifyIconSourceUpdatesIconAndIconRegion()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual("IconCollapsed", GetCurrentState(host.TitleBar, "IconGroup"));
                    host.TitleBar.IconSource = new SymbolIconSource() { Symbol = Symbol.Home };

                    var iconElement = host.TitleBar.TemplateSettings.IconElement as SymbolIcon;
                    Verify.IsNotNull(iconElement);
                    Verify.AreEqual(Symbol.Home, iconElement.Symbol);
                    Verify.AreEqual("IconVisible", GetCurrentState(host.TitleBar, "IconGroup"));
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    var iconPart = GetTemplatePart(host.TitleBar, "PART_Icon");
                    Verify.AreEqual(Visibility.Visible, iconPart.Visibility);
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Icon), iconPart);

                    host.TitleBar.IconSource = new SymbolIconSource() { Symbol = Symbol.Mail };
                    var replacedIcon = host.TitleBar.TemplateSettings.IconElement as SymbolIcon;
                    Verify.IsNotNull(replacedIcon);
                    Verify.AreEqual(Symbol.Mail, replacedIcon.Symbol);

                    host.TitleBar.IconSource = null;
                    Verify.IsNull(host.TitleBar.TemplateSettings.IconElement);
                    Verify.AreEqual("IconCollapsed", GetCurrentState(host.TitleBar, "IconGroup"));
                    Verify.AreEqual(Visibility.Collapsed, iconPart.Visibility);
                    Verify.AreEqual(0, host.GetRegionRects(NonClientRegionKind.Icon).Length, "Clearing IconSource should clear the icon region");
                });
            }
        }

        // Scenario: add and remove LeftHeader, Content and RightHeader one at a time.
        // Expected: each presenter and its visual state follow its property; the TitleBar uses the compact height when
        //           all three are empty and the expanded height while any of them is set.
        // A failure means: header/content slots are shown when empty or hidden when set, or the TitleBar has the wrong
        //                  height.
        [TestMethod]
        public void VerifyHeaderAndContentPresentersAndHeight()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()))
            {
                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    var layoutRoot = GetTemplatePart(titleBar, "PART_LayoutRoot");
                    double compactHeight = GetDoubleResource("TitleBarCompactHeight");
                    double expandedHeight = GetDoubleResource("TitleBarExpandedHeight");
                    Verify.AreNotEqual(compactHeight, expandedHeight);

                    Verify.AreEqual(compactHeight, layoutRoot.Height);
                    Verify.AreEqual("CompactHeight", GetCurrentState(titleBar, "HeightGroup"));

                    titleBar.LeftHeader = new Button() { Content = "Left" };
                    Verify.AreEqual("LeftHeaderVisible", GetCurrentState(titleBar, "LeftHeaderGroup"));
                    Verify.AreEqual(Visibility.Visible, GetTemplatePart(titleBar, "PART_LeftHeaderPresenter").Visibility);
                    Verify.AreEqual(expandedHeight, layoutRoot.Height);

                    titleBar.Content = new Button() { Content = "Center" };
                    Verify.AreEqual("ContentVisible", GetCurrentState(titleBar, "ContentGroup"));
                    Verify.AreEqual(Visibility.Visible, GetTemplatePart(titleBar, "PART_ContentPresenterGrid").Visibility);

                    titleBar.RightHeader = new Button() { Content = "Right" };
                    Verify.AreEqual("RightHeaderVisible", GetCurrentState(titleBar, "RightHeaderGroup"));
                    Verify.AreEqual(Visibility.Visible, GetTemplatePart(titleBar, "PART_RightHeaderPresenter").Visibility);

                    // Height stays expanded until the last of the three slots is cleared.
                    titleBar.LeftHeader = null;
                    Verify.AreEqual("LeftHeaderCollapsed", GetCurrentState(titleBar, "LeftHeaderGroup"));
                    Verify.AreEqual(Visibility.Collapsed, GetTemplatePart(titleBar, "PART_LeftHeaderPresenter").Visibility);
                    titleBar.Content = null;
                    Verify.AreEqual("ContentCollapsed", GetCurrentState(titleBar, "ContentGroup"));
                    Verify.AreEqual(Visibility.Collapsed, GetTemplatePart(titleBar, "PART_ContentPresenterGrid").Visibility);
                    Verify.AreEqual(expandedHeight, layoutRoot.Height);
                    Verify.AreEqual("ExpandedHeight", GetCurrentState(titleBar, "HeightGroup"));

                    titleBar.RightHeader = null;
                    Verify.AreEqual("RightHeaderCollapsed", GetCurrentState(titleBar, "RightHeaderGroup"));
                    Verify.AreEqual(Visibility.Collapsed, GetTemplatePart(titleBar, "PART_RightHeaderPresenter").Visibility);
                    Verify.AreEqual(compactHeight, layoutRoot.Height);
                    Verify.AreEqual("CompactHeight", GetCurrentState(titleBar, "HeightGroup"));
                });
            }
        }

        // Scenario: a narrow TitleBar whose Content is wider than the available space; then the TitleBar is widened;
        //           then it is made narrow again without Content.
        // Expected: it starts in the Compact display mode (title and subtitle hidden to make room), switches back to
        //           Expanded with title/subtitle visible once there is enough space, and stays Expanded without
        //           Content.
        // A failure means: title/subtitle are not hidden when space runs out, or never come back when space is
        //                  available.
        [TestMethod]
        public void VerifyCompactModeWhenContentDoesNotFit()
        {
            TitleBar titleBar = null;
            Func<TitleBar> createTitleBar = () => titleBar = new TitleBar()
            {
                Title = "Compact title",
                Subtitle = "Compact subtitle",
                HorizontalAlignment = HorizontalAlignment.Left,
                Width = 300,
            };

            using (var host = new TitleBarWindowHost(createTitleBar, () => new Border() { Width = 600, Height = 20 }))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual("Compact", GetCurrentState(titleBar, "DisplayModeGroup"));
                    Verify.AreEqual(Visibility.Collapsed, GetTemplatePart(titleBar, "PART_TitleText").Visibility);
                    Verify.AreEqual(Visibility.Collapsed, GetTemplatePart(titleBar, "PART_SubtitleText").Visibility);

                    titleBar.Width = 1200;
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual("Expanded", GetCurrentState(titleBar, "DisplayModeGroup"));
                    Verify.AreEqual("TitleTextVisible", GetCurrentState(titleBar, "TitleTextGroup"));
                    Verify.AreEqual("SubtitleTextVisible", GetCurrentState(titleBar, "SubtitleTextGroup"));
                    Verify.AreEqual(Visibility.Visible, GetTemplatePart(titleBar, "PART_TitleText").Visibility);
                    Verify.AreEqual(Visibility.Visible, GetTemplatePart(titleBar, "PART_SubtitleText").Visibility);

                    // Without Content there is nothing to make room for, so a narrow TitleBar stays expanded.
                    titleBar.Content = null;
                    titleBar.Width = 100;
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual("Expanded", GetCurrentState(titleBar, "DisplayModeGroup"));
                });
            }
        }

        // Scenario: read the window's caption-button insets and the TitleBar's left/right padding columns in
        //           left-to-right and right-to-left layout.
        // Expected: the padding columns equal the caption insets converted to effective pixels, and they swap sides in
        //           RTL. The validation VM runs at 100% scale; at other scales this test is expected to expose product
        //           bug PC-2.
        // A failure means: TitleBar content overlaps, or leaves a gap next to, the system caption buttons
        //                  (minimize/maximize/close).
        [TestMethod]
        public void VerifyPaddingColumnsMatchCaptionInsets()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()))
            {
                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    var appTitleBar = host.Window.AppWindow.TitleBar;

                    // Caption insets are reported in physical pixels; grid columns are measured in effective pixels.
                    // Note: the validation VM runs at RasterizationScale 1.0, so this only covers scale 1. At other
                    // scales this oracle is expected to expose PC-2 (UpdatePadding does not convert the insets).
                    double scale = titleBar.XamlRoot.RasterizationScale;
                    double leftInset = appTitleBar.LeftInset / scale;
                    double rightInset = appTitleBar.RightInset / scale;
                    Log.Comment($"Caption insets: left={leftInset} right={rightInset} scale={scale}");
                    Verify.AreNotEqual(leftInset, rightInset, "Insets must differ for the RTL swap to be observable");

                    var layoutRoot = GetTemplatePart(titleBar, "PART_LayoutRoot");
                    var leftColumn = (ColumnDefinition)layoutRoot.FindName("LeftPaddingColumn");
                    var rightColumn = (ColumnDefinition)layoutRoot.FindName("RightPaddingColumn");
                    Verify.IsNotNull(leftColumn);
                    Verify.IsNotNull(rightColumn);

                    VerifyColumnWidth(leftInset, leftColumn);
                    VerifyColumnWidth(rightInset, rightColumn);

                    titleBar.FlowDirection = FlowDirection.RightToLeft;
                    VerifyColumnWidth(rightInset, leftColumn);
                    VerifyColumnWidth(leftInset, rightColumn);

                    titleBar.FlowDirection = FlowDirection.LeftToRight;
                    VerifyColumnWidth(leftInset, leftColumn);
                    VerifyColumnWidth(rightInset, rightColumn);
                });
            }
        }

        private static void VerifyColumnWidth(double expectedWidth, ColumnDefinition column)
        {
            Verify.IsTrue(Math.Abs(expectedWidth - column.Width.Value) < 0.5,
                $"Expected column width {expectedWidth} but was {column.Width.Value}");
        }

        // Scenario: a TitleBar with back button, pane toggle, left header and right header; then the back button is
        //           disabled and re-enabled.
        // Expected: each interactive element gets a passthrough (clickable) non-client region that exactly matches its
        //           bounds, and a disabled back button stops being a passthrough region.
        // A failure means: clicks on those elements would be treated as window-caption drags/double-clicks instead of
        //                  reaching the controls.
        [TestMethod]
        public void VerifyPassthroughRegionsForBuiltInElements()
        {
            TitleBar titleBar = null;
            Func<TitleBar> createTitleBar = () => titleBar = new TitleBar()
            {
                IsBackButtonVisible = true,
                IsPaneToggleButtonVisible = true,
                LeftHeader = new Button() { Content = "Left" },
                RightHeader = new Button() { Content = "Right" },
            };

            using (var host = new TitleBarWindowHost(createTitleBar))
            {
                RunOnUIThread.Execute(() =>
                {
                    var backButton = GetTemplatePart(titleBar, "PART_BackButton");
                    var paneToggleButton = GetTemplatePart(titleBar, "PART_PaneToggleButton");
                    var leftHeader = GetTemplatePart(titleBar, "PART_LeftHeaderPresenter");
                    var rightHeader = GetTemplatePart(titleBar, "PART_RightHeaderPresenter");

                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), backButton, paneToggleButton, leftHeader, rightHeader);

                    // A disabled back button is not interactive, so it must stop punching a passthrough hole.
                    titleBar.IsBackButtonEnabled = false;
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), paneToggleButton, leftHeader, rightHeader);

                    titleBar.IsBackButtonEnabled = true;
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), backButton, paneToggleButton, leftHeader, rightHeader);
                });
            }
        }

        // Scenario: show the back button and a right header at runtime (which makes the TitleBar taller), then remove
        //           them.
        // Expected: after layout the passthrough regions match the new elements, and removing every interactive
        //           element clears the passthrough region.
        // A failure means: elements added or removed at runtime leave missing or stale clickable areas in the title
        //                  bar.
        [TestMethod]
        public void VerifyPassthroughRegionsAfterRuntimeChanges()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(0, host.GetRegionRects(NonClientRegionKind.Passthrough).Length);
                    host.TitleBar.IsBackButtonVisible = true;
                    host.TitleBar.RightHeader = new Button() { Content = "Right" };
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    var backButton = GetTemplatePart(host.TitleBar, "PART_BackButton");
                    var rightHeader = GetTemplatePart(host.TitleBar, "PART_RightHeaderPresenter");
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), backButton, rightHeader);

                    // Removing every interactive element clears the passthrough region entirely.
                    host.TitleBar.IsBackButtonVisible = false;
                    host.TitleBar.RightHeader = null;
                    Verify.AreEqual(0, host.GetRegionRects(NonClientRegionKind.Passthrough).Length);
                });
            }
        }

        // Scenario: like VerifyPassthroughRegionsAfterRuntimeChanges, but a left header keeps the TitleBar at its
        //           expanded height, so showing the back button and right header does not resize the TitleBar.
        // Expected: the passthrough regions still match the newly shown elements after layout.
        // A failure means: product bug PC-1 (currently the case): regions are computed before layout and not
        //                  refreshed, so the new buttons are not clickable. Ignored until PC-1 is fixed.
        [TestMethod]
        [TestProperty("Ignore", "True")] // PC-1: passthrough regions are not refreshed after layout when the TitleBar size is unchanged (UpdateDragRegion runs before layout in OnPropertyChanged). Re-enable when fixed.
        public void VerifyPassthroughRegionsRefreshWhenTitleBarSizeIsUnchanged()
        {
            // LeftHeader keeps the TitleBar at its expanded height, so showing the back button or a right header
            // later does not resize the TitleBar. Their passthrough holes must still match their laid-out bounds,
            // otherwise the newly shown elements cannot be clicked.
            using (var host = new TitleBarWindowHost(() => new TitleBar() { LeftHeader = new Button() { Content = "Left" } }))
            {
                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), GetTemplatePart(host.TitleBar, "PART_LeftHeaderPresenter"));
                    host.TitleBar.IsBackButtonVisible = true;
                    host.TitleBar.RightHeader = new Button() { Content = "Right" };
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(
                        host.GetRegionRects(NonClientRegionKind.Passthrough),
                        GetTemplatePart(host.TitleBar, "PART_BackButton"),
                        GetTemplatePart(host.TitleBar, "PART_LeftHeaderPresenter"),
                        GetTemplatePart(host.TitleBar, "PART_RightHeaderPresenter"));
                });
            }
        }

        // Scenario: TitleBar Content with enabled, disabled, collapsed and non-hit-testable buttons, plus elements and
        //           panels with IsDragRegion set to true or false.
        // Expected: only enabled, visible, hit-testable controls outside drag regions, and elements explicitly marked
        //           IsDragRegion=false, become passthrough regions (including ones nested in a disabled control or a
        //           drag panel).
        // A failure means: the rules that decide what is clickable vs. draggable inside custom title-bar content are
        //                  broken.
        [TestMethod]
        public void VerifyContentInteractiveElementDetection()
        {
            Button enabledButton = null;
            Border clickableBorder = null;
            Border nestedClickableBorder = null;
            Border clickableInDisabledControl = null;

            Func<UIElement> createContent = () =>
            {
                enabledButton = new Button() { Content = "Enabled" };
                clickableBorder = new Border() { Width = 30, Height = 20 };
                TitleBar.SetIsDragRegion(clickableBorder, false);

                var dragButton = new Button() { Content = "Drag" };
                TitleBar.SetIsDragRegion(dragButton, true);

                var dragPanel = new StackPanel() { Orientation = Orientation.Horizontal };
                TitleBar.SetIsDragRegion(dragPanel, true);
                nestedClickableBorder = new Border() { Width = 30, Height = 20 };
                TitleBar.SetIsDragRegion(nestedClickableBorder, false);
                dragPanel.Children.Add(new Button() { Content = "InDragPanel" });
                dragPanel.Children.Add(nestedClickableBorder);

                // A disabled control is not itself interactive, but its subtree is still searched.
                clickableInDisabledControl = new Border() { Width = 30, Height = 20 };
                TitleBar.SetIsDragRegion(clickableInDisabledControl, false);
                var disabledControl = new ContentControl() { IsEnabled = false, Content = clickableInDisabledControl };

                var panel = new StackPanel() { Orientation = Orientation.Horizontal };
                panel.Children.Add(enabledButton);
                panel.Children.Add(new Button() { Content = "Disabled", IsEnabled = false });
                panel.Children.Add(clickableBorder);
                panel.Children.Add(dragButton);
                panel.Children.Add(dragPanel);
                panel.Children.Add(disabledControl);
                panel.Children.Add(new Button() { Content = "Collapsed", Visibility = Visibility.Collapsed });
                panel.Children.Add(new Button() { Content = "NotHitTestable", IsHitTestVisible = false });
                return panel;
            };

            using (var host = new TitleBarWindowHost(() => new TitleBar(), createContent))
            {
                RunOnUIThread.Execute(() =>
                {
                    // Only enabled, visible, hit-testable controls outside drag regions and elements explicitly
                    // marked IsDragRegion=false become passthrough.
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), enabledButton, clickableBorder, nestedClickableBorder, clickableInDisabledControl);
                });
            }
        }

        // Scenario: flip TitleBar.IsDragRegion on a TextBlock and a Button in the Content at runtime, then clear it.
        // Expected: the passthrough regions update immediately: a TextBlock marked false becomes clickable, a Button
        //           marked true becomes draggable, and clearing the value restores the defaults.
        // A failure means: changing IsDragRegion after load has no effect until something else forces a refresh.
        [TestMethod]
        public void VerifyIsDragRegionChangesAtRuntime()
        {
            TextBlock label = null;
            Button button = null;
            Func<UIElement> createContent = () =>
            {
                label = new TextBlock() { Text = "Label" };
                button = new Button() { Content = "Button" };
                var panel = new StackPanel() { Orientation = Orientation.Horizontal };
                panel.Children.Add(label);
                panel.Children.Add(button);
                return panel;
            };

            using (var host = new TitleBarWindowHost(() => new TitleBar(), createContent))
            {
                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), button);

                    TitleBar.SetIsDragRegion(label, false);
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), label, button);

                    TitleBar.SetIsDragRegion(button, true);
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), label);

                    TitleBar.SetIsDragRegion(label, null);
                    TitleBar.SetIsDragRegion(button, null);
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), button);
                });
            }
        }

        // Scenario: add a button to Content after load with AutoRefreshDragRegions off, call RecomputeDragRegions(),
        //           then turn AutoRefreshDragRegions on and keep changing the content.
        // Expected: whatever is registered before the manual refresh is still correct, RecomputeDragRegions()
        //           registers the new button exactly, and with auto-refresh on later content changes are picked up
        //           after layout.
        // A failure means: apps cannot rely on RecomputeDragRegions() or AutoRefreshDragRegions to keep clickable
        //                  areas in sync with dynamic content.
        [TestMethod]
        public void VerifyRecomputeAndAutoRefreshDragRegions()
        {
            StackPanel panel = null;
            Button first = null;
            Func<UIElement> createContent = () =>
            {
                first = new Button() { Content = "First" };
                // Fixed width so adding buttons later does not move 'first' inside the centered content area.
                panel = new StackPanel() { Orientation = Orientation.Horizontal, Width = 400 };
                panel.Children.Add(first);
                return panel;
            };

            using (var host = new TitleBarWindowHost(() => new TitleBar(), createContent))
            {
                Button second = null;
                RectInt32[] initialRects = null;
                RunOnUIThread.Execute(() =>
                {
                    initialRects = host.GetRegionRects(NonClientRegionKind.Passthrough);
                    VerifyRegionRects(initialRects, first);

                    second = new Button() { Content = "Second" };
                    panel.Children.Add(second);
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    // AutoRefreshDragRegions defaults to false, so the TitleBar is not required to pick up content added
                    // after the first layout. Whatever it registers must still be correct: the hole for 'first' (which
                    // did not move) stays exact, every registered rect matches 'first' or 'second', and no degenerate
                    // rect is registered. Whether 'second' is already included is intentionally not asserted, so a
                    // product that refreshes eagerly is not penalized.
                    VerifyRegionRectsInclude(host.GetRegionRects(NonClientRegionKind.Passthrough), new FrameworkElement[] { first }, new FrameworkElement[] { second });

                    host.TitleBar.RecomputeDragRegions();
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), first, second);

                    host.TitleBar.AutoRefreshDragRegions = true;
                    panel.Children.Remove(first);
                });
                host.WaitForLayout();

                Button third = null;
                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), second);

                    third = new Button() { Content = "Third" };
                    panel.Children.Add(third);
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), second, third);
                });
            }
        }

        // Scenario: add a button to Content and call RecomputeDragRegions() in the same UI-thread step, before any
        //           layout pass.
        // Expected: the new button is registered with its real bounds, because RecomputeDragRegions() brings layout up
        //           to date first.
        // A failure means: calling RecomputeDragRegions() right after changing content registers zero-size or stale
        //                  regions.
        [TestMethod]
        public void VerifyRecomputeDragRegionsUsesUpToDateLayout()
        {
            StackPanel panel = null;
            Button first = null;
            Func<UIElement> createContent = () =>
            {
                first = new Button() { Content = "First" };
                // Fixed width so adding a button does not move 'first' inside the centered content area.
                panel = new StackPanel() { Orientation = Orientation.Horizontal, Width = 400 };
                panel.Children.Add(first);
                return panel;
            };

            using (var host = new TitleBarWindowHost(() => new TitleBar(), createContent))
            {
                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), first);

                    // No layout pass happens between adding the button and the call: RecomputeDragRegions must bring
                    // layout up to date itself so the new button gets its real bounds.
                    var second = new Button() { Content = "Second" };
                    panel.Children.Add(second);
                    host.TitleBar.RecomputeDragRegions();

                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), first, second);
                });
            }
        }

        // Scenario: widen a left-aligned TitleBar so its right header moves only horizontally (same count, Y and
        //           size).
        // Expected: the right header's passthrough region moves with it.
        // A failure means: the "regions unchanged" optimization ignores X-only moves and leaves the clickable area at
        //                  the old position.
        [TestMethod]
        public void VerifyPassthroughRegionFollowsHorizontalMove()
        {
            // Only the right header is interactive. Widening the TitleBar moves it along X while its count, Y and size
            // stay the same, so the registered hole must be updated rather than treated as unchanged.
            using (var host = new TitleBarWindowHost(() => new TitleBar()
            {
                HorizontalAlignment = HorizontalAlignment.Left,
                Width = 600,
                RightHeader = new Button() { Content = "Right" },
            }))
            {
                RectInt32 before = default;
                RunOnUIThread.Execute(() =>
                {
                    var rightHeader = GetTemplatePart(host.TitleBar, "PART_RightHeaderPresenter");
                    var rects = host.GetRegionRects(NonClientRegionKind.Passthrough);
                    VerifyRegionRects(rects, rightHeader);
                    before = rects[0];

                    host.TitleBar.Width = 800;
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    var rects = host.GetRegionRects(NonClientRegionKind.Passthrough);
                    VerifyRegionRects(rects, GetTemplatePart(host.TitleBar, "PART_RightHeaderPresenter"));

                    // Guard that the scenario really is an X-only move.
                    Verify.AreNotEqual(before.X, rects[0].X, "Right header should have moved horizontally");
                    Verify.AreEqual(before.Y, rects[0].Y);
                    Verify.AreEqual(before.Width, rects[0].Width);
                    Verify.AreEqual(before.Height, rects[0].Height);
                });
            }
        }

        // Scenario: read every public static DependencyProperty identifier on TitleBar and TitleBarTemplateSettings.
        // Expected: all identifiers are non-null and distinct, and repeated calls return the same identifier.
        // A failure means: an identifier is missing or points at the wrong property, so GetValue/SetValue, bindings,
        //                  styles and animations would target the wrong property.
        [TestMethod]
        public void VerifyDependencyPropertyIdentifiers()
        {
            RunOnUIThread.Execute(() =>
            {
                var properties = new DependencyProperty[]
                {
                    TitleBar.TitleProperty,
                    TitleBar.SubtitleProperty,
                    TitleBar.IconSourceProperty,
                    TitleBar.LeftHeaderProperty,
                    TitleBar.ContentProperty,
                    TitleBar.RightHeaderProperty,
                    TitleBar.IsBackButtonVisibleProperty,
                    TitleBar.IsBackButtonEnabledProperty,
                    TitleBar.IsPaneToggleButtonVisibleProperty,
                    TitleBar.TemplateSettingsProperty,
                    TitleBar.AutoRefreshDragRegionsProperty,
                    TitleBar.IsDragRegionProperty,
                    TitleBarTemplateSettings.IconElementProperty,
                };

                for (int i = 0; i < properties.Length; i++)
                {
                    Verify.IsNotNull(properties[i], "Dependency property identifier #" + i + " should not be null");
                    for (int j = 0; j < i; j++)
                    {
                        Verify.IsFalse(ReferenceEquals(properties[i], properties[j]), "Dependency property identifiers #" + j + " and #" + i + " should be distinct");
                    }
                }

                // The static getters must return the same identifier on every call.
                Verify.IsTrue(ReferenceEquals(TitleBar.TitleProperty, TitleBar.TitleProperty));
                Verify.IsTrue(ReferenceEquals(TitleBar.IsDragRegionProperty, TitleBar.IsDragRegionProperty));
            });
        }

        // Scenario: for each TitleBar property, SetValue/GetValue/ClearValue through its DependencyProperty identifier
        //           and compare with the CLR property; also IconSource, TemplateSettings and the attached
        //           IsDragRegion.
        // Expected: values set through the DP are visible through the CLR property and vice versa, ClearValue restores
        //           the documented default, setting IconSource through the DP still builds the icon element, and the
        //           TemplateSettings DP returns the same instance.
        // A failure means: XAML styles/bindings (which go through the DP) and code (which uses the CLR property) would
        //                  see different values.
        [TestMethod]
        public void VerifyDependencyPropertiesMatchClrProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var titleBar = new TitleBar();

                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.TitleProperty, () => titleBar.Title, string.Empty, "DP title");
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.SubtitleProperty, () => titleBar.Subtitle, string.Empty, "DP subtitle");
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.LeftHeaderProperty, () => titleBar.LeftHeader, null, new Button());
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.ContentProperty, () => titleBar.Content, null, new Button());
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.RightHeaderProperty, () => titleBar.RightHeader, null, new Button());
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.IsBackButtonVisibleProperty, () => titleBar.IsBackButtonVisible, false, true);
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.IsBackButtonEnabledProperty, () => titleBar.IsBackButtonEnabled, true, false);
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.IsPaneToggleButtonVisibleProperty, () => titleBar.IsPaneToggleButtonVisible, false, true);
                VerifyDependencyPropertyRoundTrip(titleBar, TitleBar.AutoRefreshDragRegionsProperty, () => titleBar.AutoRefreshDragRegions, false, true);

                // Setting IconSource through the DP must reach the TitleBar's change handler, which builds the icon element.
                var iconSource = new SymbolIconSource() { Symbol = Symbol.Home };
                Verify.IsNull(titleBar.GetValue(TitleBar.IconSourceProperty));
                titleBar.SetValue(TitleBar.IconSourceProperty, iconSource);
                Verify.IsTrue(ReferenceEquals(iconSource, titleBar.IconSource), "IconSource set through the DP should be returned by the CLR property");
                Verify.IsNotNull(titleBar.TemplateSettings.IconElement, "Setting IconSource through the DP should create the icon element");
                titleBar.ClearValue(TitleBar.IconSourceProperty);
                Verify.IsNull(titleBar.IconSource);
                Verify.IsNull(titleBar.TemplateSettings.IconElement);

                // TemplateSettings is read-only from the CLR surface; the DP returns the same instance.
                Verify.IsNotNull(titleBar.TemplateSettings);
                Verify.IsTrue(ReferenceEquals(titleBar.TemplateSettings, titleBar.GetValue(TitleBar.TemplateSettingsProperty)));

                // IsDragRegion is an attached nullable bool.
                var element = new Border();
                Verify.IsNull(element.GetValue(TitleBar.IsDragRegionProperty));
                element.SetValue(TitleBar.IsDragRegionProperty, true);
                Verify.AreEqual((bool?)true, TitleBar.GetIsDragRegion(element));
                TitleBar.SetIsDragRegion(element, false);
                Verify.AreEqual(false, (bool)element.GetValue(TitleBar.IsDragRegionProperty));
                element.ClearValue(TitleBar.IsDragRegionProperty);
                Verify.IsNull(TitleBar.GetIsDragRegion(element));
            });
        }

        // Scenario: look up TitleBar and TitleBarTemplateSettings in the controls XAML metadata provider and query
        //           each member by name.
        // Expected: TitleBar derives from Control, Content is its content property, and every public property
        //           (including IsDragRegion and IconElement) is a dependency-property member of the right type.
        // A failure means: XAML markup, bindings or tooling that rely on type metadata cannot resolve TitleBar
        //                  properties correctly.
        [TestMethod]
        public void VerifyXamlMetadata()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = new Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider();
                var titleBarType = provider.GetXamlType("Microsoft.UI.Xaml.Controls.TitleBar");
                Verify.IsNotNull(titleBarType, "TitleBar should be described by the controls metadata provider");
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.Control", titleBarType.BaseType.FullName);
                Verify.AreEqual("Content", titleBarType.ContentProperty.Name);

                var members = new (string Name, string TypeName)[]
                {
                    ("Title", "String"),
                    ("Subtitle", "String"),
                    ("IconSource", "Microsoft.UI.Xaml.Controls.IconSource"),
                    ("LeftHeader", "Microsoft.UI.Xaml.UIElement"),
                    ("Content", "Microsoft.UI.Xaml.UIElement"),
                    ("RightHeader", "Microsoft.UI.Xaml.UIElement"),
                    ("IsBackButtonVisible", "Boolean"),
                    ("IsBackButtonEnabled", "Boolean"),
                    ("IsPaneToggleButtonVisible", "Boolean"),
                    ("TemplateSettings", "Microsoft.UI.Xaml.Controls.TitleBarTemplateSettings"),
                    ("AutoRefreshDragRegions", "Boolean"),
                };

                foreach (var (name, typeName) in members)
                {
                    var member = titleBarType.GetMember(name);
                    Verify.IsNotNull(member, "TitleBar metadata should expose member " + name);
                    Verify.AreEqual(name, member.Name);
                    Verify.IsTrue(member.IsDependencyProperty, name + " should be a dependency property");
                    Verify.AreEqual(typeName, member.Type.FullName, name + " member type");
                }

                var isDragRegion = titleBarType.GetMember("IsDragRegion");
                Verify.IsNotNull(isDragRegion, "TitleBar metadata should expose IsDragRegion");
                Verify.IsTrue(isDragRegion.IsDependencyProperty);

                var templateSettingsType = provider.GetXamlType("Microsoft.UI.Xaml.Controls.TitleBarTemplateSettings");
                Verify.IsNotNull(templateSettingsType);
                var iconElement = templateSettingsType.GetMember("IconElement");
                Verify.IsNotNull(iconElement);
                Verify.IsTrue(iconElement.IsDependencyProperty);
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.IconElement", iconElement.Type.FullName);
            });
        }

        // Scenario: construct a TitleBarAutomationPeer directly from C#, as a derived control or custom peer would.
        // Expected: the peer's owner is the given TitleBar, its control type is TitleBar, its class name is
        //           Microsoft.UI.Xaml.Controls.TitleBar, and its name falls back to Title.
        // A failure means: the public TitleBarAutomationPeer constructor (activation factory) is broken for apps that
        //                  create or derive from the peer.
        [TestMethod]
        public void VerifyAutomationPeerCreatedDirectly()
        {
            RunOnUIThread.Execute(() =>
            {
                var titleBar = new TitleBar() { Title = "Direct peer title" };
                var peer = new TitleBarAutomationPeer(titleBar);

                Verify.IsTrue(ReferenceEquals(titleBar, peer.Owner), "Peer owner should be the TitleBar passed to the constructor");
                Verify.AreEqual(AutomationControlType.TitleBar, peer.GetAutomationControlType());
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TitleBar", peer.GetClassName());
                Verify.AreEqual("Direct peer title", peer.GetName());
            });
        }

        // Scenario: construct a TitleBarTemplateSettings directly and set/clear IconElement through the property and
        //           the DependencyProperty.
        // Expected: IconElement defaults to null, the property and DP stay in sync, and ClearValue resets it to null.
        // A failure means: the public TitleBarTemplateSettings constructor or its IconElement property is broken for
        //                  code and templates that use it.
        [TestMethod]
        public void VerifyTemplateSettingsCreatedDirectly()
        {
            RunOnUIThread.Execute(() =>
            {
                var templateSettings = new TitleBarTemplateSettings();
                Verify.IsNull(templateSettings.IconElement);
                Verify.IsNull(templateSettings.GetValue(TitleBarTemplateSettings.IconElementProperty));

                var icon = new SymbolIcon(Symbol.Home);
                templateSettings.IconElement = icon;
                Verify.IsTrue(ReferenceEquals(icon, templateSettings.IconElement));
                Verify.IsTrue(ReferenceEquals(icon, templateSettings.GetValue(TitleBarTemplateSettings.IconElementProperty)));

                var otherIcon = new SymbolIcon(Symbol.Mail);
                templateSettings.SetValue(TitleBarTemplateSettings.IconElementProperty, otherIcon);
                Verify.IsTrue(ReferenceEquals(otherIcon, templateSettings.IconElement));

                templateSettings.ClearValue(TitleBarTemplateSettings.IconElementProperty);
                Verify.IsNull(templateSettings.IconElement);
            });
        }

        // Scenario: start with AutoRefreshDragRegions on and add a button; turn it off at runtime and add another
        //           button; finally call RecomputeDragRegions().
        // Expected: with auto-refresh on the new button is registered after layout; after turning it off later content
        //           changes are not registered automatically; RecomputeDragRegions() then registers them.
        // A failure means: turning AutoRefreshDragRegions off does not stop the TitleBar from reacting to layout
        //                  changes.
        [TestMethod]
        public void VerifyAutoRefreshDragRegionsCanBeTurnedOff()
        {
            StackPanel panel = null;
            Button first = null;
            Func<UIElement> createContent = () =>
            {
                first = new Button() { Content = "First" };
                // Fixed width so adding buttons does not move earlier ones inside the centered content area.
                panel = new StackPanel() { Orientation = Orientation.Horizontal, Width = 400 };
                panel.Children.Add(first);
                return panel;
            };

            using (var host = new TitleBarWindowHost(() => new TitleBar() { AutoRefreshDragRegions = true }, createContent))
            {
                Button second = null;
                Button third = null;
                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), first);
                    second = new Button() { Content = "Second" };
                    panel.Children.Add(second);
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    // Auto-refresh is on: the new content is picked up after layout.
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), first, second);

                    // Opting out stops automatic refresh: later content changes are not registered...
                    host.TitleBar.AutoRefreshDragRegions = false;
                    third = new Button() { Content = "Third" };
                    panel.Children.Add(third);
                });
                host.WaitForLayout();

                RunOnUIThread.Execute(() =>
                {
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), first, second);

                    // ...until the app asks for a refresh.
                    host.TitleBar.RecomputeDragRegions();
                    VerifyRegionRects(host.GetRegionRects(NonClientRegionKind.Passthrough), first, second, third);
                });
            }
        }

        // Scenario: close the TitleBar's window, then clear/change Title and clear IconSource on the TitleBar that is
        //           still alive.
        // Expected: nothing throws; the TitleBar is detached (XamlRoot is null) and its own visual states still follow
        //           the properties.
        // A failure means: an app that updates a TitleBar after its window closed would crash (e.g. while restoring
        //                  the title of a window that no longer exists).
        [TestMethod]
        public void VerifyTitleBarChangesAfterWindowClosed()
        {
            using (var host = new TitleBarWindowHost(() => new TitleBar()
            {
                Title = "Applied title",
                IconSource = new SymbolIconSource() { Symbol = Symbol.Home },
            }))
            {
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual("Applied title", host.Window.AppWindow.Title);
                });
                host.CloseWindow();

                RunOnUIThread.Execute(() =>
                {
                    var titleBar = host.TitleBar;
                    Verify.IsNull(titleBar.XamlRoot, "Closing the window detaches the TitleBar from its XamlRoot");

                    // The window is gone: clearing or changing Title and IconSource must not throw, and the TitleBar's
                    // own state still follows the properties.
                    titleBar.Title = string.Empty;
                    Verify.AreEqual("TitleTextCollapsed", GetCurrentState(titleBar, "TitleTextGroup"));
                    titleBar.Title = "Title after close";
                    Verify.AreEqual("TitleTextVisible", GetCurrentState(titleBar, "TitleTextGroup"));

                    titleBar.IconSource = null;
                    Verify.IsNull(titleBar.TemplateSettings.IconElement);
                    Verify.AreEqual("IconCollapsed", GetCurrentState(titleBar, "IconGroup"));
                });
            }
        }

        private static void VerifyDependencyPropertyRoundTrip<T>(TitleBar titleBar, DependencyProperty property, Func<T> getClrValue, T defaultValue, T newValue)
        {
            Verify.AreEqual(defaultValue, getClrValue());
            Verify.AreEqual(defaultValue, (T)titleBar.GetValue(property));

            titleBar.SetValue(property, newValue);
            Verify.AreEqual(newValue, getClrValue(), "Value set through the DP should be visible through the CLR property");
            Verify.AreEqual(newValue, (T)titleBar.GetValue(property));

            titleBar.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue(), "ClearValue should restore the default");
        }

        private static void Invoke(Button button)
        {
            var peer = (ButtonAutomationPeer)FrameworkElementAutomationPeer.CreatePeerForElement(button);
            ((IInvokeProvider)peer).Invoke();
        }

        private static void VerifyLeftHeaderSpacing(TitleBar titleBar, string expectedState, double expectedWidth)
        {
            Verify.AreEqual(expectedState, GetCurrentState(titleBar, "LeftHeaderSpacingGroup"));
            var column = (ColumnDefinition)GetTemplatePart(titleBar, "PART_LayoutRoot").FindName("LeftHeaderPaddingColumn");
            Verify.IsNotNull(column);
            Verify.AreEqual(expectedWidth, column.Width.Value);
        }

        private static FrameworkElement GetTemplatePart(TitleBar titleBar, string name)
        {
            var part = titleBar.FindVisualChildByName(name);
            Verify.IsNotNull(part, $"Template part {name} should be realized");
            return part;
        }

        private static string GetCurrentState(TitleBar titleBar, string groupName)
        {
            var layoutRoot = (FrameworkElement)VisualTreeHelper.GetChild(titleBar, 0);
            var group = VisualStateManager.GetVisualStateGroups(layoutRoot).FirstOrDefault(g => g.Name == groupName);
            Verify.IsNotNull(group, $"Visual state group {groupName} should exist");
            return group.CurrentState?.Name;
        }

        private static double GetDoubleResource(string key)
        {
            Verify.IsTrue(Application.Current.Resources.TryGetValue(key, out object value), $"Resource {key} should exist");
            return (double)value;
        }

        // Physical-pixel window bounds of an element, which is what TitleBar registers with InputNonClientPointerSource.
        private static RectInt32 GetExpectedRegion(FrameworkElement element)
        {
            Verify.IsTrue(element.ActualWidth > 0 && element.ActualHeight > 0, $"{element.GetType().Name} '{element.Name}' should have a laid-out size");
            var bounds = element.TransformToVisual(null).TransformBounds(new Rect(0, 0, element.ActualWidth, element.ActualHeight));
            double scale = element.XamlRoot.RasterizationScale;
            return new RectInt32(
                (int)(bounds.X * scale),
                (int)(bounds.Y * scale),
                (int)(bounds.Width * scale),
                (int)(bounds.Height * scale));
        }

        private static void VerifyRegionRects(RectInt32[] actualRects, params FrameworkElement[] expectedElements)
        {
            Log.Comment("Actual region rects: " + string.Join(", ", actualRects.Select(r => $"({r.X},{r.Y},{r.Width},{r.Height})")));
            Verify.AreEqual(expectedElements.Length, actualRects.Length, "Region rect count");
            VerifyRegionRectsFor(actualRects, expectedElements);
        }

        // Every registered rect must be non-degenerate and match the region of one of the required or optional elements;
        // each required element must have a matching rect. Optional elements may or may not be registered.
        private static void VerifyRegionRectsInclude(RectInt32[] actualRects, FrameworkElement[] requiredElements, FrameworkElement[] optionalElements)
        {
            Log.Comment("Actual region rects: " + string.Join(", ", actualRects.Select(r => $"({r.X},{r.Y},{r.Width},{r.Height})")));
            Verify.IsFalse(actualRects.Any(r => r.Width <= 0 || r.Height <= 0), "No degenerate region rect should be registered");
            VerifyRegionRectsFor(actualRects, requiredElements);

            var allowed = requiredElements.Concat(optionalElements).Select(GetExpectedRegion).ToArray();
            foreach (var rect in actualRects)
            {
                Verify.IsTrue(allowed.Any(expected => RegionMatches(rect, expected)),
                    $"Region rect ({rect.X},{rect.Y},{rect.Width},{rect.Height}) does not match any expected element");
            }
        }

        private static bool RegionMatches(RectInt32 actual, RectInt32 expected)
        {
            return Math.Abs(actual.X - expected.X) <= 1 &&
                Math.Abs(actual.Y - expected.Y) <= 1 &&
                Math.Abs(actual.Width - expected.Width) <= 1 &&
                Math.Abs(actual.Height - expected.Height) <= 1;
        }

        private static void VerifyRegionRectsFor(RectInt32[] actualRects, FrameworkElement[] expectedElements)
        {
            foreach (var element in expectedElements)
            {
                var expected = GetExpectedRegion(element);
                bool found = actualRects.Any(r => RegionMatches(r, expected));
                Verify.IsTrue(found, $"Expected a region rect ({expected.X},{expected.Y},{expected.Width},{expected.Height}) for {element.GetType().Name} '{element.Name}'");
            }
        }

        // Hosts a TitleBar as the title bar of a dedicated, activated window that extends content into the title bar,
        // so title and non-client region changes never affect the shared test app window.
        private sealed class TitleBarWindowHost : IDisposable
        {
            public Window Window { get; private set; }
            public TitleBar TitleBar { get; private set; }

            public TitleBarWindowHost(Func<TitleBar> createTitleBar, Func<UIElement> createContent = null)
            {
                var loaded = new AutoResetEvent(false);

                RunOnUIThread.Execute(() =>
                {
                    Window = new Window() { Title = HostWindowTitle, ExtendsContentIntoTitleBar = true };
                });

                try
                {
                    ActivateAndWait();

                    RunOnUIThread.Execute(() =>
                    {
                        Verify.AreEqual(HostWindowTitle, Window.AppWindow.Title);

                        // XAML objects must be created on the UI thread.
                        TitleBar = createTitleBar();
                        if (createContent != null)
                        {
                            TitleBar.Content = createContent();
                        }

                        TitleBar.Loaded += (sender, args) => loaded.Set();
                        var root = new Grid();
                        root.RowDefinitions.Add(new RowDefinition() { Height = GridLength.Auto });
                        root.RowDefinitions.Add(new RowDefinition() { Height = new GridLength(1, GridUnitType.Star) });
                        root.Children.Add(TitleBar);
                        Window.Content = root;
                        Window.SetTitleBar(TitleBar);
                    });
                    Verify.IsTrue(loaded.WaitOne(DefaultWaitTimeInMS), "Waiting for the TitleBar to load");
                    WaitForLayout();
                }
                catch
                {
                    // The caller's using block never sees a host whose constructor threw, so close the window here.
                    Dispose();
                    throw;
                }
            }

            public void WaitForLayout()
            {
                RunOnUIThread.Execute(() => TitleBar.UpdateLayout());
                IdleSynchronizer.Wait();
            }

            public RectInt32[] GetRegionRects(NonClientRegionKind kind)
            {
                // InputNonClientPointerSource reports "no regions" as a null array.
                return InputNonClientPointerSource.GetForWindowId(Window.AppWindow.Id).GetRegionRects(kind) ?? Array.Empty<RectInt32>();
            }

            public void ActivateAndWait()
            {
                WaitForActivationChange(() => Window.Activate(), expectActive: true);
            }

            private void WaitForActivationChange(Action action, bool expectActive)
            {
                var changed = new AutoResetEvent(false);
                TypedEventHandler<object, WindowActivatedEventArgs> handler = (sender, args) =>
                {
                    if ((args.WindowActivationState != WindowActivationState.Deactivated) == expectActive)
                    {
                        changed.Set();
                    }
                };

                RunOnUIThread.Execute(() =>
                {
                    Window.Activated += handler;
                    action();
                });
                bool wasChanged = changed.WaitOne(DefaultWaitTimeInMS);
                RunOnUIThread.Execute(() => Window.Activated -= handler);
                Verify.IsTrue(wasChanged, expectActive ? "Waiting for the TitleBar host window to activate" : "Waiting for the TitleBar host window to deactivate");
            }

            // Closes the host window early; Dispose then only restores the main window.
            public void CloseWindow()
            {
                RunOnUIThread.Execute(() =>
                {
                    Window.Close();
                    windowClosed = true;
                });
                IdleSynchronizer.Wait();
            }

            private bool windowClosed;

            public void Dispose()
            {
                RunOnUIThread.Execute(() =>
                {
                    if (!windowClosed)
                    {
                        Window?.Close();
                    }
                    MUXControlsTestApp.App.CurrentWindow.Activate();
                });
                IdleSynchronizer.Wait();
            }
        }
    }
}
