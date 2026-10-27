// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.ObjectModel;
using System.Threading;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Markup;
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

        // Regression test for microsoft/microsoft-ui-xaml#8299. Pushing a new NumberOfPages onto a
        // PipsPager hosted in an auto sized Grid row used to fail with an HRESULT thrown out of the
        // property setter, because OnNumberOfPagesChanged forced a whole tree layout pass from inside
        // the DependencyProperty changed callback. The reporter saw it go away with a fixed row height.
        [TestMethod]
        public void VerifyChangingNumberOfPagesInAutoSizedRowDoesNotCrash()
        {
            Grid root = null;
            PipsPager pager = null;
            var items = new ObservableCollection<string>();
            var pagerLoadedEvent = new ManualResetEvent(false);
            Exception thrownException = null;

            RunOnUIThread.Execute(() =>
            {
                // x:Bind is compiled binding only and unsupported by XamlReader.Load, so the issue's
                // {x:Bind Items.Count, Mode=OneWay} is expressed here as a classic {Binding Count}.
                root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <Grid.RowDefinitions>
                            <RowDefinition Height='*' />
                            <RowDefinition Height='Auto' />
                        </Grid.RowDefinitions>
                        <FlipView Grid.Row='0' ItemsSource='{Binding}' />
                        <StackPanel Grid.Row='1'>
                            <controls:PipsPager x:Name='TestPipsPager'
                                                NumberOfPages='{Binding Count, Mode=OneWay}'
                                                HorizontalAlignment='Center' />
                        </StackPanel>
                      </Grid>");

                pager = (PipsPager)root.FindName("TestPipsPager");
                pager.Loaded += (sender, args) => pagerLoadedEvent.Set();

                root.DataContext = items;
                Content = root;
                Content.UpdateLayout();
            });

            // The template has to be applied before mutating: PipsPager::OnPropertyChanged is gated
            // on Template() != nullptr, so an earlier change would not run the code under test.
            Verify.IsTrue(pagerLoadedEvent.WaitOne(DefaultWaitTimeInMS), "Waiting for loaded event");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                try
                {
                    items.Add("Hello");
                    root.UpdateLayout();
                }
                catch (Exception e)
                {
                    // Captured rather than allowed to escape: the test app installs no
                    // Application.UnhandledException handler, so an escaping exception on the
                    // dispatcher thread takes the app down instead of failing this test.
                    thrownException = e;
                }
            });

            IdleSynchronizer.Wait();

            if (thrownException != null)
            {
                Verify.Fail("Changing NumberOfPages in an auto sized row threw: " + thrownException.ToString());
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, pager.NumberOfPages);
                Verify.AreEqual(0, pager.SelectedPageIndex);
            });
        }

        // Companion to the test above: drives NumberOfPages past MaxVisiblePips (5 by default) with no
        // layout pass in between, so every step selects a pip that the virtualizing layout has not
        // realized yet - the path that used to depend on the forced synchronous layout. The assertions
        // at the end guard against a "fix" that simply stops updating the selected pip.
        [TestMethod]
        public void VerifyRepeatedNumberOfPagesChangesInAutoSizedRowDoNotCrash()
        {
            Grid root = null;
            PipsPager pager = null;
            var pagerLoadedEvent = new ManualResetEvent(false);
            Exception thrownException = null;

            RunOnUIThread.Execute(() =>
            {
                root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <Grid.RowDefinitions>
                            <RowDefinition Height='*' />
                            <RowDefinition Height='Auto' />
                        </Grid.RowDefinitions>
                        <StackPanel Grid.Row='1'>
                            <controls:PipsPager x:Name='TestPipsPager' HorizontalAlignment='Center' />
                        </StackPanel>
                      </Grid>");

                pager = (PipsPager)root.FindName("TestPipsPager");
                pager.Loaded += (sender, args) => pagerLoadedEvent.Set();

                Content = root;
                Content.UpdateLayout();
            });

            Verify.IsTrue(pagerLoadedEvent.WaitOne(DefaultWaitTimeInMS), "Waiting for loaded event");
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                try
                {
                    for (int pageCount = 1; pageCount <= 10; pageCount++)
                    {
                        pager.NumberOfPages = pageCount;
                        pager.SelectedPageIndex = pageCount - 1;
                    }
                }
                catch (Exception e)
                {
                    thrownException = e;
                }
            });

            IdleSynchronizer.Wait();

            if (thrownException != null)
            {
                Verify.Fail("Repeated NumberOfPages changes in an auto sized row threw: " + thrownException.ToString());
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(10, pager.NumberOfPages);
                Verify.AreEqual(9, pager.SelectedPageIndex);

                var repeater = pager.FindVisualChildByType<ItemsRepeater>();
                Verify.IsNotNull(repeater, "PipsPagerItemsRepeater should be in the template.");

                var selectedPip = repeater.TryGetElement(pager.SelectedPageIndex) as FrameworkElement;
                Verify.IsNotNull(selectedPip, "The selected pip should be realized once layout has settled.");
                Verify.AreEqual(pager.SelectedPipStyle, selectedPip.Style, "The selected pip should carry SelectedPipStyle.");
            });
        }
    }
}
