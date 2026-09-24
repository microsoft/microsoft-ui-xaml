// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WEX.TestExecution;
using Private.Infrastructure;
using XamlControls = Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class ImageDerivationTests : XamlTestsBase
    {
        private sealed class DerivedImage : XamlControls.Image { }
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) => XamlTestsBase.SetupBase(context);
        [ClassCleanup] public void ClassCleanup() => base.CommonClassCleanup();
        [TestMethod] public void CanDeriveFromImage() { UIExecutor.Execute(() => { _ = new DerivedImage(); }); }
    }
}
