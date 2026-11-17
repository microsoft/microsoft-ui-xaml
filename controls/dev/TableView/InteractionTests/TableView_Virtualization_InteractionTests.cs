// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using MUXTestInfra.Shared.Infra;

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TableViewInteractionTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;
using VirtualizationFacts = Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewVirtualizationPageFacts;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // State comes from the page's readouts (TableViewVirtualizationPageFacts): which item a row shows and
    // which rows are on screen are not observable from UIA alone.
    [TestClass]
    [TestProperty("Ignore", "True")] // Disabled pending TableView page leak fix: #12491.
    public class TableViewVirtualizationInteractionTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
            RestartAppIfLongRunning();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies keyboard navigation in a template without its own ScrollViewer reveals the focused row in the app's ScrollViewer.")]
        public void KeyboardNavigationRevealsRowInAppScrollerWithoutBodyScroller()
        {
            using (var setup = new TestSetupHelper(new[] { PageName, VirtualizationFacts.NavigateButton }))
            {
                var show = FindElement.ById<Button>(VirtualizationFacts.ShowScrollerlessTableButton);
                if (show == null) { Verify.Fail(VirtualizationFacts.ShowScrollerlessTableButton + " was not found."); return; }
                show.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("0", WaitForReadoutField(VirtualizationFacts.ScrollerlessReadout, VirtualizationFacts.FocusedField, "0"),
                    "Precondition: row 0 has keyboard focus.");
                Verify.AreEqual("0", ReadReadoutField(VirtualizationFacts.ScrollerlessReadout, VirtualizationFacts.OffsetField),
                    "Precondition: the app scroller starts at the top.");

                const int downPresses = 15;
                Log.Comment("Press Down {0} times.", downPresses);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.None, downPresses);
                Wait.ForIdle();
                VerifyScrollerlessFocusRevealed(downPresses.ToString(), 0.0, "after Down");

                double offsetAfterDown = ReadScrollerlessOffset();
                Log.Comment("Press End.");
                KeyboardHelper.PressKey(Key.End);
                Wait.ForIdle();
                VerifyScrollerlessFocusRevealed((VirtualizationFacts.ScrollerlessItemCount - 1).ToString(), offsetAfterDown, "after End");
            }
        }

        #region Helpers

        private static double ReadScrollerlessOffset()
        {
            double offset;
            return double.TryParse(ReadReadoutField(VirtualizationFacts.ScrollerlessReadout, VirtualizationFacts.OffsetField),
                System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out offset) ? offset : double.NaN;
        }

        private static void VerifyScrollerlessFocusRevealed(string expectedId, double offsetBefore, string context)
        {
            WaitForReadoutField(VirtualizationFacts.ScrollerlessReadout, VirtualizationFacts.FocusedField, expectedId);
            WaitForReadoutField(VirtualizationFacts.ScrollerlessReadout, VirtualizationFacts.InViewField, "True");
            string readout = ReadPageReadout(VirtualizationFacts.ScrollerlessReadout);
            Log.Comment("Readout {0}: '{1}'.", context, readout);
            Verify.AreEqual(expectedId, Field(readout, VirtualizationFacts.FocusedField), "Focus must move to the target row (" + context + ").");
            Verify.IsTrue(ReadScrollerlessOffset() > offsetBefore + 0.5, "The app scroller must scroll to the focused row (" + context + ").");
            Verify.AreEqual("True", Field(readout, VirtualizationFacts.InViewField), "The focused row must be inside the app scroller's viewport (" + context + ").");
        }

        // The value of one "key=value" field of the readout, or a diagnostic string the assertion will print.
        private static string Field(string readout, string key)
        {
            if (string.IsNullOrEmpty(readout))
            {
                return "<no readout>";
            }

            foreach (string part in readout.Split(';'))
            {
                int split = part.IndexOf('=');
                if (split > 0 && string.Equals(part.Substring(0, split), key, StringComparison.Ordinal))
                {
                    return part.Substring(split + 1);
                }
            }

            return "<no field '" + key + "' in '" + readout + "'>";
        }

        #endregion
    }
}
