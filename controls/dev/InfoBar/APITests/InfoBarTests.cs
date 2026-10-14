// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Common;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class InfoBarTests : ApiTestBase
    {
        private InfoBar CreateLoadedInfoBar(global::System.Action<InfoBar> configure = null)
        {
            InfoBar infoBar = null;
            RunOnUIThread.Execute(() =>
            {
                infoBar = new InfoBar { Title = "Title", Message = "Message", IsOpen = true };
                configure?.Invoke(infoBar);
                Content = infoBar;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            return infoBar;
        }

        // Scenario: Constructs a new InfoBar (not added to the tree) and reads every public property without setting any.
        // Expected: IsOpen=false, Title/Message empty, Severity=Informational, IconSource/ActionButton/Content null,
        //     IsIconVisible=true, IsClosable=true, and TemplateSettings is non-null.
        // Failure means: an InfoBar dependency property default (InfoBar.idl metadata) changed, or the InfoBar constructor
        //     no longer creates the InfoBarTemplateSettings instance, breaking apps that rely on the documented defaults.
        [TestMethod]
        public void VerifyDefaultProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var infoBar = new InfoBar();
                Verify.IsFalse(infoBar.IsOpen);
                Verify.AreEqual(string.Empty, infoBar.Title);
                Verify.AreEqual(string.Empty, infoBar.Message);
                Verify.AreEqual(InfoBarSeverity.Informational, infoBar.Severity);
                Verify.IsNull(infoBar.IconSource);
                Verify.IsTrue(infoBar.IsIconVisible);
                Verify.IsTrue(infoBar.IsClosable);
                Verify.IsNull(infoBar.ActionButton);
                Verify.IsNull(infoBar.Content);
                Verify.IsNotNull(infoBar.TemplateSettings);
            });
        }

        // Scenario: Loads an open InfoBar, then sets Severity to Informational, Success, Warning and Error in turn and reads
        //     AutomationProperties.Name from the "StandardIcon" template part after each change.
        // Expected: StandardIcon exists, has a non-empty automation name for every severity, and all four names differ.
        // Failure means: InfoBar::UpdateSeverity (or OnApplyTemplate) no longer sets the localized severity name on the
        //     standard icon via GetIconSeverityLevelResourceName, so screen readers announce a missing or wrong name.
        [TestMethod]
        public void VerifySeverityUpdatesStandardIconAutomationName()
        {
            var infoBar = CreateLoadedInfoBar();
            var names = new Dictionary<InfoBarSeverity, string>();

            foreach (var severity in new[] { InfoBarSeverity.Informational, InfoBarSeverity.Success, InfoBarSeverity.Warning, InfoBarSeverity.Error })
            {
                RunOnUIThread.Execute(() =>
                {
                    infoBar.Severity = severity;
                    Content.UpdateLayout();
                    var icon = VisualTreeUtils.FindVisualChildByName(infoBar, "StandardIcon") as FrameworkElement;
                    Verify.IsNotNull(icon);
                    var name = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(icon);
                    Verify.IsFalse(string.IsNullOrEmpty(name), "Standard icon needs an automation name for " + severity);
                    names[severity] = name;
                    Verify.AreEqual(severity, infoBar.Severity);
                });
                IdleSynchronizer.Wait();
            }

            Verify.AreEqual(4, new HashSet<string>(names.Values).Count, "Each severity should have a distinct icon automation name.");
        }

        // Scenario: Loads an open InfoBar, then sets Severity to Error, Warning, Success and Informational in turn and reads
        //     the current state of the "SeverityLevels" VisualStateGroup on the template root.
        // Expected: After each change the SeverityLevels current state name equals the severity enum name.
        // Failure means: InfoBar::UpdateSeverity no longer drives the SeverityLevels visual states (or the template renamed
        //     them), so e.g. a Warning InfoBar would render with Informational background and icon styling.
        [TestMethod]
        public void VerifySeverityChangeTransitionsVisualState()
        {
            var infoBar = CreateLoadedInfoBar();

            foreach (var severity in new[] { InfoBarSeverity.Error, InfoBarSeverity.Warning, InfoBarSeverity.Success, InfoBarSeverity.Informational })
            {
                RunOnUIThread.Execute(() =>
                {
                    infoBar.Severity = severity;
                    Content.UpdateLayout();

                    var root = VisualTreeHelper.GetChild(infoBar, 0) as FrameworkElement;
                    Verify.IsNotNull(root);
                    string current = null;
                    foreach (var group in VisualStateManager.GetVisualStateGroups(root))
                    {
                        if (group.Name == "SeverityLevels")
                        {
                            current = group.CurrentState?.Name;
                        }
                    }
                    Verify.AreEqual(severity.ToString(), current);
                });
                IdleSynchronizer.Wait();
            }
        }

        // Scenario: Loads an open InfoBar with no IconSource, then sets IconSource to a SymbolIconSource (Accept), then a
        //     FontIconSource, then null, checking TemplateSettings.IconElement after each step.
        // Expected: IconElement is null initially, then a SymbolIcon with Symbol.Accept, then a FontIcon, then null again.
        // Failure means: InfoBar::UpdateIcon (OnIconSourcePropertyChanged) no longer converts IconSource into an
        //     IconElement via SharedHelpers::MakeIconElementFrom, or does not clear it, so a custom icon would be
        //     missing, stale or of the wrong type.
        [TestMethod]
        public void VerifyIconSourceCreatesAndClearsIconElement()
        {
            var infoBar = CreateLoadedInfoBar();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(infoBar.TemplateSettings.IconElement);

                infoBar.IconSource = new SymbolIconSource { Symbol = Symbol.Accept };
                Verify.IsNotNull(infoBar.TemplateSettings.IconElement);
                Verify.IsTrue(infoBar.TemplateSettings.IconElement is SymbolIcon);
                Verify.AreEqual(Symbol.Accept, (infoBar.TemplateSettings.IconElement as SymbolIcon).Symbol);

                infoBar.IconSource = new FontIconSource { Glyph = "\uE10B" };
                Verify.IsTrue(infoBar.TemplateSettings.IconElement is FontIcon);

                infoBar.IconSource = null;
                Verify.IsNull(infoBar.TemplateSettings.IconElement);
            });
        }

        // Scenario: Loads an open InfoBar, then toggles IconSource (SymbolIconSource / null) and IsIconVisible
        //     (false / true), reading the "IconStates" VisualStateGroup current state after each change.
        // Expected: StandardIconVisible -> UserIconVisible (IconSource set) -> NoIconVisible (IsIconVisible=false) ->
        //     UserIconVisible (IsIconVisible=true) -> StandardIconVisible (IconSource cleared).
        // Failure means: InfoBar::UpdateIconVisibility no longer picks the right IconStates state from IsIconVisible and
        //     IconSource, so the InfoBar would show the standard icon, a custom icon, or no icon at the wrong times.
        [TestMethod]
        public void VerifyIconVisibilityVisualStates()
        {
            var infoBar = CreateLoadedInfoBar();

            string GetIconState()
            {
                var root = VisualTreeHelper.GetChild(infoBar, 0) as FrameworkElement;
                foreach (var group in VisualStateManager.GetVisualStateGroups(root))
                {
                    if (group.Name == "IconStates")
                    {
                        return group.CurrentState?.Name;
                    }
                }
                return null;
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("StandardIconVisible", GetIconState());

                infoBar.IconSource = new SymbolIconSource { Symbol = Symbol.Accept };
                Verify.AreEqual("UserIconVisible", GetIconState());

                infoBar.IsIconVisible = false;
                Verify.AreEqual("NoIconVisible", GetIconState());

                infoBar.IsIconVisible = true;
                Verify.AreEqual("UserIconVisible", GetIconState());

                infoBar.IconSource = null;
                Verify.AreEqual("StandardIconVisible", GetIconState());
            });
        }

        // Scenario: Loads an open InfoBar, sets its Foreground to a red SolidColorBrush, then clears the local Foreground
        //     value, inspecting the "Title" and "Message" TextBlock template parts.
        // Expected: Both template parts exist; with Foreground set both TextBlocks are red; after ClearValue the Title
        //     TextBlock is no longer red.
        // Failure means: InfoBar::OnForegroundChanged/UpdateForeground no longer switches between the ForegroundSet and
        //     ForegroundNotSet visual states, so a custom InfoBar Foreground is ignored or sticks after being cleared.
        [TestMethod]
        public void VerifyForegroundPropagatesToTitleAndMessage()
        {
            var infoBar = CreateLoadedInfoBar();

            RunOnUIThread.Execute(() =>
            {
                var title = VisualTreeUtils.FindVisualChildByName(infoBar, "Title") as Microsoft.UI.Xaml.Controls.TextBlock;
                var message = VisualTreeUtils.FindVisualChildByName(infoBar, "Message") as Microsoft.UI.Xaml.Controls.TextBlock;
                Verify.IsNotNull(title);
                Verify.IsNotNull(message);

                var originalTitleBrush = title.Foreground;

                var red = new SolidColorBrush(Microsoft.UI.Colors.Red);
                infoBar.Foreground = red;
                Content.UpdateLayout();

                Verify.AreEqual(Microsoft.UI.Colors.Red, (title.Foreground as SolidColorBrush)?.Color);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (message.Foreground as SolidColorBrush)?.Color);

                infoBar.ClearValue(Control.ForegroundProperty);
                Content.UpdateLayout();

                Verify.AreNotEqual(Microsoft.UI.Colors.Red, (title.Foreground as SolidColorBrush)?.Color);
            });
        }

        // Scenario: Loads an open InfoBar (IsClosable defaults to true), then sets IsClosable to false and back to true,
        //     checking the Visibility of the "CloseButton" template part after each change.
        // Expected: CloseButton exists and is Visible, becomes Collapsed when IsClosable=false, and Visible again when true.
        // Failure means: InfoBar::UpdateCloseButton (OnIsClosablePropertyChanged) no longer drives the CloseButtonVisible /
        //     CloseButtonCollapsed states, so users could see a close button on a non-closable InfoBar or lose it entirely.
        [TestMethod]
        public void VerifyCloseButtonVisibilityFollowsIsClosable()
        {
            var infoBar = CreateLoadedInfoBar();

            RunOnUIThread.Execute(() =>
            {
                var closeButton = VisualTreeUtils.FindVisualChildByName(infoBar, "CloseButton") as FrameworkElement;
                Verify.IsNotNull(closeButton);
                Verify.AreEqual(Visibility.Visible, closeButton.Visibility);

                infoBar.IsClosable = false;
                Content.UpdateLayout();
                Verify.AreEqual(Visibility.Collapsed, closeButton.Visibility);

                infoBar.IsClosable = true;
                Content.UpdateLayout();
                Verify.AreEqual(Visibility.Visible, closeButton.Visibility);
            });
        }

        // Scenario: Loads an open InfoBar with Closing (sets e.Cancel) and Closed (records e.Reason) handlers; sets
        //     IsOpen=false with Cancel=true, then again with Cancel=false.
        // Expected: Cancelled close reverts IsOpen to true and raises no Closed; the uncancelled close leaves IsOpen false
        //     and raises Closed exactly once with Reason=Programmatic.
        // Failure means: InfoBar::RaiseClosingEvent no longer honors Cancel (reverting IsOpen) or no longer raises
        //     RaiseClosedEvent, or OnIsOpenPropertyChanged reports the wrong m_lastCloseReason for programmatic closes.
        [TestMethod]
        public void VerifyClosingCanBeCancelledAndClosedReportsReason()
        {
            var infoBar = CreateLoadedInfoBar();
            var closedReasons = new List<InfoBarCloseReason>();
            var cancelClose = true;

            RunOnUIThread.Execute(() =>
            {
                infoBar.Closing += (s, e) => { e.Cancel = cancelClose; };
                infoBar.Closed += (s, e) => closedReasons.Add(e.Reason);

                infoBar.IsOpen = false;
                Verify.IsTrue(infoBar.IsOpen, "Cancelled closing should revert IsOpen.");
                Verify.AreEqual(0, closedReasons.Count);

                cancelClose = false;
                infoBar.IsOpen = false;
                Verify.IsFalse(infoBar.IsOpen);
                Verify.AreEqual(1, closedReasons.Count);
                Verify.AreEqual(InfoBarCloseReason.Programmatic, closedReasons[0]);
            });
        }

        // Scenario: Creates an automation peer for an InfoBar, then puts the InfoBar in the tree and opens/closes it once
        //     per severity (Informational, Success, Warning, Error) so the peer raises opened/closed notifications.
        // Expected: Peer is non-null, control type is StatusBar, class name is "Microsoft.UI.Xaml.Controls.InfoBar", and
        //     the open/close loop completes without throwing.
        // Failure means: InfoBarAutomationPeer::GetAutomationControlTypeCore/GetClassNameCore changed, or the
        //     InfoBar::UpdateVisibility -> peer RaiseOpenedEvent/RaiseClosedEvent notification path throws for a severity.
        [TestMethod]
        public void VerifyAutomationPeer()
        {
            RunOnUIThread.Execute(() =>
            {
                var infoBar = new InfoBar { Title = "Title", Message = "Message" };
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(infoBar);
                Verify.IsNotNull(peer);
                Verify.AreEqual(Microsoft.UI.Xaml.Automation.Peers.AutomationControlType.StatusBar, peer.GetAutomationControlType());
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.InfoBar", peer.GetClassName());

                // Opening and closing with each severity raises notification events through the peer
                // (Warning/Error use ImportantAll processing). These must not throw.
                Content = infoBar;
                Content.UpdateLayout();
                foreach (var severity in new[] { InfoBarSeverity.Informational, InfoBarSeverity.Success, InfoBarSeverity.Warning, InfoBarSeverity.Error })
                {
                    infoBar.Severity = severity;
                    infoBar.IsOpen = true;
                    infoBar.IsOpen = false;
                }
            });
        }
    }
}
