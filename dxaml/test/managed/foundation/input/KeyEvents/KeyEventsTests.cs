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
    public class KeyEventTests : XamlTestsBase
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
        [TestProperty("Description", "Verifies enter key repeat count in key events.")]
        public void VerifyEnterKeyRepeatCount()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                         <Button Width='50' x:Name='button1' Content='Button 1'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;
            const uint expectedRepeatCount = 1;
            int hit = 0;

            var verifyRepeatCount = new Action<string, KeyRoutedEventArgs>((eventName, args) =>
            {
                Log.Comment($"Event {eventName} fired: RepeatCount={args.KeyStatus.RepeatCount}");
                Verify.AreEqual(args.KeyStatus.RepeatCount, expectedRepeatCount);
                hit++;
                Log.Comment($"Hit count={hit}");
            });

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Focus button 1");
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            using (var previewKeyDown = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "PreviewKeyDown", (s,a)=>verifyRepeatCount("PreviewKeyDown",a)))
            using (var keyDown = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "KeyDown", (s,a)=>verifyRepeatCount("KeyDown",a)))
            using (var previewKeyUp = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "PreviewKeyUp", (s,a)=>verifyRepeatCount("PreviewKeyUp",a)))
            using (var keyUp = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "KeyUp", (s,a)=>verifyRepeatCount("KeyUp",a)))
            {
                TestServices.KeyboardHelper.Enter();

                previewKeyDown.Wait();
                keyDown.Wait();
                previewKeyUp.Wait();
                keyUp.Wait();

                TestServices.WindowHelper.WaitForIdle();

                Verify.AreEqual(hit, 4);
            }
        }
    }
}
