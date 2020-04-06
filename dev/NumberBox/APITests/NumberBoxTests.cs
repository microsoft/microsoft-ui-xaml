// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using System.Linq;
using System.Collections.Generic;
using MUXControlsTestApp;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class NumberBoxTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyNumberBoxCornerRadius()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("NumberBox CornerRadius property is not available pre-rs5");
                return;
            }

            var numberBox = SetupNumberBox();

            // first test: Uniform corner radius of '2' with no spin buttons shown
            RunOnUIThread.Execute(() =>
            {
                numberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Hidden;
                numberBox.CornerRadius = new CornerRadius(2);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var textBox = TestUtilities.FindDescendents<TextBox>(numberBox).Where(e => e.Name == "InputBox").Single();
                Verify.AreEqual(new CornerRadius(2, 2, 2, 2), textBox.CornerRadius);
            });

            // second test: Uniform corner radius of '2' with spin buttons in inline mode
            RunOnUIThread.Execute(() =>
            {
                numberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var textBox = TestUtilities.FindDescendents<TextBox>(numberBox).Where(e => e.Name == "InputBox").Single();
                var spinButtonDown = TestUtilities.FindDescendents<RepeatButton>(numberBox).Where(e => e.Name == "DownSpinButton").Single();

                Verify.AreEqual(new CornerRadius(2, 0, 0, 2), textBox.CornerRadius);
                Verify.AreEqual(new CornerRadius(0, 2, 2, 0), spinButtonDown.CornerRadius);
            });

            // third test: Uniform corner radius of '0' with spin buttons in inline mode
            RunOnUIThread.Execute(() =>
            {
                numberBox.CornerRadius = new CornerRadius(0);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var textBox = TestUtilities.FindDescendents<TextBox>(numberBox).Where(e => e.Name == "InputBox").Single();
                var spinButtonDown = TestUtilities.FindDescendents<RepeatButton>(numberBox).Where(e => e.Name == "DownSpinButton").Single();

                Verify.AreEqual(new CornerRadius(0), textBox.CornerRadius);
                Verify.AreEqual(new CornerRadius(0), spinButtonDown.CornerRadius);
            });
        }

        private NumberBox SetupNumberBox()
        {
            NumberBox numberBox = null;
            RunOnUIThread.Execute(() =>
            {
                numberBox = new NumberBox();
            });

            TestUtilities.SetAsVisualTreeRoot(numberBox);
            Verify.IsNotNull(numberBox);
            return numberBox;
        }
    }
}
