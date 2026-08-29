// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using System;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public class Wait
    {
        private static Button waitForIdleInvokerButton = null;
        private static CheckBox idleStateEnteredCheckBox = null;
        private static Edit errorReportingTextBox = null;
        private static Edit logReportingTextBox = null;

        // keep call retryEvalFunc until retryEvalFunc returns true or timeout
        public static void RetryUntilEvalFuncSuccessOrTimeout(Func<bool> retryEvalFunc, int retryTimoutByMilliseconds = 500)
        {
            uint timeElapsedByMilliseconds = 0;
            uint waitTimeByMilliseconds = 100;

            while (!retryEvalFunc())
            {
                if (timeElapsedByMilliseconds >= retryTimoutByMilliseconds)
                {
                    Log.Warning("RetryUntilSuccessOrTimeout timeout after milliseconds " + timeElapsedByMilliseconds);
                    return;
                }
                ForMilliseconds(waitTimeByMilliseconds);
                timeElapsedByMilliseconds += waitTimeByMilliseconds;
            }            
        }

        public static void ForSeconds(uint time)
        {
            ForMilliseconds(time * 1000);
        }

        public static void ForMilliseconds(uint time)
        {
            new TimeWaiter(TimeSpan.FromMilliseconds(time)).Wait();
        }

        private static bool isRetrying = false;

        public static void ForIdle(bool findElementsIfNull = true)
        {
            TestEnvironment.LogVerbose("Wait.ForIdle: Begin");
            try
            {
                // If any of these are null, then we haven't initialized our idle helpers yet.
                // We should just return - we'll do that soon after this.
                if (waitForIdleInvokerButton == null ||
                    idleStateEnteredCheckBox == null ||
                    errorReportingTextBox == null ||
                    logReportingTextBox == null)
                {
                    return;
                }

                using (UIEventWaiter idleStateEnteredWaiter = idleStateEnteredCheckBox.GetToggledWaiter())
                {
                    TestEnvironment.LogSuperVerbose("Wait.ForIdle: After GetToggledWaiter");
                    using (var errorReportedWaiter = new ValueChangedEventWaiter(errorReportingTextBox))
                    {
                        TestEnvironment.LogSuperVerbose("Wait.ForIdle: Before WaitForIdleInvoker.Invoke");

                        idleStateEnteredWaiter.AddFilter((WaiterEventArgs) => idleStateEnteredCheckBox.ToggleState == ToggleState.On);

                        int triesLeft = 3;
                        while (triesLeft > 0)
                        {
                            try
                            {
                                triesLeft--;
                                waitForIdleInvokerButton.Invoke();
                                break; // succeeded, break out of loop
                            }
                            catch (System.Runtime.InteropServices.COMException ex)
                            {
                                if (triesLeft == 0)
                                {
                                    throw;
                                }
                                Log.Comment(string.Format("Wait.ForIdle failed to invoke waitForIdleInvokerButton: {0}", ex));
                                Log.Comment(string.Format("Tries left: {0}", triesLeft));

                                Wait.ForMilliseconds(100);
                            }
                        }

                        TestEnvironment.LogSuperVerbose("Wait.ForIdle: After WaitForIdleInvoker.Invoke");

                        var waiter = CompositableWaiter.WaitAny(TimeSpan.FromSeconds(300), idleStateEnteredWaiter, errorReportedWaiter);
                        if (waiter == errorReportedWaiter)
                        {
                            AutomationPropertyChangedEventArgs args = errorReportedWaiter.Arguments.EventArgs as AutomationPropertyChangedEventArgs;
                            string errorMessage = args.NewValue as string;

                            if (errorMessage.Length > 0)
                            {
                                Log.Error("Error while waiting for idle: {0}", errorMessage);
                                DumpHelper.DumpFullContext();
                                throw new WaiterException(errorMessage);
                            }
                        }
                        else
                        {
                            var value = logReportingTextBox.Value;
                            if (!String.IsNullOrEmpty(value))
                            {
                                TestEnvironment.LogSuperVerbose(value);
                                logReportingTextBox.SetValue("");
                            }
                        }
                    }
                }
            }
            catch (ElementNotAvailableException)
            {
                if (!isRetrying && findElementsIfNull)
                {
                    // If we got an element-not-available exception, we'll re-initialize our wait helper and try again.
                    // If that doesn't work, then we'll just fail.
                    ResetIdleHelper();
                    InitializeWaitHelper();

                    isRetrying = true;
                    ForIdle();
                }
                else
                {
                    if (TestEnvironment.Application.Process == null)
                    {
                        Log.Error("Tried to wait for idle before starting the test app!");
                    }
                    else if (TestEnvironment.Application.Process.HasExited)
                    {
                        Log.Error("Tried to wait for idle after the test app has closed!");
                    }

                    throw;
                }
            }
            TestEnvironment.LogVerbose("Wait.ForIdle: End");
        }

        public static bool ForScrollChanged(ScrollPresenter scrollPresenter, ScrollProperty scrollProperty)
        {
            Log.Comment("Wait.ForScrollChanged call for {0}.", scrollProperty.ToString());

            using (UIEventWaiter scrollChangedWaiter =
                (scrollProperty == ScrollProperty.HorizontallyScrollable || scrollProperty == ScrollProperty.VerticallyScrollable) ?
                    scrollPresenter.GetScrollChangedWaiter(scrollProperty, null) :
                    scrollPresenter.GetScrollChangedWaiter(scrollProperty, double.NaN))
            {
                if (!scrollChangedWaiter.TryWait(TimeSpan.FromSeconds(5)))
                {
                    Log.Comment("Timeout expired.");
                    return false;
                }
            }

            return true;
        }

        public static void ForAppDebugger()
        {
            Button waitForDebuggerInvokerButton = new Button(FindElement.ById("__WaitForDebuggerInvoker"));
            CheckBox debuggerAttachedCheckBox = new CheckBox(FindElement.ById("__DebuggerAttachedCheckBox"));

            using (UIEventWaiter debuggerAttachedWaiter = debuggerAttachedCheckBox.GetToggledWaiter())
            {
                using (var processWaiter = new ProcessClosedWaiter(TestEnvironment.Application.Process))
                {
                    Log.Comment(string.Format("Waiting for a debugger to attach (processId = {0})...", TestEnvironment.Application.Process.Id));
                    waitForDebuggerInvokerButton.Invoke();

                    var waiter = CompositableWaiter.WaitAny(TimeSpan.FromMinutes(15), debuggerAttachedWaiter, processWaiter);
                    if (waiter == processWaiter)
                    {
                        throw new WaiterException("Process exited while waiting for app debugger.");
                    }
                }
            }
        }

        internal static void InitializeWaitHelper()
        {
            UIObject waitForIdleInvoker = FindElement.ById("__WaitForIdleInvoker");
            UIObject idleStateEnteredObject = FindElement.ById("__IdleStateEnteredCheckBox");
            UIObject errorReportingTextBoxObject = FindElement.ById("__ErrorReportingTextBox");
            UIObject logReportingTextBoxObject = FindElement.ById("__LogReportingTextBox");

            waitForIdleInvokerButton = new Button(waitForIdleInvoker);
            idleStateEnteredCheckBox = new CheckBox(idleStateEnteredObject);
            errorReportingTextBox = new Edit(errorReportingTextBoxObject);
            logReportingTextBox = new Edit(logReportingTextBoxObject);
        }

        internal static void ResetIdleHelper()
        {
            waitForIdleInvokerButton = null;
            idleStateEnteredCheckBox = null;
        }
    }
}
