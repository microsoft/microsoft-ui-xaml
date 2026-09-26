// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WEX.TestExecution;
using Private.Infrastructure;
using XamlShapes = Microsoft.UI.Xaml.Shapes;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class PolylineDerivationTests : XamlTestsBase
    {
        private sealed class DerivedPolyline : XamlShapes.Polyline
        {
        }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) => XamlTestsBase.SetupBase(context);

        [ClassCleanup]
        public void Cleanup() => base.CommonClassCleanup();

        [TestMethod]
        public void CanDerive()
        {
            UIExecutor.Execute(() =>
            {
                _ = new DerivedPolyline();
            });
        }
    }
}
