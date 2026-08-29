// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <InputManager.h>

#if defined(BUILDING_PRIVATEINFRASERVER_DLL)
#define PI_API __declspec(dllexport)
#else
#define PI_API __declspec(dllimport)
#endif

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class PI_API InputRoutineHelper
        {
        public:
            static void EnsureInputIsInitialized();
            static void InjectPress(InputDevice inputDevice, POINT pt, UINT durationMs, UINT pressCount, UINT pressDeltaMs);
            static void InjectPressAndDrag(InputDevice inputDevice, POINT ptStart, POINT ptEnd, UINT durationInDragMs, UINT durationInHoldMs);
            static void InjectZoom(POINT ptStartFinger1, POINT ptStartFinger2, FLOAT dbDirection, UINT durationMs, UINT distance);
            static void InjectRotate(POINT startFinger1, POINT startFinger2, FLOAT rotationAngle, UINT durationMs, BOOLEAN privateRotate);
            static void InjectDynamicPress(InputDevice inputDevice, POINT ptPress, DWORD dwWidth, DWORD dwHeight, UINT contactID);
            static void InjectDynamicRelease(UINT contactID);
            static void SendMouseButtonInput(MouseButton button, POINT position, BOOLEAN down);
            static void SendMouseDragInput(MouseButton button, POINT positionSrc, POINT positionDest);
            static void SendMouseMoveInput(POINT position);
            static void SendMouseWheelInput(POINT position, INT numberOfWheel);
            static void InjectPenBarrelTap(POINT pt);
            static void InjectDragWithPenBarrelDown(POINT ptStart, POINT ptEnd, UINT durationInDragMs);
            static void Reset();

        private:
            static UINT DigitizeDistance(UINT digitizerId, UINT screenDistance);
            static POINT NormalizeMouseCoordinates(POINT pt);
            static void EnsureMouseInjectionReady();

            // Number of milliseconds between input packets
            static const unsigned int s_defaultPacketDeltaMs = 10;
            static const unsigned int s_unknownDigitizerId = static_cast<unsigned int>(-1);
            static bool s_inputManagerInitialized;
            static bool s_mouseInjectionReady;
        };
    }
} } } }
