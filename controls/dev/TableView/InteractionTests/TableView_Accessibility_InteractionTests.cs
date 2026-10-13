// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // TableView accessibility-route interaction tests.
    //
    // These own the ASSISTIVE-TECHNOLOGY route, which is a different entry point from both the keyboard and the
    // pointer files. A screen reader does not Tab through a page to reach an element: it calls
    // IUIAutomationElement::SetFocus on the element it wants, then sends keys. That is what these tests do, and
    // it is why they are not merged into the keyboard file - the keyboard file must reach its targets by real
    // tab-stop traversal, or it stops proving anything about keyboard navigation.
    //
    // Product finding #13 (a client asking a row peer for its children fail-fasted the app) shaped the older tests in
    // this file: they observe rows at row level, point at cells by coordinates, and read editors and visual states
    // through in-process page readouts. #11820 fixed the peers, and cell peers are now read safely; see the history
    // note in TableViewInteractionTestHelpers. The older tests are kept as written - their techniques still work.
    [TestClass]
    public class TableViewAccessibilityInteractionTests
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

        #region Assistive-technology focus route

        [TestMethod]
        [TestProperty("Description", "Verifies a group header focused through UIA SetFocus - the route a screen reader takes - still holds focus after its own group collapses, so the next key reaches the same header.")]
        public void GroupHeaderKeepsFocusAcrossCollapseWhenFocusedThroughUia()
        {
            // Scenario: a screen reader moves focus with IUIAutomationElement::SetFocus and then sends an
            //   activation key. Tab-route tests do NOT cover this: Tab and SetFocus place focus by different
            //   paths (the group-header peer's SetFocusCore must focus with FocusState::Keyboard).
            // Failure means: TableView is operable by a sighted keyboard user but not by a screen-reader user,
            //   which is an accessibility bug, not a cosmetic one.
            using (var setup = new TestSetupHelper(PageName))
            {
                UIObject tableView = SelectGroupedPivotAndGetTable();
                if (tableView == null)
                {
                    return;
                }

                UIObject rowsHost = tableView.Children[tableView.Children.Count - 1];
                int baselineRows = CountRows(rowsHost);
                Verify.IsGreaterThan(baselineRows, 0, "The grouped table should realize rows before any collapse.");

                UIObject groupHeader = GetFirstGroupHeader(rowsHost);
                if (groupHeader == null)
                {
                    Verify.Fail("No TableViewGroupHeader peer was found under the rows host of the grouped table.");
                    return;
                }

                var expandCollapse = new ExpandCollapseImplementation(groupHeader);

                Log.Comment("Place focus the way an assistive technology does: UIA SetFocus on the header itself.");
                groupHeader.SetFocus();
                Wait.ForIdle();
                Verify.IsTrue(groupHeader.HasKeyboardFocus, "Precondition: UIA SetFocus must land focus on the group header.");

                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();

                // All three observations are taken and logged BEFORE any of them is asserted. Verify throws in
                // this suite, so the first failing assertion ends the test; without this the focus defect would
                // hide whether the collapse itself was correct, and the two answers mean different bugs.
                int rowsAfterCollapse = CountRows(rowsHost);
                ExpandCollapseState stateAfterCollapse = expandCollapse.ExpandCollapseState;
                bool keptFocus = groupHeader.HasKeyboardFocus;
                Log.Comment("After Enter on a UIA-focused header: realized rows = {0} (baseline {1}), header reports {2}, header kept focus = {3}.",
                    rowsAfterCollapse, baselineRows, stateAfterCollapse, keptFocus);

                Verify.IsLessThan(rowsAfterCollapse, baselineRows, "Enter on the focused group header must collapse its rows.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, stateAfterCollapse, "After Enter the header must report Collapsed.");
                Verify.IsTrue(keptFocus, "The group header must KEEP focus across its own collapse when focus arrived through UIA SetFocus - a screen-reader user must be able to re-open the group they just closed.");

                // Only reached once the defect is fixed: the whole point of keeping focus is that the next key
                // works, so the test proves the consequence rather than just the focus flag.
                Log.Comment("Re-open the same group with a second Enter, which is only possible if focus stayed put.");
                KeyboardHelper.PressKey(Key.Enter);
                Wait.ForIdle();
                Verify.AreEqual(baselineRows, CountRows(rowsHost), "A second Enter must re-expand the same group and restore the baseline row count.");
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState, "After the second Enter the header must report Expanded.");
            }
        }

        #endregion
    }
}
