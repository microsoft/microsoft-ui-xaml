// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System;
using System.Numerics;
using Common;
using System.Threading.Tasks;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Microsoft.Windows.Apps.Test.Foundation;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class BackdropMaterialTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }


        [TestMethod]
        public void VerifySetupAndFallback()
        {
            using (var setup = new TestSetupHelper(new[] { "BackdropMaterial Tests" }))
            {
                var reportBrushes = FindElement.ByName<Button>("ReportBrushes");
                var disableBackdrop = FindElement.ByName<Button>("DisableBackdrop");
                var enableBackdrop = FindElement.ByName<Button>("EnableBackdrop");
                var testOutput = FindElement.ByName<Edit>("TestOutput");
                var toggleTheme = FindElement.ById<Button>("__ToggleThemeButton");

                // TODO: handle downlevel

                // Light theme

                // Test dark theme enabled
                Log.Comment("Clicking ReportBrushes");
                reportBrushes.InvokeAndWait();

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
                {
                    Verify.AreEqual("TestFrame.Background = #00000000,Window.Backdrop = Windows.UI.Composition.CompositionEffectBrush", testOutput.Value);
                }
                else
                {
                    Verify.AreEqual("TestFrame.Background = #FFF3F3F3,Window.Backdrop = null", testOutput.Value);
                }

                Log.Comment("Disable backdrop and check fallback");
                disableBackdrop.InvokeAndWait();
                reportBrushes.InvokeAndWait();

                Verify.AreEqual("TestFrame.Background = #FFF3F3F3,Window.Backdrop = null", testOutput.Value);

                Log.Comment("Enable backdrop and check fallback");
                enableBackdrop.InvokeAndWait();
                reportBrushes.InvokeAndWait();

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
                {
                    Verify.AreEqual("TestFrame.Background = #00000000,Window.Backdrop = Windows.UI.Composition.CompositionEffectBrush", testOutput.Value);
                }
                else
                {
                    Verify.AreEqual("TestFrame.Background = #FFF3F3F3,Window.Backdrop = null", testOutput.Value);
                }

                // Dark theme

                Log.Comment("Toggle theme and check again");
                toggleTheme.InvokeAndWait();
                reportBrushes.InvokeAndWait();

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
                {
                    Verify.AreEqual("TestFrame.Background = #00000000,Window.Backdrop = Windows.UI.Composition.CompositionEffectBrush", testOutput.Value);
                }
                else
                {
                    Verify.AreEqual("TestFrame.Background = #FF202020,Window.Backdrop = null", testOutput.Value);
                }

                Log.Comment("Disable backdrop and check fallback");
                disableBackdrop.InvokeAndWait();
                reportBrushes.InvokeAndWait();
                Verify.AreEqual("TestFrame.Background = #FF202020,Window.Backdrop = null", testOutput.Value);

                Log.Comment("Enable backdrop and check fallback");
                enableBackdrop.InvokeAndWait();
                reportBrushes.InvokeAndWait();

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.TwentyOneH1))
                {
                    Verify.AreEqual("TestFrame.Background = #00000000,Window.Backdrop = Windows.UI.Composition.CompositionEffectBrush", testOutput.Value);
                }
                else
                {
                    Verify.AreEqual("TestFrame.Background = #FF202020,Window.Backdrop = null", testOutput.Value);
                }

                // Put the theme back
                Log.Comment("Restoring theme");
                toggleTheme.InvokeAndWait();
            }
        }
    }
}
