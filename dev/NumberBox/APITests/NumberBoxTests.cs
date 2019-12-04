// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using System.Linq;
using System.Collections.Generic;
using MUXControlsTestApp;
using Microsoft.UI.Xaml.Controls;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class NumberBoxTests
    {
        [TestCleanup]
        public void TestCleanup()
        {
            TestUtilities.ClearVisualTreeRoot();
        }

        [TestMethod]
        public void VerifyVisualTree()
        {
            NumberBox numberBox = null;
            RunOnUIThread.Execute(() =>
            {
                numberBox = new NumberBox();
                numberBox.Width = 200;
                numberBox.Height = 100;
            });
            TestUtilities.SetAsVisualTreeRoot(numberBox);
            Verify.IsNotNull(numberBox);
            VisualTreeTestHelper.VerifyVisualTree(root: numberBox, masterFilePrefix: "NumberBoxHiddenSpinButtons");

            RunOnUIThread.Execute(() =>
            {
                numberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Compact;
            });
            VisualTreeTestHelper.VerifyVisualTree(root: numberBox, masterFilePrefix: "NumberBoxCompactSpinButtons");

            RunOnUIThread.Execute(() =>
            {
                numberBox.SpinButtonPlacementMode = NumberBoxSpinButtonPlacementMode.Inline;
            });
            VisualTreeTestHelper.VerifyVisualTree(root: numberBox, masterFilePrefix: "NumberBoxInlineSpinButtons");
        }
    }
}
