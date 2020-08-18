// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Runtime.InteropServices;
using System;
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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public enum GamepadButton
    {
        A = 0xC3,
        B = 0xC4,
        X = 0xC5,
        Y = 0xC6,
        RightShoulder = 0xC7, // yes it's actually right, left, left, right
        LeftShoulder = 0xC8,
        LeftTrigger = 0xC9,
        RightTrigger = 0xCA,
        DPadUp = 0xCB,
        DPadDown = 0xCC,
        DPadLeft = 0xCD,
        DPadRight = 0xCE,
        Menu = 0xCF,
        View = 0xD0,
        LeftThumbstickButton = 0xD1,
        RightThumbstick = 0xD2,
        LeftThumbstickUp = 0xD3,
        LeftThumbstickDown = 0xD4,
        LeftThumbstickRight = 0xD5,
        LeftThumbstickLeft = 0xD6,
        RightThumbstickUp = 0xD7,
        RightThumbstickDown = 0xD8,
        RightThumbstickRight = 0xD9,
        RightThumbstickLeft = 0xDA,
    }

    public class GamepadHelper
    {
        public static void PressButton(UIObject obj, GamepadButton button)
        {
            var keyInputReceivedUIObject = FindElement.ById("__KeyInputReceived");

            if(keyInputReceivedUIObject != null)
            {
                var keyInputReceivedCheckbox = new CheckBox(keyInputReceivedUIObject);

                if (keyInputReceivedCheckbox.ToggleState == ToggleState.On)
                {
                    keyInputReceivedCheckbox.Uncheck();
                }
                Verify.IsTrue(keyInputReceivedCheckbox.ToggleState == ToggleState.Off);
            }

            Log.Comment("Pressing Gamepad button: {0}", button);    
            KeyboardInput[] array = new KeyboardInput[1];
            array[0].virtualKeyCode = (ushort)button;
            InternalNativeMethods.InjectKeyboardInput(array, 1);


            // Wait until app reports it received the input
            if(keyInputReceivedUIObject == null)
            {
                // Fallback if we don't find out helper
                Wait.ForMilliseconds(50);
            }
            else
            {
                var keyInputReceivedCheckbox = new CheckBox(keyInputReceivedUIObject);
                while (keyInputReceivedCheckbox.ToggleState != ToggleState.On)
                {
                    Wait.ForMilliseconds(10);
                }
            }

            Wait.ForIdle();
            array[0].flags = KEY_EVENT_FLAGS.KEYUP;
            InternalNativeMethods.InjectKeyboardInput(array, 1);
        }

        #region Keyboard Structures
        [Flags]
        public enum KEY_EVENT_FLAGS : uint
        {
            NONE = 0x0000, // Added for error checking
            EXTENDEDKEY = 0x0001,
            KEYUP = 0x0002,
            UNICODE = 0x0004,
            SCANCODE = 0x0008,
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct KeyboardInput
        {
            public ushort virtualKeyCode;
            public ushort scanCode;
            public KEY_EVENT_FLAGS flags;
            public uint time;
            public IntPtr extraInfo;

            public override string ToString()
            {
                return "vk: 0x" + this.virtualKeyCode.ToString("X") + ", sc: 0x" + this.scanCode.ToString("X") + ", flags: " + this.flags;
            }
        }
        #endregion

        private static class InternalNativeMethods
        {
            private const string NTUSER_RIM = "ext-ms-win-ntuser-rim-l1-1-0.dll";  // in OneCoreUAP, but not OneCore

            [DllImport(NTUSER_RIM, SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool InjectKeyboardInput([In] KeyboardInput[] keyboardInput, [In] uint count);
        }
    }
}
