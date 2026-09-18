// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Windows.Input;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Common;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class SplitButtonTests : ApiTestBase
    {
        [TestMethod]
        [Description("Verifies SplitButton default properties.")]
        public void VerifyDefaultsAndBasicSetting()
        {
            SplitButton splitButton = null;
            Flyout flyout = null;
            TestCommand command = null;
            int parameter = 0;

            RunOnUIThread.Execute(() =>
            {
                flyout = new Flyout();
                command = new TestCommand();

                splitButton = new SplitButton();
                Verify.IsNotNull(splitButton);

                // Verify Defaults
                Verify.IsNull(splitButton.Flyout);
                Verify.IsNull(splitButton.Command);
                Verify.IsNull(splitButton.CommandParameter);

                // Verify basic setters
                splitButton.Flyout = flyout;
                splitButton.Command = command;
                splitButton.CommandParameter = parameter;
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(splitButton.Flyout, flyout);
                Verify.AreEqual(splitButton.Command, command);
                Verify.AreEqual(splitButton.CommandParameter, parameter);
            });
        }

        [TestMethod]
        [Description("Verifies ToggleSplitButton IsChecked property.")]
        public void VerifyIsCheckedProperty()
        {
            RunOnUIThread.Execute(() =>
            {
                ToggleSplitButton toggleSplitButton = new ToggleSplitButton();

                Verify.IsFalse(toggleSplitButton.IsChecked, "ToggleSplitButton is not unchecked");

                toggleSplitButton.SetValue(ToggleSplitButton.IsCheckedProperty, true);

                bool isChecked = (bool)toggleSplitButton.GetValue(ToggleSplitButton.IsCheckedProperty);
                Verify.IsTrue(isChecked, "ToggleSplitButton is not checked");
            });
        }

        [TestMethod]
        [Description("Verifies IsEnabled changes immediately update SplitButton visual states.")]
        public void VerifyIsEnabledChangeUpdatesVisualState()
        {
            VerifyIsEnabledChangeUpdatesVisualStateHelper();
        }

        [TestMethod]
        [Description("Verifies IsEnabled changes immediately update SplitButton visual states with legacy styles.")]
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false}")]
        public void VerifyIsEnabledChangeUpdatesVisualStateWithLegacyStyles()
        {
            VerifyIsEnabledChangeUpdatesVisualStateHelper();
        }

        private void VerifyIsEnabledChangeUpdatesVisualStateHelper()
        {
            ToggleSplitButton checkedToggleSplitButton = null;
            SplitButton disabledSplitButton = null;
            VisualStateGroup toggleSplitButtonCommonStates = null;
            VisualStateGroup splitButtonCommonStates = null;

            RunOnUIThread.Execute(() =>
            {
                checkedToggleSplitButton = new ToggleSplitButton
                {
                    IsChecked = true,
                };
                disabledSplitButton = new SplitButton
                {
                    IsEnabled = false,
                };

                var panel = new StackPanel();
                panel.Children.Add(checkedToggleSplitButton);
                panel.Children.Add(disabledSplitButton);
                Content = panel;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
                toggleSplitButtonCommonStates = GetCommonStates(checkedToggleSplitButton);
                splitButtonCommonStates = GetCommonStates(disabledSplitButton);

                Verify.IsTrue(toggleSplitButtonCommonStates.States.Any(state => state.Name == "CheckedDisabled"));
                Verify.AreEqual("Checked", toggleSplitButtonCommonStates.CurrentState.Name);
                Verify.AreEqual("Disabled", splitButtonCommonStates.CurrentState.Name);

                checkedToggleSplitButton.IsEnabled = false;
                Verify.AreEqual("CheckedDisabled", toggleSplitButtonCommonStates.CurrentState.Name);
                Verify.IsTrue(checkedToggleSplitButton.IsChecked);

                disabledSplitButton.IsEnabled = true;
                Verify.AreEqual("Normal", splitButtonCommonStates.CurrentState.Name);
                Verify.IsTrue(disabledSplitButton.IsEnabled);

                checkedToggleSplitButton.IsEnabled = true;
                Verify.AreEqual("Checked", toggleSplitButtonCommonStates.CurrentState.Name);
            });
        }

        private static VisualStateGroup GetCommonStates(Control control)
        {
            var layoutRoot = (FrameworkElement)VisualTreeHelper.GetChild(control, 0);
            return VisualStateManager.GetVisualStateGroups(layoutRoot).Single(group => group.Name == "CommonStates");
        }
    }


    // CanExecuteChanged is never used -- that's ok, disable the compiler warning.
#pragma warning disable CS0067
    public class TestCommand : ICommand
    {
        public event EventHandler CanExecuteChanged;

        public TestCommand()
        {
        }

        public bool CanExecute(object o)
        {
            return true;
        }

        public void Execute(object o) {}
    }
    #pragma warning restore CS0067
}
