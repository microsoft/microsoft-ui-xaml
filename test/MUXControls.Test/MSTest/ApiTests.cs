using Common;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ApiTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void ColorPickerTests_ValidateHueRange()
        {
            RunApiTest("ColorPickerTests_ValidateHueRange");
        }

        public void RunApiTest(string testName)
        {
            using (TestSetupHelper setup = new TestSetupHelper("API Tests"))
            {
                Edit errorReportingTextBox = FindElement.ById<Edit>("__ApiTestErrorReportingTextBox");
                Edit logReportingTextBox = FindElement.ById<Edit>("__ApiTestLogReportingTextBox");

                List<string> errorMessages = new List<string>();

                using (PropertyChangedEventSource errorSource = new PropertyChangedEventSource(errorReportingTextBox, Scope.Element, UIProperty.Get("Value.Value")))
                {
                    errorSource.Start(new ErrorSink(errorMessages));

                    using (PropertyChangedEventSource logSource = new PropertyChangedEventSource(logReportingTextBox, Scope.Element, UIProperty.Get("Value.Value")))
                    {
                        logSource.Start(new LogSink());

                        CheckBox apiTestCompleteCheckBox = FindElement.ById<CheckBox>("__ApiTestCompleteCheckBox");

                        if (apiTestCompleteCheckBox.ToggleState == ToggleState.On)
                        {
                            using (Waiter waiter = apiTestCompleteCheckBox.GetToggledWaiter())
                            {
                                apiTestCompleteCheckBox.Toggle();
                                waiter.Wait();
                            }
                        }

                        using (Waiter waiter = apiTestCompleteCheckBox.GetToggledWaiter())
                        {
                            FindElement.ByName<Button>(testName).InvokeAndWait();
                            waiter.Wait();
                        }

                        logSource.Stop();
                    }
                    
                    errorSource.Stop();

                    foreach (string errorMessage in errorMessages)
                    {
                        Verify.Fail(errorMessage);
                    }
                }
            }
        }

        private class ErrorSink : IEventSink
        {
            private IList<string> errorMessages;

            public ErrorSink(IList<string> errorMessages)
            {
                this.errorMessages = errorMessages;
            }

            public void SinkEvent(WaiterEventArgs eventArgs)
            {
                AutomationPropertyChangedEventArgs args = eventArgs.EventArgs as AutomationPropertyChangedEventArgs;
                string errorMessage = args.NewValue as string;

                if (errorMessage.Length > 0)
                {
                    // For some reason, exception messages use \r as the only newline character, which console windows don't like.
                    // We need to make sure that it uses the correct newline character everywhere.
                    errorMessage = errorMessage.Replace("\n\r", "\n").Replace("\r", "\n").Replace("\n", Environment.NewLine);
                    errorMessages.Add(errorMessage);
                    Log.Error(errorMessage);
                }
            }
        }

        private class LogSink : IEventSink
        {
            public void SinkEvent(WaiterEventArgs eventArgs)
            {
                AutomationPropertyChangedEventArgs args = eventArgs.EventArgs as AutomationPropertyChangedEventArgs;
                string message = args.NewValue as string;

                if (message.Length > 0)
                {
                    Log.Comment(message);
                }
            }
        }
    }
}
