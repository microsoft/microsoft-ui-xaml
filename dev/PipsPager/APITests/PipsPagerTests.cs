// Copyright (c) Microsoft Corporation. All rights reserved.
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
                Verify.AreEqual(AutomationLandmarkType.Navigation, peer.GetLandmarkType());
            });
        }

        [TestMethod]
        public void VerifyNumberPanelButtonUIABehavior()
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
                var rootGrid = VisualTreeHelper.GetChild(Content, 0) as Grid;
                var scrollViewer = VisualTreeHelper.GetChild(rootGrid, 1) as ScrollViewer;
                var repeater = VisualTreeHelper.GetChild(scrollViewer, 0) as ItemsRepeater;

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
            var previousIndex = -2;
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
                VerifySelectionChanged(-1, 0);

                pager.NumberOfPages = 10;
                VerifySelectionChanged(-1, 0);

                pager.SelectedPageIndex = 9;
                VerifySelectionChanged(0, 9);

                pager.SelectedPageIndex = 4;
                VerifySelectionChanged(9, 4);
            });

            void Pager_SelectedIndexChanged(PipsPager sender, PipsPagerSelectedIndexChangedEventArgs args)
            {
                previousIndex = args.PreviousPageIndex;
                newIndex = args.NewPageIndex;
            }

            void VerifySelectionChanged(int expectedPreviousIndex, int expectedNewIndex)
            {
                Verify.AreEqual(expectedPreviousIndex, previousIndex, "Expected PreviousPageIndex:" + expectedPreviousIndex + ", actual: " + previousIndex);
                Verify.AreEqual(expectedNewIndex, newIndex, "Expected PreviousPageIndex:" + expectedNewIndex + ", actual: " + newIndex);
            }
        }

        // TODO: Verify infinite behaviour once it's ready

        [TestMethod]
        public void BasicTest()
        {
            Log.Comment("PipsPager Basic Test");
        }
    }
}
