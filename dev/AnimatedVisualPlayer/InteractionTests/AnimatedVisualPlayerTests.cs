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

#if BUILD_WINDOWS
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif


// CatGates requires that test namespaces begin with "Windows.UI.Xaml.Tests",
// so we need to make sure that our test namespace begins with that to ensure that we get picked up.
namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    class AnimatedVisualPlayerTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void AccessibilityTest()
        {
            using (var setup = new TestSetupHelper("AnimatedVisualPlayer Tests"))
            {
                var progressTextBox = FindElement.ByName<Edit>("ProgressTextBox");
                var isPlayingTextBoxBeforePlaying = FindElement.ByName<Edit>("IsPlayingTextBoxBeforePlaying");
                var isPlayingTextBoxBeingPlaying = FindElement.ByName<Edit>("IsPlayingTextBoxBeingPlaying");
                var playButton = FindElement.ByName<Button>("PlayButton");

                if (playButton != null &&
                    progressTextBox != null &&
                    isPlayingTextBoxBeforePlaying != null &&
                    isPlayingTextBoxBeingPlaying != null)
                {
                    using (var progressTextBoxWaiter = new PropertyChangedEventWaiter(progressTextBox, UIProperty.Get("Value.Value")))
                    {
                        playButton.Click();

                        Log.Comment("Waiting until AnimatedVisualPlayer playing ends.");
                        progressTextBoxWaiter.Wait();
                        Log.Comment("EventWaiter of progressTextBox is raised.");

                        Log.Comment("Value of isPlayingTextBoxBeforePlaying: \"{0}\".", isPlayingTextBoxBeforePlaying.Value);
                        Verify.AreEqual(Constants.FalseText, isPlayingTextBoxBeforePlaying.Value);

                        //
                        // isPlayingTextBoxBeingPlaying value is supposed to be updated
                        // inside the event handler function of Click for playButton in
                        // the UI test.
                        //
                        Log.Comment("Value of isPlayingTextBoxBeingPlaying: \"{0}\".", isPlayingTextBoxBeingPlaying.Value);
                        Verify.AreEqual(Constants.TrueText, isPlayingTextBoxBeingPlaying.Value);

                        Log.Comment("Value of progressTextBox: \"{0}\".", progressTextBox.Value);
                        Verify.AreEqual(Constants.PlayingEndedText, progressTextBox.Value);
                    }
                }
                else
                {
                    Verify.Fail("PlayButton or any other UIElement is not found.");
                }

                ToZeroKeyframeAnimationAccessibilityTest();
                FromOneKeyframeAnimationAccessibilityTest();
                ReverseNegativePlaybackRateAnimationAccessibilityTest();
                ReversePositivePlaybackRateAnimationAccessibilityTest();
                HittestingAccessibilityTest();
                FallenBackTest();
            }
        }

        private void ToZeroKeyframeAnimationAccessibilityTest()
        {
            var textBox = FindElement.ByName<Edit>("ToZeroKeyframeAnimationProgressTextBox");
            var playButton = FindElement.ByName<Button>("ToZeroKeyframeAnimationPlayButton");

            if (playButton != null && textBox != null)
            {
                using (var textBoxWaiter = new PropertyChangedEventWaiter(textBox, UIProperty.Get("Value.Value")))
                {
                    playButton.Click();

                    Log.Comment("ToZeroKeyframeAnimationAccessibilityTest: textBoxWaiter: Waiting until AnimatedVisualPlayer playing ends.");
                    textBoxWaiter.Wait();
                    Log.Comment("ToZeroKeyframeAnimationAccessibilityTest: EventWaiter of textBox is raised.");

                    Log.Comment("ToZeroKeyframeAnimationAccessibilityTest: Value of textBox: \"{0}\".", textBox.Value);
                    Verify.AreEqual(Constants.PlayingEndedText, textBox.Value);
                }
            }
            else
            {
                Verify.Fail("ToZeroKeyframeAnimationAccessibilityTest: playButton or any other UIElement is not found.");
            }
        }

        private void FromOneKeyframeAnimationAccessibilityTest()
        {
            var textBox = FindElement.ByName<Edit>("FromOneKeyframeAnimationProgressTextBox");
            var playButton = FindElement.ByName<Button>("FromOneKeyframeAnimationPlayButton");

            if (playButton != null && textBox != null)
            {
                using (var textBoxWaiter = new PropertyChangedEventWaiter(textBox, UIProperty.Get("Value.Value")))
                {
                    playButton.Click();

                    Log.Comment("FromOneKeyframeAnimationAccessibilityTest: textBoxWaiter: Waiting until AnimatedVisualPlayer playing ends.");
                    textBoxWaiter.Wait();
                    Log.Comment("FromOneKeyframeAnimationAccessibilityTest: EventWaiter of textBox is raised.");

                    Log.Comment("FromOneKeyframeAnimationAccessibilityTest: Value of textBox: \"{0}\".", textBox.Value);
                    Verify.AreEqual(Constants.PlayingEndedText, textBox.Value);
                }
            }
            else
            {
                Verify.Fail("FromOneKeyframeAnimationAccessibilityTest: playButton or any other UIElement is not found.");
            }
        }

        // Reverse backward playing using negative playback rate by setting playback rate to positive.
        private void ReverseNegativePlaybackRateAnimationAccessibilityTest()
        {
            var textBox = FindElement.ByName<Edit>("ReverseNegativePlaybackRateAnimationTextBox");
            var playButton = FindElement.ByName<Button>("ReverseNegativePlaybackRateAnimationPlayButton");

            if (playButton != null && textBox != null)
            {
                using (var textBoxWaiter = new PropertyChangedEventWaiter(textBox, UIProperty.Get("Value.Value")))
                {
                    playButton.Click();

                    Log.Comment("ReverseNegativePlaybackRateAnimationAccessibilityTest: textBoxWaiter: Waiting until AnimatedVisualPlayer playing ends.");
                    textBoxWaiter.Wait();
                    Log.Comment("ReverseNegativePlaybackRateAnimationAccessibilityTest: EventWaiter of textBox is raised.");

                    Log.Comment("ReverseNegativePlaybackRateAnimationAccessibilityTest: Value of textBox: \"{0}\".", textBox.Value);
                    Verify.AreEqual(Constants.OneText, textBox.Value);
                }
            }
            else
            {
                Verify.Fail("ReverseNegativePlaybackRateAnimationAccessibilityTest: playButton or any other UIElement is not found.");
            }
        }

        // Reverse forward playing using positive playback rate by setting playback rate to negative.
        private void ReversePositivePlaybackRateAnimationAccessibilityTest()
        {
            var textBox = FindElement.ByName<Edit>("ReversePositivePlaybackRateAnimationTextBox");
            var playButton = FindElement.ByName<Button>("ReversePositivePlaybackRateAnimationPlayButton");

            if (playButton != null && textBox != null)
            {
                using (var textBoxWaiter = new PropertyChangedEventWaiter(textBox, UIProperty.Get("Value.Value")))
                {
                    playButton.Click();

                    Log.Comment("ReversePositivePlaybackRateAnimationAccessibilityTest: textBoxWaiter: Waiting until AnimatedVisualPlayer playing ends.");
                    textBoxWaiter.Wait();
                    Log.Comment("EventWaiter of reversePositivePlaybackRateAnimationProgressTextBox is raised.");

                    Log.Comment("ReversePositivePlaybackRateAnimationAccessibilityTest: Value of textBox: \"{0}\".", textBox.Value);
                    Verify.AreEqual(Constants.ZeroText, textBox.Value);
                }
            }
            else
            {
                Verify.Fail("ReversePositivePlaybackRateAnimationAccessibilityTest: playButton or any other UIElement is not found.");
            }
        }
        
        private void HittestingAccessibilityTest()
        {
            if (!IsRS5OrHigher())
            {
                return;
            }

            var textBox = FindElement.ByName<Edit>("HittestingTextBox");

            using (var textBoxWaiter = new PropertyChangedEventWaiter(textBox, UIProperty.Get("Value.Value")))
            {
                TestEnvironment.Application.CoreWindow.Click();

                Log.Comment("HittestingAccessibilityTest: Waiting until OnPointerMoved handler in UI test returns.");
                textBoxWaiter.Wait();
                Log.Comment("HittestingAccessibilityTest: EventWaiter of HittestingAccessibilityTest is raised.");

                Log.Comment("HittestingAccessibilityTest: Value of textBox: \"{0}\".", textBox.Value);
                Verify.AreEqual(Constants.PointerMovedText, textBox.Value);
            }
        }

        private void FallenBackTest()
        {
            var textBox = FindElement.ByName<Edit>("FallenBackTextBox");
            var testButton = FindElement.ByName<Button>("FallenBackButton");

            if (testButton != null && textBox != null)
            {
                using (var textBoxWaiter = new PropertyChangedEventWaiter(textBox, UIProperty.Get("Value.Value")))
                {
                    testButton.Click();

                    Log.Comment("FallenBackTest: textBoxWaiter: Waiting until fallenback screencapture and results checking.");
                    textBoxWaiter.Wait();
                    Log.Comment("EventWaiter of FallenBackTextBox is raised.");

                    Log.Comment("FallenBackTest: Value of textBox: \"{0}\".", textBox.Value);
                    Verify.AreEqual(Constants.TrueText, textBox.Value);
                }
            }
            else
            {
                Verify.Fail("FallenBackTest: FallenBackButton or any other UIElement is not found.");
            }
        }

        private bool IsRS5OrHigher()
        {
            return ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 7);
        }
    }
}