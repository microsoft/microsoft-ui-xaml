using WEX.TestExecution;
using Private.Infrastructure;
using XamlControls = Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class SymbolIconDerivationTests : XamlTestsBase
    {
        private sealed class DerivedSymbolIcon : XamlControls.SymbolIcon { }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) => XamlTestsBase.SetupBase(context);

        [ClassCleanup]
        public void ClassCleanup() => base.CommonClassCleanup();

        [TestMethod]
        public void CanDeriveFromSymbolIcon()
        {
            UIExecutor.Execute(() =>
            {
                var icon = new DerivedSymbolIcon { Symbol = XamlControls.Symbol.Accept };
                Verify.AreEqual(XamlControls.Symbol.Accept, icon.Symbol);
            });
        }
    }
}
