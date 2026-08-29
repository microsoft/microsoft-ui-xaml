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
    public class BounceEaseTests : EasingFunctionTestBase
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
        [Description("Validates that BounceEase easing function can switch easing modes and provide correct output with various Bounce/Bouncieness configurations.")]
        public void VerifyBounceEase()
        {
            BounceEase bounceEase = null;
            Dictionary<double, Tuple<double, double, double>> easeGroundTruth =
                new Dictionary<double, Tuple<double/*EaseIn*/, double/*EaseOut*/, double/*EaseInOut*/>>();

            UIExecutor.Execute(() =>
            {
                bounceEase = new BounceEase();
            });
            VerifyCanSwitchEasingModes(bounceEase);

            // Output ground truth dictionary has output for easing function in EaseMode::EaseIn, EaseMode::EaseOut, EaseMode::InOut 
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.25, Tuple.Create(0.10937502980232239, 0.47265625, 0.23437498509883881));
            easeGroundTruth.Add(0.50, Tuple.Create(0.46874997019767761, 0.53125, 0.5));
            easeGroundTruth.Add(0.75, Tuple.Create(0.52734375, 0.890625, 0.765625));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(bounceEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                bounceEase.Bounces = 0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.25, Tuple.Create(0.4375, 0.0625, 0.375));
            easeGroundTruth.Add(0.50, Tuple.Create(0.75, 0.25, 0.5));
            easeGroundTruth.Add(0.75, Tuple.Create(0.9375, 0.5625, 0.625));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(bounceEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                bounceEase.Bounces = -5;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.25, Tuple.Create(0.4375, 0.0625, 0.375));
            easeGroundTruth.Add(0.50, Tuple.Create(0.75, 0.25, 0.5));
            easeGroundTruth.Add(0.75, Tuple.Create(0.9375, 0.5625, 0.625));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(bounceEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                bounceEase.Bounces = 2;
                bounceEase.Bounciness = 0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.25, Tuple.Create(0.91401487588882446, 0.58547568321228027, 0.37855434417724609));
            easeGroundTruth.Add(0.50, Tuple.Create(0.75710868835449219, 0.24289131164550781, 0.5));
            easeGroundTruth.Add(0.75, Tuple.Create(0.41452434659004211, 0.085985124111175537, 0.62144565582275391));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(bounceEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                bounceEase.Bounciness = -5;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.25, Tuple.Create(0.91401487588882446, 0.58547568321228027, 0.37855434417724609));
            easeGroundTruth.Add(0.50, Tuple.Create(0.75710868835449219, 0.24289131164550781, 0.5));
            easeGroundTruth.Add(0.75, Tuple.Create(0.41452434659004211, 0.085985124111175537, 0.62144565582275391));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(bounceEase, easeGroundTruth);
        }
    }
}
