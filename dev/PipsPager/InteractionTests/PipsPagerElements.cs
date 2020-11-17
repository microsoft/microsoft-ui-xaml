using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;
using Microsoft.Windows.Apps.Test.Foundation;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    public class PipsPagerElements
    {
        private UIObject PipsPager;
        private UIObject NextPageButton;
        private UIObject PreviousPageButton;

        public UIObject GetPipsPager()
        {
            return GetElement(ref PipsPager, "PipsPager");
        }
        public UIObject GetPreviousPageButton()
        {
            return GetElementWithinPager(ref PreviousPageButton, "PreviousPageButton");
        }

        public UIObject GetNextPageButton()
        {
            return GetElementWithinPager(ref NextPageButton, "NextPageButton");
        }

        public UIObject GetPipWithPageNumber(string elementName)
        {
            foreach (var element in GetPipsPager().Children)
            {
                if (element.Name == elementName)
                {
                    return element;
                }
            }
            return null;
        }
        private T GetElement<T>(ref T element, string elementName) where T : UIObject
        {
            if (element == null)
            {
                Log.Comment("Find the " + elementName);
                element = FindElement.ByNameOrId<T>(elementName);
                Verify.IsNotNull(element);
            }
            return element;
        }
        private T GetElementWithinPager<T>(ref T element, string elementName) where T : UIObject
        {
            if (element == null)
            {
                Log.Comment("Find the " + elementName);

                foreach (T child in GetPipsPager().Children)
                {
                    if (child.AutomationId == elementName)
                    {
                        element = child;
                        break;
                    }
                }
                Verify.IsNotNull(element);
            }
            return element;
        }
    }
}
