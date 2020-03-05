// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using AnimatedVisualPlayerTests;
using Common;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public sealed class AnimatedVisualPlayerTests
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
        public void AccessibilityTest()
        {
            using (var setup = new TestSetupHelper("AnimatedVisualPlayer Tests"))
            {
                var isPlayingTextBoxBeforePlaying = Edit("IsPlayingTextBoxBeforePlaying");
                var isPlayingTextBoxBeingPlaying = Edit("IsPlayingTextBoxBeingPlaying");

                if (isPlayingTextBoxBeforePlaying is null ||
                    isPlayingTextBoxBeingPlaying is null)
                {
                    Verify.Fail("UIElement not found.");
                    return;
                }

                ClickWaitAndVerifyText(
                    clickTarget: Button("PlayButton"),
                    edit: Edit("ProgressTextBox"),
                    expectedValue: Constants.PlayingEndedText);


                Log.Comment("Value of isPlayingTextBoxBeforePlaying: \"{0}\".", isPlayingTextBoxBeforePlaying.Value);
                Verify.AreEqual(Constants.FalseText, isPlayingTextBoxBeforePlaying.Value);

                //
                // isPlayingTextBoxBeingPlaying value is supposed to be updated
                // inside the event handler function of Click for playButton in
                // the UI test.
                //
                Log.Comment("Value of isPlayingTextBoxBeingPlaying: \"{0}\".", isPlayingTextBoxBeingPlaying.Value);
                Verify.AreEqual(Constants.TrueText, isPlayingTextBoxBeingPlaying.Value);

                ToZeroKeyframeAnimationTest();
                AroundTheEndAnimationTest();
                FromOneKeyframeAnimationTest();
                ReverseNegativePlaybackRateAnimationTest();
                ReversePositivePlaybackRateAnimationTest();
                HittestingTest();
                FallenBackTest();
            }
        }

        private void ToZeroKeyframeAnimationTest()
        {
            ClickWaitAndVerifyText(
                clickTarget: Button("ToZeroKeyframeAnimationPlayButton"),
                edit: Edit("ToZeroKeyframeAnimationProgressTextBox"),
                expectedValue: Constants.PlayingEndedText);
        }

        private void AroundTheEndAnimationTest()
        {
            ClickWaitAndVerifyText(
                clickTarget: Button("AroundTheEndAnimationPlayButton"),
                edit: Edit("AroundTheEndAnimationProgressTextBox"),
                expectedValue: Constants.PlayingEndedText);
        }


        private void FromOneKeyframeAnimationTest()
        {
            ClickWaitAndVerifyText(
                clickTarget: Button("FromOneKeyframeAnimationPlayButton"),
                edit: Edit("FromOneKeyframeAnimationProgressTextBox"),
                expectedValue: Constants.PlayingEndedText);
        }

        // Reverse backward playing using negative playback rate by setting playback rate to positive.
        private void ReverseNegativePlaybackRateAnimationTest()
        {
            ClickWaitAndVerifyText(
                clickTarget: Button("ReverseNegativePlaybackRateAnimationPlayButton"),
                edit: Edit("ReverseNegativePlaybackRateAnimationTextBox"),
                expectedValue: Constants.OneText);
        }

        // Reverse forward playing using positive playback rate by setting playback rate to negative.
        private void ReversePositivePlaybackRateAnimationTest()
        {
            ClickWaitAndVerifyText(
                clickTarget: Button("ReversePositivePlaybackRateAnimationPlayButton"),
                edit: Edit("ReversePositivePlaybackRateAnimationTextBox"),
                expectedValue: Constants.ZeroText);
        }

        // Tests that moving the mouse over the AnimatedVisualPlayer causes the
        // PointerMoved event to be fired.
        private void HittestingTest()
        {
            const string testName = nameof(HittestingTest);

            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                var textBox = Edit("HittestingTextBox");

                // Move the mouse away from the AnimatedVisualPlayer.
                InputHelper.MoveMouse(textBox, 0, 0);

                // Clear the value of the Edit.
                textBox.SetValueAndWait(string.Empty);

                using (var waiter = new ValueChangedEventWaiter(textBox))
                {
                    // Move the mouse over the AnimatedVisualPlayer.
                    var player = FindElement.ByName("Player");
                    InputHelper.MoveMouse(player, 20, 20);

                    Log.Comment($"{testName}: Waiting until OnPointerMoved handler in UI test returns.");
                    waiter.Wait();
                }

                Log.Comment($"{testName}: Value of textBox: \"{textBox.Value}\".");
                Verify.AreEqual(Constants.PointerMovedText, textBox.Value);
            }
        }

        // Tests that replacing the source with something invalid causes the
        // fallback content to be displayed.
        private void FallenBackTest()
        {
            ClickWaitAndVerifyText(
                clickTarget: Button("FallenBackButton"),
                edit: Edit("FallenBackTextBox"),
                expectedValue: Constants.TrueText);
        }

        // Clicks the clickTarget, waits for the edit's text to change its value, 
        // and returns the value of the edit.
        void ClickWaitAndVerifyText(
            UIObject clickTarget,
            Edit edit,
            string expectedValue,
            [global::System.Runtime.CompilerServices.CallerMemberName] string testName = "")
        {
            if (clickTarget is null)
            {
                Verify.Fail($"{testName}: {nameof(clickTarget)} UIElement not found.");
                return;
            }

            if (edit is null)
            {
                Verify.Fail($"{testName}: {nameof(edit)} UIElement not found.");
                return;
            }

            // Clear out the text box so it starts in a known state.
            edit.SetValueAndWait(string.Empty);

            using (var waiter = new ValueChangedEventWaiter(edit))
            {
                clickTarget.Click();
                Log.Comment($"{testName}: Waiting for text to change.");
                waiter.Wait();
            }

            Log.Comment($"{testName}: Value of text: \"{edit.Value}\".");
            Verify.AreEqual(expectedValue, edit.Value);
        }

        static Button Button(string name) => FindElement.ByName<Button>(name);

        static Edit Edit(string name) => FindElement.ByName<Edit>(name);
    }
}