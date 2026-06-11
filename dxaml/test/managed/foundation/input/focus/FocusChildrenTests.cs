// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Linq;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Focus
{
    [TestClass]
    public partial class FocusChildrenTests : XamlTestsBase
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
        public void ValidateGetChildrenInTabFocusOrderAPI()
        {
            UIExecutor.Execute(() =>
            {
                Verify.IsNull((new CustomFocusChildrenPanel()).InvokeBaseGetChildrenInTabFocusOrder());

                var panel = new CustomFocusChildrenPanel();

                panel.Children.Add(new Button());
                Verify.IsTrue(panel.InvokeBaseGetChildrenInTabFocusOrder().SequenceEqual(panel.Children));

                panel.Children.Add(new Button());
                Verify.IsTrue(panel.InvokeBaseGetChildrenInTabFocusOrder().SequenceEqual(panel.Children));

                panel.Children.Clear();
                Verify.IsNull(panel.InvokeBaseGetChildrenInTabFocusOrder());
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void ValidateDefaultChildrenCollectionDuringTabNavigation()
        {
            Grid root = null;
            object[] expectedFocusedElementSequence = null;
            const string visualTreeXaml = @"
                <Grid HorizontalAlignment='Left' Width='300' Height='30' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Grid.ColumnDefinitions>
                        <ColumnDefinition />
                        <ColumnDefinition />
                        <ColumnDefinition />
                    </Grid.ColumnDefinitions>
                    <Button x:Name='start'>Start</Button>
                    <RichTextBlock x:Name='rtb' Grid.Column='0' FontSize='16' OverflowContentTarget='{ Binding ElementName=overflowContainer}' >
                        <Paragraph>
                            <Hyperlink x:Name='hyperlink1' > This is a long text</Hyperlink>
                            <Hyperlink x:Name='hyperlink2' > that will overflow.</Hyperlink>
                        </Paragraph>
                    </RichTextBlock>
                    <Rectangle HorizontalAlignment='Stretch' VerticalAlignment='Stretch' Grid.Column='1' Fill='Gold' />
                    <RichTextBlockOverflow x:Name='overflowContainer' Grid.Column='1' />
                    <TextBlock Grid.Column='2'><Run>Try</Run><Hyperlink x:Name='hyperlink3' NavigateUri='http://bing.com'> Bing.com</Hyperlink><Run>today!</Run></TextBlock>
                </Grid>";

            UIExecutor.Execute(() =>
            {
                root = (Grid)XamlReader.Load(visualTreeXaml);
                expectedFocusedElementSequence = new object[]
                {
                    root.FindName("start"),
                    root.FindName("hyperlink1"),
                    root.FindName("hyperlink2"),
                    root.FindName("hyperlink3")
                };

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            ValidateFocusSequence(expectedFocusedElementSequence);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]   // Focus engagement bugs in lifted islands
        public void ValidateCustomChildrenCollectionDuringTabNavigation()
        {
            StackPanel root = null;
            StackPanel defaultTabOrderPanel = null;
            CustomFocusChildrenPanel customTabOrderPanel = null;
            CustomFocusChildrenPanel customTabOrderPanelWithTabIndex = null;
            object[] expectedFocusedElementSequence = null;

            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                defaultTabOrderPanel = new StackPanel { Orientation = Orientation.Horizontal };
                customTabOrderPanel = new CustomFocusChildrenPanel((p) => p.Children.Reverse());
                customTabOrderPanelWithTabIndex = new CustomFocusChildrenPanel((p) => p.Children.Reverse());

                foreach (var panel in new Panel[] { defaultTabOrderPanel, customTabOrderPanel, customTabOrderPanelWithTabIndex })
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        var button = new Button { Content = $"Button {i}" };
                        panel.Children.Add(button);

                        if (panel == customTabOrderPanelWithTabIndex)
                        {
                            button.TabIndex = (i + 1) % 3;
                        }
                    }
                    root.Children.Add(panel);
                }

                expectedFocusedElementSequence = new object[]
                {
                    customTabOrderPanelWithTabIndex.Children[2], customTabOrderPanelWithTabIndex.Children[0], customTabOrderPanelWithTabIndex.Children[1],
                    defaultTabOrderPanel.Children[0], defaultTabOrderPanel.Children[1], defaultTabOrderPanel.Children[2],
                    customTabOrderPanel.Children[2], customTabOrderPanel.Children[1], customTabOrderPanel.Children[0]
                };

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            ValidateFocusSequence(expectedFocusedElementSequence);
        }

        [TestMethod]
        public void ValidateXYFocusDoesntInvokeGetChildrenInTabFocusOrder()
        {
            CustomFocusChildrenPanel panel = null;
            Button button1 = null;
            Button button2 = null;

            UIExecutor.Execute(() =>
            {
                panel = new CustomFocusChildrenPanel((p) => new[] { p.Children[0] });
                button1 = new Button { Content = "First Button" };
                button2 = new Button { Content = "Second Button" };

                panel.XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled;

                panel.Children.Add(button1);
                panel.Children.Add(button2);
                TestServices.WindowHelper.WindowContent = panel;
            });
            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);
            panel.ResetGetChildrenInTabFocusOrderInvokeCount();

            TestServices.KeyboardHelper.GamepadDpadRight();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(button2, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                Verify.AreEqual(0, panel.GetChildrenInTabFocusOrderInvokeCount);
            });

            TestServices.KeyboardHelper.GamepadDpadLeft();
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(button1, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                Verify.AreEqual(0, panel.GetChildrenInTabFocusOrderInvokeCount);
            });
        }

        private void ValidateFocusSequence(object[] sequence)
        {
            FocusHelper.EnsureFocus(sequence.Last(), FocusState.Keyboard);

            Log.Comment("Cycle through the visual tree programmatically with FocusManager.TryMoveFocus.");
            UIExecutor.Execute(() =>
            {
                FindNextElementOptions options = new FindNextElementOptions();
                options.SearchRoot = TestServices.WindowHelper.WindowContent;
                foreach (var element in sequence)
                {
                    FocusManager.TryMoveFocus(FocusNavigationDirection.Next, options);
                    Verify.AreEqual(element, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                }

                foreach (var element in sequence.Reverse())
                {
                    Verify.AreEqual(element, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    FocusManager.TryMoveFocus(FocusNavigationDirection.Previous, options);
                }
            });

            Log.Comment("Cycle through the visual tree with TAB and SHIFT-TAB input.");
            {
                foreach (var element in sequence)
                {
                    TestServices.KeyboardHelper.Tab();
                    TestServices.WindowHelper.WaitForIdle();

                    UIExecutor.Execute(() =>
                    {
                        Verify.AreEqual(element, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    });
                }

                foreach (var element in sequence.Reverse())
                {
                    UIExecutor.Execute(() =>
                    {
                        Verify.AreEqual(element, FocusManager.GetFocusedElement(TestServices.WindowHelper.WindowContent.XamlRoot));
                    });

                    TestServices.KeyboardHelper.ShiftTab();
                    TestServices.WindowHelper.WaitForIdle();
                }
            }

            Log.Comment("Validate first/last focusable element.");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(sequence.First(), FocusManager.FindFirstFocusableElement(TestServices.WindowHelper.WindowContent));
                Verify.AreEqual(sequence.Last(), FocusManager.FindLastFocusableElement(TestServices.WindowHelper.WindowContent));
            });
        }

        private partial class CustomFocusChildrenPanel : StackPanel
        {
            Func<CustomFocusChildrenPanel, IEnumerable<DependencyObject>> _getChildrenInTabFocusOrderFunc;

            public int GetChildrenInTabFocusOrderInvokeCount { get; private set; }

            public CustomFocusChildrenPanel()
            {
            }

            public CustomFocusChildrenPanel(Func<CustomFocusChildrenPanel, IEnumerable<DependencyObject>> getChildrenInTabFocusOrderFunc)
            {
                _getChildrenInTabFocusOrderFunc = getChildrenInTabFocusOrderFunc;
                Orientation = Orientation.Horizontal;
            }

            public IEnumerable<DependencyObject> InvokeBaseGetChildrenInTabFocusOrder()
            {
                return base.GetChildrenInTabFocusOrder();
            }

            public void ResetGetChildrenInTabFocusOrderInvokeCount()
            {
                GetChildrenInTabFocusOrderInvokeCount = 0;
            }

            protected override IEnumerable<DependencyObject> GetChildrenInTabFocusOrder()
            {
                ++GetChildrenInTabFocusOrderInvokeCount;
                return _getChildrenInTabFocusOrderFunc(this);
            }
        }
    }
}
