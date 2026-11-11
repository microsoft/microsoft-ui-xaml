// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    // Tests for the experimental Trigger and MultiTrigger conditional setters.
    [TestClass]
    public class TriggerTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        private const string TriggerXaml =
            "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>" +
            "  <Grid.Triggers>" +
            "    <Trigger Property='Tag' Value='on'>" +
            "      <Setter Target='Part.Visibility' Value='Visible' />" +
            "    </Trigger>" +
            "    <MultiTrigger>" +
            "      <MultiTrigger.Conditions>" +
            "        <Condition Property='Tag' Value='on' />" +
            "        <Condition Property='Width' Value='100' />" +
            "      </MultiTrigger.Conditions>" +
            "      <Setter Target='Part.Opacity' Value='0.5' />" +
            "      <Setter Target='Part.Height' Value='20' />" +
            "    </MultiTrigger>" +
            "  </Grid.Triggers>" +
            "  <Border x:Name='Part' Visibility='Collapsed' />" +
            "</Grid>";

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")]
        public void TriggersParseAndCanEnterTheTree()
        {
            Grid root = null;

            UIExecutor.Execute(() =>
            {
                root = (Grid)XamlReader.Load(TriggerXaml);
                Verify.AreEqual(2, root.Triggers.Count, "Both triggers should be parsed.");

                var trigger = root.Triggers[0] as Trigger;
                Verify.IsNotNull(trigger, "First trigger is a Trigger.");
                Verify.AreEqual("Tag", trigger.Property);
                Verify.AreEqual("on", trigger.Value as string);
                Verify.AreEqual(1, trigger.Setters.Count);

                var multi = root.Triggers[1] as MultiTrigger;
                Verify.IsNotNull(multi, "Second trigger is a MultiTrigger.");
                Verify.AreEqual(2, multi.Conditions.Count);
                Verify.AreEqual("Width", multi.Conditions[1].Property);
                Verify.AreEqual("100", multi.Conditions[1].Value as string);
                Verify.AreEqual(2, multi.Setters.Count);

                TestServices.WindowHelper.WindowContent = root;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(root.IsLoaded, "The element with triggers should be in the live tree.");
            });
        }
    }
}
