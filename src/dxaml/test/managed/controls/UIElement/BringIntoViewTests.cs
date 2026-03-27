// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml;
using Windows.Foundation;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls.Primitives;

using System.Runtime.InteropServices.WindowsRuntime;


namespace Microsoft.UI.Xaml.Tests.Controls.BringIntoViewTests
{
    [TestClass]
    public partial class BringIntoViewTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        StringBuilder eventOrder;

        [TestMethod]
        [TestProperty("Description", "Validates that BringIntoViewRequested does not trigger twice on the same UIElement/Control combo.")]
        public void VerifyBringIntoViewRequestedDoesNotDoubleFire()
        {
            StackPanel root = null;
            StackPanelWithOverride spOverride = null;
            ButtonWithOverride buOverride = null;

            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                spOverride = new StackPanelWithOverride();
                buOverride = new ButtonWithOverride();
                buOverride.Width = 100;
                buOverride.Height = 100;

                spOverride.Children.Add(buOverride);
                root.Children.Add(spOverride);
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            var onBringIntoViewRequestedHandler = new Action<object, BringIntoViewRequestedEventArgs>((source, args) =>
            {
                Log.Comment("BringIntoViewRequested fired");

                Verify.IsTrue(args.TargetRect.Width == 110);
                Verify.IsTrue(args.TargetRect.Height == 110);
            });

            using (var BringIntoViewRequested = new EventTester<StackPanel, BringIntoViewRequestedEventArgs>(root, "BringIntoViewRequested", onBringIntoViewRequestedHandler))
            {
                UIExecutor.Execute(() =>
                {
                    buOverride.StartBringIntoView();
                });
                BringIntoViewRequested.Wait();
                Verify.IsTrue(buOverride.HasFired);
                Verify.IsTrue(spOverride.HasFired);
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates that BringIntoViewRequested calls in the correct order.")]
        public void VerifyBringIntoViewRequestedOrder()
        {
            StackPanel root = null;
            StackPanelWithOverride spOverride = null;
            ButtonWithOverride buOverride = null;
            eventOrder = new StringBuilder();

            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                root.Name = "root";
                spOverride = new StackPanelWithOverride();
                spOverride.BringIntoViewRequested += UIElementBringIntoViewRequestedHandler;
                spOverride.eventOrder = eventOrder;
                spOverride.Name = "spOverride";
                buOverride = new ButtonWithOverride();
                buOverride.BringIntoViewRequested += ControlBringIntoViewRequestedHandler;
                buOverride.eventOrder = eventOrder;
                buOverride.Name = "buOverride";
                buOverride.Width = 100;
                buOverride.Height = 100;

                spOverride.Children.Add(buOverride);
                root.Children.Add(spOverride);
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            using (var BringIntoViewRequested = new EventTester<StackPanel, BringIntoViewRequestedEventArgs>(root, "BringIntoViewRequested"))
            {
                UIExecutor.Execute(() =>
                {
                    buOverride.StartBringIntoView();
                });
                BringIntoViewRequested.Wait();

                Verify.AreEqual("[buOverrideOnBringIntoViewRequested][buOverrideBringIntoViewRequested][spOverrideOnBringIntoViewRequested][spOverrideBringIntoViewRequested]", eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates that OriginalSource is not null.")]
        [TestProperty("Hosting:Mode", "UAP")]
        public void VerifyBringIntoViewRequestedOriginialSourceIsNotNull()
        {
            StackPanel root = null;
            Button button = null;


            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                StackPanel stackPanel = new StackPanel();
                button = new Button();
                button.Width = 100;
                button.Height = 100;

                stackPanel.Children.Add(button);
                root.Children.Add(stackPanel);
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            var onBringIntoViewRequestedHandler = new Action<object, BringIntoViewRequestedEventArgs>((source, args) =>
            {
                Log.Comment("BringIntoViewRequested fired");

                Verify.IsTrue(args.OriginalSource != null);
                Verify.AreEqual(args.OriginalSource, button);
            });

            using (var BringIntoViewRequested = new EventTester<StackPanel, BringIntoViewRequestedEventArgs>(root, "BringIntoViewRequested", onBringIntoViewRequestedHandler))
            {
                UIExecutor.Execute(() =>
                {
                    button.StartBringIntoView();
                });
                BringIntoViewRequested.Wait();
            }

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates that BringIntoViewRequested still fires if it was handled lower in the tree.")]
        public void VerifyBringIntoViewRequestedOrderWithHandling()
        {
            StackPanel root = null;
            StackPanelWithOverride spOverride = null;
            ButtonWithOverride buOverride = null;
            eventOrder = new StringBuilder();

            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                root.Name = "root";
                spOverride = new StackPanelWithOverride();
                spOverride.AddHandler(UIElement.BringIntoViewRequestedEvent, new TypedEventHandler<UIElement, BringIntoViewRequestedEventArgs>(UIElementBringIntoViewRequestedHandler), true);
                spOverride.eventOrder = eventOrder;
                spOverride.Name = "spOverride";
                buOverride = new ButtonWithOverride();
                buOverride.AddHandler(UIElement.BringIntoViewRequestedEvent, new TypedEventHandler<UIElement, BringIntoViewRequestedEventArgs>(ControlBringIntoViewRequestedHandler), false);
                buOverride.eventOrder = eventOrder;
                buOverride.Name = "buOverride";
                buOverride.Width = 100;
                buOverride.Height = 100;
                buOverride.ShouldHandle = true;

                spOverride.Children.Add(buOverride);
                root.Children.Add(spOverride);
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                buOverride.StartBringIntoView();
            });
            TestServices.WindowHelper.WaitForIdle();

            Verify.AreEqual("[buOverrideHandled][buOverrideOnBringIntoViewRequested][spOverrideBringIntoViewRequested]", eventOrder.ToString());
        }

        [TestMethod]
        [TestProperty("Description", "Validates BringIntoViewRequested eventing for altered BringIntoViewOptions.")]
        public void VerifyBringIntoViewRequestedWithAlteredOptions()
        {
            StackPanel root = null;
            CanvasWithOverride cvOverride = null;
            ButtonWithOverride buOverride = null;
            eventOrder = new StringBuilder();

            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                root.Name = "root";
                cvOverride = new CanvasWithOverride();
                cvOverride.BringIntoViewRequested += UIElementBringIntoViewRequestedHandler;
                cvOverride.eventOrder = eventOrder;
                cvOverride.Name = "cvOverride";
                buOverride = new ButtonWithOverride();
                buOverride.BringIntoViewRequested += ControlBringIntoViewRequestedHandler;
                buOverride.eventOrder = eventOrder;
                buOverride.Name = "buOverride";
                buOverride.Width = 100;
                buOverride.Height = 100;

                cvOverride.Children.Add(buOverride);
                root.Children.Add(cvOverride);
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            using (var BringIntoViewRequested = new EventTester<StackPanel, BringIntoViewRequestedEventArgs>(root, "BringIntoViewRequested"))
            {
                UIExecutor.Execute(() =>
                {
                    BringIntoViewOptions options = new BringIntoViewOptions();
                    Rect myRect = new Rect();
                    myRect.X = 10;
                    myRect.Y = 11;
                    myRect.Width = 12;
                    myRect.Height = 13;

                    options.TargetRect = myRect;
                    options.HorizontalAlignmentRatio = 0.65;
                    options.VerticalAlignmentRatio = double.NaN;
                    options.HorizontalOffset = -12.25;
                    options.VerticalOffset = 7.75;
                    options.AnimationDesired = false;

                    buOverride.StartBringIntoView(options);
                });
                BringIntoViewRequested.Wait();

                Verify.AreEqual("[buOverrideOnBringIntoViewRequested][buOverrideBringIntoViewRequested][cvOverrideOnBringIntoViewRequested][cvOverrideBringIntoViewRequested]", eventOrder.ToString());
            }

            TestServices.WindowHelper.WaitForIdle();
        }
        
        public void ValidateTargetRectForThirdPartyScrollers()
        {
            UIElement scroller = null;
            Button buttonToFocus = null;
            List<Rect> targetRects = new List<Rect>();

            UIExecutor.Execute(() =>
            {
                var root = (Grid)XamlMarkup.XamlReader.Load(
                @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <Button Content='Button 1' />
                    <Border x:Name='scroller' Width='200' Height='400'>
                      <StackPanel Height='800'>
                        <Button x:Name='buttonToFocus' Content='Button 2' HorizontalAlignment='Stretch' Height='40' />
                      </StackPanel>
                    </Border>
                  </Grid>");

                scroller = (UIElement)root.FindName("scroller");
                buttonToFocus = (Button)root.FindName("buttonToFocus");
                UIElement.RegisterAsScrollPort(scroller);

                scroller.BringIntoViewRequested += (o, e) =>
                {
                    targetRects.Add(e.TargetRect);
                };

                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                buttonToFocus.Focus(FocusState.Keyboard);
            });
            TestServices.WindowHelper.WaitForIdle();

            Verify.AreEqual(1, targetRects.Count);
            Verify.AreEqual(new Rect(0, 0, 200, 40), targetRects[0]);
        }

        private void UIElementBringIntoViewRequestedHandler(UIElement source, BringIntoViewRequestedEventArgs e)
        {
            FrameworkElement sourceAsFE = (FrameworkElement)source;
            eventOrder.Append("[" + sourceAsFE.Name + "BringIntoViewRequested]");
        }

        private void ControlBringIntoViewRequestedHandler(UIElement source, BringIntoViewRequestedEventArgs e)
        {
            Control sourceAsControl = (Control)source;
            eventOrder.Append("[" + sourceAsControl.Name + "BringIntoViewRequested]");
        }

        public partial class ButtonWithOverride : Button
        {
            public StringBuilder eventOrder = null;
            public bool HasFired = false;
            public bool ShouldHandle = false;

            protected override void OnBringIntoViewRequested(BringIntoViewRequestedEventArgs args)
            {
                Rect myRect = new Rect();
                myRect.X = args.TargetRect.X;
                myRect.Y = args.TargetRect.Y;
                myRect.Height = args.TargetRect.Height + 10;
                myRect.Width = args.TargetRect.Width;
                args.TargetRect = myRect;
                if (ShouldHandle)
                {
                    args.Handled = true;
                    eventOrder.Append("[" + this.Name + "Handled]");
                }
                HasFired = true;
                if (eventOrder != null)
                {
                    eventOrder.Append("[" + this.Name + "OnBringIntoViewRequested]");
                }
            }
        }

        public partial class StackPanelWithOverride : StackPanel
        {
            public StringBuilder eventOrder = null;
            public bool HasFired = false;
            public bool ShouldHandle = false;

            protected override void OnBringIntoViewRequested(BringIntoViewRequestedEventArgs args)
            {
                Rect myRect = new Rect();
                myRect.X = args.TargetRect.X;
                myRect.Y = args.TargetRect.Y;
                myRect.Height = args.TargetRect.Height;
                myRect.Width = args.TargetRect.Width + 10;
                args.TargetRect = myRect;
                if (ShouldHandle)
                {
                    args.Handled = true;
                    eventOrder.Append("[" + this.Name + "Handled]");
                }
                HasFired = true;
                if (eventOrder != null)
                {
                    eventOrder.Append("[" + this.Name + "OnBringIntoViewRequested]");
                }
            }
        }

        public partial class CanvasWithOverride : Canvas
        {
            public StringBuilder eventOrder = null;
            public bool HasFired = false;
            public bool ShouldHandle = false;

            protected override void OnBringIntoViewRequested(BringIntoViewRequestedEventArgs args)
            {
                args.HorizontalOffset += 5.0;
                args.VerticalOffset -= 5.0;
                args.AnimationDesired = !args.AnimationDesired;
                args.TargetElement = this;
                if (ShouldHandle)
                {
                    args.Handled = true;
                    eventOrder.Append("[" + this.Name + "Handled]");
                }
                HasFired = true;
                if (eventOrder != null)
                {
                    eventOrder.Append("[" + this.Name + "OnBringIntoViewRequested]");
                }
            }
        }
    }
}
