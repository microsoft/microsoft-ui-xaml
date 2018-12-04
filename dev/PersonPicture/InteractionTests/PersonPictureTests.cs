// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS || USING_TESTNET
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class PersonPictureTests
    {

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
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
        public void BasicInteractionTest()
        {
            using (var setup = new TestSetupHelper("PersonPicture Tests"))
            {
                TextBlock PersonPictureInitialBlock = new TextBlock(FindElement.ByName("InitialsTextBlock"));

                // Ensure the person glyph is shown
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "\uE77B");

                // Ensure the group glyph is shown
                CheckBox cb = new CheckBox(FindElement.ByName("GroupCheckBox"));
                cb.Check();
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "\uE716");
                cb.Uncheck();

                // Ensure the initals are shown
                TextBlock tb = new TextBlock(FindElement.ByName("InitialTextBox"));
                WriteInTextBox("InitialTextBox", "AS");
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "AS");
                WriteInTextBox("InitialTextBox", "");

                // Ensure display name is converted to initials as expected.
                TextBlock tb1 = new TextBlock(FindElement.ByName("DisplayNameTextBox"));
                WriteInTextBox("DisplayNameTextBox", "Some Name");
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "SN");

                WriteInTextBox("DisplayNameTextBox", "Another Name (OSG)");
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "AN");

                // Ensure the badge number is shown
                TextBlock tb2 = new TextBlock(FindElement.ByName("BadgeNumberTextBox"));
                WriteInTextBox("BadgeNumberTextBox", "1");
                TextBlock PersonPictureBadgeNumberBlock = new TextBlock(FindElement.ByName("BadgeNumberTextBlock"));
                Verify.AreEqual(PersonPictureBadgeNumberBlock.DocumentText, "1");

                // Ensure the badge number is shown 99+
                WriteInTextBox("BadgeNumberTextBox", "125");
                Verify.AreEqual(PersonPictureBadgeNumberBlock.DocumentText, "99+");
            }
        }

        [TestMethod]
        public void ControlSizing()
        {
            using (var setup = new TestSetupHelper("PersonPicture Tests"))
            {
                RangeValueSlider heightSlider = new RangeValueSlider(FindElement.ByName("HeightSlider"));
                RangeValueSlider widthSlider = new RangeValueSlider(FindElement.ByName("WidthSlider"));
                CheckBox cb = new CheckBox(FindElement.ByName("DimensionsMatch"));

                using (var waiter = cb.GetToggledWaiter())
                {
                    // Confirm expected initial state
                    cb.Toggle();
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.Off);
                    waiter.Reset();

                    // Update height and verify the width changes to match
                    heightSlider.SetValue(85);
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.On);
                    waiter.Reset();

                    // Reset to unchecked state
                    cb.Toggle();
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.Off);
                    waiter.Reset();

                    // Update width and verify the height changes to match
                    widthSlider.SetValue(115);
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.On);
                    waiter.Reset();
                }
            }
        }

        [TestMethod]
        public void DependencyPropertyOrdering()
        {
            using (var setup = new TestSetupHelper("PersonPicture Tests"))
            {
                TextBlock PersonPictureInitialBlock = new TextBlock(FindElement.ByName("InitialsTextBlock"));
                Button SetContactBtn = new Button(FindElement.ByName("ContactBtn"));
                Button ClearContactBtn = new Button(FindElement.ByName("ClearContactBtn"));

                // Set all properties that can generate initials.
                WriteInTextBox("InitialTextBox", "AL");
                WriteInTextBox("DisplayNameTextBox", "Some Name");
                SetContactBtn.Invoke(); // Contact.DisplayName = "Test Contact"

                // Clear them in priority order and verify displayed initials.
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "AL");
                WriteInTextBox("InitialTextBox", "");
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "SN");
                WriteInTextBox("DisplayNameTextBox", "");
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "TC");
                ClearContactBtn.Invoke();
                Verify.AreEqual(PersonPictureInitialBlock.DocumentText, "\uE77B");
            }
        }

        [TestMethod]
        public void ImageLoading()
        {
            using (var setup = new TestSetupHelper("PersonPicture Tests"))
            {
                Button SetContactImageBtn = new Button(FindElement.ByName("ContactImageBtn"));
                Button ClearContactBtn = new Button(FindElement.ByName("ClearContactBtn"));
                Button SetImageBtn = new Button(FindElement.ByName("ImageBtn"));
                Button ClearImageBtn = new Button(FindElement.ByName("ClearImageBtn"));
                CheckBox cb = new CheckBox(FindElement.ByName("BgEllipseFilled"));

                Verify.AreEqual(cb.ToggleState, ToggleState.Off);

                using (var waiter = cb.GetToggledWaiter())
                {
                    // Set/Clear contact and ensure contact image is shown/cleared.
                    SetContactImageBtn.Invoke();
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.On);
                    waiter.Reset();
                    ClearContactBtn.Invoke();
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.Off);
                    waiter.Reset();

                    // Set/Clear image and ensure image is shown/cleared.
                    SetImageBtn.Invoke();
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.On);
                    waiter.Reset();
                    ClearImageBtn.Invoke();
                    waiter.Wait();
                    Verify.AreEqual(cb.ToggleState, ToggleState.Off);
                    waiter.Reset();
                }
            }
        }

        [TestMethod]
        [Description("Validates ability to scroll a list of PersonPicture in a ListView with automation.")]
        public void ScrollPeopleList()
        {
            ScrollPeopleListPrivate("PeopleLV");
        }

        [TestMethod]
        [Description("Validates ability to scroll a list of PersonPicture in a grouped ListView with automation.")]
        public void ScrollPeopleGroupedList()
        {
            ScrollPeopleListPrivate("PeopleGLV");
        }

        private void ScrollPeopleListPrivate(string listViewId)
        {
            using (var setup = new TestSetupHelper("PersonPicture Tests"))
            {
                Log.Comment("Finding ListView");
                ListView peopleListView = new ListView(FindElement.ById(listViewId));
                Verify.IsNotNull(peopleListView, "ListView found");

                Log.Comment("Retrieving first ListView");
                ListViewItem firstLVI = peopleListView.AllItems[0];
                Verify.IsNotNull(firstLVI, "First ListViewItem retrieved");

                Log.Comment("Retrieving fifth ListView at index 4");
                ListViewItem fifthLVI = peopleListView.AllItems[4];
                Verify.IsNotNull(fifthLVI, "Fifth ListViewItem retrieved");

                Verify.IsFalse(firstLVI.IsOffscreen, "First item is on screen");
                Verify.IsTrue(fifthLVI.IsOffscreen, "Fifth item is off screen");

                firstLVI.SetFocus();
                Wait.ForIdle();

                AutomationElement firstItemAE = AutomationElement.FocusedElement;

                fifthLVI.SetFocus();
                Wait.ForIdle();

                AutomationElement fifthItemAE = AutomationElement.FocusedElement;

                Log.Comment("Scroll to the fifth item using the automation ScrollItemPattern");
                ScrollItemPattern fifthItemSIP = fifthItemAE.GetCurrentPattern(ScrollItemPattern.Pattern) as ScrollItemPattern;
                fifthItemSIP.ScrollIntoView();
                Wait.ForIdle();

                Verify.IsTrue(firstLVI.IsOffscreen, "First item is off screen");
                Verify.IsFalse(fifthLVI.IsOffscreen, "Fifth item is on screen");

                Log.Comment("Scroll back to the first item using the automation ScrollItemPattern");
                ScrollItemPattern firstItemSIP = firstItemAE.GetCurrentPattern(ScrollItemPattern.Pattern) as ScrollItemPattern;
                firstItemSIP.ScrollIntoView();
                Wait.ForIdle();

                Verify.IsFalse(firstLVI.IsOffscreen, "First item is on screen");
                Verify.IsTrue(fifthLVI.IsOffscreen, "Fifth item is off screen");
            }
        }

        private void WriteInTextBox(string textBoxName, string s)
        {
            Log.Comment("Retrieve text box with name '{0}'.", textBoxName);
            Edit textBox = new Edit(FindElement.ByName(textBoxName));

            if (string.IsNullOrEmpty(s))
            {
                s = "{BACKSPACE}";

                Log.Comment("Give it keyboard focus.");
                textBox.SetFocus();
                Wait.ForIdle();
                Log.Comment("Select its current text.");
                textBox.DocumentRange.Select();
                Wait.ForIdle();
                Log.Comment("Type '{0}'.", s);
                textBox.SendKeys(s);
                Wait.ForIdle();
            }
            else
            {
                KeyboardHelper.EnterText(textBox, s);
            }
        }
    }
}
