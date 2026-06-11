// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class CommonInputHelper
    {
        public enum InputDevice
        {
            Keyboard,
            Gamepad
        }

        public static void Left(InputDevice device)
        {
            if(device == InputDevice.Keyboard)
            {
                TestServices.KeyboardHelper.Left();
            }
            else if(device == InputDevice.Gamepad)
            {
                TestServices.KeyboardHelper.GamepadDpadLeft();
            }
        }

        public static void Right(InputDevice device)
        {
            if (device == InputDevice.Keyboard)
            {
                TestServices.KeyboardHelper.Right();
            }
            else if (device == InputDevice.Gamepad)
            {
                TestServices.KeyboardHelper.GamepadDpadRight();
            }
        }

        public static void Up(InputDevice device)
        {
            if (device == InputDevice.Keyboard)
            {
                TestServices.KeyboardHelper.Up();
            }
            else if (device == InputDevice.Gamepad)
            {
                TestServices.KeyboardHelper.GamepadDpadUp();
            }
        }

        public static void Down(InputDevice device)
        {
            if (device == InputDevice.Keyboard)
            {
                TestServices.KeyboardHelper.Down();
            }
            else if (device == InputDevice.Gamepad)
            {
                TestServices.KeyboardHelper.GamepadDpadDown();
            }
        }
    }
}
