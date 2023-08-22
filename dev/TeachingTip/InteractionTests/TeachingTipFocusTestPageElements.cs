// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

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
    class TeachingTipFocusTestPageElements
    {
        public Button GetOpenButton()
        {
            return GetElement(ref openButton, "OpenButton");
        }
        private Button openButton;

        public Button GetCloseButton()
        {
            return GetElement(ref closeButton, "CloseButton");
        }
        private Button closeButton;

        public CheckBox GetIsOpenCheckBox()
        {
            return GetElement(ref isOpenCheckBox, "IsOpenCheckBox");
        }
        private CheckBox isOpenCheckBox;

        public CheckBox GetIsIdleCheckBox()
        {
            return GetElement(ref isIdleCheckBox, "IsIdleCheckBox");
        }
        private CheckBox isIdleCheckBox;

        public CheckBox GetIsLightDismissEnabledCheckBox()
        {
            return GetElement(ref isLightDismissEnabledCheckBox, "IsLightDismissEnabledCheckBox");
        }
        private CheckBox isLightDismissEnabledCheckBox;

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
