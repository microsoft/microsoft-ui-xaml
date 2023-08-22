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
    class RadioButtonsFocusTestPageElements
    {
        public UIObject GetTestRadioButtons1()
        {
            return GetElement(ref TestRadioButtons1, "TestRadioButtons1");
        }
        private UIObject TestRadioButtons1;

        public UIObject GetTestRadioButtons2()
        {
            return GetElement(ref TestRadioButtons2, "TestRadioButtons2");
        }
        private UIObject TestRadioButtons2;

        public TextBlock GetSelectedIndexTextBlock1()
        {
            return GetElement(ref SelectedIndexTextBlock1, "SelectedIndexTextBlock1");
        }
        private TextBlock SelectedIndexTextBlock1;

        public TextBlock GetSelectedIndexTextBlock2()
        {
            return GetElement(ref SelectedIndexTextBlock2, "SelectedIndexTextBlock2");
        }
        private TextBlock SelectedIndexTextBlock2;

        public TextBlock GetFocusedIndexTextBlock1()
        {
            return GetElement(ref FocusedIndexTextBlock1, "FocusedIndexTextBlock1");
        }
        private TextBlock FocusedIndexTextBlock1;

        public TextBlock GetFocusedIndexTextBlock2()
        {
            return GetElement(ref FocusedIndexTextBlock2, "FocusedIndexTextBlock2");
        }
        private TextBlock FocusedIndexTextBlock2;

        public CheckBox GetRadioButtons1HasFocusCheckBox()
        {
            return GetElement(ref RadioButtons1HasFocusCheckBox, "TestRadioButtons1HasFocusCheckBox");
        }
        private CheckBox RadioButtons1HasFocusCheckBox;

        public CheckBox GetRadioButtons2HasFocusCheckBox()
        {
            return GetElement(ref RadioButtons2HasFocusCheckBox, "TestRadioButtons2HasFocusCheckBox");
        }
        private CheckBox RadioButtons2HasFocusCheckBox;

        public Edit GetIndexToSelectTextBox1()
        {
            return GetElement(ref IndexToSelectTextBox1, "IndexToSelectTextBox1");
        }
        private Edit IndexToSelectTextBox1;

        public Edit GetIndexToSelectTextBox2()
        {
            return GetElement(ref IndexToSelectTextBox2, "IndexToSelectTextBox2");
        }
        private Edit IndexToSelectTextBox2;

        public Button GetSelectByIndexButton1()
        {
            return GetElement(ref SelectByIndexButton, "SelectByIndexButton1");
        }
        private Button SelectByIndexButton;

        public Button GetSelectByIndexButton2()
        {
            return GetElement(ref SelectByIndexButton2, "SelectByIndexButton2");
        }
        private Button SelectByIndexButton2;

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
    }
}
