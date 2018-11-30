// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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
    public class ValueChangedEventWaiter : UIEventWaiter
    {
        private UIObject root;
        private string expectedValue;

        public ValueChangedEventWaiter(IValue root, string expectedValue = "")
            : base(new PropertyChangedEventSource(root as UIObject, Scope.Element, UIProperty.Get("Value.Value")))
        {
            this.root = root as UIObject;
            this.expectedValue = expectedValue;

            if (expectedValue.Length > 0)
            {
                this.AddFilter(new Predicate<WaiterEventArgs>(args => root.Value == expectedValue));
            }

            this.Start();
        }
        
        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
        }

        public override string ToString()
        {
            if (expectedValue.Length > 0)
            {
                return string.Format("ValueChangedEventWaiter with Condition:  Value of {0} changed to '{1}'.", root.AutomationId, expectedValue);
            }
            else
            {
                return string.Format("ValueChangedEventWaiter with Condition:  Value of {0} changed.", root.AutomationId);
            }
        }
    }
}
