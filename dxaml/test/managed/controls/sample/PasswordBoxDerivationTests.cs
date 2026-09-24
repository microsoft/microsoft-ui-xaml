using WEX.TestExecution;
using Private.Infrastructure;
using XamlControls = Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class PasswordBoxDerivationTests : XamlTestsBase
    {
        private sealed class DerivedPasswordBox : XamlControls.PasswordBox { }
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext c) => XamlTestsBase.SetupBase(c);

        [ClassCleanup]
        public void Cleanup() => base.CommonClassCleanup();

        [TestMethod]
        public void CanDerive()
        {
            UIExecutor.Execute(() => { _ = new DerivedPasswordBox(); });
        }
    }
}