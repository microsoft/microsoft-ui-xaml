// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Input;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using Windows.System;
using Windows.UI.Core;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Tests.Input.KeyEvents
{
    [TestClass]
    public class KeyboardTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            TestServicesExtensions.EnsureInitialized();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies InputKeyboardSource.GetKeyStateForCurrentThread")]
        public void VerifyGetKeyState()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                         <Button Width='50' x:Name='button1' Content='Button 1'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            EnsureKeysUnlocked();

            // Try pressing the 'a' key down, and verify that its state shows as 'down'
            // We verify the 'down' state twice to ensure that observing the state does not destroy it
            Log.Comment($"Pressing 'a' key down");
            TestServices.KeyboardHelper.PressKeySequence("$d$_a");

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.Down | CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Down | CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // Now we let go of the key, and verify the key is no longer down
            Log.Comment($"Letting go of 'a' key");
            TestServices.KeyboardHelper.PressKeySequence("$u$_a");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // Now we press Ctrl+A and verify
            Log.Comment($"Pressing Ctrl+A keys down");
            TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.Down, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Down | CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // Now we let go of Ctrl
            Log.Comment($"Letting go of Ctrl");
            TestServices.KeyboardHelper.PressKeySequence("$u$_ctrlscan");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.Down, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // And then let go of 'a'
            Log.Comment($"Letting go of 'a' key");
            TestServices.KeyboardHelper.PressKeySequence("$u$_a");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // Then we can test CapsLock since it's a key that supports the 'locked' state
            Log.Comment($"Pressing CapsLock down");
            TestServices.KeyboardHelper.PressKeySequence("$d$_capslock");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.Down | CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // Then we let go of the caps-lock key.  It should now still be 'locked' but no longer be down
            Log.Comment($"Letting go of CapsLock");
            TestServices.KeyboardHelper.PressKeySequence("$u$_capslock");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // If we press another key now, while CapsLock is locked, CapsLock is unaffected
            Log.Comment($"Pressing 'a' key down while CapsLock is locked");
            TestServices.KeyboardHelper.PressKeySequence("$d$_a");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.Down | CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            Log.Comment($"Letting go of 'a' key while CapsLock is locked");
            TestServices.KeyboardHelper.PressKeySequence("$u$_a");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            // Toggle CapsLock back to starting state
            Log.Comment($"Toggle CapsLock back off by pressing CapsLock down then up");
            TestServices.KeyboardHelper.PressKeySequence("$d$_capslock#$u$_capslock");
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies InputKeyboardSource.GetKeyStateForCurrentThread inside a UIElement.Key[Down|Up] handler")]
        public void VerifyGetKeyStateInKeyEventHandler()
        {
            const string rootPanelXaml =
                    @"<StackPanel Orientation='Vertical' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootPanel'>
                         <Button Width='50' x:Name='button1' Content='Button 1'/>
                    </StackPanel>";

            StackPanel rootPanel = null;
            Button button1 = null;

            UIExecutor.Execute(() =>
            {
                rootPanel = (StackPanel)XamlReader.Load(rootPanelXaml);
                button1 = (Button)rootPanel.FindName("button1");
                TestServices.WindowHelper.WindowContent = rootPanel;
            });

            TestServices.WindowHelper.WaitForIdle();
            FocusHelper.EnsureFocus(button1, FocusState.Keyboard);

            EnsureKeysUnlocked();

            var verifyGetKeyStateInKeyDown = new Action<string, KeyRoutedEventArgs>((eventName, args) =>
            {
                Log.Comment($"Event {eventName} fired");
                if(args.Key == VirtualKey.A)
                {
                    Verify.AreEqual(CoreVirtualKeyStates.Down | CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                    Verify.AreEqual(CoreVirtualKeyStates.Down | CoreVirtualKeyStates.Locked, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                }
            });

            var verifyGetKeyStateInKeyUp = new Action<string, KeyRoutedEventArgs>((eventName, args) =>
            {
                Log.Comment($"Event {eventName} fired");
                if(args.Key == VirtualKey.A)
                {
                    // During KeyUp, Ctrl and A are both in an 'up' state (they happen to be locked, but the important point is that they're not Down)
                    Verify.IsFalse(InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control).HasFlag(CoreVirtualKeyStates.Down));
                    Verify.IsFalse(InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A).HasFlag(CoreVirtualKeyStates.Down));
                }
            });

            using (var keyDown = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "KeyDown", (s,a)=>verifyGetKeyStateInKeyDown("KeyDown",a)))
            using (var keyUp = EventTester<Button, KeyRoutedEventArgs>.FromRoutedEvent(button1, "KeyUp", (s,a)=>verifyGetKeyStateInKeyUp("KeyUp",a)))
            {
                // Ctrl Down, A Down, Ctrl Up, A Up (to test Ctrl being down during KeyDown processing, and Ctrl being up during KeyUp processing)
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$d$_a#$u$_ctrlscan#$u$_a");
                keyDown.Wait();
                keyUp.Wait();

                TestServices.WindowHelper.WaitForIdle();
            }
        }

        private void EnsureKeysUnlocked()
        {
            // When this test launches, we need to check which keys are already in a 'locked' state,
            // since that is system-wide and previous tests may have pressed a key an odd number of
            // times, so the keys could be locked through no fault of anyone.  If they're locked, we
            // need to press them down then up once to un-lock them so we have 'None' as the starting
            // state of each key we care about.
            bool isALocked = false, isCtrlLocked = false, isShiftLocked = false, isCapsLockLocked = false;
            UIExecutor.Execute(() =>
            {
                // Start by verifying the state of the keys we're going to be checking, before we go pressing keys
                isALocked = InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A).HasFlag(CoreVirtualKeyStates.Locked);
                isCtrlLocked = InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control).HasFlag(CoreVirtualKeyStates.Locked);
                isShiftLocked = InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift).HasFlag(CoreVirtualKeyStates.Locked);
                isCapsLockLocked = InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock).HasFlag(CoreVirtualKeyStates.Locked);
            });

            if (isALocked)
            {
                Log.Comment($"Detected the 'a' key is locked on test start - pressing it down and up to unlock it.");
                TestServices.KeyboardHelper.PressKeySequence("$d$_a#$u$_a");
            }
            if (isCtrlLocked)
            {
                Log.Comment($"Detected the 'Ctrl' key is locked on test start - pressing it down and up to unlock it.");
                TestServices.KeyboardHelper.PressKeySequence("$d$_ctrlscan#$u$_ctrlscan");
            }
            if (isShiftLocked)
            {
                Log.Comment($"Detected the 'Shift' key is locked on test start - pressing it down and up to unlock it.");
                TestServices.KeyboardHelper.PressKeySequence("$d$_shift#$u$_shift");
            }
            if (isCapsLockLocked)
            {
                Log.Comment($"Detected the 'CapsLock' key is locked on test start - pressing it down and up to unlock it.");
                TestServices.KeyboardHelper.PressKeySequence("$d$_capslock#$u$_capslock");
            }

            Log.Comment("Verifying all keys are now in the 'None' state before starting the real test.  If these fail, then the system wasn't reset enough between tests.");
            UIExecutor.Execute(() =>
            {
                // Start by verifying the state of the keys we're going to be checking, before we go pressing keys
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.A));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Control));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.Shift));
                Verify.AreEqual(CoreVirtualKeyStates.None, InputKeyboardSource.GetKeyStateForCurrentThread(VirtualKey.CapitalLock));
            });
        }
    }
}
