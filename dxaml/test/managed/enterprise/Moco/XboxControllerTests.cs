// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Windows.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco
{
    [TestClass]
    public class XBoxKeyNavigationTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("IsolationLevel", "Class")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            TestDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        public enum GamePadKey
        {
            DpadDown,
            DpadUp,
            DpadLeft,
            DpadRight,
            A,
            B,
            LeftThumbUp,
            LeftThumbDown,
            LeftThumbLeft,
            LeftThumbRight,
            RightThumbUp,
            RightThumbDown,
            RightThumbLeft,
            RightThumbRight,
            RightTrigger,
            LeftTrigger,
            RightShoulder,
            LeftShoulder
        };

        public void PerformXboxKeyPress(GamePadKey key)
        {
            TestServices.KeyboardHelper.SetWaitKind(KeyboardWaitKind.WaitForIdleBeforeAndAfter);
            try
            {
                switch (key)
                {
                    case GamePadKey.A:
                        TestServices.KeyboardHelper.GamepadA();
                        break;

                    case GamePadKey.B:
                        TestServices.KeyboardHelper.GamepadB();
                        break;

                    case GamePadKey.DpadDown:
                        TestServices.KeyboardHelper.GamepadDpadDown();
                        break;

                    case GamePadKey.DpadUp:
                        TestServices.KeyboardHelper.GamepadDpadUp();
                        break;

                    case GamePadKey.DpadLeft:
                        TestServices.KeyboardHelper.GamepadDpadLeft();
                        break;

                    case GamePadKey.DpadRight:
                        TestServices.KeyboardHelper.GamepadDpadRight();
                        break;

                    case GamePadKey.LeftThumbUp:
                        TestServices.KeyboardHelper.GamePadLeftThumbStickUp();
                        break;

                    case GamePadKey.LeftThumbDown:
                        TestServices.KeyboardHelper.GamePadLeftThumbStickDown();
                        break;

                    case GamePadKey.LeftThumbLeft:
                        TestServices.KeyboardHelper.GamePadLeftThumbStickLeft();
                        break;

                    case GamePadKey.LeftThumbRight:
                        TestServices.KeyboardHelper.GamePadLeftThumbStickRight();
                        break;

                    case GamePadKey.RightThumbUp:
                        TestServices.KeyboardHelper.GamePadRightThumbStickUp();
                        break;

                    case GamePadKey.RightThumbDown:
                        TestServices.KeyboardHelper.GamePadRightThumbStickDown();
                        break;

                    case GamePadKey.RightThumbLeft:
                        TestServices.KeyboardHelper.GamePadRightThumbStickLeft();
                        break;

                    case GamePadKey.RightThumbRight:
                        TestServices.KeyboardHelper.GamePadRightThumbStickRight();
                        break;

                    case GamePadKey.RightTrigger:
                        TestServices.KeyboardHelper.GamepadRightTrigger();
                        break;

                    case GamePadKey.LeftTrigger:
                        TestServices.KeyboardHelper.GamepadLeftTrigger();
                        break;

                    case GamePadKey.RightShoulder:
                        TestServices.KeyboardHelper.GamepadRightShoulder();
                        break;

                    case GamePadKey.LeftShoulder:
                        TestServices.KeyboardHelper.GamepadLeftShoulder();
                        break;

                    default:
                        break;
                }
            }
            finally
            {
                TestServices.KeyboardHelper.SetWaitKind(KeyboardWaitKind.Default);
            }
        }

        [TestMethod]
        public void HorizontalListViewsInsideVerticalScrollviewer_BringIntoViewPadding()
        {
            Grid rootGrid = null;

            UIExecutor.Execute(() =>
            {
                rootGrid = (Grid)XamlReader.Load(@"
                       <Grid Width='200' Height='200'
                              HorizontalAlignment='Left'
                              VerticalAlignment='Top'
                              xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <ScrollViewer HorizontalScrollMode='Disabled' HorizontalScrollBarVisibility='Disabled'>
                                <StackPanel Orientation='Vertical' Background='Gray'>
                                    <!-- First -->
                                    <ListView Margin='0,100,0,0' Background='CornflowerBlue'
                                              ScrollViewer.HorizontalScrollBarVisibility='Visible'
                                              ScrollViewer.HorizontalScrollMode='Enabled'
                                              ScrollViewer.VerticalScrollBarVisibility='Disabled'
                                              ScrollViewer.VerticalScrollMode='Disabled'
                                              >
                                        <ListView.ItemsPanel>
                                            <ItemsPanelTemplate>
                                                <ItemsStackPanel Orientation='Horizontal' />
                                            </ItemsPanelTemplate>
                                        </ListView.ItemsPanel>
                                        <ListView.Items>
                                            <ListViewItem>01</ListViewItem>
                                            <ListViewItem>02</ListViewItem>
                                            <ListViewItem>03</ListViewItem>
                                            <ListViewItem>04</ListViewItem>
                                        </ListView.Items>
                                    </ListView>

                                    <!-- Second -->
                                    <ListView Margin='0,100,0,0' Background='CornflowerBlue'
                                               ScrollViewer.HorizontalScrollBarVisibility='Visible'
                                              ScrollViewer.HorizontalScrollMode='Enabled'
                                              ScrollViewer.VerticalScrollBarVisibility='Disabled'
                                              ScrollViewer.VerticalScrollMode='Disabled'>
                                        <ListView.ItemsPanel>
                                            <ItemsPanelTemplate>
                                                <ItemsStackPanel Orientation='Horizontal' />
                                            </ItemsPanelTemplate>
                                        </ListView.ItemsPanel>
                                        <ListView.Items>
                                            <ListViewItem>11</ListViewItem>
                                            <ListViewItem>12</ListViewItem>
                                            <ListViewItem>13</ListViewItem>
                                            <ListViewItem>14</ListViewItem>
                                        </ListView.Items>
                                    </ListView>

                                    <!-- Third -->
                                    <ListView Margin='0,100,0,0' Background='CornflowerBlue'
                                               ScrollViewer.HorizontalScrollBarVisibility='Visible'
                                              ScrollViewer.HorizontalScrollMode='Enabled'
                                              ScrollViewer.VerticalScrollBarVisibility='Disabled'
                                              ScrollViewer.VerticalScrollMode='Disabled'>
                                        <ListView.ItemsPanel>
                                            <ItemsPanelTemplate>
                                                <ItemsStackPanel Orientation='Horizontal' />
                                            </ItemsPanelTemplate>
                                        </ListView.ItemsPanel>
                                        <ListView.Items>
                                            <ListViewItem>21</ListViewItem>
                                            <ListViewItem>22</ListViewItem>
                                            <ListViewItem>23</ListViewItem>
                                            <ListViewItem>24</ListViewItem>
                                        </ListView.Items>
                                    </ListView>
                                </StackPanel>
                            </ScrollViewer>
                        </Grid>");

                TestServices.WindowHelper.WindowContent = rootGrid;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(rootGrid);
                (rootGrid.Children[0] as ScrollViewer).Focus(FocusState.Keyboard);
            });

            TestServices.WindowHelper.WaitForIdle();


            Log.Comment("Pressing Down Key:");
            PerformXboxKeyPress(GamePadKey.DpadDown);

            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                Verify.IsNotNull(focusedElement, "focused element is null");

                // bottom should be 20 pixels above the edge
                var distance = GetFocusedElementDistanceFromEdges(rootGrid);
                Log.Comment("DistanceFromEdges=" + distance.ToString());
                Verify.IsGreaterThanOrEqual(distance.Bottom, 20.0);
            });

            Log.Comment("Pressing Right Key:");
            PerformXboxKeyPress(GamePadKey.DpadRight);
            Log.Comment("Pressing Right Key:");
            PerformXboxKeyPress(GamePadKey.DpadRight);

            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                Verify.IsNotNull(focusedElement, "focused element is null");

                // bottom should be 20 pixels left of the edge
                var distance = GetFocusedElementDistanceFromEdges(rootGrid);
                Log.Comment("DistanceFromEdges=" + distance.ToString());
                Verify.IsGreaterThanOrEqual(distance.Right, 20.0);
            });

            Log.Comment("Pressing Left Key:");
            PerformXboxKeyPress(GamePadKey.DpadLeft);
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                Verify.IsNotNull(focusedElement, "focused element is null");

                // bottom should be 20 pixels right of the edge
                var distance = GetFocusedElementDistanceFromEdges(rootGrid);
                Log.Comment("DistanceFromEdges=" + distance.ToString());
                Verify.IsGreaterThanOrEqual(distance.Left, 20.0);
            });


            Log.Comment("Pressing Up Key:");
            PerformXboxKeyPress(GamePadKey.DpadUp);
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                Verify.IsNotNull(focusedElement, "focused element is null");

                // bottom should be 20 pixels below of the edge
                var distance = GetFocusedElementDistanceFromEdges(rootGrid);
                Log.Comment("DistanceFromEdges=" + distance.ToString());
                Verify.IsGreaterThanOrEqual(distance.Top, 20.0);
            });
        }

        private Thickness GetFocusedElementDistanceFromEdges(FrameworkElement parent)
        {
            Thickness distanceFromEdge = new Thickness(0);

            var element = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as FrameworkElement;

            if (element != null)
            {
                GeneralTransform transformToGrid = element.TransformToVisual(parent);
                var topLeft = transformToGrid.TransformPoint(new Point(0, 0));
                var width = element.ActualWidth;
                var height = element.ActualHeight;

                distanceFromEdge.Top = topLeft.Y;
                distanceFromEdge.Left = topLeft.X;
                var bottom = topLeft.Y + height;
                var right = topLeft.X + width;

                distanceFromEdge.Bottom = parent.Height - bottom;
                distanceFromEdge.Right = parent.Width - right;
            }

            return distanceFromEdge;
        }

        [TestMethod]
        public void SingleSelectionFollowsFocus_ListViewSingleSelectionXboxNavigation()
        {
            ListView listView = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView();
                listView.SingleSelectionFollowsFocus = false;
                listView.SelectionMode = ListViewSelectionMode.Single;
                listView.ItemsSource = Enumerable.Range(0, 10).ToList();
                TestServices.WindowHelper.WindowContent = listView;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                listView.SelectedIndex = 0;
                var container = listView.ContainerFromIndex(0) as ListViewItem;
                Verify.IsNotNull(container);
                container.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(listView.SelectedIndex == 0, "Selected index is 0");
            });

            // key, expected focus index,  expected selected index
            Tuple<GamePadKey, int, int>[] actions = {
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown, 1, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown, 2, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.A, 2, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp, 1, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp, 0, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.A, 0, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbDown, 1, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbUp, 0, 0),
            };

            foreach (var action in actions)
            {
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(listView.SelectedIndex == action.Item3, "Selected index does not match expected");
                    var container = listView.ContainerFromIndex(action.Item2) as ListViewItem;
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Verify.AreEqual(focusedElement, container, "focused element does not match expected container");
                });
            }
        }

        [TestMethod]
        public void SingleSelectionFollowsFocus_GridViewSingleSelectionXboxNavigation()
        {
            GridView gridView = null;

            UIExecutor.Execute(() =>
            {
                gridView = new GridView() { Width = 400, Height = 400 };
                gridView.SingleSelectionFollowsFocus = false;
                gridView.SelectionMode = ListViewSelectionMode.Single;
                gridView.ItemsSource = Enumerable.Range(0, 50).ToList();
                TestServices.WindowHelper.WindowContent = gridView;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                gridView.SelectedIndex = 0;
                var container = gridView.ContainerFromIndex(0) as GridViewItem;
                Verify.IsNotNull(container);
                container.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(gridView.SelectedIndex == 0, "Selected index is 0");
            });

            // 0   1   2   3   4   5   6   7
            // 8   9  10  11  12  13  14  15
            //16  17  18  19  20  21  22  23
            // key, expected focus index,  expected selected index
            Tuple<GamePadKey, int, int>[] actions = {
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown, 8, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown,  16, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,        16, 16),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp,   8, 16),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp,    0, 16),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,     0, 0),

                new Tuple<GamePadKey, int, int>(GamePadKey.DpadRight,1, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadRight, 2, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,        2, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadLeft, 1, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadLeft,  0, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,     0, 0),

                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbDown, 8, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbUp,   0, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbRight, 1, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbLeft,  0, 0),
            };

            foreach (var action in actions)
            {
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(gridView.SelectedIndex == action.Item3, "Selected index does not match expected");
                    var container = gridView.ContainerFromIndex(action.Item2) as GridViewItem;
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Verify.AreEqual(focusedElement, container, "focused element does not match expected container");
                });
            }
        }

        [TestMethod]
        public void SingleSelectionFollowsFocus_ListBoxSingleSelectionXboxNavigation()
        {
            ListBox listBox = null;

            UIExecutor.Execute(() =>
            {
                listBox = new ListBox();
                listBox.SingleSelectionFollowsFocus = false;
                listBox.SelectionMode = SelectionMode.Single;
                listBox.ItemsSource = Enumerable.Range(0, 10).ToList();
                TestServices.WindowHelper.WindowContent = listBox;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                listBox.SelectedIndex = 0;
                var container = listBox.ContainerFromIndex(0) as ListBoxItem;
                Verify.IsNotNull(container);
                container.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(listBox.SelectedIndex == 0, "Selected index is 0");
            });

            // key, expected focus index,  expected selected index
            Tuple<GamePadKey, int, int>[] actions = {
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown, 1, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown,  2, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,        2, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp,   1, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp,    0, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,     0, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbDown, 1, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbUp,   0, 0),
            };

            foreach (var action in actions)
            {
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    Verify.IsTrue(listBox.SelectedIndex == action.Item3, "Selected index does not match expected");
                    var container = listBox.ContainerFromIndex(action.Item2) as ListBoxItem;
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Verify.AreEqual(focusedElement, container, "focused element does not match expected container");
                });
            }
        }

        [TestMethod]
        public void SingleSelectionFollowsFocus_ListBoxXboxNavigation_Multiple()
        {
            ListBox listBox = null;

            UIExecutor.Execute(() =>
            {
                listBox = new ListBox();
                listBox.SingleSelectionFollowsFocus = false;
                listBox.ItemsSource = Enumerable.Range(0, 10).ToList();
                TestServices.WindowHelper.WindowContent = listBox;
                listBox.SelectionMode = SelectionMode.Multiple;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var container = listBox.ContainerFromIndex(0) as ListBoxItem;
                Verify.IsNotNull(container);
                container.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key, expected focus index,  expected selected count
            Tuple<GamePadKey, int, int>[] actions = {
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown, 1, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown,  2, 0),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,        2, 1),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp,   1, 1),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadUp,    0, 1),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,     0, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown, 1, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.DpadDown,  2, 2),
                new Tuple<GamePadKey, int, int>(GamePadKey.A,        2, 1),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbDown, 3, 1),
                new Tuple<GamePadKey, int, int>(GamePadKey.LeftThumbUp,   2, 1),
            };

            foreach (var action in actions)
            {
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var container = listBox.ContainerFromIndex(action.Item2) as ListBoxItem;
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Verify.AreEqual(focusedElement, container, "focused element does not match expected container");
                    Verify.IsTrue(listBox.SelectedItems.Count == action.Item3, "selected items count does not match");
                });
            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ListView2DNavigation()
        {
            ListView listView = null;
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='600' Width='200' HorizontalAlignment='Left' SingleSelectionFollowsFocus='False'>
                        <ListView.GroupStyle>
                          <GroupStyle>
                            <GroupStyle.HeaderTemplate>
                              <DataTemplate>
                                  <Button Content='{Binding GroupName}'/>
                              </DataTemplate>
                            </GroupStyle.HeaderTemplate>
                          </GroupStyle>
                        </ListView.GroupStyle>
                    </ListView>");

                CollectionViewSource cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;

                var groups = new ObservableCollection<MyGroup>();
                for (int i = 0; i < 2; i++)
                {
                    var group = new MyGroup();
                    group.GroupName = string.Format("Group#{0}", i);
                    for (int j = 0; j < 2; j++)
                    {
                        group.Add(i * 10 + j);
                    }
                    groups.Add(group);
                }

                cvs.Source = groups;
                listView.ItemsSource = cvs.View;
                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                headerButton = new Button { Content = "Header" };
                footerButton = new Button { Content = "Footer" };
                listView.Header = headerButton;
                listView.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(listView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Group#0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Group#1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "10"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "11"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Footer"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "After"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Footer"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "11"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "10"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Group#1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Group#0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Before"),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        public void ListView2DNavigationWithoutScrollViewerTemplatePart()
        {
            ListView listView = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"
                   <ListView
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        HorizontalAlignment='Left'
                        Width='600' Height='200'>
                        <ListView.Resources>
                            <Style TargetType='ListView'>
                                <Setter Property='Template'>
                                    <Setter.Value>
                                        <ControlTemplate TargetType='ListView'>
                                            <ItemsPresenter />
                                        </ControlTemplate>
                                    </Setter.Value>
                                </Setter>
                            </Style>
                        </ListView.Resources>
                        <ListViewItem>1</ListViewItem>
                        <ListViewItem>2</ListViewItem>
                        <ListViewItem>3</ListViewItem>
                        <ListViewItem>4</ListViewItem>
                    </ListView>");

                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };

                sp.Children.Add(beforeButton);
                sp.Children.Add(listView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "3"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "4"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "After"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "4"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "3"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Before"),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerticalListView2DPageNavigation()
        {
            ListView listView = null;
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' Height='600' Width='200' HorizontalAlignment='Left'>
                        <ListView.Resources>
                          <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>
                        </ListView.Resources>
                        <ListView.GroupStyle>
                          <GroupStyle>
                            <GroupStyle.HeaderTemplate>
                              <DataTemplate>
                                  <Button Content='{Binding GroupName}'/>
                              </DataTemplate>
                            </GroupStyle.HeaderTemplate>
                          </GroupStyle>
                        </ListView.GroupStyle>
                    </ListView>");

                CollectionViewSource cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;

                var groups = new ObservableCollection<MyGroup>();
                for (int i = 0; i < 10; i++)
                {
                    var group = new MyGroup();
                    group.GroupName = string.Format("Group#{0}", i);
                    for (int j = 0; j < 10; j++)
                    {
                        group.Add(i * 10 + j);
                    }
                    groups.Add(group);
                }

                cvs.Source = groups;
                listView.ItemsSource = cvs.View;
                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                headerButton = new Button { Content = "Header" };
                footerButton = new Button { Content = "Footer" };
                listView.Header = headerButton;
                listView.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(listView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });
            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'

                // Before
                // Header
                // Group#0
                // 0
                // 1
                // ...
                // 9
                //Group#1
                // 10
                // 11
                // ...
                // Footer
                // After

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightShoulder, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftShoulder, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "21"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "31"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "10"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header")
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.AreEqual(action.Item2, focusedElement.Content.ToString(), "focused element does not match expected");
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        // verify that if there are no items in the listview, we handle
        // gracefully without crashing
        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VerticalEmptyListView2DPageNavigation()
        {
            ListView listView = null;
            Button beforeButton = null;


            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='600' Width='200' HorizontalAlignment='Left'>
                        <ListView.Header>
                            <Button>Header</Button>
                        </ListView.Header>
                        <ListView.Footer>
                            <Button>Footer</Button>
                        </ListView.Footer>
                    </ListView>");

                // item source with no items
                listView.ItemsSource = new List<int>();

                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                Button afterButton = new Button { Content = "After" };
                Button headerButton = new Button { Content = "Header" };
                Button footerButton = new Button { Content = "Footer" };
                listView.Header = headerButton;
                listView.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(listView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // Before
                // Header
                // Footer
                // After

                // focus is on 'Before'
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Before"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightShoulder, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftShoulder, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "Footer"),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.AreEqual(action.Item2, focusedElement.Content.ToString(), "focused element does not match expected");
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        public void HorizontalListView2DPageNavigation()
        {
            ListView listView = null;
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='200' Width='400' HorizontalAlignment='Left'
                        ScrollViewer.HorizontalScrollMode='Enabled' ScrollViewer.HorizontalScrollBarVisibility='Visible'
                        ScrollViewer.VerticalScrollMode='Disabled' ScrollViewer.VerticalScrollBarVisibility='Disabled'>
                        <ListView.ItemsPanel>
                            <ItemsPanelTemplate>
                                <ItemsStackPanel Orientation='Horizontal'/>
                            </ItemsPanelTemplate>
                        </ListView.ItemsPanel>
                        <ListView.GroupStyle>
                          <GroupStyle>
                            <GroupStyle.HeaderTemplate>
                              <DataTemplate>
                                  <Button Content='{Binding GroupName}'/>
                              </DataTemplate>
                            </GroupStyle.HeaderTemplate>
                          </GroupStyle>
                        </ListView.GroupStyle>
                    </ListView>");

                CollectionViewSource cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;

                var groups = new ObservableCollection<MyGroup>();
                for (int i = 0; i < 10; i++)
                {
                    var group = new MyGroup();
                    group.GroupName = string.Format("Group#{0}", i);
                    for (int j = 0; j < 10; j++)
                    {
                        group.Add(i * 10 + j);
                    }
                    groups.Add(group);
                }

                cvs.Source = groups;
                listView.ItemsSource = cvs.View;

                // This test was built assuming the old 15 font size, not 14. Button has the same
                // vertical footprint at these sizes, but will measure to different widths. To keep
                // the test behavior consistent we re-style the button to the old metrics.
                var sp = (StackPanel)XamlReader.Load(
                   @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                   <StackPanel.Resources>
                     <Style TargetType='Button'>
                       <Setter Property='FontSize' Value='15'/>
                       <Setter Property='Padding' Value='8,4,8,4'/>
                     </Style>
                   </StackPanel.Resources>
                 </StackPanel>");

                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                headerButton = new Button { Content = "Header" };
                footerButton = new Button { Content = "Footer" };
                listView.Header = headerButton;
                listView.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(listView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });
            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'

                // Before
                //              Group#0              Group#1
                //       Header 0 1 2 3 4 5 6 7 8 9  10 11 12 ... Footer
                // After

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftShoulder, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightShoulder, "7"),
                new Tuple<GamePadKey, string>(GamePadKey.RightShoulder, "11"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftShoulder, "4"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftShoulder, "Header")
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.AreEqual(action.Item2, focusedElement.Content.ToString(), "focused element does not match expected");
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        public void ListView2DNavigationNonFocusableHeaderContent()
        {
            ListView listView = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@"
                    <ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                              Height='200' Width='200' HorizontalAlignment='Left'>
                        <ListView.Header>
                            <StackPanel>
                                <Button>Header1</Button>
                                <Rectangle Fill='Red' Width='200' Height='300'></Rectangle>
                                <Button>Header2</Button>
                            </StackPanel>
                        </ListView.Header>
                        <ListView.Footer>
                            <StackPanel>
                                <Button>Footer1</Button>
                                <Rectangle Fill='Blue' Width='200' Height='300'></Rectangle>
                                <Button>Footer2</Button>
                            </StackPanel>
                        </ListView.Footer>
                    </ListView>");

                listView.ItemsSource = new List<int>() { 1, 2 };
                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };

                sp.Children.Add(beforeButton);
                sp.Children.Add(listView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Footer1"), // scrolling through footer, non focusable rectangle
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Footer1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Footer2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "After"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Footer2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Footer1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Header2"), // scrolling through header, non focusable rectangle
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Header2"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Header1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Before"),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ListViewDelayLoadNoInitilFocus()
        {
            ListView listView = null;

            UIExecutor.Execute(() =>
            {
                listView = new ListView()
                {
                    Width = 200,
                    Height = 200
                };

                // When doing keydown, input manager routes events to the focused
                // element. If nothing is focused it sends it to the root, which if it is a
                // ListView will get a keydown without having focus - that causes wierd behavior.
                var root = new StackPanel();
                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // nothing to focus, so focus is null
                Verify.IsNull(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // nothing to focus, so focus is null
                listView.Items.Add("Delayed Item1");
            });

            TestServices.WindowHelper.WaitForIdle();

            // doing any key now, auto focus should move focus to first focusable element so that
            // focus is not lost and stuck
            TestServices.KeyboardHelper.SetWaitKind(KeyboardWaitKind.WaitForIdleBeforeAndAfter);
            try
            {
                TestServices.KeyboardHelper.GamepadDpadDown();
            }
            finally
            {
                TestServices.KeyboardHelper.SetWaitKind(KeyboardWaitKind.Default);
            }

            UIExecutor.Execute(() =>
            {
                // nothing to focus, so focus is null
                var focused = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                if (focused != null)
                {
                    Log.Comment(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot).ToString());
                }

                Verify.IsNotNull(FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
            });
        }

        [TestMethod]
        public void ListView2DNavigationWithinAndAcrossItem()
        {
            StackPanel rootPanel = null;
            Button leftButton = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(@"
                    <StackPanel Orientation='Horizontal'  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <Button Content='Left' VerticalAlignment='Top' Margin='0,25,10,0'/>
                        <ListView Header='Guidance 3: Unbox' SelectionMode='None'>
                            <ListViewItem IsTabStop='False'>
                                <StackPanel Orientation='Horizontal'>
                                    <Button Content='J' Width='40'/>
                                    <Button Content='K' Width='40'/>
                                    <Button Content='L' Width='40'/>
                                </StackPanel>
                            </ListViewItem>
                            <ListViewItem IsTabStop='False'>
                                <StackPanel Orientation='Horizontal'>
                                    <Button Content='M' Width='40'/>
                                    <Button Content='N' Width='40'/>
                                    <Button Content='O' Width='40'/>
                                </StackPanel>
                            </ListViewItem>
                            <ListViewItem IsTabStop='False'>
                                <StackPanel Orientation='Horizontal'>
                                    <Button Content='P' Width='40'/>
                                    <Button Content='Q' Width='40'/>
                                    <Button Content='R' Width='40'/>
                                </StackPanel>
                            </ListViewItem>
                        </ListView>
                        <Button Content='Right' VerticalAlignment='Top' Margin='0,25,10,0'/>
                    </StackPanel>");

                TestServices.WindowHelper.WindowContent = rootPanel;
                leftButton = rootPanel.Children[0] as Button;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(rootPanel);
                leftButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // Left   J  K  L  Right
                //        M  N  O
                //        P  Q  R
                // focus is on 'Left'
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "J"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "K"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "L"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "Right"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "L"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "O"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "R"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "Q"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "N"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "K"),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void GridView2DNavigationDown()
        {
            GridView2DNavigation(true);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void GridView2DNavigationUp()
        {
            GridView2DNavigation(false);
        }

        public void GridView2DNavigation(bool navigateDown)
        {
            GridView gridView = null;
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;

            UIExecutor.Execute(() =>
            {
                // 300 pixel high gridview cause the headers to become sticky
                gridView = (GridView)XamlReader.Load(@"
                    <GridView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='300' Width='200' HorizontalAlignment='Left' SingleSelectionFollowsFocus='False'>
                        <GridView.GroupStyle>
                          <GroupStyle>
                            <GroupStyle.HeaderTemplate>
                              <DataTemplate>
                                  <Button Content='{Binding GroupName}' />
                              </DataTemplate>
                            </GroupStyle.HeaderTemplate>
                          </GroupStyle>
                        </GridView.GroupStyle>
                    </GridView>");

                CollectionViewSource cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;

                var groups = new ObservableCollection<MyGroup>();
                groups.Add(new MyGroup() { GroupName = "EmptyGroup" });
                for (int i = 0; i < 2; i++)
                {
                    var group = new MyGroup();
                    group.GroupName = string.Format("Group#{0}", i);
                    for (int j = 0; j < 8; j++)
                    {
                        group.Add(i * 10 + j);
                    }
                    groups.Add(group);
                }

                cvs.Source = groups;
                gridView.ItemsSource = cvs.View;
                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                headerButton = new Button { Content = "Header" };
                footerButton = new Button { Content = "Footer" };
                gridView.Header = headerButton;
                gridView.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(gridView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();


            Tuple<GamePadKey, string>[] actions = null;

            if (navigateDown)
            {

                UIExecutor.Execute(() =>
                {
                    beforeButton.Focus(FocusState.Programmatic);
                });

                TestServices.WindowHelper.WaitForIdle();

                // key,  focused element content,
                Tuple<GamePadKey, string>[] downActions = {

                    // focus is on 'Before'

                    // Before
                    // Header
                    // EmptyGroup
                    // Group#0
                    // 0 1 2 3
                    // 4 5 6 7
                    // Group#1
                    // 8 9 10 11
                    // 12 13 14 15
                    // Footer
                    // After

                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "EmptyGroup"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Group#0"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "0"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "4"),

                    new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "5"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "4"),

                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Group#1"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "10"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "14"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Footer"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "After")
                };
                actions = downActions;
            }
            else
            {
                UIExecutor.Execute(() =>
                {
                    afterButton.Focus(FocusState.Programmatic);
                });

                TestServices.WindowHelper.WaitForIdle();

                // key,  focused element content,
                Tuple<GamePadKey, string>[] upActions = {

                    // focus is on 'After'

                    // Before
                    // Header
                    // EmptyGroup
                    // Group#0
                    // 0 1 2 3
                    // 4 5 6 7
                    // Group#1
                    // 8 9 10 11  // out of viewport
                    // 12 13 14 15 // out of viewport
                    // Footer // out of viewport
                    // After
                    new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Group#1"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "4"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "0"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Group#0"),

                    new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "EmptyGroup"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Header"),
                    new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Before")
                };
                actions = upActions;
            }

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void GridView2DPageNavigation()
        {
            GridView gridView = null;
            Button beforeButton = null;
            Button afterButton = null;
            Button headerButton = null;
            Button footerButton = null;

            UIExecutor.Execute(() =>
            {
                // 300 pixel high gridview cause the headers to become sticky
                gridView = (GridView)XamlReader.Load(@"
                    <GridView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' Height='300' Width='200' HorizontalAlignment='Left' SingleSelectionFollowsFocus='False'>
                        <GridView.GroupStyle>
                          <GroupStyle>
                            <GroupStyle.HeaderTemplate>
                              <DataTemplate>
                                  <Button Content='{Binding GroupName}' />
                              </DataTemplate>
                            </GroupStyle.HeaderTemplate>
                          </GroupStyle>
                        </GridView.GroupStyle>
                    </GridView>");

                CollectionViewSource cvs = new CollectionViewSource();
                cvs.IsSourceGrouped = true;

                var groups = new ObservableCollection<MyGroup>();
                groups.Add(new MyGroup() { GroupName = "EmptyGroup" });
                for (int i = 0; i < 4; i++)
                {
                    var group = new MyGroup();
                    group.GroupName = string.Format("Group#{0}", i);
                    for (int j = 0; j < 8; j++)
                    {
                        group.Add(i * 10 + j);
                    }
                    groups.Add(group);
                }

                cvs.Source = groups;
                gridView.ItemsSource = cvs.View;
                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                headerButton = new Button { Content = "Header" };
                footerButton = new Button { Content = "Footer" };
                gridView.Header = headerButton;
                gridView.Footer = footerButton;

                sp.Children.Add(beforeButton);
                sp.Children.Add(gridView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'

                // Before
                // Header
                // EmptyGroup
                // Group#0
                // 0 1 2 3
                // 4 5 6 7
                // Group#1
                // 8 9 10 11
                // 12 13 14 15
                // Group#2
                // 16 17 18 19
                // 20 21 22 23
                // Group#3
                // 24 25 26 27
                // 28 29 30 31
                // Footer
                // After

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightShoulder, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftShoulder, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "Group#3"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "Footer"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "10"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "Header"),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        public void NestedListView2DNavigation()
        {
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                var stackPanel = (StackPanel)XamlReader.Load(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel.Resources>
                                <Style TargetType='ListViewItem'>
                                    <Setter Property='IsTabStop' Value='False'/>
                                </Style>
                                <Style TargetType='GridViewItem'>
                                    <Setter Property='IsTabStop' Value='False'/>
                                </Style>
                                <Style  TargetType='GridView'>
                                    <Setter Property='IsTabStop' Value='False'/>
                                    <Setter Property='HorizontalAlignment' Value='Left'/>
                                    <Setter Property='VerticalAlignment' Value='Top'/>
                                    <Setter Property='ScrollViewer.HorizontalScrollBarVisibility' Value='Hidden'/>
                                    <Setter Property='ScrollViewer.VerticalScrollBarVisibility' Value='Disabled'/>
                                    <Setter Property='ScrollViewer.HorizontalScrollMode' Value='Enabled'/>
                                    <Setter Property='ScrollViewer.IsHorizontalRailEnabled' Value='False'/>
                                    <Setter Property='ScrollViewer.VerticalScrollMode' Value='Disabled'/>
                                    <Setter Property='ScrollViewer.IsVerticalRailEnabled' Value='False'/>
                                    <Setter Property='ItemsPanel'>
                                        <Setter.Value>
                                            <ItemsPanelTemplate>
                                                <ItemsStackPanel Orientation='Horizontal'/>
                                            </ItemsPanelTemplate>
                                        </Setter.Value>
                                    </Setter>
                                </Style>
                                <Style TargetType='ListView'>
                                    <Setter Property='IsTabStop' Value='False'/>
                                    <Setter Property='HorizontalAlignment' Value='Left'/>
                                    <Setter Property='VerticalAlignment' Value='Top'/>
                                    <Setter Property='ScrollViewer.HorizontalScrollBarVisibility' Value='Disabled'/>
                                    <Setter Property='ScrollViewer.VerticalScrollBarVisibility' Value='Visible'/>
                                    <Setter Property='ScrollViewer.HorizontalScrollMode' Value='Disabled'/>
                                    <Setter Property='ScrollViewer.IsHorizontalRailEnabled' Value='False'/>
                                    <Setter Property='ScrollViewer.VerticalScrollMode' Value='Enabled'/>
                                    <Setter Property='ScrollViewer.IsVerticalRailEnabled' Value='False'/>
                                </Style>
                            </StackPanel.Resources>
                            <ListView Width='200' Height='100' SelectionMode='None'>
                                <GridView>
                                    <Button>Grid0 - 0</Button>
                                    <Button>Grid0 - 1</Button>
                                    <Button>Grid0 - 2</Button>
                                    <Button>Grid0 - 3</Button>
                                    <Button>Grid0 - 4</Button>
                                </GridView>
                                <GridView>
                                    <Button>Grid1 - 0</Button>
                                    <Button>Grid1 - 1</Button>
                                    <Button>Grid1 - 2</Button>
                                    <Button>Grid1 - 3</Button>
                                    <Button>Grid1 - 4</Button>
                                </GridView>
                                <GridView>
                                    <Button>Grid2 - 0</Button>
                                    <Button>Grid2 - 1</Button>
                                    <Button>Grid2 - 2</Button>
                                    <Button>Grid2 - 3</Button>
                                    <Button>Grid2 - 4</Button>
                                </GridView>
                            </ListView>
                        </StackPanel>");


                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };
                sp.Children.Add(beforeButton);
                sp.Children.Add(stackPanel);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {
                // focus is on 'Before'
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Grid0 - 0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Grid1 - 0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Grid2 - 0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "After"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Grid2 - 0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Grid1 - 0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Grid0 - 0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Before"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Grid0 - 0"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "Grid0 - 1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "Grid0 - 2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "Grid0 - 3"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "Grid0 - 4"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "Grid0 - 3"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "Grid0 - 2"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "Grid0 - 1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadLeft, "Grid0 - 0"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadRight, "Grid0 - 1"),

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Grid1 - 1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "Grid2 - 1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "After"),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates that when there is a selected item and SingleSelectionFollowsFocus=True, that 2D navigation will move onto the selected item.")]
        public void ListViewBase2DNavigationFocusesSelectedItemWhenSSFF()
        {
            Action<string> runScenario = (string listViewBaseType) =>
            {
                ListViewBase listViewBase = null;
                Button button = null;
                const int expectedSelectedItem = 3;

                UIExecutor.Execute(() =>
                {
                    var root = (StackPanel)XamlReader.Load(string.Format(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='Button'/>
                        <{0} x:Name='listViewBase' SelectedIndex='{1}' SingleSelectionFollowsFocus='True'>
                            <{0}Item Content='Item 1'/>
                            <{0}Item Content='Item 2'/>
                            <{0}Item Content='Item 3'/>
                            <{0}Item Content='Item 4'/>
                            <{0}Item Content='Item 5'/>
                        </{0}>
                    </StackPanel>", listViewBaseType, expectedSelectedItem));

                    button = (Button)root.FindName("button");
                    listViewBase = (ListViewBase)root.FindName("listViewBase");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    button.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                PerformXboxKeyPress(GamePadKey.DpadDown);
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsTrue(focusedElement.Equals(listViewBase.Items[expectedSelectedItem]));
                });
            };

            Log.Comment("Run the scenario with ListView");
            runScenario("ListView");

            Log.Comment("Run the scenario with GridView");
            runScenario("GridView");
        }

        [TestMethod]
        [TestProperty("Description", "Validates that when there is a selected item and SingleSelectionFollowsFocus=True, that 2D navigation can still move onto a focusable header object.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ListViewBase2DNavigationFocusesHeaderWithSelectedItemAndSSFF()
        {
            Action<string> runScenario = (string listViewBaseType) =>
            {
                ListViewBase listViewBase = null;
                Button button = null;
                const int selectedItem = 3;

                UIExecutor.Execute(() =>
                {
                    var root = (StackPanel)XamlReader.Load(string.Format(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='Button'/>
                        <{0} x:Name='listViewBase' SelectedIndex='{1}' SingleSelectionFollowsFocus='True'>
                            <{0}.Header>
                                <Button Content='Header' HorizontalAlignment='Stretch'/>
                            </{0}.Header>
                            <{0}Item Content='Item 1'/>
                            <{0}Item Content='Item 2'/>
                            <{0}Item Content='Item 3'/>
                            <{0}Item Content='Item 4'/>
                            <{0}Item Content='Item 5'/>
                        </{0}>
                    </StackPanel>", listViewBaseType, selectedItem));

                    button = (Button)root.FindName("button");
                    listViewBase = (ListViewBase)root.FindName("listViewBase");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    button.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                PerformXboxKeyPress(GamePadKey.DpadDown);
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsTrue(focusedElement.Equals(listViewBase.Header));
                });
            };

            Log.Comment("Run the scenario with ListView");
            runScenario("ListView");

            Log.Comment("Run the scenario with GridView");
            runScenario("GridView");
        }

        [TestMethod]
        [TestProperty("Description", "Validates that when there is a selected item and SingleSelectionFollowsFocus=True, that 2D navigation can still move onto a focusable footer object.")]
        public void ListViewBase2DNavigationFocusesFooterWithSelectedItemAndSSFF()
        {
            Action<string> runScenario = (string listViewBaseType) =>
            {
                ListViewBase listViewBase = null;
                Button button = null;
                const int selectedItem = 3;

                UIExecutor.Execute(() =>
                {
                    var root = (StackPanel)XamlReader.Load(string.Format(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <{0} x:Name='listViewBase' SelectedIndex='{1}' SingleSelectionFollowsFocus='True'>
                            <{0}.Footer>
                                <Button Content='Footer' HorizontalAlignment='Stretch'/>
                            </{0}.Footer>
                            <{0}Item Content='Item 1'/>
                            <{0}Item Content='Item 2'/>
                            <{0}Item Content='Item 3'/>
                            <{0}Item Content='Item 4'/>
                            <{0}Item Content='Item 5'/>
                        </{0}>
                        <Button x:Name='button' Content='Button'/>
                    </StackPanel>", listViewBaseType, selectedItem));

                    button = (Button)root.FindName("button");
                    listViewBase = (ListViewBase)root.FindName("listViewBase");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    button.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                PerformXboxKeyPress(GamePadKey.DpadUp);
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsTrue(focusedElement.Equals(listViewBase.Footer));
                });
            };

            Log.Comment("Run the scenario with ListView");
            runScenario("ListView");

            Log.Comment("Run the scenario with GridView");
            runScenario("GridView");
        }

        [TestMethod]
        public void ScrollViewer2DNavigation()
        {
            ScrollViewer sv = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                var sp = (StackPanel)XamlReader.Load(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='before'>Before</Button>
                        <ScrollViewer
                            x:Name='sv'
                            Width='400'
                            Height='400'
                            HorizontalAlignment='Left'
                            HorizontalScrollMode='Enabled'
                            VerticalScrollMode='Enabled'
                            HorizontalScrollBarVisibility='Auto'>
                            <Canvas Width='600' Height='600'>
                                <Button Canvas.Top='0' Canvas.Left='0'>1</Button>
                                <Button Canvas.Top='200' Canvas.Left='0'>2</Button>
                                <Button Canvas.Top='500' Canvas.Left='0'>3</Button>
                                <Button Canvas.Top='0' Canvas.Left='200'>5</Button>
                                <Button Canvas.Top='0' Canvas.Left='500'>6</Button>
                            </Canvas>
                        </ScrollViewer>
                        <Button x:Name='after'>After</Button>
                    </StackPanel>");

                beforeButton = (Button)sp.FindName("before");
                sv = (ScrollViewer)sp.FindName("sv");
                afterButton = (Button)sp.FindName("after");

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content, sv.HorizontalOffset, sv.VerticalOffset
            Tuple<GamePadKey, string, double, double>[] actions = {

                // focus is on 'Before'
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "1", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "2", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "3", 0, 152),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "3", 0, 200),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "After", 0, 200),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "3", 0, 200),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "2", 0, 180),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "1", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "Before", 0, 0),

                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "1", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadRight, "5", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadRight, "6", 146, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadRight, "6", 200, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadLeft, "5", 180, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadLeft, "1", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "Before", 0, 0),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");

                    Log.Comment("sv.HorizontalOffset:" + sv.HorizontalOffset + "sv.VerticalOffset:" + sv.VerticalOffset);
                    Verify.AreEqual(sv.HorizontalOffset, action.Item3, "Horizontal offset does not match");
                    Verify.AreEqual(sv.VerticalOffset, action.Item4, "Vertical offset does not match");
                });
            }
        }

        [TestMethod]
        public void ScrollViewer2DPageNavigation()
        {
            ScrollViewer2DPageNavigationTest();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Ignore", "TRUE")]    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        public void ScrollViewer2DPageNavigationHiDpi()
        {
            TestServices.WindowHelper.ResetWindowContentAndScaleWaitForIdle(2.0f);
            ScrollViewer2DPageNavigationTest();
            TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
        }

        private void ScrollViewer2DPageNavigationTest()
        {
            ScrollViewer sv = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                var sp = (StackPanel)XamlReader.Load(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='before'>Before</Button>
                        <ScrollViewer
                                            x:Name='sv'
                                            Width='200'
                                            Height='200'
                                            HorizontalAlignment='Left'
                                            HorizontalScrollMode='Enabled'
                                            VerticalScrollMode='Enabled'
                                            HorizontalScrollBarVisibility='Auto'>
                            <Canvas Width='800' Height='800'>
                                <Button Canvas.Top='0' Canvas.Left='0'>1</Button>
                                <Button Canvas.Top='100' Canvas.Left='10'>2</Button>
                                <Button Canvas.Top='250' Canvas.Left='10'>3</Button>
                                <Button Canvas.Top='300' Canvas.Left='10'>4</Button>
                                <Button Canvas.Top='750' Canvas.Left='20'>5</Button>
                                <Button Canvas.Top='10' Canvas.Left='450'>6</Button>
                                <Button Canvas.Top='450' Canvas.Left='250'>7</Button>
                            </Canvas>
                        </ScrollViewer>
                        <Button x:Name='after'>After</Button>
                    </StackPanel>");

                beforeButton = (Button)sp.FindName("before");
                sv = (ScrollViewer)sp.FindName("sv");
                afterButton = (Button)sp.FindName("after");

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content, sv.HorizontalOffset, sv.VerticalOffset
            Tuple<GamePadKey, string, double, double>[] actions = {

                //   [Before]
                //
                //    0        200       400      600      800
                //       1
                //          2                  6
                //
                //    200
                //          3
                //          4
                //
                //    400
                //
                //                             7
                //
                //    600
                //
                //           5
                //    800
                //
                //    [After]

                // focus is on 'Before'
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "1", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightTrigger, "4", 0, 200),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightTrigger, "4", 0, 400),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightTrigger, "5", 0, 600),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightTrigger, "5", 0, 600), // stay at the end

                new Tuple<GamePadKey, string, double, double>(GamePadKey.LeftTrigger, "5", 0, 400),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.LeftTrigger, "3", 0, 200),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.LeftTrigger, "1", 0, 0),

                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightShoulder, "2", 200, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightShoulder, "6", 400, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightShoulder, "6", 600, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightShoulder, "6", 600, 0),

                new Tuple<GamePadKey, string, double, double>(GamePadKey.LeftShoulder, "6", 400, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.LeftShoulder, "6", 200, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightTrigger, "6", 200, 200),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.RightTrigger, "7", 200, 400),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");

                    Log.Comment("sv.HorizontalOffset:" + sv.HorizontalOffset + "sv.VerticalOffset:" + sv.VerticalOffset);
                    Verify.IsTrue(sv.HorizontalOffset == action.Item3, "Horizontal offset does not match");
                    Verify.IsTrue(sv.VerticalOffset == action.Item4, "Vertical offset does not match");
                });
            }
        }

        [TestMethod]
        public void ScrollViewer2DNavigationWithHyperLink()
        {
            ScrollViewer sv = null;
            Button beforeButton = null;

            UIExecutor.Execute(() =>
            {
                var sp = (StackPanel)XamlReader.Load(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='before'>Before</Button>
                        <ScrollViewer
                            x:Name='sv'
                            Width='400'
                            Height='400'
                            HorizontalAlignment='Left'
                            HorizontalScrollMode='Enabled'
                            VerticalScrollMode='Enabled'
                            HorizontalScrollBarVisibility='Auto'>
                            <Canvas Width='600' Height='600'>
                                <Button Canvas.Top='0' Canvas.Left='0'>1</Button>
                                <TextBlock Canvas.Top='200'>
                                  <Hyperlink>Hyperlink Text</Hyperlink>
                                </TextBlock>
                            </Canvas>
                        </ScrollViewer>
                        <Button x:Name='after'>After</Button>
                    </StackPanel>");

                beforeButton = (Button)sp.FindName("before");
                sv = (ScrollViewer)sp.FindName("sv");

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content
            Tuple<GamePadKey, Type>[] actions = {

                // focus is on 'Before'
                new Tuple<GamePadKey, Type>(GamePadKey.DpadDown, typeof(Button)),
                new Tuple<GamePadKey, Type>(GamePadKey.DpadDown, typeof(Hyperlink)),
                new Tuple<GamePadKey, Type>(GamePadKey.DpadDown, typeof(Hyperlink)),
                new Tuple<GamePadKey, Type>(GamePadKey.DpadDown, typeof(Button)),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement);
                    Verify.IsTrue(focusedElement.GetType() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ScrollViewerEngagement()
        {
            ScrollViewerEngagementTest();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "TRUE")]    // [DCPPTest] Xaml tests are failing because Xaml no longer applies the plateau scale
        public void ScrollViewerEngagementHiDpi()
        {
            TestServices.WindowHelper.ResetWindowContentAndScaleWaitForIdle(2.0f);
            ScrollViewerEngagementTest();
            TestServices.WindowHelper.ResetWindowContentAndWaitForIdle();
        }

        public void ScrollViewerEngagementTest()
        {
            ScrollViewer sv = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                // Even though by tab order '2' is the first focusable element, '1' should get focus once engaged because it is near the top.
                var sp = (StackPanel)XamlReader.Load(@"
                    <StackPanel Margin='40' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='before'>Before</Button>
                        <ScrollViewer
                            x:Name='sv'
                            Width='240'
                            Height='240'
                            HorizontalAlignment='Left'
                            HorizontalScrollMode='Enabled'
                            VerticalScrollMode='Enabled'
                            HorizontalScrollBarVisibility='Auto'>
                            <Canvas Width='500' Height='500'>
                                <Button Canvas.Top='200' Canvas.Left='0'>2</Button>
                                <Button Canvas.Top='400' Canvas.Left='0'>3</Button>
                                <Button Canvas.Top='0' Canvas.Left='200'>5</Button>
                                <Button Canvas.Top='0' Canvas.Left='400'>6</Button>
                                <Button Canvas.Top='0' Canvas.Left='0'>1</Button>
                            </Canvas>
                        </ScrollViewer>
                        <Button x:Name='after'>After</Button>
                    </StackPanel>");

                beforeButton = (Button)sp.FindName("before");
                sv = (ScrollViewer)sp.FindName("sv");
                afterButton = (Button)sp.FindName("after");

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                sv.IsFocusEngagementEnabled = true;
                sv.UseSystemFocusVisuals = true;
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content, sv.HorizontalOffset, sv.VerticalOffset
            Tuple<GamePadKey, string, double, double>[] actions = {

                // focus is on 'Before'
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "Microsoft.UI.Xaml.Controls.Canvas", 0, 0),

                new Tuple<GamePadKey, string, double, double>(GamePadKey.A, "1", 0, 0),

                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "2", 0, 12),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "3", 0, 212),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "3", 0, 260),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadDown, "3", 0, 260), // should not go out

                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "2", 0, 180),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "1", 0, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadUp, "1", 0, 0), // should not go out

                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadRight, "5", 6, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadRight, "6", 206, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadRight, "6", 260, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadLeft, "5", 180, 0),
                new Tuple<GamePadKey, string, double, double>(GamePadKey.DpadLeft, "1", 0, 0),

                new Tuple<GamePadKey, string, double, double>(GamePadKey.B, "Microsoft.UI.Xaml.Controls.Canvas", 0, 0),

            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");

                    Log.Comment("Expected sv.HorizontalOffset:" + action.Item3 + ", sv.VerticalOffset:" + action.Item4);
                    Log.Comment("Actual sv.HorizontalOffset:" + sv.HorizontalOffset + ", sv.VerticalOffset:" + sv.VerticalOffset);
                    Verify.IsTrue(Math.Abs(sv.HorizontalOffset - action.Item3) < 1.0, "Horizontal offset does not match");
                    Verify.IsTrue(Math.Abs(sv.VerticalOffset - action.Item4) < 1.0, "Vertical offset does not match");
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to rendering scopeguard not working in WPF yet
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ListBoxEngagement()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))

            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                ListBox lb = null;
                Button before = null;
                Button after = null;

                UIExecutor.Execute(() =>
                {
                    var root = (StackPanel)XamlReader.Load(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='before' Content='before'/>
                        <ListBox x:Name='lb' IsFocusEngagementEnabled='True' Width='200' Height='200'>
                            <ListBoxItem>ListBox Item 1</ListBoxItem>
                            <ListBoxItem>ListBox Item 2</ListBoxItem>
                            <ListBoxItem>ListBox Item 3</ListBoxItem>
                            <ListBoxItem>ListBox Item 4</ListBoxItem>
                            <ListBoxItem>ListBox Item 5</ListBoxItem>
                            <ListBoxItem>ListBox Item 6</ListBoxItem>
                            <ListBoxItem>ListBox Item 7</ListBoxItem>
                            <ListBoxItem>ListBox Item 8</ListBoxItem>
                            <ListBoxItem>ListBox Item 9</ListBoxItem>
                            <ListBoxItem>ListBox Item 10</ListBoxItem>
                        </ListBox>
                        <Button x:Name='after' Content='after'/>
                    </StackPanel>");

                    before = (Button)root.FindName("before");
                    lb = (ListBox)root.FindName("lb");
                    after = (Button)root.FindName("after");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Verify.IsNotNull(before);
                    before.Focus(FocusState.Programmatic);
                });
                TestServices.WindowHelper.WaitForIdle();

                PerformXboxKeyPress(GamePadKey.DpadDown);
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
            }
        }

        [TestMethod]
        public void ListBox2DPageNavigation()
        {
            ListBox listBox = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                listBox = (ListBox)XamlReader.Load(
                    @"<ListBox xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                          Height='600' Width='200' HorizontalAlignment='Left'>
                      </ListBox>");
                listBox.ItemsSource = Enumerable.Range(0, 99).ToList();
                StackPanel sp = new StackPanel();
                beforeButton = new Button { Content = "Before" };
                afterButton = new Button { Content = "After" };

                sp.Children.Add(beforeButton);
                sp.Children.Add(listBox);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });
            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content,
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'

                // Before
                // 0
                // 1
                // 2
                // ...
                // 99
                // After

                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "0"),
                new Tuple<GamePadKey, string>(GamePadKey.RightShoulder, "0"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftShoulder, "0"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "0"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "14"),
                new Tuple<GamePadKey, string>(GamePadKey.RightTrigger, "29"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "15"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "0"),
                new Tuple<GamePadKey, string>(GamePadKey.LeftTrigger, "0")
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.AreEqual(action.Item2, focusedElement.Content.ToString(), "focused element does not match expected");
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        public void ScrollViewerNavigationOnDisabledScrollMode()
        {
            ScrollViewer sv = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                var sp = (StackPanel)XamlReader.Load(@"
                    <StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'   xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='before'>Before</Button>
                        <ScrollViewer
                            x:Name='sv'
                            Width='400'
                            Height='400'
                            HorizontalAlignment='Left'
                            HorizontalScrollMode='Disabled'
                            VerticalScrollMode='Disabled'
                            HorizontalScrollBarVisibility='Auto'>
                            <Canvas Width='600' Height='600'>
                                <Button Canvas.Top='0' Canvas.Left='0'>1</Button>
                            </Canvas>
                        </ScrollViewer>
                        <Button x:Name='after'>After</Button>
                    </StackPanel>");

                beforeButton = (Button)sp.FindName("before");
                sv = (ScrollViewer)sp.FindName("sv");
                afterButton = (Button)sp.FindName("after");

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element content, sv.HorizontalOffset, sv.VerticalOffset
            Tuple<GamePadKey, string>[] actions = {

                // focus is on 'Before'
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadDown, "After"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "1"),
                new Tuple<GamePadKey, string>(GamePadKey.DpadUp, "Before")
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot) as ContentControl;
                    Verify.IsNotNull(focusedElement, "focused element is null");
                    Log.Comment("Focused element is " + focusedElement + "Content " + focusedElement.Content.ToString());
                    Verify.IsTrue(focusedElement.Content.ToString() == action.Item2, "focused element does not match expected");
                });
            }
        }

        [TestMethod]
        public void CalendarViewEngagement()
        {
            CalendarView calendarView = null;
            Button beforeButton = null;
            Button afterButton = null;

            UIExecutor.Execute(() =>
            {
                StackPanel sp = new StackPanel();
                beforeButton = new Button() { Content = "Before" };
                calendarView = new CalendarView();
                afterButton = new Button() { Content = "After" };

                sp.Children.Add(beforeButton);
                sp.Children.Add(calendarView);
                sp.Children.Add(afterButton);

                TestServices.WindowHelper.WindowContent = sp;

                calendarView.SetDisplayDate(DateTimeOffset.Parse("11/09/2008"));
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            // key,  focused element, display mode
            Tuple<GamePadKey, string, CalendarViewDisplayMode>[] actions = {

                // focus is on 'Before'
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadDown, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadRight, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadRight, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadLeft, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadLeft, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadDown, "Microsoft.UI.Xaml.Controls.ScrollViewer", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.A, "CalendarViewDayItem", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadUp, "CalendarViewDayItem", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadUp, "CalendarViewDayItem", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadUp, "CalendarViewDayItem", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadDown, "CalendarViewDayItem", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.B, "Microsoft.UI.Xaml.Controls.ScrollViewer", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadDown, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadUp, "Microsoft.UI.Xaml.Controls.ScrollViewer", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadUp, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.A, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Year),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.A, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Decade),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadDown, "Microsoft.UI.Xaml.Controls.ScrollViewer", CalendarViewDisplayMode.Decade),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.A, "Microsoft.UI.Xaml.Controls.Control", CalendarViewDisplayMode.Decade),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadRight, "Microsoft.UI.Xaml.Controls.Control", CalendarViewDisplayMode.Decade),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.A, "Microsoft.UI.Xaml.Controls.Control", CalendarViewDisplayMode.Year),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadRight, "Microsoft.UI.Xaml.Controls.Control", CalendarViewDisplayMode.Year),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.A, "CalendarViewDayItem", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadRight, "CalendarViewDayItem", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.A, "CalendarViewDayItem", CalendarViewDisplayMode.Month),

                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.B, "Microsoft.UI.Xaml.Controls.ScrollViewer", CalendarViewDisplayMode.Month),
                new Tuple<GamePadKey, string, CalendarViewDisplayMode>(GamePadKey.DpadDown, "Microsoft.UI.Xaml.Controls.Button", CalendarViewDisplayMode.Month),
            };

            foreach (var action in actions)
            {
                Log.Comment("Pressing Key:" + action.Item1.ToString());
                PerformXboxKeyPress(action.Item1);

                UIExecutor.Execute(() =>
                {
                    var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                    Verify.IsNotNull(focusedElement, "focused element is null");

                    string item = focusedElement.GetType().ToString();
                    if (focusedElement is CalendarViewDayItem)
                    {
                        item = "CalendarViewDayItem";
                    }

                    Log.Comment("Focused element is " + item + " DisplayMode:" + calendarView.DisplayMode.ToString());
                    Verify.IsTrue(item == action.Item2, "focused element does not match expected");
                    Verify.IsTrue(calendarView.DisplayMode == action.Item3);
                });
            }
        }

        [TestMethod]
        public void CalendarViewEngagementFocus()
        {
            CalendarView calendarView = null;
            DateTimeOffset focusedDate = new DateTimeOffset();

            UIExecutor.Execute(() =>
            {
                StackPanel sp = new StackPanel();
                calendarView = new CalendarView();
                sp.Children.Add(calendarView);

                TestServices.WindowHelper.WindowContent = sp;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(calendarView);
                calendarView.Focus(FocusState.Programmatic);
            });

            TestServices.WindowHelper.WaitForIdle();

            PerformXboxKeyPress(GamePadKey.DpadDown);
            TestServices.WindowHelper.WaitForIdle();

            // Engage on month view
            PerformXboxKeyPress(GamePadKey.A);
            TestServices.WindowHelper.WaitForIdle();

            // find the focused element
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsNotNull(focusedElement, "focused element is null");

                var dayItem = focusedElement as CalendarViewDayItem;

                Verify.IsNotNull(dayItem);
                focusedDate = dayItem.Date;
            });

            // DisEngage
            PerformXboxKeyPress(GamePadKey.B);
            TestServices.WindowHelper.WaitForIdle();

            // Go to the button
            PerformXboxKeyPress(GamePadKey.DpadUp);
            TestServices.WindowHelper.WaitForIdle();

            // YearView
            PerformXboxKeyPress(GamePadKey.A);
            TestServices.WindowHelper.WaitForIdle();

            // DecadeView
            PerformXboxKeyPress(GamePadKey.A);
            TestServices.WindowHelper.WaitForIdle();

            PerformXboxKeyPress(GamePadKey.DpadDown);
            TestServices.WindowHelper.WaitForIdle();

            // Engage in decade view
            PerformXboxKeyPress(GamePadKey.A);
            TestServices.WindowHelper.WaitForIdle();

            // Move to Year View
            PerformXboxKeyPress(GamePadKey.A);
            TestServices.WindowHelper.WaitForIdle();

            // Move to Month View
            PerformXboxKeyPress(GamePadKey.A);
            TestServices.WindowHelper.WaitForIdle();

            // verify that we got back to the exact same date
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsNotNull(focusedElement, "focused element is null");

                var dayItem = focusedElement as CalendarViewDayItem;
                Verify.IsTrue(dayItem.Date == focusedDate);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CalendarView2DPageNavigation()
        {
            CalendarView calendarView = null;
            Button beforeButton = null;
            DateTimeOffset focusedDate = new DateTimeOffset(DateTime.Now);

            UIExecutor.Execute(() =>
            {
                StackPanel sp = new StackPanel();

                beforeButton = new Button { Content = "Before" };
                sp.Children.Add(beforeButton);
                calendarView = new CalendarView();
                sp.Children.Add(calendarView);

                TestServices.WindowHelper.WindowContent = sp;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(beforeButton);
                beforeButton.Focus(FocusState.Programmatic);
            });
            TestServices.WindowHelper.WaitForIdle();

            // tab to the day items section of the CalendarView
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();
            TestServices.KeyboardHelper.Tab();
            TestServices.WindowHelper.WaitForIdle();

            // find the focused element
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsNotNull(focusedElement, "focused element is null");

                var dayItem = focusedElement as CalendarViewDayItem;

                Verify.IsNotNull(dayItem);
                focusedDate = dayItem.Date;
            });
            TestServices.WindowHelper.WaitForIdle();

            PerformXboxKeyPress(GamePadKey.RightTrigger);

            // find the focused element
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsNotNull(focusedElement, "focused element is null");

                var dayItem = focusedElement as CalendarViewDayItem;

                Verify.IsNotNull(dayItem);
                Verify.IsGreaterThan(dayItem.Date, focusedDate, "New focus date is not greater after paging down");

                focusedDate = dayItem.Date;
            });
            TestServices.WindowHelper.WaitForIdle();

            PerformXboxKeyPress(GamePadKey.LeftTrigger);

            // find the focused element
            UIExecutor.Execute(() =>
            {
                var focusedElement = FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot);
                Verify.IsNotNull(focusedElement, "focused element is null");

                var dayItem = focusedElement as CalendarViewDayItem;

                Verify.IsNotNull(dayItem);
                Verify.IsLessThan(dayItem.Date, focusedDate, "New focus date is not less after paging up");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        public void GamepadAKeyPress()
        {
            ListView list = null;
            var listClicked = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = new ListView();
                list.ItemsSource = Enumerable.Range(0, 5).Select(i => "Item #" + i).ToList();
                list.IsItemClickEnabled = true;
                TestServices.WindowHelper.WindowContent = list;
                list.ItemClick += (s, e) =>
                {
                    listClicked.Set();
                };
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                list.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            // GamepadA key down and up on same ListViewItem should fire Clicked event
            TestServices.KeyboardHelper.Down();
            TestServices.KeyboardHelper.GamepadA();
            Verify.IsTrue(listClicked.WaitOne(500), "Recieved cliked event from list");
            TestServices.WindowHelper.WaitForIdle();

            // GamepadA key down and up on different ListViewItem should not fire Clicked event
            TestServices.KeyboardHelper.PressKeySequence("$d$_GamepadA");
            TestServices.KeyboardHelper.Down();
            TestServices.KeyboardHelper.PressKeySequence("$u$_GamepadA");
            Verify.IsFalse(listClicked.WaitOne(500), "Did not recieve cliked event from list");
        }
    }

    public class MyGroup : ObservableCollection<int>
    {
        public string GroupName { get; set; }
        public override String ToString()
        {
            return GroupName;
        }
    }
}

