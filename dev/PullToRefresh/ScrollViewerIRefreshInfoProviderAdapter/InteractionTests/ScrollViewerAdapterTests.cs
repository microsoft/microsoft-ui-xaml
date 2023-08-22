// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ScrollViewerAdapterTests
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
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125 and internal issue 11754081
        public void DeconstructionHandlesCorrectlyTest()
        {
            using (var setup = new TestSetupHelper("ScrollViewerAdapter Tests")) // This literally clicks the button corresponding to the test page.
            {
                Log.Comment("Retrieve AdaptOnSVWithoutWaitingForLoadedButton button.");
                UIObject AdaptOnSVWithoutWaitingForLoadedButtonUIObject = FindElement.ByName("AdaptOnSVWithoutWaitingForLoadedButton");
                Verify.IsNotNull(AdaptOnSVWithoutWaitingForLoadedButtonUIObject, "Verifying that we found a UIElement called AdaptOnSVWithoutWaitingForLoadedButton");

                Log.Comment("Retrieve RemoveButton button.");
                UIObject RemoveButtonUIObject = FindElement.ByName("RemoveButton");
                Verify.IsNotNull(RemoveButtonUIObject, "Verifying that we found a UIElement called RemoveButton");

                Log.Comment("Retrieve GCButton button.");
                UIObject GCButtonUIObject = FindElement.ByName("GCButton");
                Verify.IsNotNull(GCButtonUIObject, "Verifying that we found a UIElement called GCButton");

                Log.Comment("Retrieve CheckLeaksButton button.");
                UIObject CheckLeaksButtonUIObject = FindElement.ByName("CheckLeaksButton");
                Verify.IsNotNull(CheckLeaksButtonUIObject, "Verifying that we found a UIElement called CheckLeaksButton");

                InputHelper.Tap(AdaptOnSVWithoutWaitingForLoadedButtonUIObject);
                InputHelper.Tap(RemoveButtonUIObject);
                InputHelper.Tap(GCButtonUIObject);
                InputHelper.Tap(CheckLeaksButtonUIObject);
            }
        }
    }
}
