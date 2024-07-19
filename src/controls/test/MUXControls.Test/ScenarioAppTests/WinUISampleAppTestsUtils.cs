using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using System;
using System.Numerics;
using System.Threading;
using System.Threading.Tasks;


using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    public static class WinUISampleAppTestsUtils
    {
        public static void VerifyPropertyAccessor(string buttonName, string textName, string expectedValue)
        {
            InvokeButton(buttonName);
            VerifyText(textName, expectedValue);
            VerifyNoErrorReport();
        }

        public static void ClearLogEvents()
        {
            Log.Comment("Clearing log events");
            InvokeButton("buttonClearEvents");
        }

        public static void ClearErrorReport()
        {
            Log.Comment("Clearing error report");
            ResetText("buttonClearError", "textBoxError");
        }

        public static void VerifyNoErrorReport()
        {
            var textBoxErrorAsUIObject = FindElement.ByName("textBoxError");
            Verify.IsNotNull(textBoxErrorAsUIObject);
            var textBoxError = new TextBlock(textBoxErrorAsUIObject);
            Verify.IsNotNull(textBoxError);
            Verify.AreEqual(string.Empty, textBoxError.DocumentText);
        }

        public static void VerifyLogEvents(string[] expectedLogEvents)
        {
            Log.Comment("Verifying log events:");
            var comboBoxEventsAsUIObject = FindElement.ByName("comboBoxEvents");
            Verify.IsNotNull(comboBoxEventsAsUIObject);
            var comboBoxEvents = new ComboBox(comboBoxEventsAsUIObject);
            Verify.IsNotNull(comboBoxEvents);

            bool doesEventOrderMatchExpected = (expectedLogEvents.Length == comboBoxEvents.AllItems.Count);

            // if the expected events match the number of events in the combo box, verify the order
            if (doesEventOrderMatchExpected)
            {
                int logIndex = 0;
                foreach (ComboBoxItem logEvent in comboBoxEvents.AllItems)
                {
                    if (expectedLogEvents[logIndex++] != logEvent.Name)
                    {
                        doesEventOrderMatchExpected = false;
                        break;
                    }
                }
            }

            // print an error if the events do not match
            if (!doesEventOrderMatchExpected)
            {
                Log.Comment("Event quantity or order does not match!");
                Log.Comment("=== EXPECTED ===");
                foreach (var s in expectedLogEvents)
                {
                    Log.Comment(s);
                }
                Log.Comment("=== OBSERVED ===");
                foreach (var logEvent in comboBoxEvents.AllItems)
                {
                    Log.Comment(logEvent.Name);
                }
                Log.Comment("=== END ===");
            }
            Verify.IsTrue(doesEventOrderMatchExpected);
        }

        public static void VerifyText(string textName, string expectedValue)
        {
            var textAsUIObject = FindElement.ByName(textName);
            Verify.IsNotNull(textAsUIObject);
            var text = new TextBlock(textAsUIObject);
            Verify.IsNotNull(text);
            if (expectedValue != null)
            {
                TestEnvironment.VerifyAreEqualWithRetry(20, () => expectedValue, () => text.DocumentText);
            }
            Log.Comment($"{textName}={text.DocumentText}");
        }

        public static void ResetText(string buttonName, string textName)
        {
            Log.Comment($"Clearing text {textName} with button {buttonName}");
            InvokeButton(buttonName);
            VerifyText(textName, string.Empty);
        }

        public static void SetEditText(string editName, string editText)
        {
            Log.Comment($"Setting edit {editName} to {editText}");
            var editAsUIObject = FindElement.ByName(editName);
            Verify.IsNotNull(editAsUIObject);
            var edit = new Edit(editAsUIObject);
            Verify.IsNotNull(edit);
            edit.SetValue(editText);
            Wait.ForIdle();
        }

        public static void InvokeButton(string buttonName, bool skipWait=false)
        {
            Log.Comment($"Invoking button {buttonName}");
            var buttonAsUIObject = FindElement.ByName(buttonName);
            Verify.IsNotNull(buttonAsUIObject);
            var button = new Button(buttonAsUIObject);
            Verify.IsNotNull(button);
            Verify.IsTrue(button.IsEnabled);
            button.Invoke();

            if (!skipWait)
            {
                Wait.ForIdle();
            }
        }

        public static void InvokeButtonForRoot(UIObject root, string buttonName)
        {
            UIObject buttonAsUIObject = FindElement.GetDescendantByName(root, buttonName);
            Verify.IsNotNull(buttonAsUIObject);
            Verify.IsTrue(buttonAsUIObject.IsEnabled);
            Button button = new Button(buttonAsUIObject);
            button.Invoke();
            Wait.ForIdle();
        }

        public static bool InvokeButtonIfEnabled(string buttonName)
        {
            var buttonAsUIObject = FindElement.ByName(buttonName);
            Verify.IsNotNull(buttonAsUIObject);
            var button = new Button(buttonAsUIObject);
            Verify.IsNotNull(button);
            if (button.IsEnabled)
            {
                Log.Comment($"Invoking button {buttonName}");
                button.Invoke();
                Wait.ForIdle();
                return true;
            }
            return false;
        }

        public static void SelectRadioButton(string radioButtonName)
        {
            Log.Comment($"Selecting radio button {radioButtonName}");
            var radioButtonAsUIObject = FindElement.ByName(radioButtonName);
            Verify.IsNotNull(radioButtonAsUIObject);
            var radioButton = new RadioButton(radioButtonAsUIObject);
            Verify.IsNotNull(radioButton);
            Verify.IsTrue(radioButton.IsEnabled);
            radioButton.Select();
            Wait.ForIdle();
        }

        public static void SelectCheckbox(string checkboxName)
        {
            Log.Comment($"Clicking checkbox {checkboxName}");
            var checkBoxAsUIObject = FindElement.ByName(checkboxName);
            Verify.IsNotNull(checkBoxAsUIObject);
            var checkBox = new CheckBox(checkBoxAsUIObject);
            Verify.IsNotNull(checkBox);
            Verify.IsTrue(checkBox.IsEnabled);
            checkBox.Toggle();
            Wait.ForIdle();
        }
        
        public static void VerifyTextForRoot(UIObject root, string textName, string expectedValue)
        {
            UIObject textBlockAsUIObject = FindElement.GetDescendantByName(root, textName);
            Verify.IsNotNull(textBlockAsUIObject);
            TextBlock textBlock = new TextBlock(textBlockAsUIObject);
            TestEnvironment.VerifyAreEqualWithRetry(20, () => expectedValue, () => textBlock.DocumentText);
        }
    }
}