// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class XamlControlsResourcesDerivationTests : ApiTestBase
    {
        private sealed class DerivedXamlControlsResources : XamlControlsResources
        {
        }

        [TestMethod]
        public void CanDeriveFromXamlControlsResources()
        {
            RunOnUIThread.Execute(() =>
            {
                var resources = new DerivedXamlControlsResources
                {
                    UseCompactResources = true
                };

                Verify.IsTrue(resources.UseCompactResources);
            });
        }
    }
}
