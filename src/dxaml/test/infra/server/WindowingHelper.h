// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "wincodec.h"

#if defined(BUILDING_PRIVATEINFRASERVER_DLL)
#define PI_API __declspec(dllexport)
#else
#define PI_API __declspec(dllimport)
#endif

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class PI_API WindowingHelper
        {
        public:
            static void GetRightsAndSetForegroundWindow(HWND handle);
            static void SetDesktopWindowSize(HWND handle, unsigned int width, unsigned int height);
            static void MoveDesktopWindow(HWND handle, unsigned int x, unsigned int y);
            static void MaximizeDesktopWindow(HWND handle);
            static BOOL IsDesktopWindowMaximized(HWND handle);
        private:
            static BOOL CALLBACK FindTopLevelForegroundWindow(_In_ HWND hwnd, _In_ LPARAM lParam);
            static HWND GetTopLevelForegroundWindow();
        };
    }
} } } }

