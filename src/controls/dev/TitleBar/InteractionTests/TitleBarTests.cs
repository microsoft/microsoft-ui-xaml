// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using MUXTestInfra.Shared.Infra;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TitleBarTests
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

        private string GetStatus()
        {
            TextBlock statusBlock = FindElement.ByName<TextBlock>("DragRegionStatusTextBlock");
            Verify.IsNotNull(statusBlock, "Should find DragRegionStatusTextBlock");
            return statusBlock.DocumentText ?? "";
        }

        [TestMethod]
        public void AutoRefreshDragRegionsDefaultValueTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Get default AutoRefreshDragRegions value");
                Button btn = FindElement.ByName<Button>("GetAutoRefreshValueButton");
                Verify.IsNotNull(btn, "Should find GetAutoRefreshValueButton");
                btn.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("AutoRefresh:False", GetStatus(), "Default AutoRefreshDragRegions should be False");
            }
        }

        [TestMethod]
        public void SetAutoRefreshDragRegionsTrueTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Set AutoRefreshDragRegions to True");
                Button btn = FindElement.ByName<Button>("SetAutoRefreshTrueButton");
                Verify.IsNotNull(btn, "Should find SetAutoRefreshTrueButton");
                btn.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("AutoRefresh:True", GetStatus(), "AutoRefreshDragRegions should be True");
            }
        }

        [TestMethod]
        public void SetAutoRefreshDragRegionsFalseTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Set AutoRefreshDragRegions to True then back to False");
                Button setTrue = FindElement.ByName<Button>("SetAutoRefreshTrueButton");
                setTrue.InvokeAndWait();
                Wait.ForIdle();

                Button setFalse = FindElement.ByName<Button>("SetAutoRefreshFalseButton");
                Verify.IsNotNull(setFalse, "Should find SetAutoRefreshFalseButton");
                setFalse.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("AutoRefresh:False", GetStatus(), "AutoRefreshDragRegions should be False after setting back");
            }
        }

        [TestMethod]
        public void RecomputeDragRegionsTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Call RecomputeDragRegions");
                Button btn = FindElement.ByName<Button>("RecomputeDragRegionsButton");
                Verify.IsNotNull(btn, "Should find RecomputeDragRegionsButton");
                btn.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("RecomputeDragRegions:Success", GetStatus(), "RecomputeDragRegions should succeed");
            }
        }

        [TestMethod]
        public void IsDragRegionSetTrueTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Set IsDragRegion=True");
                Button btn = FindElement.ByName<Button>("SetIsDragRegionTrueButton");
                Verify.IsNotNull(btn, "Should find SetIsDragRegionTrueButton");
                btn.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("IsDragRegion:True", GetStatus(), "IsDragRegion should be True");
            }
        }

        [TestMethod]
        public void IsDragRegionSetFalseTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Set IsDragRegion=False");
                Button btn = FindElement.ByName<Button>("SetIsDragRegionFalseButton");
                Verify.IsNotNull(btn, "Should find SetIsDragRegionFalseButton");
                btn.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("IsDragRegion:False", GetStatus(), "IsDragRegion should be False");
            }
        }

        [TestMethod]
        public void IsDragRegionDefaultValueTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Get default IsDragRegion value (should be null)");
                Button btn = FindElement.ByName<Button>("GetIsDragRegionValueButton");
                Verify.IsNotNull(btn, "Should find GetIsDragRegionValueButton");
                btn.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("IsDragRegion:null", GetStatus(), "Default IsDragRegion should be null");
            }
        }

        [TestMethod]
        public void IsDragRegionClearValueTest()
        {
            using (var setup = new TestSetupHelper("TitleBar Tests"))
            {
                Log.Comment("Set IsDragRegion then clear it");
                Button setBtn = FindElement.ByName<Button>("SetIsDragRegionTrueButton");
                setBtn.InvokeAndWait();
                Wait.ForIdle();

                Button clearBtn = FindElement.ByName<Button>("ClearIsDragRegionButton");
                Verify.IsNotNull(clearBtn, "Should find ClearIsDragRegionButton");
                clearBtn.InvokeAndWait();
                Wait.ForIdle();

                Verify.AreEqual("IsDragRegion:null", GetStatus(), "IsDragRegion should be null after clearing");
            }
        }
    }
}
