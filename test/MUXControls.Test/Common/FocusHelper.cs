// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.InteropServices;
using System;

#if BUILD_WINDOWS
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{

    public static class FocusHelper
    {
        public static void SetFocus(UIObject control)
        {
            if (!control.HasKeyboardFocus)
            {
                var uiCondition =
                    string.IsNullOrEmpty(control.AutomationId) ?
                    UICondition.CreateFromName(control.Name) :
                    UICondition.CreateFromId(control.AutomationId);

                using (var focusChangedWaiter = new FocusAcquiredWaiter(uiCondition))
                {
                    control.SetFocus();
                    focusChangedWaiter.Wait();
                }
            }
        }
    }
}
