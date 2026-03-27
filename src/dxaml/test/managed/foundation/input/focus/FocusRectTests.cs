// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Windows.Foundation;
using Windows.UI;
using Microsoft.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Data;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Input;

namespace Microsoft.UI.Xaml.Tests.Focus
{
    [TestClass]
    public partial class FocusRectTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Validate Transform Rect don't crash even if it receives non invertible matrix")]
        public void SetFocusToInvalidTransformedButton()
        {
            using (new TestCleanupWrapper())
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                Button FocusedButton = null;
                Button TransformedButton = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel>
                                <Button x:Name='FocusedButton' Content = 'FocusedButton'/>
                                <Button x:Name='TransformedButton' Content = 'TransformedButton'>
                                    <Button.RenderTransform>
                                        <CompositeTransform ScaleX = '0' ScaleY = '0'/>
                                    </Button.RenderTransform>
                                </Button>
                            </StackPanel>
                          </Page>");

                    FocusedButton = (Button)root.FindName("FocusedButton");
                    TransformedButton = (Button)root.FindName("TransformedButton");

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                FocusHelper.EnsureFocus(FocusedButton, FocusState.Keyboard);

                Log.Comment("Press Tab key");
                TestServices.KeyboardHelper.Tab();

                Log.Comment("Make sure we don't crash!");
            }
        }

        [TestMethod]
        [TestProperty("Description", "When focus changes to a hyperlink during page load, don't crash")]
        public void SetFocusToHyperlinkDuringPageLoad()
        {
            using (new TestCleanupWrapper())
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                TextBlock textBlock = null;

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Start with a blank frame to ensure we have focus");
                    TestServices.WindowHelper.WindowContent = new Frame();
                });
                TestServices.WindowHelper.WaitForIdle();

                XamlRoot xamlRoot = null;
                UIExecutor.Execute(() =>
                {
                    xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
                });

                TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Keyboard, xamlRoot); // ensure focus rect

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel><TextBlock x:Name='textBlock' /></StackPanel>
                    </Page>");
                    textBlock = (TextBlock)root.FindName("textBlock");
                    root.Loaded += (s, e) =>
                    {
                        Hyperlink h = new Hyperlink();
                        h.Inlines.Add(new Run() { Text = "The hyperlink" });
                        textBlock.Inlines.Add(h);

                        Log.Comment("Setting focus to hyperlink");
                        h.Focus(FocusState.Keyboard);
                    };
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Make sure we don't crash!");
            }
        }

        [TestMethod]
        [TestProperty("Description", "When focus changes but the tree doesn't change, make sure the focus rect moves")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ChangeFocusWithoutChangingTree()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                Button button = null;
                Hyperlink hyperlink = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <Button x:Name='button'>Initial focus</Button>
                            <TextBlock><Hyperlink x:Name='hyperlink'>Hyperlink</Hyperlink></TextBlock>
                        </StackPanel>
                    </Page>");
                    button = (Button)root.FindName("button");
                    hyperlink = (Hyperlink)root.FindName("hyperlink");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                XamlRoot xamlRoot = null;
                UIExecutor.Execute(() =>
                {
                    xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
                });

                TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Keyboard, xamlRoot); // ensure focus rect

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
            }
        }

        [TestMethod]
        [TestProperty("Description", "If another element is occluding the focused element, the focus rect should be occluded as well.  Currently occlusion isn't perfect in all cases.")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to rendering scopeguard not working in WPF yet
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void OccludedFocusedElement()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                Button button = null;
                Button button2 = null;
                Hyperlink hyperlink = null;
                Rectangle rect = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <Button x:Name='button'>Initial focus</Button>
                            <TextBlock><Hyperlink x:Name='hyperlink'>Hyperlink</Hyperlink></TextBlock>
                            <Button x:Name='button2'>
                                <Button.Template>
                                    <ControlTemplate>
                                        <TextBlock Text='b' Width='200' />
                                    </ControlTemplate>
                                </Button.Template>
                            </Button>
                            <Rectangle x:Name='rect' Fill='Red' Width='100' Height='100' HorizontalAlignment='Left'/>
                        </StackPanel>
                    </Page>");
                    button = (Button)root.FindName("button");
                    button2 = (Button)root.FindName("button2");
                    hyperlink = (Hyperlink)root.FindName("hyperlink");
                    rect = (Rectangle)root.FindName("rect");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    button.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.WindowHelper.SynchronouslyTickUIThread(2); // WaitForIdle doesn't always wait long enough for focus rects
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "button_visible");

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "hyperlink_visible");

                UIExecutor.Execute(() =>
                {
                    button2.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "button2_visible");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Position rectangle so it occludes the buttons and the hyperlink.");
                    rect.RenderTransform = new Microsoft.UI.Xaml.Media.TranslateTransform() { X = 50.0, Y = -40.0 };
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "button2_occluded");

                UIExecutor.Execute(() =>
                {
                    hyperlink.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "hyperlink_occluded");

                UIExecutor.Execute(() =>
                {
                    button.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "button_occluded");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Focusable textelements like hyperlink from RichTextBlockOverflow should draw focus rect when focused")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void VerifyRichTextBlockOverflowHyperlinkDrawsFocusRect()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(600, 400));

                Page root = null;
                Hyperlink hyperlink1 = null;
                Hyperlink hyperlink2 = null;

                const string rootPageXaml =
                    @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Grid x:Name='columnGrid' Background='White' Width='900' Height='300'>
                                <Grid.ColumnDefinitions>
                                    <ColumnDefinition/>
                                    <ColumnDefinition/>
                                    <ColumnDefinition/>
                                    <ColumnDefinition/>
                                </Grid.ColumnDefinitions>
                                <RichTextBlock Grid.Column = '0' IsTextSelectionEnabled = 'True' TextAlignment = 'Justify'
                                            OverflowContentTarget = '{Binding ElementName=firstOverflowContainer}'
                                            TextIndent = '12'
                                            FontSize = '12' FontFamily = 'Segoe UI' Foreground = '#2a2a2a' Margin = '20,0'>
                                    <Paragraph>
                                        <Bold>
                                            <Span Foreground = 'DarkSlateBlue' FontSize = '16'> Mission Mars:</Span>
                                        </Bold>
                                        While working on the Mars
                                        <Hyperlink TabIndex='0' x:Name='hyperlink1'> Pathfinder </Hyperlink>
                                        mission at NASA\92s Jet Propulsion Laboratory in Pasadena, California, I came to a fork in the road.It was a moment when I had to decide whether to move out of my comfort zone and take a risk.
                                        NASA was putting together a 25 - person launch team that would temporarily move from California to Florida near the Kennedy Space Center.All of my best buddies were going to be on this team.
                                        I knew it was going to be a \93work hard, play hard\94 scenario, and I really wanted to go.
                                        Up until that point I had been working as the lead for the simulation software that was used to prepare for the mission. At one point, a colleague and I were tasked with fixing the flight simulation code,
                                        including the star scanner\97the eyeball of the spacecraft.The flight simulation code fakes out the spacecraft so it thinks it\92s flying to Mars when it\92s really sitting in the lab in Pasadena.
                                        The work had fallen nine months behind schedule, and my teammate Miguel and I had been given eight weeks to complete the code.Neither of us had ever written this kind of code before, but
                                        <Hyperlink TabIndex='1' x:Name='hyperlink2'> failure was not an option.</Hyperlink>
                                    </Paragraph>
                                    <Paragraph>
                                        In one way, the entire Pathfinder mission was a risk\97it was an attempt to reinvent space travel, an experiment to see just how cheap and how fast we could put a spacecraft on another planet.
                                        And we didn\92t want to disappoint America, The United States.
                                    </Paragraph>
                                </RichTextBlock>
                                <RichTextBlockOverflow x:Name = 'firstOverflowContainer' Grid.Column = '1' Margin = '20,0'
                                                    OverflowContentTarget = '{Binding ElementName=secondOverflowContainer}'/>
                                <RichTextBlockOverflow x:Name = 'secondOverflowContainer' Grid.Column = '2' Margin = '20,0'/>
                            </Grid>
                        </Page>";
                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(rootPageXaml);
                    hyperlink1 = (Hyperlink)(root.FindName("hyperlink1"));
                    hyperlink2 = (Hyperlink)(root.FindName("hyperlink2"));

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                FocusHelper.EnsureFocus(hyperlink2, FocusState.Keyboard);

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // DCPP: unreliable test: Focus.FocusRectTests.ShyHeader
        [TestProperty("Description", "Ensure focus rect displays underneath a 'shy header' rather than on top.  (Shy header implementation modeled after RS2-era Groove app)")]
        [TestProperty("IsolationLevel", "Method")]  // There are some lifetime problems with this test that result in the next test crashing.
                                                    // Likely because of the way it uses MockDComp and hand-off visuals.
        [TestProperty("Hosting:Mode", "UAP")]
        public void ShyHeader()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))

            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                Rectangle header = null;
                ListView listView = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Grid>
                                <ListView x:Name='listView'>
                                    <ListView.Header>
                                        <Rectangle x:Name='header' Fill='LightBlue' Width='200' Height='180' />
                                    </ListView.Header>
                                    <x:String>Item 1</x:String>
                                    <x:String>Item 2</x:String>
                                    <x:String>Item 3</x:String>
                                    <x:String>Item 4</x:String>
                                    <x:String>Item 5</x:String>
                                    <x:String>Item 6</x:String>
                                    <x:String>Item 7</x:String>
                                    <x:String>Item 8</x:String>
                                    <x:String>Item 9</x:String>
                                    <x:String>Item 10</x:String>
                                </ListView>
                            </Grid>
                    </Page>");
                    listView = (ListView)root.FindName("listView");
                    header = (Rectangle)root.FindName("header");
                    listView.Transitions = new Media.Animation.TransitionCollection();
                    listView.ItemContainerTransitions = new Media.Animation.TransitionCollection();
                    ScrollViewer.SetHorizontalScrollBarVisibility(listView, ScrollBarVisibility.Hidden);
                    ScrollViewer.SetVerticalScrollBarVisibility(listView, ScrollBarVisibility.Hidden);
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                // This code was borrowed from an app team as a minimal sample of how their shy header works:
                UIExecutor.Execute(() =>
                {
                    ContentControl parentContentControl = null;
                    ScrollViewer parentScrollViewer = null;

                    DependencyObject candidate = header;

                    while (candidate != null)
                    {
                        candidate = VisualTreeHelper.GetParent(candidate);
                        if (parentContentControl == null)
                        {
                            parentContentControl = candidate as ContentControl;
                        }
                        if (parentScrollViewer == null)
                        {
                            parentScrollViewer = candidate as ScrollViewer;
                        }
                    }

                    Canvas.SetZIndex(parentContentControl, 1);

                    Verify.IsNotNull(parentContentControl);
                    Verify.IsNotNull(parentScrollViewer);

                    var rectVisual = ElementCompositionPreview.GetElementVisual(parentContentControl);
                    var scrollerPropSet = ElementCompositionPreview.GetScrollViewerManipulationPropertySet(parentScrollViewer);
                    var animation = rectVisual.Compositor.CreateExpressionAnimation();
                    animation.SetReferenceParameter("scroller", scrollerPropSet);
                    animation.Expression = "Matrix4x4.CreateFromTranslation(vector3(0, -scroller.Translation.Y, 0))";
                    rectVisual.StartAnimation("TransformMatrix", animation);
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.Tab(); // Get keyboard focus
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    listView.ScrollIntoView("Item 7");
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
                TestServices.WindowHelper.WaitForIdle();

                Log.Comment("Setting window content to null and forcing garbage collect");

                UIExecutor.Execute(() =>
                {
                    root = null;
                    header = null;
                    listView = null;
                    TestServices.WindowHelper.WindowContent = null;
                });
                TestServices.WindowHelper.WaitForIdle();
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
        }


        [TestMethod]
        [TestProperty("Description", "The focused element is inside a ScrollViewer and is occluded by another element in the ScrollViewer.")]
        [TestProperty("Hosting:Mode", "UAP")] // fails with a crash in WPF Islands
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void FocusedElementOccludedInScrollViewer()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))

            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                StackPanel stackPanel = null;
                Button button5 = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Grid Margin='10'>
                                <ScrollViewer Width='200' Height='200' Background='Navy' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>
                                    <Grid Width='200' Height='400'>
                                        <StackPanel x:Name='stackPanel'>
                                            <Button>button 1</Button>
                                            <Button>button 2</Button>
                                            <Button>button 3</Button>
                                            <Button>button 4</Button>
                                            <Button x:Name='button5'>button 5</Button>
                                            <Button>button 6</Button>
                                            <Button>button 7</Button>
                                        </StackPanel>
                                        <Rectangle Width='100' Height='100' Fill='Green' />
                                    </Grid>
                                </ScrollViewer>
                            </Grid>
                    </Page>");
                    stackPanel = (StackPanel)root.FindName("stackPanel");
                    button5 = (Button)root.FindName("button5");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    button5.Focus(FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.WindowHelper.SynchronouslyTickUIThread(2); // WaitForIdle doesn't always wait long enough for focus rects

                Log.Comment("When the focus rect escapes the clip of the ScrollViewer, it draws on top of the green rectangle.  We haven't fixed this case yet.");
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "1");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Give the focus rect enough room to fit within the StackPanel using the padding property.");
                    stackPanel.Padding = new Thickness(10);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "2");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Give the focus rect enough room to fit within the StackPanel using the margin property.");
                    stackPanel.Padding = new Thickness(0);
                    stackPanel.Margin = new Thickness(10);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "3");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Give the focus rect enough room to fit within the StackPanel by changing the the button's FocusVisualMargin.");
                    stackPanel.Padding = new Thickness(0);
                    stackPanel.Margin = new Thickness(0);
                    button5.FocusVisualMargin = new Thickness(0);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "4");
            }
        }

        // [TestMethod] 24023385
        [TestProperty("Description", "Ensure when a focused list view item scrolls out of view, the focus rect is clipped rather than nudged")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ListViewItemOnEdgeOfView()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))

            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                ListView listView = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Grid Margin='20' Height='100'>
                                <ListView x:Name='listView' Background='Blue'>
                                    <x:String>Item 1</x:String>
                                    <x:String>Item 2</x:String>
                                    <x:String>Item 3</x:String>
                                </ListView>
                            </Grid>
                    </Page>");
                    listView = (ListView)root.FindName("listView");
                    listView.Transitions = new Media.Animation.TransitionCollection();
                    listView.ItemContainerTransitions = new Media.Animation.TransitionCollection();
                    ScrollViewer.SetHorizontalScrollBarVisibility(listView, ScrollBarVisibility.Hidden);
                    ScrollViewer.SetVerticalScrollBarVisibility(listView, ScrollBarVisibility.Hidden);
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                XamlRoot xamlRoot = null;
                UIExecutor.Execute(() =>
                {
                    xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
                });

                TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Mouse, xamlRoot); // Don't show focus rect at first

                TestServices.KeyboardHelper.Tab();
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    listView.ScrollIntoView("Item 3");
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
            }
        }

        [TestMethod]
        [TestProperty("Description", "When a button is clipped by a parent, the focus rect should be nudged rather than clipped")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ButtonContainerClipsFocusRect()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))

            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Margin='20' Height='100' Background='Blue'>
                                <StackPanel.Clip>
                                    <RectangleGeometry Rect='0,0,400,30' />
                                </StackPanel.Clip>
                                <Button Margin='10'>I am a button</Button>
                            </StackPanel>
                        </Page>");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.KeyboardHelper.Tab(); // Get keyboard focus
                TestServices.WindowHelper.WaitForIdle();

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Bring newly focused MenuFlyoutItem into view when root element is a Canvas")]
        [TestProperty("Hosting:Mode", "UAP")]   
        public void BringFocusedMenuFlyoutItemIntoViewWithRootCanvas()
        {
            BringFocusedMenuFlyoutItemIntoView(useRootCanvas: true);
        }

        [TestMethod]
        [TestProperty("Description", "Bring newly focused MenuFlyoutItem into view when root element is a ScrollViewer")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void BringFocusedMenuFlyoutItemIntoViewWithRootScrollViewer()
        {
            BringFocusedMenuFlyoutItemIntoView(useRootCanvas: false);
        }

        private void BringFocusedMenuFlyoutItemIntoView(bool useRootCanvas)
        { 
            using (new TestCleanupWrapper())
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(300, 300));

                Button button = null;
                MenuFlyoutItem menuFlyoutItem1 = null;
                ScrollViewer scrollViewer = null;

                UIExecutor.Execute(() =>
                {
                    StackPanel stackPanel = XamlReader.Load(
                        @"<StackPanel Margin='10' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name='button' Content='Button'>
                                <Button.ContextFlyout>
                                    <MenuFlyout>
                                        <MenuFlyoutItem x:Name='menuFlyoutItem1' Height='165'>Item1</MenuFlyoutItem>
                                        <MenuFlyoutItem x:Name='menuFlyoutItem2' Height='165'>Item2</MenuFlyoutItem>
                                        <MenuFlyoutItem x:Name='menuFlyoutItem3' Height='165'>Item3</MenuFlyoutItem>
                                        <MenuFlyoutItem x:Name='menuFlyoutItem4' Height='165'>Item4</MenuFlyoutItem>
                                        <MenuFlyoutItem x:Name='menuFlyoutItem5' Height='165'>Item5</MenuFlyoutItem>
                                        <MenuFlyoutItem x:Name='menuFlyoutItem6' Height='165'>Item6</MenuFlyoutItem>
                                        <MenuFlyoutItem x:Name='menuFlyoutItem7' Height='165'>Item7</MenuFlyoutItem>
                                    </MenuFlyout>
                                </Button.ContextFlyout>
                            </Button>
                        </StackPanel>") as StackPanel;

                    button = stackPanel.FindName("button") as Button;
                    Verify.IsNotNull(button);

                    menuFlyoutItem1 = stackPanel.FindName("menuFlyoutItem1") as MenuFlyoutItem;
                    Verify.IsNotNull(menuFlyoutItem1);

                    if (useRootCanvas)
                    {
                        Canvas canvas = new Canvas()
                        {
                            Width = 300,
                            Height = 300
                        };

                        canvas.Children.Add(stackPanel);

                        TestServices.WindowHelper.WindowContent = canvas;

                    }
                    else
                    {
                        Page page = new Page();
                        page.Content = stackPanel;

                        TestServices.WindowHelper.WindowContent = page;
                    }
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.InputHelper.LeftMouseClick(button);
                TestServices.WindowHelper.WaitForIdle();

                try
                {
                    Log.Comment("Press shift-f10");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_shift#$d$_f10#$u$_f10#$u$_shift");

                    Log.Comment("Wait for menu flyout");
                    TestServices.WindowHelper.WaitForIdle();

                    UIExecutor.Execute(() =>
                    {
                        Verify.AreEqual(FocusState.Keyboard, menuFlyoutItem1.FocusState);

                        scrollViewer = menuFlyoutItem1.GetParentOfType<ScrollViewer>();
                        Verify.IsNotNull(scrollViewer);
                    });

                    for (uint item = 1; item < 7; item++)
                    {
                        Log.Comment("Press down arrow");
                        TestServices.KeyboardHelper.PressKeySequence("$d$_down#$u$_down");

                        Log.Comment("Wait for item menu flyout focus change and scroll into view");
                        TestServices.WindowHelper.WaitForIdle();

                        UIExecutor.Execute(() =>
                        {
                            Log.Comment("scrollViewer.VerticalOffset=" + scrollViewer.VerticalOffset);
                        });
                    }

                    UIExecutor.Execute(() =>
                    {
                        Verify.AreEqual(511.0, scrollViewer.VerticalOffset);
                    });
                }
                finally
                {
                    Log.Comment("Hide the flyout");
                    TestServices.KeyboardHelper.Escape();
                }
            }
        }

        // [TestMethod] 24023385
        [TestProperty("Description", "Once ContextMenyKey input opens up the flyout, focus rect should be visible on first list item")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ButtonFlyoutFocusRectOnKeyboardContextKeyInput()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))

            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));
                Page root = null;
                Button FocusedButton = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                             <StackPanel Margin='20' Height='100' Background='Blue'>
                                <Button x:Name='FocusedButton'>FocusedButton
                                    <Button.ContextFlyout>
                                        <Flyout>
                                            <ListView>
                                                <ListViewItem >Item</ListViewItem>
                                                <ListViewItem>Item2</ListViewItem>
                                                <ListViewItem>Item 3</ListViewItem>
                                            </ListView>
                                        </Flyout>
                                    </Button.ContextFlyout>
                                </Button>
                            </StackPanel>
                        </Page>");

                    FocusedButton = (Button)root.FindName("FocusedButton");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                FocusHelper.EnsureFocus(FocusedButton, FocusState.Keyboard);

                try
                {
                    Log.Comment("Press ContextMenu Key");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_apps#$u$_apps"); // Get keyboard focus.
                    TestServices.KeyboardHelper.PressKeySequence("$d$_apps#$u$_apps"); // Should not take focus rect away.

                    TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
                }
                finally
                {
                    Log.Comment("Hide the flyout");
                    TestServices.KeyboardHelper.Escape();
                }
            }
        }

        // [TestMethod] 24023385
        [TestProperty("Description", "When releasing long pressed GamepadMenuKey should not take focus rect away from first list item")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ButtonFlyoutFocusRectOnGamepadMenuKeyInput()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))

            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));
                Page root = null;
                Button FocusedButton = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                             <StackPanel Margin='20' Height='100' Background='Blue'>
                                <Button x:Name='FocusedButton'>FocusedButton
                                    <Button.ContextFlyout>
                                        <Flyout>
                                            <ListView>
                                                <ListViewItem >Item</ListViewItem>
                                                <ListViewItem>Item2</ListViewItem>
                                                <ListViewItem>Item 3</ListViewItem>
                                            </ListView>
                                        </Flyout>
                                    </Button.ContextFlyout>
                                </Button>
                            </StackPanel>
                        </Page>");

                    FocusedButton = (Button)root.FindName("FocusedButton");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                FocusHelper.EnsureFocus(FocusedButton, FocusState.Keyboard);

                try
                {
                    Log.Comment("Press ContextMenu Key");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_GamePadMenu#$d$_GamePadMenu#$d$_GamePadMenu#$d$_GamePadMenu#$d$_GamePadMenu#$u$_GamePadMenu");
                    TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
                }
                finally
                {
                    Log.Comment("Hide the flyout");
                    TestServices.KeyboardHelper.Escape();
                }
            }
        }


        [TestMethod]
        [TestProperty("Description", "When the user presses shift-f10 to show a button flyout, the FocusState of the MenuFlyoutItem should be 'Keyboard' so that the focus rect shows")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ButtonContextFlyoutShowsFocusRectAfterShiftF10()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: false,
                resetDevice: false))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                Button button = null;
                MenuFlyoutItem theMenuFlyoutItem = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Margin='20' Height='100' Background='Black'>
                                <Button x:Name='button'>I am a button
                                    <Button.ContextFlyout>
                                        <MenuFlyout>
                                            <MenuFlyoutItem x:Name='theMenuFlyoutItem'>Item 1</MenuFlyoutItem>
                                            <MenuFlyoutItem>Item 2</MenuFlyoutItem>
                                            <MenuFlyoutItem>Item 3</MenuFlyoutItem>
                                        </MenuFlyout>
                                    </Button.ContextFlyout>
                                </Button>
                            </StackPanel>
                        </Page>");
                    button = (Button)root.FindName("button");
                    theMenuFlyoutItem = (MenuFlyoutItem)root.FindName("theMenuFlyoutItem");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                TestServices.InputHelper.LeftMouseClick(button);

                try
                {
                    Log.Comment("Press shift-f10");
                    TestServices.KeyboardHelper.PressKeySequence("$d$_shift#$d$_f10#$u$_f10#$u$_shift");

                    Log.Comment("Wait for menu flyout");
                    TestServices.WindowHelper.WaitForIdle();

                    UIExecutor.Execute(() =>
                    {
                        Verify.AreEqual(FocusState.Keyboard, theMenuFlyoutItem.FocusState);
                    });
                    TestServices.WindowHelper.WaitForIdle();
                }
                finally
                {
                    Log.Comment("Hide the flyout");
                    TestServices.KeyboardHelper.Escape();
                }
            }
        }

        // [TestMethod] 24023385
        [TestProperty("Description", "If the UseSystemFocusVisuals resource is overridden to false, we should not render a focus rect.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void UseSystemFocusVisualsStaticResource()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            using (new OverrideAppResources(@"
                <ResourceDictionary xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <x:Boolean x:Key='UseSystemFocusVisuals'>False</x:Boolean>
                </ResourceDictionary>"))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                Button btn = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel>
                                <Button x:Name='btn' Content='button'/>
                            </StackPanel>
                    </Page>");

                    btn = (Button)root.FindName("btn");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                XamlRoot xamlRoot = null;
                UIExecutor.Execute(() =>
                {
                    xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
                });

                TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Mouse, xamlRoot); // Don't show focus rect at first

                FocusHelper.EnsureFocus(btn, FocusState.Keyboard);

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ValidateFocusRectInListView()
        {
            // In this test, we validate that the focus rect is displayed correctly in
            // ListView and GridView. The test setup flat and grouped scenarios, scrollable
            // and non-scrollable lists. We validate that sticky headers clip the focus rect.

            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                TestServices.InputHelper.MoveMouse(new Point(0, 0)); // Move the pointer out of the way.
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(500, 500));

                foreach (var isGrid in new bool[] { false, true })
                {
                    ListViewBase list = null;
                    var rootLoadedEvent = new AutoResetEvent(initialState: false);
                    UIExecutor.Execute(() =>
                    {
                        list = isGrid ? (ListViewBase)new GridView() : new ListView();

                        list.Width = 400;
                        list.Height = 400;
                        list.Background = new SolidColorBrush(Colors.Gray);

                        // To speed up test execution, we disable the cache buffer.
                        if (isGrid)
                        {
                            list.ItemsPanel = (ItemsPanelTemplate)XamlReader.Load(
                                @"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                    <ItemsWrapGrid CacheLength='0' Orientation='Horizontal' ItemWidth='100' ItemHeight='100' />
                                  </ItemsPanelTemplate>");
                        }
                        else
                        {
                            list.ItemsPanel = (ItemsPanelTemplate)XamlReader.Load(
                                @"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                    <ItemsStackPanel CacheLength='0' />
                                  </ItemsPanelTemplate>");
                        }

                        var root = new Grid();
                        root.Width = 500;
                        root.Height = 500;
                        root.Children.Add(list);

                        TestServices.WindowHelper.WindowContent = root;

                        root.Loaded += delegate
                        {
                            Log.Comment("Root loaded.");
                            rootLoadedEvent.Set();
                        };
                    });
                    Verify.IsTrue(rootLoadedEvent.WaitOne(TimeSpan.FromSeconds(5)));

                    foreach (var isGrouped in new bool[] { false, true })
                    {
                        if (isGrouped)
                        {
                            UIExecutor.Execute(() =>
                            {
                                list.GroupStyle.Add(new GroupStyle
                                {
                                    HeaderTemplate = (DataTemplate)XamlReader.Load(
                                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                                            <TextBlock Text='{Binding Name}' />
                                          </DataTemplate>")
                                });
                            });
                        }

                        foreach (var isScrollable in new bool[] { false, true })
                        {
                            UIExecutor.Execute(() =>
                            {
                                var viewSource = new CollectionViewSource();
                                viewSource.IsSourceGrouped = isGrouped;
                                viewSource.Source = isGrouped ?
                                    (object)new List<Group>(
                                        Enumerable.Range(0, isScrollable ? 10 : 1).Select(i => new Group(
                                            $"Group #{i}",
                                            Enumerable.Range(0, 4).Select(j => $"Item {i}.{j}")))) :
                                    new List<string>(Enumerable.Range(0, isScrollable ? 25 : 4).Select(i => $"Item #{i}"));

                                list.ItemsSource = viewSource.View;

                                // To speed up test execution, we disable animations.
                                list.ItemContainerTransitions = new TransitionCollection();
                            });
                            TestServices.WindowHelper.WaitForIdle();

                            UIExecutor.Execute(() =>
                            {
                                ((Control)list.ContainerFromIndex(0)).Focus(FocusState.Keyboard);
                            });

                            TestServices.WindowHelper.SynchronouslyTickUIThread(2); // Wait for vertical scrollbar to start disappearing
                            TestServices.WindowHelper.WaitForIdle();

                            TestServices.Utilities.VerifyMockDCompOutput(
                                    MockDComp.SurfaceComparison.NoComparison,
                                    $"{(isGrid ? "grid" : "list")}_{(isGrouped ? "grouped" : "flat")}_{(isScrollable ? "scrollable" : "not_scrollable")}");

                            if (isScrollable)
                            {
                                UIExecutor.Execute(() =>
                                {
                                    var scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                                    scrollViewer.ChangeView(null, 20.0, null, disableAnimation: true);
                                });

                                TestServices.WindowHelper.SynchronouslyTickUIThread(2); // Wait for vertical scrollbar to start disappearing
                                TestServices.WindowHelper.WaitForIdle();

                                TestServices.Utilities.VerifyMockDCompOutput(
                                    MockDComp.SurfaceComparison.NoComparison,
                                    $"{(isGrid ? "grid" : "list")}_{(isGrouped ? "grouped" : "flat")}_{(isScrollable ? "scrollable" : "not_scrollable")}_offset20");
                            }
                        }
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // fails with a crash in WPF islands
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void ValidateFocusRectInThirdPartyScrollingSurface()
        {
            // In this test, we setup a third party horizontal scrolling surface with a fake
            // scroll bar. We validate that the focus rect is clipped horizontally by the
            // scrolling surface. We also validate that focus rect is occluded by the scroll bar.

            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                TestServices.InputHelper.MoveMouse(new Point(0, 0)); // Move the pointer out of the way.
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(200, 200));

                ScrollingContentControl scrollingSurface = null;
                var buttons = new Button[3];

                UIExecutor.Execute(() =>
                {
                    var panel = new StackPanel();
                    panel.Orientation = Orientation.Horizontal;

                    for (int i = 0; i < buttons.Length; ++i)
                    {
                        buttons[i] = new Button { Content = $"Button {i}", Width = 100, VerticalAlignment = VerticalAlignment.Stretch };
                        panel.Children.Add(buttons[i]);
                    }

                    scrollingSurface = new ScrollingContentControl();
                    scrollingSurface.Content = panel;
                    scrollingSurface.VerticalContentAlignment = VerticalAlignment.Stretch;
                    scrollingSurface.UpdateConfiguration(isHorizontallyScrollable: true, isVerticallyScrollable: false);

                    var scrollBar = new Rectangle();
                    scrollBar.Height = 10;
                    scrollBar.Fill = new SolidColorBrush(Colors.Crimson);
                    scrollBar.HorizontalAlignment = HorizontalAlignment.Stretch;
                    scrollBar.VerticalAlignment = VerticalAlignment.Bottom;

                    var grid = new Grid();
                    grid.Width = 150;
                    grid.Height = 100;
                    grid.Background = new SolidColorBrush(Colors.Gray);
                    grid.Children.Add(scrollingSurface);
                    grid.Children.Add(scrollBar);

                    TestServices.WindowHelper.WindowContent = grid;
                });
                TestServices.WindowHelper.WaitForIdle();

                FocusHelper.EnsureFocus(buttons[0], FocusState.Keyboard);
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "left");

                FocusHelper.EnsureFocus(buttons[1], FocusState.Keyboard);
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "center");

                UIExecutor.Execute(() =>
                {
                    scrollingSurface.Presenter.RenderTransform = new TranslateTransform { X = -150 };
                });
                FocusHelper.EnsureFocus(buttons[2], FocusState.Keyboard);
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "right");
            }

        }

        [TestMethod]
        [TestProperty("Description", "Validate rendering of focus rect on basic UIElements")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to rendering scopeguard not working in WPF yet
        async public void UIElementTests()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                StackPanel sp1 = null;
                StackPanel sp2 = null;
                TextBlock tb = null;
                RichTextBlock rtb = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel>
                            <StackPanel x:Name='sp1' HorizontalAlignment = 'Left' Width='50' Height='20' Background='Red' IsTabStop='True' UseSystemFocusVisuals='True'/>
                            <StackPanel x:Name='sp2' HorizontalAlignment = 'Left' Width='50' Height='20' Background='Green' IsTabStop='True' UseSystemFocusVisuals='True'/>
                            <TextBlock x:Name='tb' Text='TextBlock' IsTabStop='True' UseSystemFocusVisuals='True'/>
                            <RichTextBlock x:Name='rtb' IsTabStop='True' UseSystemFocusVisuals='True'>
                              <Paragraph>RichTextBlock</Paragraph>
                            </RichTextBlock>
                        </StackPanel>
                    </Page>");
                    sp1 = (StackPanel)root.FindName("sp1");
                    sp2 = (StackPanel)root.FindName("sp2");
                    tb = (TextBlock)root.FindName("tb");
                    rtb = (RichTextBlock)root.FindName("rtb");
                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "nofocus");

                await UIExecutor.ExecuteAsync(async () =>
                {
                    await FocusManager.TryFocusAsync(sp1, FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "sp1");

                await UIExecutor.ExecuteAsync(async () =>
                {
                    await FocusManager.TryFocusAsync(sp2, FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "sp2");

                await UIExecutor.ExecuteAsync(async () =>
                {
                    await FocusManager.TryFocusAsync(tb, FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "tb");

                await UIExecutor.ExecuteAsync(async () =>
                {
                    await FocusManager.TryFocusAsync(rtb, FocusState.Keyboard);
                });
                TestServices.WindowHelper.WaitForIdle();
                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, "rtb");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates that round focus rectangles work")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode due to rendering scopeguard not working in WPF yet
        public void ValidateRoundedFocusRect()
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
            {
                TestServices.WindowHelper.SetWindowSizeOverride(new Size(400, 400));

                Page root = null;
                Grid targetGrid = null;
                Control targetControl = null;
                Border targetControlBorder = null;

                UIExecutor.Execute(() =>
                {
                    root = (Page)XamlReader.Load(
                        @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Grid>
                            <Grid x:Name='targetGrid' Margin='20,20,20,20' HorizontalAlignment='Left' VerticalAlignment='Top'>
                                <Button x:Name='targetButton' Height='50' Width='100'/>
                             </Grid>
                        </Grid>
                    </Page>");
                    targetGrid = (Grid)root.FindName("targetGrid");
                    targetControl = (Control)root.FindName("targetButton");
                    TestServices.WindowHelper.WindowContent = root;
                });

                TestServices.WindowHelper.WaitForIdle();

                XamlRoot xamlRoot = null;

                UIExecutor.Execute(() =>
                {
                    xamlRoot = TestServices.WindowHelper.WindowContent.XamlRoot;
                });

                Log.Comment("Validate Button without corner radius");
                ValidateCornerRadius(xamlRoot, targetControl, "ButtonNoRounding");

                Log.Comment("Validate Button with corner radius");
                UIExecutor.Execute(() =>
                {
                    targetControl.CornerRadius = new CornerRadius(5, 5, 5, 5);
                });
                ValidateCornerRadius(xamlRoot, targetControl, "Button");

                // It is possible for application to override the FocusVisualMargin property in a way that produces bad corner
                // radius values for the focus rect.  Make sure that we are handling this.
                //
                Log.Comment("Validate Button with zero focus visual margin");
                UIExecutor.Execute(() =>
                {
                    targetControl.CornerRadius = new CornerRadius(.5, .5, .5, .5);
                    targetControl.FocusVisualMargin = new Thickness(0, 0, 0, 0);
                });
                ValidateCornerRadius(xamlRoot, targetControl, "ZeroFocusVisualMargin");

                Log.Comment("Validate UserControl without target or corner radius");
                UIExecutor.Execute(() =>
                {
                    targetControlBorder = new Border();
                    targetControlBorder.MinHeight = 50;
                    targetControlBorder.MinWidth = 100;
                    targetControlBorder.Background = new SolidColorBrush(Colors.Gray);

                    UserControl userControl = new UserControl();
                    userControl.Content = targetControlBorder;
                    userControl.UseSystemFocusVisuals = true;
                    userControl.AllowFocusOnInteraction = true;
                    userControl.IsTabStop = true;

                    targetControl = userControl;
                    targetGrid.Children.Clear();
                    targetGrid.Children.Add(targetControl);
                });
                ValidateCornerRadius(xamlRoot, targetControl, "UserControlNoRounding");

                Log.Comment("Validate UserControl without target");
                UIExecutor.Execute(() =>
                {
                    targetControl.CornerRadius = new CornerRadius(20, 20, 20, 20); // Use an extra large corner radius so when we add a target we know which radius is used
                });
                ValidateCornerRadius(xamlRoot, targetControl, "UserControl");

                // Note: for target tests, we don't bother with the no corner radius scenario because we have already tested
                //       the "whether or not" code paths previously.

                Log.Comment("Validate Border Target");
                UIExecutor.Execute(() =>
                {
                    Border border = new Border();
                    border.Background = new SolidColorBrush(Colors.Green);
                    border.CornerRadius = new CornerRadius(5, 5, 5, 5);
                    border.Margin = new Thickness(5, 5, 5, 5);
                    border.Height = 50;
                    border.Width = 100;
                    Control.SetIsTemplateFocusTarget(border, true);
                    targetControlBorder.Child = border;
                });
                ValidateCornerRadius(xamlRoot, targetControl, "BorderTarget");

                Log.Comment("Validate Grid Target");
                UIExecutor.Execute(() =>
                {
                    Grid grid = new Grid();
                    grid.Background = new SolidColorBrush(Colors.Green);
                    grid.CornerRadius = new CornerRadius(5, 5, 5, 5);
                    grid.Margin = new Thickness(5, 5, 5, 5);
                    grid.Height = 50;
                    grid.Width = 100;
                    Control.SetIsTemplateFocusTarget(grid, true);
                    targetControlBorder.Child = grid;
                });
                ValidateCornerRadius(xamlRoot, targetControl, "GridTarget");


                Log.Comment("Validate StackPanel Target");
                UIExecutor.Execute(() =>
                {
                    StackPanel stackPanel = new StackPanel();
                    stackPanel.Background = new SolidColorBrush(Colors.Green);
                    stackPanel.CornerRadius = new CornerRadius(5, 5, 5, 5);
                    stackPanel.Margin = new Thickness(5, 5, 5, 5);
                    stackPanel.Height = 50;
                    stackPanel.Width = 100;
                    Control.SetIsTemplateFocusTarget(stackPanel, true);
                    targetControlBorder.Child = stackPanel;
                });
                ValidateCornerRadius(xamlRoot, targetControl, "StackPanelTarget");

                Log.Comment("Validate RelativePanel Target");
                UIExecutor.Execute(() =>
                {
                    RelativePanel relativePanel = new RelativePanel();
                    relativePanel.Background = new SolidColorBrush(Colors.Green);
                    relativePanel.CornerRadius = new CornerRadius(5, 5, 5, 5);
                    relativePanel.Margin = new Thickness(5, 5, 5, 5);
                    relativePanel.Height = 50;
                    relativePanel.Width = 100;
                    Control.SetIsTemplateFocusTarget(relativePanel, true);
                    targetControlBorder.Child = relativePanel;
                });
                ValidateCornerRadius(xamlRoot, targetControl, "RelativePanelTarget");

                Log.Comment("Validate Panel Target");
                UIExecutor.Execute(() =>
                {
                    CustomCornerRadiusPanel customPanel = new CustomCornerRadiusPanel();
                    customPanel.Background = new SolidColorBrush(Colors.Green);
                    customPanel.Margin = new Thickness(5, 5, 5, 5);
                    customPanel.Height = 50;
                    customPanel.Width = 100;
                    Control.SetIsTemplateFocusTarget(customPanel, true);
                    targetControlBorder.Child = customPanel;
                });
                ValidateCornerRadius(xamlRoot, targetControl, "PanelTarget");
            }
        }
        public partial class CustomCornerRadiusPanel : Panel
        {
            public CustomCornerRadiusPanel()
            {
                CornerRadiusProtected = new CornerRadius(5, 5, 5, 5);
            }
        }

        private void ValidateCornerRadius(XamlRoot xamlRoot, Control targetControl, String compareLabel)
        {
            TestServices.WindowHelper.WaitForIdle();

            TestServices.WindowHelper.SetLastInputMethod(global::Private.Infrastructure.LastInputDeviceType.Keyboard, xamlRoot); // ensure focus rect

            UIExecutor.Execute(() =>
            {
                targetControl.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison, compareLabel);
        }

        public class Group : List<string>
        {
            public string Name { get; }

            public Group(string name, IEnumerable<string> items)
                : base(items)
            {
                Name = name;
            }
        }
    }
}

