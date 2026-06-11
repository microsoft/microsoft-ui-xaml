// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using XamlPeers = Microsoft.UI.Xaml.Automation.Peers;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class ButtonIntegrationTests : XamlTestsBase
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
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Ignore", "True")]
        [TestProperty("Description", "Validates that we can create a button instance instance.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void CanInstantiate()
        {
            UIExecutor.Execute(() =>
            {
                var button = new XamlControls.Button();
                Verify.IsNotNull(button);

                var buttonFromParser = XamlMarkup.XamlReader.Load(@"<Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'/>"); ;
                Verify.IsNotNull(buttonFromParser);
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")]
        [TestProperty("Description", "Validates that we can successfully add/remove a Button from the live tree.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void CanEnterAndLeaveLiveTree()
        {
            XamlControls.Button button = null;

            UIExecutor.Execute(() =>
            {
                button = new XamlControls.Button();
            });

            using (var btnLoaded = new EventTester<FrameworkElement, RoutedEventArgs>(button, "Loaded"))
            using (var btnUnloaded = new EventTester<FrameworkElement, RoutedEventArgs>(button, "Unloaded"))
            {
                UIExecutor.Execute(() =>
                {
                    TestServices.WindowHelper.WindowContent = button;
                });
                btnLoaded.Wait();

                UIExecutor.Execute(() =>
                {
                    TestServices.WindowHelper.WindowContent = null;
                });
                btnUnloaded.Wait();
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")]
        [TestProperty("Description", "Validates that an event is launched when the button is tapped.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void CanClickUsingTap()
        {
            XamlControls.Button button = null;

            UIExecutor.Execute(() =>
            {
                button = new XamlControls.Button();
                TestServices.WindowHelper.WindowContent = button;
            });
            TestServices.WindowHelper.WaitForIdle();
            using (var btnClicked = new EventTester<FrameworkElement, RoutedEventArgs>(button, "Click"))
            {
                UIExecutor.Execute(() =>
                {
                    TestServices.InputHelper.Tap(button);
                });
                btnClicked.Wait();
            }
        }

        [TestMethod]        
        [TestProperty("Description", "Validates that raising an automation event does not crash")]
        public void ValidateAutomationEventDoesNotCrash()
        {
            XamlControls.Grid grid = null;
            XamlControls.Button button = null;
            UIExecutor.Execute(() =>
            {
                grid = new XamlControls.Grid();
                button = new XamlControls.Button();

                grid.Children.Add(button);

                TestServices.WindowHelper.WindowContent = grid;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                XamlPeers.AutomationPeer peer = XamlPeers.FrameworkElementAutomationPeer.FromElement(button);
                peer.RaiseAutomationEvent(XamlPeers.AutomationEvents.MenuOpened);
                grid = null;
                button = null;
                TestServices.WindowHelper.WindowContent = null;
            });

            TestServices.WindowHelper.WaitForIdle();
        }
    }
}
