// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WEX.TestExecution;
using Private.Infrastructure;
using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlPrimitives = Microsoft.UI.Xaml.Controls.Primitives;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class ThumbDerivationTests : XamlTestsBase
    {
        private sealed class DerivedThumb : XamlPrimitives.Thumb
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
                _ = new DerivedThumb();
            });
        }
    }
}
