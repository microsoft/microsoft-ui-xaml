// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Tests.Input.KeyEvents
{
    [TestClass]
    public class CharacterReceivedEventTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CharacterReceivedEvent is raised by TextBox/ComboBox controls.")]
        public void VerifyCharacterReceivedEventRaised()
        {
            const string rootPanelXaml =
                @"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                    <TextBox x:Name='textBox'/>
                    <ComboBox x:Name='comboBox' IsEditable='true'/>
                </StackPanel>";

            StackPanel rootPanel = null;
            TextBox textBox = null;
            ComboBox comboBox = null;
            bool received = false;

            var verifyCharEvent = new Action<string, CharacterReceivedRoutedEventArgs>((eventName, args) =>
            {
                Log.Comment($"Event {eventName} raised for " + args.Character);
                received = true;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = XamlReader.Load(rootPanelXaml) as StackPanel;
                textBox = rootPanel.FindName("textBox") as TextBox;
                comboBox = rootPanel.FindName("comboBox") as ComboBox;
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focusing TextBox for keyboard input.");
            FocusHelper.EnsureFocus(textBox, FocusState.Keyboard);

            using (var characterReceived = EventTester<TextBox, CharacterReceivedRoutedEventArgs>.FromRoutedEvent(textBox, "CharacterReceived", (sender, args) => verifyCharEvent("CharacterReceived", args)))
            {
                Log.Comment("Sending t keystroke.");
                TestServices.KeyboardHelper.PressKeySequence("$d$_t#$u$_t");

                characterReceived.Wait();
                TestServices.WindowHelper.WaitForIdle();

                Verify.IsTrue(received);
            }

            received = false;

            Log.Comment("Focusing ComboBox for keyboard input.");
            FocusHelper.EnsureFocus(comboBox, FocusState.Keyboard);

            using (var characterReceived = EventTester<ComboBox, CharacterReceivedRoutedEventArgs>.FromRoutedEvent(comboBox, "CharacterReceived", (sender, args) => verifyCharEvent("CharacterReceived", args)))
            {
                Log.Comment("Sending c keystroke.");
                TestServices.KeyboardHelper.PressKeySequence("$d$_c#$u$_c");

                characterReceived.Wait();
                TestServices.WindowHelper.WaitForIdle();

                Verify.IsTrue(received);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CharacterReceivedEvent can be subclassed by a user control")]
        public void VerifyCharacterReceivedEventSubclass()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                            <UserControl x:Name='engagedControl' IsFocusEngagementEnabled='true'>
                                 <Button Width='50' x:Name='button1' Content='Button 1'/>
                            </UserControl>
                    </StackPanel>";

            StackPanel rootPanel = null;
            UserControl engagedControl = null;
            Button button1 = null;

            bool received = false;

            var verifyCharEvent = new Action<string, CharacterReceivedRoutedEventArgs>((eventName, args) =>
            {
                Log.Comment($"Event {eventName} fired");
                received = true;
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                engagedControl = (UserControl)rootPanel.FindName("engagedControl");
                button1 = (Button)rootPanel.FindName("button1");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var characterReceived = EventTester<Button, CharacterReceivedRoutedEventArgs>.FromRoutedEvent(engagedControl, "CharacterReceived", (s,a)=>verifyCharEvent("CharacterReceived",a)))
            {
                TestServices.KeyboardHelper.PressKeySequence("$d$_a#$u$_a");                   

                characterReceived.Wait();
                TestServices.WindowHelper.WaitForIdle();

                Verify.AreEqual(received, true);
            }        
        }
    }
}
