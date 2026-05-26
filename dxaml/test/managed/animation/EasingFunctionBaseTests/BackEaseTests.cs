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
    public class BackEaseTests : EasingFunctionTestBase
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
        [Description("Validates that BackEase easing function can switch easing modes and provide correct output.")]
        public void VerifyBackEase()
        {
            BackEase backEase = null;
            Dictionary<double, Tuple<double, double, double>> easeGroundTruth =
                new Dictionary<double, Tuple<double/*EaseIn*/, double/*EaseOut*/, double/*EaseInOut*/>>();

            // Boilerplate code below shared with all test cases 
            // Could/Should be condensed into a single method that uses reflection to set up
            // the object but issues with CLR compilation (e.g. Type doesn't seem to have GetMethod() etc in the 
            // libraries that build uses) Unknown if this is a local config issue or a gap in features.
            UIExecutor.Execute(() =>
            {
                backEase = new BackEase();
            });
            VerifyCanSwitchEasingModes(backEase);

            // Output ground truth dictionary has output for easing function in EaseMode::EaseIn, EaseMode::EaseOut, EaseMode::InOut 
            // This table is a snapshot of behvaiour (including any errors introduced in finite precision math operations and FP type conversions)
            easeGroundTruth.Add(-1.0, Tuple.Create(0.0, -6.9999995231628418, 0.0));
            easeGroundTruth.Add(0.0, Tuple.Create(0.0, -1.1920928955078125E-07, 0.0));
            easeGroundTruth.Add(0.5, Tuple.Create(-0.375, 1.375, 0.49999994039535522));
            easeGroundTruth.Add(1.0, Tuple.Create(1.0000001192092896, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, 1.0, 1.0));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(0.0, double.NaN, 0.0));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(backEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                backEase.Amplitude = 0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.0, Tuple.Create(0.0, -7.0, 0.0));
            easeGroundTruth.Add(0.0, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.5, Tuple.Create(0.125, 0.875, 0.5));
            easeGroundTruth.Add(1.0, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, 1.0, 1.0));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(0.0, double.NaN, 0.0));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(backEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                backEase.Amplitude = -1;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.0, Tuple.Create(0.0, -7.0, 0.0));
            easeGroundTruth.Add(0.0, Tuple.Create(0.0, 5.9604644775390625E-08, 0.0));
            easeGroundTruth.Add(0.5, Tuple.Create(0.625, 0.375, 0.5));
            easeGroundTruth.Add(1.0, Tuple.Create(0.99999994039535522, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, 1.0, 1.0));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(0.0, double.NaN, 0.0));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(backEase, easeGroundTruth);
        }

    }
}
