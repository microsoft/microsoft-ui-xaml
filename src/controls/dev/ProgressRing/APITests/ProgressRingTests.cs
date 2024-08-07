﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Automation;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ProgressRingTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyAccessibilityView()
        {
            RunOnUIThread.Execute(() =>
            {
                var progressRing = new ProgressRing();
                progressRing.IsActive = true;

                Verify.AreNotEqual(AccessibilityView.Raw, AutomationProperties.GetAccessibilityView(progressRing));

                progressRing.IsActive = false;
                Verify.AreEqual(AccessibilityView.Raw, AutomationProperties.GetAccessibilityView(progressRing));
            });
        }
    }
}
