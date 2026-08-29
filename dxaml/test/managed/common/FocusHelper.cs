// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class FocusHelper
    {
        public static void EnsureFocus(dynamic element, FocusState focusState, UInt32 Attempts = 1)
        {
            bool gotFocus = false;

            while (Attempts > 0 && !gotFocus)
            {
                Attempts--;

                TestServicesExtensions.EnsureForegroundWindow();

                using (var eventTester = new EventTester<dynamic, RoutedEventArgs>(element, "GotFocus"))
                using (var eventTesterFocusMgr = EventTester<object, FocusManagerGotFocusEventArgs>.FromStaticEvent<FocusManager>("GotFocus"))
                {
                    UIExecutor.Execute(() =>
                    {
                        if (element.FocusState == focusState && FocusManager.GetFocusedElement(element.XamlRoot).Equals(element))
                        {
                            // The element is already focused with the expected focus state
                            Log.Comment("Focus was already set on this element");
                            gotFocus = true;
                        }
                        else
                        {
                            Log.Comment("Setting focus to the element...");
                            element.Focus(focusState);
                        }
                    });

                    eventTester.WaitForNoThrow(TimeSpan.FromMilliseconds(4000));
                    eventTesterFocusMgr.WaitForNoThrow(TimeSpan.FromMilliseconds(4000));
                    gotFocus = gotFocus == false ? eventTester.HasFired : true;
                }

                TestServices.WindowHelper.WaitForIdle();
            }

            if (!gotFocus)
            {
                global::Private.Infrastructure.TestServices.Utilities.CaptureScreen("FocusTestHelper");
            }
            Verify.IsTrue(gotFocus);
        }
    }
}
