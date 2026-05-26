// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Private.Infrastructure;
using Microsoft.UI.Xaml.Controls.Primitives;
using Windows.ApplicationModel.Core;
using Windows.UI.Core;

namespace Microsoft.UI.Xaml.Tests.Lifetime
{
    [TestClass]
    public class LifetimeTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            // Make sure you always call XamlTestsBase.SetupBase
            // here to ensure the test services are initialized.
            XamlTestsBase.SetupBase(context);

            TestServices.WindowHelper.InitializeXaml(new Microsoft.UI.Xaml.Tests.Lifetime.XamlMetaDataProvider());
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Description", "Validates that a custom VSM state trigger doesn't leak or crash.")]
        public void TestCustomStateTrigger()
        {
            using (new TestCleanupWrapper())
            {
                Page rootPage = null;
                WeakReference rootPageWeakRef = null;

                UIExecutor.Execute(() =>
                {
                    rootPage = new global::CustomTypes.CustomStateTriggerPage();
                    Application.LoadComponent(
                        rootPage,
                        new Uri("ms-appx:///resources/managed/Lifetime/CustomStateTriggerPage.xaml"),
                        ComponentResourceLocation.Nested);
                    Verify.IsNotNull(rootPage);
                    rootPageWeakRef = new WeakReference(rootPage);
                    TestServices.WindowHelper.WindowContent = rootPage;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Removing tree");
                    rootPage = null;
                    TestServices.WindowHelper.WindowContent = null;
                });
                TestServices.WindowHelper.WaitForIdle();

                while (rootPageWeakRef.IsAlive) {
                    UIExecutor.Execute(() => {
                        Log.Comment("Calling GC");
                        GC.Collect();
                    });
                    TestServices.WindowHelper.WaitForIdle();
                }
            }
        }

        // Create a cycles between a control and an ICommand.  This shouldn't leak and shouldn't crash.  (It doesn't crash, because
        // we insert an IWeakReference between the ICommand and the CanExecuteChanged event handler object.)
        // Disabling due to instability, likely due to timing
        //[TestMethod]
        //[TestProperty("IsolationLevel", "Method")]
        //[TestProperty("Description", "Validates that a Button/Command cycle doesn't leak or crash.")]
        //[TestProperty("Ignore", "True")]
        public void TestCommandWeakReference()
        {
            WeakReference buttonWeakRef = null;
            TestCommand testCommandButton = null;

            WeakReference menuFlyoutItemWeakRef = null;
            TestCommand testCommandMenu = null;

            UIExecutor.Execute(() =>
            {
                // Set up the cycles, but don't keep any strong refs on anything.

                var button = new Button();
                buttonWeakRef = new WeakReference(button);

                testCommandButton = new TestCommand();
                button.Command = testCommandButton;

                var menuFlyoutItem = new MenuFlyoutItem();
                menuFlyoutItemWeakRef = new WeakReference(menuFlyoutItem);

                testCommandMenu = new TestCommand();
                menuFlyoutItem.Command = testCommandMenu;

            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                // Run GC, validate that the objects got cleaned up.

                GC.Collect();
                testCommandButton.RaiseChange();
                testCommandMenu.RaiseChange();

                Verify.IsFalse(buttonWeakRef.IsAlive);
                Verify.IsFalse(menuFlyoutItemWeakRef.IsAlive);

            });
        }

        // Disabling due to instability, likely due to timing
        //[TestMethod]
        //[TestProperty("Description", "Make sure XAML doesn't crash when content changes while a button has mouse capture")]
        //[TestProperty("Ignore", "True")]
        public void SetContentToNullWhileAButtonHasCapture()
        {
            Page page = null;
            Button button = null;

            UIExecutor.Execute(() => {
                page = (Page)Markup.XamlReader.Load(
                    @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <StackPanel Margin='10'>
                            <Button x:Name='button'>button</Button>
                        </StackPanel>
                    </Page>");
                button = (Button)page.FindName("button");
                TestServices.WindowHelper.WindowContent = page;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Left mouse button down on button to trigger capture");
            TestServices.InputHelper.MouseButtonDown(button, 0, 0, MouseButton.Left);

            UIExecutor.Execute(() => {
                // This is successful if it doesn't crash!
                Log.Comment("Setting content to null");
                TestServices.WindowHelper.WindowContent = null;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() => {
                Log.Comment("We didn't crash.");
            });
        }

        // Disabling due to instability, likely due to timing
        //[TestMethod]
        //[TestProperty("Description", "Validate the weak reference that CContentControl keeps to its ContentPresenter in support of the ContentTemplateRoot property")]
        public void ContentTemplateRootWeakReference()
        {
            Page page = null;
            object root = null;

            UIExecutor.Execute(() => {
                page = (Page)Markup.XamlReader.Load(
                    @"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    </Page>");
                TestServices.WindowHelper.WindowContent = page;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() => {
                // Clear out any pending need to GC so that we don't get any spontaneous ones in here
                GC.Collect();

                // Create a ContentControl with a custom control template, so that we can
                // throw the template away
                var cc = new ContentControl();
                page.Content = cc;
                cc.Content = "Hello";
                cc.Template = cc.Template; // Change the BaseValueSource form Style to Local, so that it's a "change" later to set it to null
                cc.UpdateLayout();

                // Create an RCW to the control template's ContentPresenter, so that it will
                // stick around after being thrown away (wating for the next GC).  We have to do this is a
                // separate method to make the GC predictable; sometimes it does some optimizations that are
                // difficult to predict.
                ContentTemplateRootWeakReferenceHelper(cc);

                // Orphan that ContentPresenter
                cc.Template = null;
                cc.UpdateLayout();

                // Collect the ContentPresenter
                GC.Collect();

                // Try to get the ContentTemplateRoot, not crashing is success.
                // (This property getter relies on a weak reference to the above ContentPresenter.)
                root = cc.ContentTemplateRoot;
            });
            TestServices.WindowHelper.WaitForIdle();

        }

        void ContentTemplateRootWeakReferenceHelper(ContentControl cc)
        {
            var cp = VisualTreeHelper.GetChild(cc, 0) as ContentPresenter;
            cp = null;
        }

        // Disabling due to instability, likely due to timing
        //[TestMethod]
        //[TestProperty("Description", "Validate that a non-WUX subclass can be run on a separate thread and shutdown cleanly")]
        public void PivotOnSecondThread()
        {
            var autoResetEvent = new AutoResetEvent(false);
            CoreApplicationView coreView = null;

            UIExecutor.Execute(async () => {
                coreView = CoreApplication.CreateNewView();

                // Create a new window (on a new thread) with a custom pivot in it.  The custom pivot is necessary
                // to exercise a code path where the custom pivot is destructing, calls a final release on the Pivot,
                // which cleans its control template, which tries to call back to the custom pivot (the controlling unknown
                // of the templated parent).

                await coreView.Dispatcher.RunAsync(
                    CoreDispatcherPriority.Normal,
                    () =>
                    {
                        TestServices.WindowHelper.WindowContent = new CustomPivot();
                        Window.Current.Activate();
                    });
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(async () => {
                Window.Current.Activate();

                // Close the secondary window
                await coreView.Dispatcher.RunAsync(
                    CoreDispatcherPriority.Normal,
                    () =>
                    {
                        Window.Current.Close();
                    });

                autoResetEvent.Set();
            });

            Verify.IsTrue(autoResetEvent.WaitOne(TimeSpan.FromSeconds(5)));
        }

    }

    public class CustomPivot : Pivot
    {
        public CustomPivot()
        {
        }
    }

    public class TestCommand : global::Microsoft.UI.Xaml.Input.ICommand
    {
        public event EventHandler<object> CanExecuteChanged;

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            return;
        }

        public void RaiseChange()
        {
            if (CanExecuteChanged != null)
                CanExecuteChanged(this, new EventArgs());
        }
    }
}
