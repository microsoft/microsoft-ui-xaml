// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    // Base class for ScrollPresenterTestsWithAutomationPeer and ScrollPresenterTestsWithInputHelper with common private debugging facilities.
    public class ScrollPresenterTestsBase
    {
        // Sets the MUXControlsTestApp.ScrollPresenterPage's txtMouseWheelScrollLines text to the provided value for custom mouse wheel scrolling.
        protected void SetMouseWheelScrollLines(int mouseWheelScrollLines)
        {
            Log.Comment("Retrieving txtMouseWheelScrollLines");
            UIObject mouseWheelScrollLinesUIObject = FindElement.ById("txtMouseWheelScrollLines");
            Verify.IsNotNull(mouseWheelScrollLinesUIObject);
            Edit txtMouseWheelScrollLines = new Edit(mouseWheelScrollLinesUIObject);
            Verify.IsNotNull(txtMouseWheelScrollLines);
            txtMouseWheelScrollLines.SetValueAndWait(mouseWheelScrollLines.ToString());
        }

        // outputDebugStringLevel can be "None", "Info" or "Verbose"
        protected void SetOutputDebugStringLevel(string outputDebugStringLevel)
        {
            Log.Comment("Retrieving cmbScrollPresenterOutputDebugStringLevel");
            ComboBox cmbScrollPresenterOutputDebugStringLevel = new ComboBox(FindElement.ByName("cmbScrollPresenterOutputDebugStringLevel"));
            Verify.IsNotNull(cmbScrollPresenterOutputDebugStringLevel, "Verifying that cmbScrollPresenterOutputDebugStringLevel was found");

            Log.Comment("Changing output-debug-string-level selection to " + outputDebugStringLevel);
            cmbScrollPresenterOutputDebugStringLevel.SelectItemByName(outputDebugStringLevel);
            Log.Comment("Selection is now {0}", cmbScrollPresenterOutputDebugStringLevel.Selection[0].Name);
        }

        protected void SetLoggingLevel(bool isPrivateLoggingEnabled)
        {
            Log.Comment("Retrieving chkLogScrollPresenterMessages");
            CheckBox chkLogScrollPresenterMessages = new CheckBox(FindElement.ById("chkLogScrollPresenterMessages"));
            Verify.IsNotNull(chkLogScrollPresenterMessages, "Verifying that chkLogScrollPresenterMessages was found");

            if (isPrivateLoggingEnabled && chkLogScrollPresenterMessages.ToggleState != ToggleState.On ||
                !isPrivateLoggingEnabled && chkLogScrollPresenterMessages.ToggleState != ToggleState.Off)
            {
                Log.Comment("Toggling chkLogScrollPresenterMessages.IsChecked to " + isPrivateLoggingEnabled);
                chkLogScrollPresenterMessages.Toggle();
                Wait.ForIdle();
            }
        }

        protected void LogTraces()
        {
            LogTraces(false /*recordWarning*/);
        }

        protected void LogTraces(bool recordWarning)
        {
            List<string> traces = recordWarning ? new List<string>() : null;

            Log.Comment("Reading full log:");

            UIObject fullLogUIObject = FindElement.ById("cmbFullLog");
            Verify.IsNotNull(fullLogUIObject);
            ComboBox cmbFullLog = new ComboBox(fullLogUIObject);
            Verify.IsNotNull(cmbFullLog);

            UIObject getFullLogUIObject = FindElement.ById("btnGetFullLog");
            Verify.IsNotNull(getFullLogUIObject);
            Button getFullLogButton = new Button(getFullLogUIObject);
            Verify.IsNotNull(getFullLogButton);

            getFullLogButton.Invoke();
            Wait.ForIdle();

            foreach (ComboBoxItem item in cmbFullLog.AllItems)
            {
                string trace = item.Name;

                if (recordWarning)
                {
                    traces.Add(trace);
                }
                Log.Comment(trace);
            }

            if (recordWarning)
            {
                string warning = "Non-final test pass failed.";
                Log.Warning(warning);

                WarningReportHelper.Record(warning, traces);
            }
        }

        protected void ClearTraces()
        {
            Log.Comment("Clearing full log.");

            UIObject clearFullLogUIObject = FindElement.ById("btnClearFullLog");
            Verify.IsNotNull(clearFullLogUIObject);
            Button clearFullLogButton = new Button(clearFullLogUIObject);
            Verify.IsNotNull(clearFullLogButton);

            clearFullLogButton.Invoke();
            Wait.ForIdle();
        }

        protected void LogAndClearTraces()
        {
            LogAndClearTraces(false /*recordTraces*/);
        }

        protected void LogAndClearTraces(bool recordWarning)
        {
            LogTraces(recordWarning);
            ClearTraces();
        }

        protected bool WaitForEditValue(string editName, string editValue, double secondsTimeout = 2.0, bool throwOnError = true)
        {
            Edit edit = new Edit(FindElement.ById(editName));
            Verify.IsNotNull(edit);
            Log.Comment("Current value for " + editName + ": " + edit.Value);
            if (edit.Value != editValue)
            {
                using (var waiter = new ValueChangedEventWaiter(edit, editValue))
                {
                    Log.Comment("Waiting for " + editName + " to be set to " + editValue);

                    bool success = waiter.TryWait(TimeSpan.FromSeconds(secondsTimeout));
                    Log.Comment("Current value for " + editName + ": " + edit.Value);

                    if (success)
                    {
                        Log.Comment("Wait succeeded");
                    }
                    else
                    {
                        if (edit.Value == editValue)
                        {
                            Log.Warning("Wait failed but TextBox contains expected Text");
                            LogAndClearTraces();
                        }
                        else
                        {
                            if (throwOnError)
                            {
                                Log.Error("Wait for edit value failed");
                                LogAndClearTraces();
                                throw new WaiterException();
                            }
                            else
                            {
                                Log.Warning("Wait for edit value failed");
                                LogAndClearTraces();
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }
    }
}
