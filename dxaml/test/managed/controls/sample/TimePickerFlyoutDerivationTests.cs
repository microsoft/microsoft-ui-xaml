using WEX.TestExecution;
using Private.Infrastructure;
using XamlControls = Microsoft.UI.Xaml.Controls;
namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class TimePickerFlyoutDerivationTests : XamlTestsBase
    {
        private sealed class DerivedTimePickerFlyout : XamlControls.TimePickerFlyout { }
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) => XamlTestsBase.SetupBase(context);
        [ClassCleanup] public void ClassCleanup() => base.CommonClassCleanup();
        [TestMethod] public void CanDeriveFromTimePickerFlyout() { UIExecutor.Execute(() => { _ = new DerivedTimePickerFlyout(); }); }
    }
}
