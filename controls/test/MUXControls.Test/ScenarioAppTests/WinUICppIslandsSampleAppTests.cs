// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Diagnostics;

using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class WinUICppIslandsSampleAppTests
    {
        private static Process s_process;
        private static UIObject s_root;

        public static TestApplicationInfo WinUICppIslandsSampleApp
        {
            get
            {
                return new TestApplicationInfo(
                    "WinUICppIslandsSampleApp",
                    "WinUICppIslandsSampleApp_6f07fta6qpts2!App",
                    "WinUICppIslandsSampleApp_6f07fta6qpts2",
                    "IslandSampleWinUI3",
                    "WinUICppIslandsSampleApp.exe",
                    "WinUICppIslandsSampleApp",
                    isUwpApp: false,
                    TestApplicationInfo.MUXCertSerialNumber,
                    TestApplicationInfo.MUXBaseAppxDir);
            }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("IgnoreForValidateWindowsAppSDK", "True")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Method")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestApplicationInfo testAppInfo = WinUICppIslandsSampleApp;
            TestAppInstallHelper.EnableSideloadingApps();
            Verify.IsTrue(TestAppInstallHelper.InstallTestAppFromPackageIfNeeded(
                testContext.TestDeploymentDir,
                testAppInfo.TestAppPackageName,
                testAppInfo.TestAppPackageFamilyName,
                testAppInfo.InstallerName,
                testAppInfo.ProcessName));
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            if (s_process != null)
            {
                if (!s_process.HasExited)
                {
                    s_process.Kill(entireProcessTree: true);
                    s_process.WaitForExit();
                }

                s_process.Dispose();
                s_process = null;
            }
        }

        [TestMethod]
        [Description("Verify MUXC metadata remains usable after process-wide WinUI shutdown and restart.")]
        public void MuxcMetadataAfterWinUIRestart()
        {
            var windowCondition = UICondition.CreateFromName(
                WinUICppIslandsSampleApp.TestAppMainWindowTitle);
            using (var launchWaiter = new AppLaunchWaiter(windowCondition))
            {
                uint processId = Application.ActivateApplication(
                    WinUICppIslandsSampleApp.TestAppName,
                    null);
                s_process = Process.GetProcessById((int)processId);
                launchWaiter.TryWait(TimeSpan.FromSeconds(20));
                s_root = launchWaiter.Source;
            }

            Verify.IsNotNull(s_root, "Sample app window");
            if (s_root == null)
            {
                return;
            }

            InvokeButton("Create DesktopWindowXamlSource");
            WaitForElement(s_root, "Inline CommandBarFlyout", expectedToExist: true);

            InvokeMoreMenuItem("Shut down WinUI (public APIs)");
            WaitForElement(s_root, "Inline CommandBarFlyout", expectedToExist: false);

            InvokeMoreMenuItem("Start WinUI");

            InvokeButton("Create DesktopWindowXamlSource");
            WaitForElement(s_root, "Inline CommandBarFlyout", expectedToExist: true);
        }

        private static void InvokeButton(string name)
        {
            UIObject buttonObject = FindElement.GetDescendantByName(s_root, name);
            Verify.IsNotNull(buttonObject, name);
            if (buttonObject == null)
            {
                throw new InvalidOperationException($"Could not find button '{name}'.");
            }

            new Button(buttonObject).Invoke();
        }

        private static void InvokeMoreMenuItem(string name)
        {
            UIObject moreButton = FindElement.GetDescendantByName(s_root, "More...");
            Verify.IsNotNull(moreButton, "More...");
            if (moreButton == null)
            {
                throw new InvalidOperationException("Could not find the 'More...' button.");
            }

            InputHelper.LeftClick(moreButton);

            UIObject menuItem = WaitForElement(UIObject.Root, name, expectedToExist: true);
            Verify.IsNotNull(menuItem, name);
            if (menuItem == null)
            {
                throw new InvalidOperationException($"Could not find menu item '{name}'.");
            }

            InputHelper.LeftClick(menuItem);
        }

        private static UIObject WaitForElement(UIObject root, string name, bool expectedToExist)
        {
            // TestEnvironment.WaitUntilElementLoadedByName uses ElementCache, which requires
            // TestEnvironment.Application. This plain Win32 sample is launched directly instead.
            var condition = UICondition.CreateFromName(name);
            for (int attempt = 0; attempt < 50; attempt++)
            {
                root.Descendants.TryFind(condition, out UIObject element);
                if ((element != null) == expectedToExist)
                {
                    return element;
                }

                Wait.ForMilliseconds(100);
            }

            throw new WaiterTimedOutException(
                expectedToExist
                    ? $"Expected to find '{name}'."
                    : $"Expected '{name}' to be removed.");
        }
    }
}
