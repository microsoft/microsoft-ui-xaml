﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Automation;
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
    public class PipsPagerTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyAutomationPeerBehavior()
        {
            RunOnUIThread.Execute(() =>
            {
                var pipsControl = new PipsPager();
                pipsControl.NumberOfPages = 5;
                Content = pipsControl;

                var peer = PipsPagerAutomationPeer.CreatePeerForElement(pipsControl);
                var selectionPeer = peer as ISelectionProvider;
                Verify.AreEqual(false, selectionPeer.CanSelectMultiple);
                Verify.AreEqual(true, selectionPeer.IsSelectionRequired);
            });
        }

        [TestMethod]
        public void VerifyPipsPagerButtonUIABehavior()
        {
            RunOnUIThread.Execute(() =>
            {
                var pipsPager = new PipsPager();
                pipsPager.NumberOfPages = 5;
                Content = pipsPager;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rootPanel = VisualTreeHelper.GetChild(Content, 0) as StackPanel;
                var repeaterRootParent = VisualTreeHelper.GetChild(rootPanel, 1);
                ItemsRepeater repeater = null;
                while (repeater == null)
                {
                    var nextChild = VisualTreeHelper.GetChild(repeaterRootParent, 0);
                    repeater = nextChild as ItemsRepeater;
                    repeaterRootParent = nextChild;
                }
                for (int i = 0; i < 5; i++)
                {
                    var button = repeater.TryGetElement(i);
                    Verify.IsNotNull(button);
                    Verify.AreEqual(i + 1, button.GetValue(AutomationProperties.PositionInSetProperty));
                    Verify.AreEqual(5, button.GetValue(AutomationProperties.SizeOfSetProperty));
                }
            });
        }

        [TestMethod]
        public void VerifyEmptyPagerDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                Content = new PipsPager();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(Content);
            });
        }

        [TestMethod]
        public void VerifySelectedIndexChangedEventArgs()
        {
            PipsPager pager = null;
            var newIndex = -2;
            RunOnUIThread.Execute(() =>
            {
                pager = new PipsPager();
                pager.SelectedIndexChanged += Pager_SelectedIndexChanged;
                Content = pager;

            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifySelectionChanged(0);

                pager.NumberOfPages = 10;
                VerifySelectionChanged(0);

                pager.SelectedPageIndex = 9;
                VerifySelectionChanged(9);

                pager.SelectedPageIndex = 4;
                VerifySelectionChanged(4);
            });

            void Pager_SelectedIndexChanged(PipsPager sender, PipsPagerSelectedIndexChangedEventArgs args)
            {
                newIndex = sender.SelectedPageIndex;
            }

            void VerifySelectionChanged(int expectedNewIndex)
            {
                Verify.AreEqual(expectedNewIndex, newIndex, "Expected PreviousPageIndex:" + expectedNewIndex + ", actual: " + newIndex);
            }
        }
    }
}
