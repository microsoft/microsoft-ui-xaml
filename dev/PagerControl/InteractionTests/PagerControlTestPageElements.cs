using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System;
using System.Numerics;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Microsoft.Windows.Apps.Test.Foundation;
using System.Collections.Generic;
using Windows.UI.Xaml.Media;
using Windows.UI.Composition;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    class PagerControlTestPageElements
    {
        public UIObject GetPagerControl()
        {
            return GetElement(ref PagerControl, "TestPager");
        }
        private UIObject PagerControl;

        public UIObject GetPagerNumberBox()
        {
            return GetElementWithinPager(ref PagerNumberBox, "NumberBoxDisplay");
        }
        private UIObject PagerNumberBox;

        public UIObject GetPagerComboBox()
        {
            return GetElementWithinPager(ref PagerComboBox, "ComboBoxDisplay");
        }
        private UIObject PagerComboBox;

        public UIObject GetNumberPanelButton(string elementName)
        {
            foreach (var element in GetPagerControl().Children)
            {
                if (element.AutomationId == elementName)
                {
                    return element;
                }
            }
            return null;
        }

        public UIObject GetFirstPageButton()
        {
            return GetElementWithinPager(ref FirstPageButton, "FirstPageButton");
        }
        private UIObject FirstPageButton;

        public UIObject GetPreviousPageButton()
        {
            return GetElementWithinPager(ref PreviousPageButton, "PreviousPageButton");
        }
        private UIObject PreviousPageButton;

        public UIObject GetNextPageButton()
        {
            return GetElementWithinPager(ref NextPageButton, "NextPageButton");
        }
        private UIObject NextPageButton;

        public UIObject GetLastPageButton()
        {
            return GetElementWithinPager(ref LastPageButton, "LastPageButton");
        }
        private UIObject LastPageButton;

        public ComboBox GetFirstPageButtonVisibilityComboBox()
        {
            return GetElement(ref FirstPageButtonVisibilityComboBox, "FirstPageButtonVisibilityComboBox");
        }
        private ComboBox FirstPageButtonVisibilityComboBox;

        public ComboBox GetPreviousPageButtonVisibilityComboBox()
        {
            return GetElement(ref PreviousPageButtonVisibilityComboBox, "PreviousPageButtonVisibilityComboBox");
        }
        private ComboBox PreviousPageButtonVisibilityComboBox;

        public ComboBox GetNextPageButtonVisibilityComboBox()
        {
            return GetElement(ref NextPageButtonVisibilityComboBox, "NextPageButtonVisibilityComboBox");
        }
        private ComboBox NextPageButtonVisibilityComboBox;

        public ComboBox GetLastPageButtonVisibilityComboBox()
        {
            return GetElement(ref LastPageButtonVisibilityComboBox, "LastPageButtonVisibilityComboBox");
        }
        private ComboBox LastPageButtonVisibilityComboBox;

        public ComboBox GetDisplayModeComboBox()
        {
            return GetElement(ref DisplayModeComboBox, "PagerDisplayModeComboBox");
        }
        private ComboBox DisplayModeComboBox;

        public CheckBox GetFirstPageButtonVisibilityCheckBox()
        {
            return GetElement(ref FirstPageButtonVisibilityCheckBox, "FirstPageButtonVisibilityCheckBox");
        }
        private CheckBox FirstPageButtonVisibilityCheckBox;

        public CheckBox GetPreviousPageButtonVisibilityCheckBox()
        {
            return GetElement(ref PreviousPageButtonVisibilityCheckBox, "PreviousPageButtonVisibilityCheckBox");
        }
        private CheckBox PreviousPageButtonVisibilityCheckBox;

        public CheckBox GetNextPageButtonVisibilityCheckBox()
        {
            return GetElement(ref NextPageButtonVisibilityCheckBox, "NextPageButtonVisibilityCheckBox");
        }
        private CheckBox NextPageButtonVisibilityCheckBox;

        public CheckBox GetLastPageButtonVisibilityCheckBox()
        {
            return GetElement(ref LastPageButtonVisibilityCheckBox, "LastPageButtonVisibilityCheckBox");
        }
        private CheckBox LastPageButtonVisibilityCheckBox;

        public CheckBox GetFirstPageButtonIsEnabledCheckBox()
        {
            return GetElement(ref FirstPageButtonIsEnabledCheckBox, "FirstPageButtonIsEnabledCheckBox");
        }
        private CheckBox FirstPageButtonIsEnabledCheckBox;

        public CheckBox GetPreviousPageButtonIsEnabledCheckBox()
        {
            return GetElement(ref PreviousPageButtonIsEnabledCheckBox, "PreviousPageButtonIsEnabledCheckBox");
        }
        private CheckBox PreviousPageButtonIsEnabledCheckBox;

        public CheckBox GetNextPageButtonIsEnabledCheckBox()
        {
            return GetElement(ref NextPageButtonIsEnabledCheckBox, "NextPageButtonIsEnabledCheckBox");
        }
        private CheckBox NextPageButtonIsEnabledCheckBox;

        public CheckBox GetLastPageButtonIsEnabledCheckBox()
        {
            return GetElement(ref LastPageButtonIsEnabledCheckBox, "LastPageButtonIsEnabledCheckBox");
        }
        private CheckBox LastPageButtonIsEnabledCheckBox;

        public CheckBox GetNumberBoxVisibilityCheckBox()
        {
            return GetElement(ref NumberBoxVisibilityCheckBox, "NumberBoxVisibilityCheckBox");
        }
        private CheckBox NumberBoxVisibilityCheckBox;

        public CheckBox GetNumberBoxIsEnabledCheckBox()
        {
            return GetElement(ref NumberBoxIsEnabledCheckBox, "NumberBoxIsEnabledCheckBox");
        }
        private CheckBox NumberBoxIsEnabledCheckBox;

        public CheckBox GetComboBoxVisibilityCheckBox()
        {
            return GetElement(ref ComboBoxVisibilityCheckBox, "ComboBoxVisibilityCheckBox");
        }
        private CheckBox ComboBoxVisibilityCheckBox;

        public CheckBox GetComboBoxIsEnabledCheckBox()
        {
            return GetElement(ref ComboBoxIsEnabledCheckBox, "ComboBoxIsEnabledCheckBox");
        }
        private CheckBox ComboBoxIsEnabledCheckBox;

        public CheckBox GetNumberPanelVisibilityCheckBox()
        {
            return GetElement(ref NumberPanelVisibilityCheckBox, "NumberPanelVisibilityCheckBox");
        }
        private CheckBox NumberPanelVisibilityCheckBox;

        public TextBlock GetNumberPanelContentTextBlock()
        {
            return GetElement(ref NumberPanelContentTextBlock, "NumberPanelContentTextBlock");
        }
        private TextBlock NumberPanelContentTextBlock;

        public Button GetUpdateMarginTextBlockButton()
        {
            return GetElement(ref UpdateMarginTextBlockButton, "UpdateMarginTextBlockButton");
        }
        private Button UpdateMarginTextBlockButton;

        public TextBlock GetCurrentPageIdentifierLeftMarginTextBlock()
        {
            return GetElement(ref CurrentPageIdentifierLeftMarginTextBlock, "CurrentPageIdentifierLeftMarginTextBlock");
        }
        private TextBlock CurrentPageIdentifierLeftMarginTextBlock;

        public TextBlock GetNumberOfPagesTextBlock()
        {
            return GetElement(ref NumberOfPagesTextBlock, "NumberOfPagesTextBlock");
        }
        private TextBlock NumberOfPagesTextBlock;

        public Button GetNumberOfPagesSetterButton()
        {
            return GetElement(ref NumberOfPagesSetterButton, "NumberOfPagesSetterButton");
        }
        private Button NumberOfPagesSetterButton;

        public TextBlock GetCurrentPageTextBlock()
        {
            return GetElement(ref CurrentPageTextBlock, "CurrentPageTextBlock");
        }
        private TextBlock CurrentPageTextBlock;

        public TextBlock GetPreviousPageTextBlock()
        {
            return GetElement(ref PreviousPageTextBlock, "PreviousPageTextBlock");
        }
        private TextBlock PreviousPageTextBlock;

        public Button GetIncreaseNumberOfPagesButton()
        {
            return GetElement(ref IncreaseNumberOfPagesButton, "IncreaseNumberOfPagesButton");
        }
        private Button IncreaseNumberOfPagesButton;

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

                foreach (T child in GetPagerControl().Children)
                {
                    if (child.AutomationId == elementName)
                    {
                        element = child;
                    }
                }
                Verify.IsNotNull(element);
            }
            return element;
        }

        public enum DisplayModes
        {
            Auto,
            NumberBox,
            ComboBox,
            NumberPanel
        };

        public enum ButtonVisibilityModes
        {
            Auto,
            AlwaysVisible,
            HiddenOnEdge,
            None
        }
    }
}
