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
        public void VerifyTextAlignmentPropogates()
        {
            var numberBox = SetupNumberBox();
            TextBox textBox = null;

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();

                textBox = TestUtilities.FindDescendents<TextBox>(numberBox).Where(e => e.Name == "InputBox").Single();
                Verify.AreEqual(TextAlignment.Left, textBox.TextAlignment, "The default TextAlignment should be left.");

                numberBox.TextAlignment = TextAlignment.Right;
                Content.UpdateLayout();

                Verify.AreEqual(TextAlignment.Right, textBox.TextAlignment, "The TextAlignment should have been updated to Right.");
            });
        }

        [TestMethod]
        public void VerifyNumberBoxCornerRadius()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("NumberBox CornerRadius property is not available pre-rs5");
                return;
            }

            var numberBox = SetupNumberBox();

            RepeatButton spinButtonDown = null;
            TextBox textBox = null;
            RunOnUIThread.Execute(() =>
            {
                // first test: Uniform corner radius of '2' with no spin buttons shown
                numberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Hidden;
                numberBox.CornerRadius = new CornerRadius(2);

                Content.UpdateLayout();

                textBox = TestUtilities.FindDescendents<TextBox>(numberBox).Where(e => e.Name == "InputBox").Single();
                Verify.AreEqual(new CornerRadius(2, 2, 2, 2), textBox.CornerRadius);

                // second test: Uniform corner radius of '2' with spin buttons in inline mode (T-rule applies now)
                numberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline;
                Content.UpdateLayout();

                spinButtonDown = TestUtilities.FindDescendents<RepeatButton>(numberBox).Where(e => e.Name == "DownSpinButton").Single();

                Verify.AreEqual(new CornerRadius(2, 0, 0, 2), textBox.CornerRadius);
                Verify.AreEqual(new CornerRadius(0, 2, 2, 0), spinButtonDown.CornerRadius);

                // third test: Set uniform corner radius to '4' with spin buttons in inline mode
                numberBox.CornerRadius = new CornerRadius(4);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // This check makes sure that updating the CornerRadius values of the numberbox in inline mode
                // does not break the T-rule.
                Verify.AreEqual(new CornerRadius(4, 0, 0, 4), textBox.CornerRadius);
                Verify.AreEqual(new CornerRadius(0, 4, 4, 0), spinButtonDown.CornerRadius);

                // fourth test: Update the spin button placement mode to 'compact' and verify that all corners
                // of the textbox are now rounded again
                numberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Compact;
                Content.UpdateLayout();

                Verify.AreEqual(new CornerRadius(4), textBox.CornerRadius);

                // fifth test: Check corner radius of 0 in compact mode.
                numberBox.CornerRadius = new CornerRadius(0);
                Content.UpdateLayout();

                Verify.AreEqual(new CornerRadius(0), textBox.CornerRadius);
            });
        }

        private NumberBox SetupNumberBox()
        {
            NumberBox numberBox = null;
            RunOnUIThread.Execute(() =>
            {
                numberBox = new NumberBox();
                Content = numberBox;
            });

            return numberBox;
        }
    }
}
