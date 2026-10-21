// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using XamlControls = Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class DatePickerFlyoutItemDerivationTests : XamlTestsBase
    {
        private sealed class DerivedDatePickerFlyoutItem : XamlControls.DatePickerFlyoutItem
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
                var item = new DerivedDatePickerFlyoutItem
                {
                    PrimaryText = "Primary",
                    SecondaryText = "Secondary"
                };

                Verify.AreEqual("Primary", item.PrimaryText);
                Verify.AreEqual("Secondary", item.SecondaryText);
            });
        }
    }
}
