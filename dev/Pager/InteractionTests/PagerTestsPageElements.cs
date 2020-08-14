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
    class PagerTestsPageElements
    {
        public UIObject GetPager()
        {
            return GetElement(ref Pager, "TestPager");
        }
        private UIObject Pager;

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

        public UIObject GetPageNumberPanel()
        {
            return GetElementWithinPager(ref PagerNumberPanel, "NumberPanelItemsRepeater");
        }
        private UIObject PagerNumberPanel;

        public UIObject GetPagerNumberPanelCurrentPageIdentifier()
        {
            return GetElementWithinPager(ref PagerNumberPanelCurrentPageIdentifier, "NumberPanelCurrentPageIdentifier");
        }
        private UIObject PagerNumberPanelCurrentPageIdentifier;

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

        public ListView GetEventOutputListView()
        {
            return GetElement(ref EventOutputListView, "PagerEventListViewDisplay");
        }
        private ListView EventOutputListView;

        public string GetLastEventOutput()
        {
            return GetEventOutputListView().LastChild.Name;
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

        private T GetElementWithinPager<T>(ref T element, string elementName) where T: UIObject
        {
            if (element == null)
            {
                Log.Comment("Find the " + elementName);

                foreach(T child in GetPager().Children)
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

        public bool ComboBoxDisplayModeActive = true;
        public bool NumberBoxDisplayModeActive = false;
    }
}
