using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    public class PipsPageRTestBase
    {
        protected PipsPagerElements elements;

        protected void SelectPageInNumberPanel(int index)
        {
            InputHelper.LeftClick(elements.GetPipWithPageNumber("Page " + index.ToString()));
            Wait.ForIdle();
        }

        public enum ButtonVisibilityMode
        {
            Visible,
            VisibleOnHover,
            Collapsed
        }
    }
}
