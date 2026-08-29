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
    public class ExponentialEaseTests : EasingFunctionTestBase
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
        [Description("Validates that ExponentialEase easing function can switch easing modes and provide correct output with various exponent configurations.")]
        public void VerifyExponentialEase()
        {
            ExponentialEase exponentialEase = null;
            Dictionary<double, Tuple<double, double, double>> easeGroundTruth =
                new Dictionary<double, Tuple<double/*EaseIn*/, double/*EaseOut*/, double/*EaseInOut*/>>();

            UIExecutor.Execute(() =>
            {
                exponentialEase = new ExponentialEase();
            });
            VerifyCanSwitchEasingModes(exponentialEase);

            // Output ground truth dictionary has output for easing function in EaseMode::EaseIn, EaseMode::EaseOut, EaseMode::InOut 
            easeGroundTruth.Add(-1.0, Tuple.Create(-0.1353352963924408, -7.3890552520751953, -0.076825462281703949));
            easeGroundTruth.Add(0.0, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.5, Tuple.Create(0.26894143223762512, 0.73105859756469727, 0.5));
            easeGroundTruth.Add(1.0, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.PositiveInfinity, 1.1565176248550415, 1.0782588720321655));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(-0.15651765465736389, double.NegativeInfinity, -0.078258827328681946));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(exponentialEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                exponentialEase.Exponent = 0.0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.0, Tuple.Create(-1.0, -1.0, -1.0));
            easeGroundTruth.Add(0.0, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.5, Tuple.Create(0.5, 0.5, 0.5));
            easeGroundTruth.Add(1.0, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.PositiveInfinity, double.PositiveInfinity, double.PositiveInfinity));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NegativeInfinity, double.NegativeInfinity, double.NegativeInfinity));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(exponentialEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                exponentialEase.Exponent = -1.0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.0, Tuple.Create(-2.7182817459106445, -0.36787939071655273, -5.0536689758300781));
            easeGroundTruth.Add(0.0, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.5, Tuple.Create(0.622459352016449, 0.377540647983551, 0.5));
            easeGroundTruth.Add(1.0, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(1.5819767713546753, double.PositiveInfinity, double.PositiveInfinity));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NegativeInfinity, -0.58197677135467529, double.NegativeInfinity));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(exponentialEase, easeGroundTruth);
        }
    }
}
