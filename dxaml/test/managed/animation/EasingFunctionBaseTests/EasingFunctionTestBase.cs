// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Media.Animation;
using System.Reflection;

namespace Microsoft.UI.Xaml.Tests.Animation.EasingFunctionBaseTests
{
    [TestClass]
    public class EasingFunctionTestBase : XamlTestsBase
    {
        protected void VerifyCanSwitchEasingModes(EasingFunctionBase easingFunction)
        {
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(easingFunction.EasingMode == EasingMode.EaseOut, "Verify default easing mode is EasingMode::EastOut");

                easingFunction.EasingMode = EasingMode.EaseInOut;
                Verify.IsTrue(easingFunction.EasingMode == EasingMode.EaseInOut, "Verify can switch easing mode to EasingMode::EaseInOut");

                easingFunction.EasingMode = EasingMode.EaseIn;
                Verify.IsTrue(easingFunction.EasingMode == EasingMode.EaseIn, "Verify can switch easing mode to EasingMode::EaseIn");

                easingFunction.EasingMode = EasingMode.EaseOut;
                Verify.IsTrue(easingFunction.EasingMode == EasingMode.EaseOut, "Verify can switch easing mode to EasingMode::EaseOut");
            });       
        }

        private static void VerifyAreClose(double expected, double actual, string message)
        {
            const double epsilon = 0.00001;
            bool same =
                   (double.IsNaN(expected) && double.IsNaN(actual))
                || (double.IsInfinity(expected) && double.IsInfinity(actual))
                || ((Math.Abs(expected) - Math.Abs(actual)) < epsilon);

            Verify.IsTrue(same, message + $" (expected: {expected}, actual: {actual})");
        }

        protected void VerifyEaseOutput(EasingFunctionBase easingFunction, Dictionary<double, Tuple<double/*EaseIn*/, double/*EaseOut*/, double/*EaseInOut*/>> easeGroundTruth)
        {
            UIExecutor.Execute(() =>
            {
                easingFunction.EasingMode = EasingMode.EaseIn;
                foreach (double key in easeGroundTruth.Keys)
                {
                    var actual = easingFunction.Ease(key);
                    var expected = easeGroundTruth[key].Item1;
                    VerifyAreClose(expected, actual, "Verify Ease(" + key + ") method call output in EasingMode::" + easingFunction.EasingMode);
                }

                easingFunction.EasingMode = EasingMode.EaseOut;
                foreach (double key in easeGroundTruth.Keys)
                {
                    var actual = easingFunction.Ease(key);
                    var expected = easeGroundTruth[key].Item2;
                    VerifyAreClose(expected, actual, "Verify Ease(" + key + ") method call output in EasingMode::" + easingFunction.EasingMode);
                }

                easingFunction.EasingMode = EasingMode.EaseInOut;
                foreach (double key in easeGroundTruth.Keys)
                {
                    var actual = easingFunction.Ease(key);
                    var expected = easeGroundTruth[key].Item3;
                    VerifyAreClose(expected, actual, "Verify Ease(" + key + ") method call output in EasingMode::" + easingFunction.EasingMode);
                }
            });
        }

        // This helper function prints out the results of an easing function's Ease() call given the key.  It is provided to facilatate easier generation of new
        // Input/Output test cases as it will print a formatted string that uses the naming conventions in this file and is ready for direct integration into the test code. 
        protected void PrintVerifyEaseOutput(EasingFunctionBase easingFunction, Dictionary<double, Tuple<double/*EaseIn*/, double/*EaseOut*/, double/*EaseInOut*/>> easeGroundTruth)
        {
            double ein, eout, einout;

            UIExecutor.Execute(() =>
            {
                foreach (double key in easeGroundTruth.Keys)
                {
                    easingFunction.EasingMode = EasingMode.EaseIn;
                    ein = easingFunction.Ease(key);

                    easingFunction.EasingMode = EasingMode.EaseOut;
                    eout = easingFunction.Ease(key);

                    easingFunction.EasingMode = EasingMode.EaseInOut;
                    einout = easingFunction.Ease(key);

                    Log.Comment("easeGroundTruth.Add("+key.ToString("F2")+", Tuple.Create(" + ein.ToString("R") + "," + eout.ToString("R") + "," + einout.ToString("R") + "));");                                       
                }
            });
        }
    }
}
