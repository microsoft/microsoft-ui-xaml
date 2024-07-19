﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System.Linq;
using System.Collections.Generic;
using MUXControlsTestApp;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using MUXControls.TestAppUtils;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class NumberBoxTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyTextAlignmentPropogates()
        {
            var numberBox = SetupNumberBox();
            TextBox textBox = null;

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();

                textBox = TestUtilities.FindDescendents<TextBox>(numberBox).Where(e => e.Name == "InputBox").Single();
                Verify.AreEqual(TextAlignment.Left, textBox.TextAlignment, "The default TextAlignment should be left.");

                numberBox.TextAlignment = TextAlignment.Right;
                Content.UpdateLayout();

                Verify.AreEqual(TextAlignment.Right, textBox.TextAlignment, "The TextAlignment should have been updated to Right.");
            });
        }

        [TestMethod]
        public void VerifyInputScopePropogates()
        {
            var numberBox = SetupNumberBox();

            RunOnUIThread.Execute(() =>
            {
                Content.UpdateLayout();
                var inputTextBox = TestUtilities.FindDescendents<TextBox>(numberBox).Where(e => e.Name == "InputBox").Single();

                Verify.AreEqual(1, inputTextBox.InputScope.Names.Count);
                Verify.AreEqual(InputScopeNameValue.Number, inputTextBox.InputScope.Names[0].NameValue, "The default InputScope should be 'Number'.");

                var scopeName = new InputScopeName();
                scopeName.NameValue = InputScopeNameValue.CurrencyAmountAndSymbol;
                var scope = new InputScope();
                scope.Names.Add(scopeName);

                numberBox.InputScope = scope;
                Content.UpdateLayout();

                Verify.AreEqual(1, inputTextBox.InputScope.Names.Count);
                Verify.AreEqual(InputScopeNameValue.CurrencyAmountAndSymbol, inputTextBox.InputScope.Names[0].NameValue, "The InputScope should be 'CurrencyAmountAndSymbol'.");
            });

            return;
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Task 31597497: DCPP Bug: Unreliable test, UAP.Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.NumberBoxTests.VerifyIsEnabledChangeUpdatesVisualState
        public void VerifyIsEnabledChangeUpdatesVisualState()
        {
            var numberBox = SetupNumberBox();

            VisualStateGroup commonStatesGroup = null;
            RunOnUIThread.Execute(() =>
            {
                // Check 1: Set IsEnabled to true.
                numberBox.IsEnabled = true;
                Content.UpdateLayout();

                var numberBoxLayoutRoot = (FrameworkElement)VisualTreeHelper.GetChild(numberBox, 0);
                commonStatesGroup = VisualStateManager.GetVisualStateGroups(numberBoxLayoutRoot).First(vsg => vsg.Name.Equals("CommonStates"));

                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);

                // Check 2: Set IsEnabled to false.
                numberBox.IsEnabled = false;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Disabled", commonStatesGroup.CurrentState.Name);

                // Check 3: Set IsEnabled back to true.
                numberBox.IsEnabled = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Normal", commonStatesGroup.CurrentState.Name);
            });
        }

        [TestMethod]
        public void VerifyUIANameBehavior()
        {
            NumberBox numberBox = null;
            TextBox textBox = null;

            RunOnUIThread.Execute(() =>
            {
                numberBox = new NumberBox();
                Content = numberBox;
                Content.UpdateLayout();

                textBox = TestPage.FindVisualChildrenByType<TextBox>(numberBox)[0];
                Verify.IsNotNull(textBox);
                numberBox.Header = "Some header";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyUIAName("Some header");
                numberBox.Header = new Button();
                AutomationProperties.SetName(numberBox, "Some UIA name");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyUIAName("Some UIA name");
                numberBox.Header = new Button();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyUIAName("Some UIA name");
                numberBox.Minimum = 0;
                numberBox.Maximum = 10;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyUIAName("Some UIA name Minimum0 Maximum10");
                numberBox.Minimum = 50;
                numberBox.Maximum = 100;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyUIAName("Some UIA name Minimum50 Maximum100");
            });

            void VerifyUIAName(string value)
            {
                Verify.AreEqual(value, FrameworkElementAutomationPeer.CreatePeerForElement(textBox).GetName());
            }
        }

        private NumberBox SetupNumberBox()
        {
            NumberBox numberBox = null;
            RunOnUIThread.Execute(() =>
            {
                numberBox = new NumberBox();
                Content = numberBox;
            });

            return numberBox;
        }
    }
}
