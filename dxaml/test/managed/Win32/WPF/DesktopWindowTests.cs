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

using WPFMarkup = System.Windows.Markup;

using Microsoft.UI.Xaml.Tests.Common;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls.Primitives;
using Xaml = Microsoft.UI.Xaml;

using WPFControls = System.Windows.Controls;
using WPFSystem = System.Windows;
using System.Runtime.InteropServices.WindowsRuntime;

using Private.Infrastructure.Hosting.WPF;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{
    [TestClass]
    public class DesktopWindowTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Hosting:Mode", "WPF")]
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
                
        [TestMethod]
        [TestProperty("Description", "Validates life of DesktopWindow which is not activated.")]
        public void ValidateLifeOfNonActivatedDesktopWindow()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='button' />
                    </StackPanel>";

            StackPanel rootPanel = null;
            WeakReference<Window> wref = null;

            UIExecutor.Execute(() =>
            {
                // Test window lifecycle w/o activation
                {
                    Window desktopWindow = new Window();
                    wref = new WeakReference<Window>(desktopWindow);

                    rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    desktopWindow.Content = rootPanel;
                    desktopWindow = null;
                }
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Calling GC");
            GC.Collect();
            GC.WaitForPendingFinalizers();

            UIExecutor.Execute(() =>
            {
                Window resurrectWindow = null;
                wref.TryGetTarget(out resurrectWindow);
                if (resurrectWindow != null)
                {
                    Verify.Fail("We should not be here, window should have GCed by now.");
                    resurrectWindow.Close();
                }
            });
            Log.Comment("Test executed as per specifications, all is well.");

            TestServices.WindowHelper.WaitForIdle();

        }

        [TestMethod]
        [TestProperty("Description", "Validates life of activated DesktopWindow.")]
        public void ValidateLifeOfActivatedDesktopWindow()
        {
            const string rootPanelXaml =
                    @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                        <Button x:Name='button' Content='button' />
                    </StackPanel>";

            StackPanel rootPanel = null;
            WeakReference<Window> wref = null;

            UIExecutor.Execute(() =>
            {
                // Test window lifecycle with activation
                {
                    Window desktopWindow = new Window();
                    wref = new WeakReference<Window>(desktopWindow);

                    rootPanel = (StackPanel)XamlMarkup.XamlReader.Load(rootPanelXaml);
                    desktopWindow.Content = rootPanel;
                    desktopWindow.Activate();
                    desktopWindow = null;
                }
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Calling GC");
            GC.Collect();
            GC.WaitForPendingFinalizers();

            UIExecutor.Execute(() =>
            {
                Window resurrectWindow = null;
                wref.TryGetTarget(out resurrectWindow);
                // Window must be alive.
                Verify.IsTrue(resurrectWindow != null);
                if (resurrectWindow != null)
                {
                    resurrectWindow.Close();
                    Log.Comment("Closing window.");
                }
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that we can retrieve a Window instance from the framework application in desktop mode")]
        public void ValidateWindowCanBeRetrievedFromApplication()
        {
            // verify GetWindows returns an empty list
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 0);
            });
            TestServices.WindowHelper.WaitForIdle();

            // create and activate a single window
            UIExecutor.Execute(() =>
            {
                Window desktopWindow = new Window();
                desktopWindow.Content = new StackPanel();
                desktopWindow.Activate();
            });
            TestServices.WindowHelper.WaitForIdle();

            // get the list of windows and verify that the main window exists, close the returned window
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 1);
                Verify.IsNotNull(windows[0]);
                windows[0].Close();
            });
            TestServices.WindowHelper.WaitForIdle();

            // verify GetWindows once again returns an empty list
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 0);
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that we can activate and retrieve multiple Window instances from the framework application in desktop mode")]
        public void ValidateMultipleWindowsCanBeRetrievedFromApplication()
        {
            // create and activate a single window
            UIExecutor.Execute(() =>
            {
                Window desktopWindow = new Window();
                desktopWindow.Content = new StackPanel();
                desktopWindow.Activate();
            });
            TestServices.WindowHelper.WaitForIdle();

            // create and activate a second window
            UIExecutor.Execute(() =>
            {
                Window desktopWindow = new Window();
                desktopWindow.Content = new StackPanel();
                desktopWindow.Activate();
            });
            TestServices.WindowHelper.WaitForIdle();

            // get the list of windows and verify that the main window exists, close the returned window
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 2);
                Verify.IsNotNull(windows[0]);
                Verify.IsNotNull(windows[1]);
                windows[0].Close();
            });
            TestServices.WindowHelper.WaitForIdle();

            // get the list of windows and verify that the main window exists, close the returned window
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 1);
                Verify.IsNotNull(windows[0]);
                windows[0].Close();
            });
            TestServices.WindowHelper.WaitForIdle();

            // verify GetWindows once again returns an empty list
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 0);
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Creates a large number of Window instances from the framework application in desktop mode")]
        public void ValidateManyWindowsCanBeCreatedRetrievedAndClosed()
        {
            const int NUMBER_OF_WINDOWS_CREATED = 10;
            for (int i = 0; i < NUMBER_OF_WINDOWS_CREATED; ++i)
            {
                // create and activate a single window
                UIExecutor.Execute(() =>
                {
                    Window desktopWindow = new Window();
                    desktopWindow.Content = new StackPanel();
                    desktopWindow.Activate();
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            // get the list of windows and verify that all windows are present
            int numRemainingWindows = 0;
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == NUMBER_OF_WINDOWS_CREATED);
                numRemainingWindows = windows.Count;
            });
            TestServices.WindowHelper.WaitForIdle();

            // verify each window can be closed
            while (numRemainingWindows > 0)
            {
                // create and activate a single window
                UIExecutor.Execute(() =>
                {
                    IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                    Verify.IsNotNull(windows);
                    Verify.IsTrue(windows.Count == numRemainingWindows);
                    windows[0].Close();
                    --numRemainingWindows;
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            // verify GetWindows once again returns an empty list
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 0);
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Creates multiple Window instances and rotates activating them all multiple times, closes all at once")]
        public void ValidateMultipleWindowsCanBeRetrievedAndRotateActivations()
        {
            const int NUMBER_OF_WINDOWS_CREATED = 3;
            for (int i = 0; i < NUMBER_OF_WINDOWS_CREATED; ++i)
            {
                // create and activate a single window
                UIExecutor.Execute(() =>
                {
                    Window desktopWindow = new Window();
                    desktopWindow.Content = new StackPanel();
                    desktopWindow.Activate();
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            // rotate window activations a number of times which should not crash or fail
            const int NUMBER_OF_ACTIVATIONS = 20;
            int currentActivatedIndex = NUMBER_OF_WINDOWS_CREATED - 1;
            for (int i = 0; i < NUMBER_OF_ACTIVATIONS; ++i)
            {
                UIExecutor.Execute(() =>
                {
                    IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                    Verify.IsNotNull(windows);
                    Verify.IsTrue(windows.Count == NUMBER_OF_WINDOWS_CREATED);
                    int activateIndex = (currentActivatedIndex + 1) % NUMBER_OF_WINDOWS_CREATED;
                    windows[activateIndex].Activate();
                    currentActivatedIndex = activateIndex;
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            // close all the windows at once!
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == NUMBER_OF_WINDOWS_CREATED);
                for (int i = 0; i < NUMBER_OF_WINDOWS_CREATED; ++i)
                {
                    windows[i].Close();
                }
            });
            TestServices.WindowHelper.WaitForIdle();

            // verify GetWindows once again returns an empty list
            UIExecutor.Execute(() =>
            {
                IReadOnlyList<Window> windows = TestServices.WindowHelper.GetWindows();
                Verify.IsNotNull(windows);
                Verify.IsTrue(windows.Count == 0);
            });
            TestServices.WindowHelper.WaitForIdle();
        }
    }

} // namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
