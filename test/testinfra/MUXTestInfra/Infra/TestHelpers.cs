﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.IO;
using System.Text;
using System.Threading.Tasks;
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
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    public class TestSetupHelper : IDisposable
    {
        private int OpenedTestPages = 0;

        public class TestSetupHelperOptions
        {
            public string LanguageOverride {get; set;}

            public string ClassNameOfNavigationItemToInvoke { get; set; } = "Button";

            public string AutomationIdOfSafeItemToClick { get; set; } = "__CurrentPage";
        }

        public TestSetupHelper(string testName, TestSetupHelperOptions options = null)
            :this(new[] { testName }, options)
        {}

        private TestSetupHelperOptions Options { get; set; }

        private void PreTestSetup()
        {
            bool restartedTestApp = false;

            if (TestEnvironment.ShouldRestartApplication)
            {
                ElementCache.Clear();
                Wait.ResetIdleHelper();

                TestEnvironment.ShouldRestartApplication = false;

                Log.Comment("Restarting application to ensure test stability...");
                TestEnvironment.Application.Close();

                restartedTestApp = true;
            }

            if (restartedTestApp ||
                (TestEnvironment.Application.Process != null && TestEnvironment.Application.Process.HasExited))
            {
                if (!restartedTestApp)
                {
                    // If the test application process exited because something crashed,
                    // we'll restart it in order to make sure that other tests aren't affected.
                    Log.Comment("Application exited unexpectedly. Reinitializing...");
                }

                ElementCache.Clear();
                Wait.ResetIdleHelper();

#if USING_TAEF
                string deploymentDir = TestEnvironment.TestContext.TestDeploymentDir;
#else
                // TestDeploymentDir doesn't exist on MSTest's TestContext.  The only thing we use it for
                // is to install the AppX, and we install the AppX in other ways when using MSTest, so
                // we don't need it there.
                string deploymentDir = null;
#endif

                TestEnvironment.Application.Initialize(true, deploymentDir);
            }

            ElementCache.Clear();
            Wait.ForIdle();

            if (!String.IsNullOrEmpty(Options.LanguageOverride))
            {
                // Before we navigate to the test page, we need to make sure that we've set the language to what was requested.
                var languageChooser = TryFindElement.ById("LanguageChooser");

                // Sometimes TestSetupHelper is used to navigate off of a page other than the main page.  In those circumstances,
                // we won't have a pseudo-loc check box on the page, which is fine - we can just skip this step when that's the case,
                // as we'll have already gone into whatever language we wanted previously.
                if (languageChooser != null)
                {
                    ComboBox languageChooserComboBox = new ComboBox(languageChooser);

                    languageChooserComboBox.SelectItemById(Options.LanguageOverride);
                }
            }
        }

        // this function returns true for successful run and false for unsuccessful run
        // in case of false, the calling function will retry calling this function
        private bool TestSingleRun(ICollection<string> testNames, bool shouldRestrictInnerFrameSize)
        {   
            PreTestSetup();

            // We were hitting an issue in the lab where sometimes the very first click would fail to go through resulting in 
            // test instability. We work around this by clicking on element when the app launches. 
            if(!string.IsNullOrEmpty(Options.AutomationIdOfSafeItemToClick))
            {
                var safeItemToClick = FindElement.ById(Options.AutomationIdOfSafeItemToClick);
                if (safeItemToClick == null)
                {
                    string errorMessage = $"Cannot find object with automation id '{Options.AutomationIdOfSafeItemToClick}'";
                    Log.Warning(errorMessage);
                    DumpHelper.DumpFullContext();
                    return false;  //retry needed
                }
                InputHelper.LeftClick(safeItemToClick);
            }


            foreach (string testName in testNames)
            {
                Log.Comment(testName + " initializing TestSetupHelper");

                UIObject uiObject;
                if (!String.IsNullOrEmpty(Options.ClassNameOfNavigationItemToInvoke))
                {
                    uiObject = FindElement.ByNameAndClassName(testName, Options.ClassNameOfNavigationItemToInvoke);
                }
                else
                {
                    uiObject = FindElement.ByName(testName);
                }

                if (uiObject == null)
                {
                    string errorMessage = string.Format("Cannot find test page for: {0}.", testName);

                    // We'll raise the error message first so the dump has proper context preceding it,
                    // and will then throw it as an exception so we immediately cease execution.
                    Log.Warning(errorMessage);
                    DumpHelper.DumpFullContext();
                    return false;   //retry needed
                }

                // We're now entering a new test page, so everything has changed.  As such, we should clear our
                // element cache in order to ensure that we don't accidentally retrieve any stale UI objects.
                ElementCache.Clear();

                Log.Comment("Waiting until __TestContentLoadedCheckBox to be checked by test app.");
                CheckBox cb = new CheckBox(FindElement.ById("__TestContentLoadedCheckBox"));

                using (var waiter = cb.GetToggledWaiter())
                {
                    var testButton = new Button(uiObject);
                    testButton.Invoke();

                    if (cb.ToggleState != ToggleState.On)
                    {
                        waiter.Wait();
                    }
                }

                Wait.ForIdle();

                Log.Comment("__TestContentLoadedCheckBox checkbox checked, page has loaded");

                SetInnerFrameInLabDimensions(shouldRestrictInnerFrameSize);

                OpenedTestPages++;
            }

            TestCleanupHelper.TestSetupHelperPendingDisposals++;
            
            return true;  //no retry needed, it is a success
        }

        // The value of 'testName' should match that which was used when
        // registering the test in TestInventory.cs in the test app project.
        public TestSetupHelper(ICollection<string> testNames, TestSetupHelperOptions options = null, bool shouldRestrictInnerFrameSize = true)
        {
            if(options == null)
            {
                options = new TestSetupHelperOptions();
            }
            this.Options = options;
            
            // If a test crashes, it can take a little bit of time before we can 
            // restart the app again especially if watson is collecting dumps. Adding a 
            // delayed retry can help avoid the case where we might otherwise fail a slew of
            // tests that come after the one that crashes the app.
            var retryCount = 10;
            while (retryCount-- > 0)
            {
                var success = TestSingleRun(testNames, shouldRestrictInnerFrameSize);
                if(success)
                {
                    break; // no need to retry
                }
                else
                {
                    Log.Warning("Failed to setup test. pending retries: " + retryCount);
                    if (retryCount > 0)
                    {
                        Log.Comment("Waiting before retry...");
                        TestEnvironment.ShouldRestartApplication = true;
                        Task.Delay(5000);
                    }
                    else
                    {
                        Log.Error("Failed to set up test!");
                        try
                        {
                            DumpHelper.DumpFullContext();
                        }
                        catch (Exception e)
                        {
                            Log.Error("Also failed to dump context because of an exception: {0}", e.ToString());
                            throw;
                        }
                        throw new InvalidOperationException("All attempts to set up test failed.");
                    }
                }
            }
        }
        public static void SetInnerFrameInLabDimensions(bool shouldRestrictInnerFrameSize)
        {
            ToggleButton tb = new ToggleButton(FindElement.ById("__InnerFrameInLabDimensions"));
            if (tb.ToggleState != ToggleState.On && shouldRestrictInnerFrameSize)
            {
                Log.Comment("toggling the __InnerFrameInLabDimensions toggle button to On.");
                tb.Toggle();
                Wait.ForIdle();
            }
            else if (tb.ToggleState != ToggleState.Off && !shouldRestrictInnerFrameSize)
            {
                Log.Comment("toggling the __InnerFrameInLabDimensions toggle button to Off.");
                tb.Toggle();
                Wait.ForIdle();
            }
        }

        public static void SetInnerFrameFlowDirection(FlowDirection flowDirection)
        {
            ToggleButton tb = new ToggleButton(FindElement.ById("__EnableRTL"));
            if (tb.ToggleState != ToggleState.On && flowDirection == FlowDirection.RightToLeft)
            {
                Log.Comment("toggling the __EnableRTL toggle button to On.");
                tb.Toggle();
                Wait.ForIdle();
            }
            else if (tb.ToggleState != ToggleState.Off && flowDirection == FlowDirection.LeftToRight)
            {
                Log.Comment("toggling the __EnableRTL toggle button to Off.");
                tb.Toggle();
                Wait.ForIdle();
            }
        }

        public static void GoBack()
        {
            try
            {
                // Once a test has completed, we'll go back to the main page to get ready for the next test,
                // if there is one.
                TestEnvironment.Application.GoBack();
            }
            catch (Exception)
            {
                Log.Comment("Failed to go back. Queueing an app restart, since we may now be in an unknown state.");
                TestEnvironment.ShouldRestartApplication = true;
                throw;
            }
        }

        // In order to allow multiple consecutive runs of the same test - a requirement imposed by CatGates -
        // every TestSetupHelper *must* call Dispose to go back, in order to ensure that the test can be run again
        // without going through the cleanup -> startup cycle again.  We'll set TestSetupHelperDisposed to indicate that this
        // has occurred - if it hasn't, we should fail the test.
        public virtual void Dispose()
        {
            TestEnvironment.LogVerbose("TestSetupHelper.Dispose()");
            TestCleanupHelper.TestSetupHelperPendingDisposals--;

            while (OpenedTestPages > 0)
            {
                GoBack();
                OpenedTestPages--;
            }
        }
    }

    public class TestCleanupHelper
    {
        public static int TestSetupHelperPendingDisposals { get; set; }

        public static void Cleanup()
        {
            Log.Comment("Cleaning up test...");

            if (TestEnvironment.TestContext == null || TestEnvironment.Application == null)
            {
                Log.Error("TestContext or Application is not initialized successfully! TestContext is null: {0}", 
                    TestEnvironment.TestContext == null);
                return;
            }

            bool shouldRestart = TestEnvironment.TestContext.CurrentTestOutcome != UnitTestOutcome.Passed;

            if (TestSetupHelperPendingDisposals > 0)
            {
                Log.Error("A TestSetupHelper was not disposed!  Make sure that all uses of TestSetupHelper are wrapped in a using () statement.");

                // Having Log.Error() here doesn't change the value of CurrentTestOutcome, so we need to manually say that we should restart.
                shouldRestart = true;
            }
            else if (TestEnvironment.Application.Process == null || TestEnvironment.Application.Process.HasExited)
            {
                Log.Error("Failed to create test app process (Process is null: {0}) or " +
                    "process exited unexpectedly. Failing the current test and queueing an app restart.",
                    TestEnvironment.Application.Process == null);
                shouldRestart = true;
            }

            if (shouldRestart)
            {
                Log.Comment("Test failed. Queueing an app restart, since we may now be in an unknown state.");
                TestEnvironment.ShouldRestartApplication = true;
            }

            Log.Comment("Test cleanup complete.");
        }
    }

    public static class DumpHelper
    {
        public static void DumpFullContext()
        {
            Log.Comment("Dumping UIA tree...");
            TestEnvironment.LogDumpTree();
            Log.Comment("Dumping element cache...");
            ElementCache.Dump();
        }
    }

    public static class WarningReportHelper
    {
        public static void Record(string warning, List<string> traces)
        {
            try
            {
                string directory = Environment.GetEnvironmentVariable("TEMP") + @"\"; // For instance C:\Users\TDPUser\AppData\Local\Temp\
                string filename = "TAEFWarnings." + DateTime.Now.Month + "_" + DateTime.Now.Day + "_" + DateTime.Now.ToFileTime() + ".txt";

                using (FileStream fs = new FileStream(directory + filename, FileMode.Append, FileAccess.Write))
                {
                    using (StreamWriter sw = new StreamWriter(fs))
                    {
                        sw.WriteLine("TestName: " + TestEnvironment.TestContext.TestName);
                        sw.WriteLine("Warning: " + warning);
                        sw.WriteLine("Traces:");
                        foreach (string trace in traces)
                        {
                            sw.WriteLine(trace);
                        }

                        sw.WriteLine(sw.NewLine);
                    }
                }
            }
            catch
            {
                Log.Warning("WarningReportHelper::Record - Failed to output warning and traces.");
            }
        }
    }

    public class LocalizationHelper
    {
        public static void CompareStringSets(IList<string> englishStrings, IList<string> otherLanguageStrings)
        {
            Log.Comment("Comparing string sets...");
            Verify.AreEqual(englishStrings.Count, otherLanguageStrings.Count, "String sets must be the same length.");
            Log.Comment(Environment.NewLine + "English strings:" + Environment.NewLine);

            foreach (string englishString in englishStrings)
            {
                Log.Comment(englishString);
            }

            Log.Comment(Environment.NewLine + "Non-English strings:" + Environment.NewLine);

            foreach (string otherLanguageString in otherLanguageStrings)
            {
                Log.Comment(otherLanguageString);
            }
            
            Log.Comment(Environment.NewLine);

            // We want to make sure that every English string is different than its non-English counterpart -
            // if this is the case, then that means that every string was properly localized.
            for (int i = 0; i < Math.Min(englishStrings.Count, otherLanguageStrings.Count); i++)
            {
                Verify.AreNotEqual(englishStrings[i], otherLanguageStrings[i]);
            }
        }
    }

    public class TestAppCrashDetector : IEventSink
    {
        // When the test app crashes because of a catchable exception,
        // it posts the exception's contents to a TextBox immediately before closing,
        // which enables us to find out what the exception was.
        // We normally can't since the application exists in a different process,
        // so we don't catch any exceptions that it encounters, but this works around that.
        public void SinkEvent(WaiterEventArgs eventArgs)
        {
            AutomationPropertyChangedEventArgs args = eventArgs.EventArgs as AutomationPropertyChangedEventArgs;
            string exceptionMessage = args.NewValue as string;

            // For some reason, the exception message uses \r as the only newline character, which console windows don't like.
            // We need to make sure that it uses the correct newline character everywhere.
            exceptionMessage = exceptionMessage.Replace("\n\r", "\n").Replace("\r", "\n").Replace("\n", Environment.NewLine);

            Log.Error("Test app has crashed! This will likely be the root cause of any ElementNotAvailableExceptions for this test.");
            Log.Error("Exception from app: " + exceptionMessage);
        }
    }
}
