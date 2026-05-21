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

namespace Microsoft.UI.Xaml.Tests.Animation.EasingFunctionBaseTests
{
    [TestClass]
    public class QuarticEaseTests : EasingFunctionTestBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("__ExecutionUnit", "9D70DB22-1712-4654-BADE-C2F5565945E1")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [Description("Validates that QuarticEase easing function can switch easing modes and provide correct output.")]
        public void VerifyQuarticEase()
        {
            QuarticEase quarticEase = null;
            Dictionary<double, Tuple<double, double, double>> easeGroundTruth =
                new Dictionary<double, Tuple<double/*EaseIn*/, double/*EaseOut*/, double/*EaseInOut*/>>();

            UIExecutor.Execute(() =>
            {
                quarticEase = new QuarticEase();
            });
            VerifyCanSwitchEasingModes(quarticEase);

            // Output ground truth dictionary has output for easing function in EaseMode::EaseIn, EaseMode::EaseOut, EaseMode::InOut 
            easeGroundTruth.Add(-1.0, Tuple.Create(1.0, -15.0, 8.0));
            easeGroundTruth.Add(0.0, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(1.0, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(5.0, Tuple.Create(625.0, -255.0, -2047.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.PositiveInfinity, double.NegativeInfinity, double.NegativeInfinity));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.PositiveInfinity, double.NegativeInfinity, double.PositiveInfinity));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(quarticEase, easeGroundTruth);
        }
    }
}
