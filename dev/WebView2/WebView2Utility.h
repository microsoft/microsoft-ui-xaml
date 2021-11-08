// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#pragma warning( disable : 26496)

// These utilities convert WinRT event args into Win32 event args which are sent to the webview
// Referencing winuser.h input codes : https://docs.microsoft.com/en-us/windows/desktop/api/winuser/
namespace WebView2Utility
{
    namespace winrtsystem = winrt::Windows::System;

    LPARAM PackIntoWin32StylePointerArgs_lparam(
        const UINT message,
        const winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs& args, winrt::Windows::Foundation::Point point)
    {
        // These are the same for WM_POINTER and WM_MOUSE based events
        // Pointer: https://msdn.microsoft.com/en-us/ie/hh454929(v=vs.80)
        // Mouse: https://docs.microsoft.com/en-us/windows/desktop/inputdev/wm-mousemove
        LPARAM lParam = MAKELPARAM((signed int)(point.X), (signed int)(point.Y));
        return lParam;
    }

    WPARAM PackIntoWin32StyleMouseArgs_wparam(
        const UINT message,
        const winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs& args, winrt::PointerPoint pointerPoint)
    {
        WORD lowWord = 0x0;       // unsigned modifier flags
        short highWord = 0x0;     // signed wheel delta
        
        winrtsystem::VirtualKeyModifiers modifiers(args.KeyModifiers());

        // can support cases like Ctrl|Alt + Scroll where Alt will be ignored and it will be treated as Ctrl + Scroll
        if (static_cast<int>(modifiers) & static_cast<int>(winrtsystem::VirtualKeyModifiers::Control))
        {
            lowWord |= MK_CONTROL;
        }
        if (static_cast<int>(modifiers) & static_cast<int>(winrtsystem::VirtualKeyModifiers::Shift))
        {
            lowWord |= MK_SHIFT;
        }

        winrt::PointerPointProperties properties{ pointerPoint.Properties() };

        if (properties.IsLeftButtonPressed())
        {
            lowWord |= MK_LBUTTON;
        }
        if (properties.IsRightButtonPressed())
        {
            lowWord |= MK_RBUTTON;
        }
        if (properties.IsMiddleButtonPressed())
        {
            lowWord |= MK_MBUTTON;
        }
        if (properties.IsXButton1Pressed())
        {
            lowWord |= MK_XBUTTON1;
        }
        if (properties.IsXButton2Pressed())
        {
            lowWord |= MK_XBUTTON2;
        }

        // Mouse wheel : https://docs.microsoft.com/en-us/windows/desktop/inputdev/wm-mousewheel
        if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
        {
            // TODO_WebView2 : See if this needs to be multiplied with scale for different dpi scenarios
            highWord = static_cast<short>(properties.MouseWheelDelta());
        }
        else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONUP)
        {
            // highWord Specifies which of the two XButtons is referenced by the message
            winrt::Windows::UI::Input::PointerUpdateKind pointerUpdateKind{ properties.PointerUpdateKind() };
            if (pointerUpdateKind == winrt::PointerUpdateKind::XButton1Pressed || 
                pointerUpdateKind == winrt::PointerUpdateKind::XButton1Released)
            {
                highWord |= XBUTTON1;
            }
            else if (pointerUpdateKind == winrt::PointerUpdateKind::XButton2Pressed ||
                     pointerUpdateKind == winrt::PointerUpdateKind::XButton2Released)
            {
                highWord |= XBUTTON2;
            }
        }

        WPARAM wParam = MAKEWPARAM(lowWord, highWord);
        return wParam;
    }
}
