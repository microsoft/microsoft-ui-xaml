// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    public enum class InputDevice
    {
        Keyboard,
        Gamepad
    };

    class CommonInputHelper
    {
    public:
        static void Left(InputDevice device)
        {
            switch (device)
            {
                case InputDevice::Keyboard:
                    TestServices::KeyboardHelper->Left();
                    break;
                    
                case InputDevice::Gamepad:
                    TestServices::KeyboardHelper->GamepadDpadLeft();
                    break;

                default:
                    WEX::Common::Throw::Exception(E_FAIL, L"Invalid input device.");
            }
        }
        
        static void Right(InputDevice device)
        {
            switch (device)
            {
                case InputDevice::Keyboard:
                    TestServices::KeyboardHelper->Right();
                    break;
                    
                case InputDevice::Gamepad:
                    TestServices::KeyboardHelper->GamepadDpadRight();
                    break;

                default:
                    WEX::Common::Throw::Exception(E_FAIL, L"Invalid input device.");
            }
        }
            
        static void Up(InputDevice device)
        {
            switch (device)
            {
                case InputDevice::Keyboard:
                    TestServices::KeyboardHelper->Up();
                    break;
                    
                case InputDevice::Gamepad:
                    TestServices::KeyboardHelper->GamepadDpadUp();
                    break;

                default:
                    WEX::Common::Throw::Exception(E_FAIL, L"Invalid input device.");
            }
        }
            
        static void Down(InputDevice device)
        {
            switch (device)
            {
                case InputDevice::Keyboard:
                    TestServices::KeyboardHelper->Down();
                    break;
                    
                case InputDevice::Gamepad:
                    TestServices::KeyboardHelper->GamepadDpadDown();
                    break;

                default:
                    WEX::Common::Throw::Exception(E_FAIL, L"Invalid input device.");
            }
        }
            
        static void Accept(InputDevice device)
        {
            switch (device)
            {
                case InputDevice::Keyboard:
                    TestServices::KeyboardHelper->Space();
                    break;
                    
                case InputDevice::Gamepad:
                    TestServices::KeyboardHelper->GamepadA();
                    break;

                default:
                    WEX::Common::Throw::Exception(E_FAIL, L"Invalid input device.");
            }
        }
            
        static void Cancel(InputDevice device)
        {
            switch (device)
            {
                case InputDevice::Keyboard:
                    TestServices::KeyboardHelper->Escape();
                    break;
                    
                case InputDevice::Gamepad:
                    TestServices::KeyboardHelper->GamepadB();
                    break;

                default:
                    WEX::Common::Throw::Exception(E_FAIL, L"Invalid input device.");
            }
        }
    };

} } } } }

