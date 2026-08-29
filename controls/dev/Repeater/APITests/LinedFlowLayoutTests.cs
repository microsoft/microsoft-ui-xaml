// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using Common;
using Microsoft.UI.Xaml.Controls;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class LinedFlowLayoutTests : ApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies the LinedFlowLayout default property values.")]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                LinedFlowLayout linedFlowLayout = new LinedFlowLayout();
                Verify.IsNotNull(linedFlowLayout);

                Log.Comment("Verifying LinedFlowLayout default property values");
                Verify.AreEqual(0.0, linedFlowLayout.ActualLineHeight);
                Verify.AreEqual(double.NaN, linedFlowLayout.LineHeight);
                Verify.AreEqual(0.0, linedFlowLayout.LineSpacing);
                Verify.AreEqual(0.0, linedFlowLayout.MinItemSpacing);
                Verify.AreEqual(LinedFlowLayoutItemsJustification.Start, linedFlowLayout.ItemsJustification);
                Verify.AreEqual(LinedFlowLayoutItemsStretch.None, linedFlowLayout.ItemsStretch);
            });
        }
    }
}
