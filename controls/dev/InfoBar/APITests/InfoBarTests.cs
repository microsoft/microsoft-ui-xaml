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
