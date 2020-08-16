// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    class RadioButtonsTestPageElements
    {
        public UIObject GetTestRadioButtons()
        {
            return GetElement(ref TestRadioButtons, "TestRadioButtons");
        }
        private UIObject TestRadioButtons;

        public Edit GetMaxColumnsTextBlock()
        {
            return GetElement(ref MaxColumnsTextBlock, "MaxColumnsTextBlock");
        }
        private Edit MaxColumnsTextBlock;

        public Button GetSetMaxColumnsButton()
        {
            return GetElement(ref SetMaxColumnsButton, "SetMaxColumnsButton");
        }
        private Button SetMaxColumnsButton;

        public Edit GetNumberOfItemsTextBlock()
        {
            return GetElement(ref NumberOfItemsTextBlock, "NumberOfItemsTextBlock");
        }
        private Edit NumberOfItemsTextBlock;

        public Button GetSetNumberOfItemsButton()
        {
            return GetElement(ref SetNumberOfItemsButton, "SetNumberOfItemsButton");
        }
        private Button SetNumberOfItemsButton;
        
        public Edit GetIndexToSelectTextBlock()
        {
            return GetElement(ref IndexToSelectTextBlock, "IndexToSelectTextBlock");
        }
        private Edit IndexToSelectTextBlock;

        public Button GetSelectByIndexButton()
        {
            return GetElement(ref SelectByIndexButton, "SelectByIndexButton");
        }
        private Button SelectByIndexButton;

        public Button GetSelectByItemButton()
        {
            return GetElement(ref SelectByItemButton, "SelectByItemButton");
        }
        private Button SelectByItemButton;

        public Button GetInsertDisplayRadioButtonButton()
        {
            return GetElement(ref InsertDisplayRadioButtonButton, "InsertDisplayRadioButtonButton");
        }
        private Button InsertDisplayRadioButtonButton;

        public Edit GetCustomIndexTextBox()
        {
            return GetElement(ref CustomIndexTextBox, "CustomIndexTextBox");
        }
        private Edit CustomIndexTextBox;

        public Edit GetCustomContentTextBox()
        {
            return GetElement(ref CustomContentTextBox, "CustomContentTextBox");
        }
        private Edit CustomContentTextBox;

        public CheckBox GetCustomDisabledCheckBox()
        {
            return GetElement(ref CustomDisabledCheckBox, "CustomDisabledCheckBox");
        }
        private CheckBox CustomDisabledCheckBox;

        public CheckBox GetCustomCheckedCheckBox()
        {
            return GetElement(ref CustomCheckedCheckBox, "CustomCheckedCheckBox");
        }
        private CheckBox CustomCheckedCheckBox;

        public TextBlock GetSelectedIndexTextBlock()
        {
            return GetElement(ref SelectedIndexTextBlock, "SelectedIndexTextBlock");
        }
        private TextBlock SelectedIndexTextBlock;

        public TextBlock GetReproTextBlock()
        {
            return GetElement(ref ReproTextBlock, "ReproTextBlock");
        }
        private TextBlock ReproTextBlock;

        public TextBlock GetSelectedItemTextBlock()
        {
            return GetElement(ref SelectedItemTextBlock, "SelectedItemTextBlock");
        }
        private TextBlock SelectedItemTextBlock;

        public TextBlock GetSelectedPositionInSetTextBlock()
        {
            return GetElement(ref SelectedPositionInSetTextBlock, "SelectedPositionInSetTextBlock");
        }
        private TextBlock SelectedPositionInSetTextBlock;

        public TextBlock GetSelectedSizeOfSetTextBlock()
        {
            return GetElement(ref SelectedSizeOfSetTextBlock, "SelectedSizeOfSetTextBlock");
        }
        private TextBlock SelectedSizeOfSetTextBlock;

        public CheckBox GetRadioButtonsHasFocusCheckBox()
        {
            return GetElement(ref RadioButtonsHasFocusCheckBox, "RadioButtonsHasFocusCheckBox");
        }
        private CheckBox RadioButtonsHasFocusCheckBox;

        public TextBlock GetFocusedIndexTextBlock()
        {
            return GetElement(ref FocusedIndexTextBlock, "FocusedIndexTextBlock");
        }
        private TextBlock FocusedIndexTextBlock;

        public TextBlock GetFocusedItemTextBlock()
        {
            return GetElement(ref FocusedItemTextBlock, "FocusedItemTextBlock");
        }
        private TextBlock FocusedItemTextBlock;

        public TextBlock GetLayoutNumberOfRowsTextBlock()
        {
            return GetElement(ref LayoutNumberOfRowsTextBlock, "LayoutNumberOfRowsTextBlock");
        }
        private TextBlock LayoutNumberOfRowsTextBlock;

        public TextBlock GetLayoutNumberOfColumnsTextBlock()
        {
            return GetElement(ref LayoutNumberOfColumnsTextBlock, "LayoutNumberOfColumnsTextBlock");
        }
        private TextBlock LayoutNumberOfColumnsTextBlock;

        public TextBlock GetLayoutNumberOfLargerColumnsTextBlock()
        {
            return GetElement(ref LayoutNumberOfLargerColumnsTextBlock, "LayoutNumberOfLargerColumnsTextBlock");
        }
        private TextBlock LayoutNumberOfLargerColumnsTextBlock;

        public Button GetClearRadioButtonsEventsButton()
        {
            return GetElement(ref ClearRadioButtonsEventsButton, "ClearRadioButtonsEventsButton");
        }
        private Button ClearRadioButtonsEventsButton;

        public ComboBox GetItemTypeComboBox()
        {
            return GetElement(ref ItemTypeComboBox, "ItemTypeComboBox");
        }
        private ComboBox ItemTypeComboBox;

        public ComboBox GetSourceComboBox()
        {
            return GetElement(ref SourceComboBox, "SourceComboBox");
        }
        private ComboBox SourceComboBox;

        public Edit GetBorderWidthTextBox()
        {
            return GetElement(ref BorderWidthTextBox, "BorderWidthTextBox");
        }
        private Edit BorderWidthTextBox;

        public Button GetSetBorderWidthButton()
        {
            return GetElement(ref SetBorderWidthButton, "SetBorderWidthButton");
        }
        private Button SetBorderWidthButton;

        public Button GetFocusSelectedItemButton()
        {
            return GetElement(ref FocusSelectedItemButton, "FocusSelectedItemButton");
        }
        private Button FocusSelectedItemButton;

        public ListBox GetRadioButtonsEventsList()
        {
            return GetElement(ref RadioButtonsEventsList, "RadioButtonsEventsList");
        }
        private ListBox RadioButtonsEventsList;


        public CheckBox GetLogRadioButtonsEventsCheckBox()
        {
            return GetElement(ref LogRadioButtonsEventsCheckBox, "LogRadioButtonsEventsCheckBox");
        }
        private CheckBox LogRadioButtonsEventsCheckBox;

        public CheckBox GetDisplayLogsCheckBox()
        {
            return GetElement(ref DisplayLogsCheckBox, "DisplayLogsCheckBox");
        }
        private CheckBox DisplayLogsCheckBox;

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

        public enum RadioButtonsSourceType
        {
            String,
            RadioButton
        }

        public enum RadioButtonsSourceLocation
        {
            Items,
            ItemSource
        }
    }
}
