// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests
{
    [TestClass]
    public class ItemsRepeaterScrollHostDerivationTests : ApiTestBase
    {
        private sealed class DerivedItemsRepeaterScrollHost : ItemsRepeaterScrollHost
        {
        }

        [TestMethod]
        public void CanDeriveFromItemsRepeaterScrollHost()
        {
            RunOnUIThread.Execute(() =>
            {
                var scrollHost = new DerivedItemsRepeaterScrollHost
                {
                    HorizontalAnchorRatio = 0.5,
                    VerticalAnchorRatio = 0.5
                };

                Verify.AreEqual(0.5, scrollHost.HorizontalAnchorRatio);
                Verify.AreEqual(0.5, scrollHost.VerticalAnchorRatio);
            });
        }
    }
}
