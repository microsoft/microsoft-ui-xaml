// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Automation;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
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

        #region Focus after ItemsSource changes (task.ms/56189664)

        // Scenario: NumberOfPages = 5, SelectedPageIndex = 4, keyboard focus on the last pip; NumberOfPages = 3 removes the
        //           focused pip.
        // Expected: no exception; after layout focus is on a realized pip (PipsPager redirects focus in the pips area to the
        //           selected pip) or outside the pips repeater, never on a cleared pip.
        // A failure means: focus is left on the removed pip.
        [TestMethod]
        [TestProperty("Description", "Verifies that reducing NumberOfPages so that the focused pip is removed moves focus to a realized pip or out of the pips, not onto a cleared pip.")]
        public void VerifyRemovingFocusedPipDoesNotLeaveFocusOnClearedPip()
        {
            PipsPager pager = null;
            ItemsRepeater repeater = null;
            UIElement lastPip = null;
            var gotFocus = new global::System.Threading.ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                pager = new PipsPager { NumberOfPages = 5, SelectedPageIndex = 4 };
                var root = new StackPanel();
                root.Children.Add(new Button { Content = "Before" });
                root.Children.Add(pager);
                root.Children.Add(new Button { Content = "After" });
                Content = root;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                repeater = (ItemsRepeater)pager.FindVisualChildByName("PipsPagerItemsRepeater");
                Verify.IsNotNull(repeater, "The pips ItemsRepeater could not be retrieved");
                lastPip = repeater.TryGetElement(4);
                Verify.IsNotNull(lastPip, "Precondition: the last pip is realized.");
                lastPip.GotFocus += delegate { gotFocus.Set(); };
                Verify.IsTrue(lastPip.Focus(FocusState.Keyboard), "Precondition: the last pip takes focus.");
            });
            Verify.IsTrue(gotFocus.WaitOne(DefaultWaitTimeInMS), "Waiting for the last pip to get focus.");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => pager.NumberOfPages = 3);
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var focused = Microsoft.UI.Xaml.Input.FocusManager.GetFocusedElement(pager.XamlRoot) as DependencyObject;
                Log.Comment($"Focused: {focused?.GetType().Name}; SelectedPageIndex={pager.SelectedPageIndex}");
                for (var node = focused; node != null; node = VisualTreeHelper.GetParent(node))
                {
                    if (VisualTreeHelper.GetParent(node) == repeater)
                    {
                        int index = repeater.GetElementIndex((UIElement)node);
                        Log.Comment($"The focused element is pip {index}.");
                        Verify.IsTrue(index >= 0, "Focus is not on a cleared pip.");
                        break;
                    }
                }
            });
        }

        #endregion
    }
}
