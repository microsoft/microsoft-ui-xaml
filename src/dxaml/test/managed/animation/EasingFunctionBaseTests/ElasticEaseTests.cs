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
    public class ElasticEaseTests : EasingFunctionTestBase
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
        [Description("Validates that ElasticEase easing function can switch easing modes and provide correct output with various springiness and oscillation configurations.")]
        public void VerifyElasticEase()
        {
            ElasticEase elasticEase = null;
            Dictionary<double, Tuple<double, double, double>> easeGroundTruth =
                new Dictionary<double, Tuple<double/*EaseIn*/, double/*EaseOut*/, double/*EaseInOut*/>>();

            UIExecutor.Execute(() =>
            {
                elasticEase = new ElasticEase();
            });
            VerifyCanSwitchEasingModes(elasticEase);

            // Output ground truth dictionary has output for easing function in EaseMode::EaseIn, EaseMode::EaseOut, EaseMode::InOut 
            easeGroundTruth.Add(-1.00, Tuple.Create(0.049787074327468872, 1.0000289678573608, -3.5930526109950733E-08));
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.50, Tuple.Create(-0.12899437546730042, 1.128994345664978, 0.5));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(elasticEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                elasticEase.Springiness = 0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.00, Tuple.Create(1.0, 1.0000027418136597, -1.3749147456110222E-06));
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.50, Tuple.Create(-0.35355350375175476, 1.3535535335540772, 0.5));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(elasticEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                elasticEase.Springiness = -1;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.00, Tuple.Create(2.7182817459106445, 1.0000019073486328, -6.9483639890677296E-06));
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.50, Tuple.Create(-0.44014537334442139, 1.4401453733444214, 0.5));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN));

            VerifyEaseOutput(elasticEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                elasticEase.Springiness = 2;
                elasticEase.Oscillations = 0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.00, Tuple.Create(0.1353352963924408, 1.0000007152557373, -6.7162950756483042E-09));
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.50, Tuple.Create(0.19017030298709869, 0.8098297119140625, 0.5));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN)); ;

            VerifyEaseOutput(elasticEase, easeGroundTruth);

            UIExecutor.Execute(() =>
            {
                elasticEase.Oscillations = 0;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.00, Tuple.Create(0.1353352963924408,1.0000007152557373,-6.716295075648304E-09));
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.50, Tuple.Create(0.1901703029870987,0.8098297119140625,0.5));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN)); ;

            // Since we clamp negative values to 0, these should be the same as above.

            UIExecutor.Execute(() =>
            {
                elasticEase.Oscillations = -1;
            });

            easeGroundTruth.Clear();
            easeGroundTruth.Add(-1.00, Tuple.Create(0.1353352963924408,1.0000007152557373,-6.716295075648304E-09));
            easeGroundTruth.Add(0.00, Tuple.Create(0.0, 0.0, 0.0));
            easeGroundTruth.Add(0.50, Tuple.Create(0.1901703029870987,0.8098297119140625,0.5));
            easeGroundTruth.Add(1.00, Tuple.Create(1.0, 1.0, 1.0));
            easeGroundTruth.Add(double.MaxValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.MinValue, Tuple.Create(double.NaN, double.NaN, double.NaN));
            easeGroundTruth.Add(double.NaN, Tuple.Create(double.NaN, double.NaN, double.NaN)); ;

            VerifyEaseOutput(elasticEase, easeGroundTruth);
        }
    }
}
