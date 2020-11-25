using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;

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
        private ComboBox PreviousPageButtonVisibilityComboBox;
        private ComboBox NextPageButtonVisibilityComboBox;
        private ComboBox NumberOfPagesComboBox;
        private ComboBox MaxVisualIndicatorsComboBox;
        private ComboBox OrientationComboBox;
        private CheckBox PreviousPageButtonIsVisibleCheckBox;
        private CheckBox PreviousPageButtonIsEnabledCheckBox;
        private CheckBox NextPageButtonIsVisibleCheckBox;
        private CheckBox NextPageButtonIsEnabledCheckBox;
        private TextBlock CurrentPageTextBlock;
        private TextBlock CurrentNumberOfPagesTextBlock;
        private TextBlock CurrentMaxVisualIndicatorsTextBlock;
        private TextBlock CurrentOrientationTextBlock;

        public UIObject GetPipsPager()
        {
            return GetElement(ref PipsPager, "TestPipsPager");
        }
        public UIObject GetPreviousPageButton()
        {
            return GetElementWithinPager(ref PreviousPageButton, "PreviousPageButton");
        }

        public UIObject GetNextPageButton()
        {
            return GetElementWithinPager(ref NextPageButton, "NextPageButton");
        }

        public ComboBox GetPreviousPageButtonVisibilityComboBox()
        {
            return GetElement(ref PreviousPageButtonVisibilityComboBox, "PreviousPageButtonVisibilityComboBox");
        }

        public ComboBox GetNextPageButtonVisibilityComboBox()
        {
            return GetElement(ref NextPageButtonVisibilityComboBox, "NextPageButtonVisibilityComboBox");
        }

        public CheckBox GetPreviousPageButtonIsVisibleCheckBox()
        {
            return GetElement(ref PreviousPageButtonIsVisibleCheckBox, "PreviousPageButtonIsVisibleCheckBox");
        }
        public CheckBox GetPreviousPageButtonIsEnabledCheckBox()
        {
            return GetElement(ref PreviousPageButtonIsEnabledCheckBox, "PreviousPageButtonIsEnabledCheckBox");
        }
        public CheckBox GetNextPageButtonIsVisibleCheckBox()
        {
            return GetElement(ref NextPageButtonIsVisibleCheckBox, "NextPageButtonIsVisibleCheckBox");
        }
        public CheckBox GetNextPageButtonIsEnabledCheckBox()
        {
            return GetElement(ref NextPageButtonIsEnabledCheckBox, "NextPageButtonIsEnabledCheckBox");
        }

        public ComboBox GetNumberOfPagesComboBox()
        {
            return GetElement(ref NumberOfPagesComboBox, "TestPipsPagerNumberOfPagesComboBox");
        }

        public ComboBox GetMaxVisualIndicatorsComboBox()
        {
            return GetElement(ref MaxVisualIndicatorsComboBox, "TestPipsPagerMaxVisualIndicatorsComboBox");
        }
        public ComboBox GetOrientationComboBox()
        {
            return GetElement(ref OrientationComboBox, "TestPipsPagerOrientationComboBox");
        }
        public TextBlock GetCurrentPageTextBlock()
        {
            return GetElement(ref CurrentPageTextBlock, "CurrentPageIndexTextBlock");
        }

        public TextBlock GetCurrentMaxVisualIndicatorsTextBlock()
        {
            return GetElement(ref CurrentMaxVisualIndicatorsTextBlock, "CurrentMaxVisualIndicatorsTextBlock");
        }

        public TextBlock GetCurrentNumberOfPagesTextBlock()
        {
            return GetElement(ref CurrentNumberOfPagesTextBlock, "CurrentNumberOfPagesTextBlock");
        }

        public TextBlock GetCurrentOrientationTextBlock()
        {
            return GetElement(ref CurrentOrientationTextBlock, "CurrentOrientationTextBlock");
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
