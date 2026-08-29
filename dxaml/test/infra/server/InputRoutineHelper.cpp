// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "InputRoutineHelper.h"
#include "UtilitiesRoutineHelper.h"
#include <ShellScalingApi.h>

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        bool InputRoutineHelper::s_inputManagerInitialized = false;
        bool InputRoutineHelper::s_mouseInjectionReady = false;

        void InputRoutineHelper::EnsureInputIsInitialized()
        {
            if (!s_inputManagerInitialized)
            {
                LOG_OUTPUT(L"Initializing input manager.");

                if (UtilitiesRoutineHelper::IsDesktop())
                {
                    LOG_OUTPUT(L"Server: Setting thread DPI awareness context to DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2");
                    DPI_AWARENESS_CONTEXT oldDpiAwarenessContext = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

                    // DPI awareness contexts contain informational flags and can't be bitwise compared.
                    if (!::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                    {
                        if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
                        {
                            LOG_OUTPUT(L"[IMInitializeEx]: DPI awareness was DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE");
                        }
                        else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
                        {
                            LOG_OUTPUT(L"[IMInitializeEx]: DPI awareness was DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED");
                        }
                        else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
                        {
                            LOG_OUTPUT(L"[IMInitializeEx]: DPI awareness was DPI_AWARENESS_CONTEXT_SYSTEM_AWARE");
                        }
                        else if (::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_UNAWARE))
                        {
                            LOG_OUTPUT(L"[IMInitializeEx]: DPI awareness was DPI_AWARENESS_CONTEXT_UNAWARE");
                        }

                        Throw::If(::AreDpiAwarenessContextsEqual(oldDpiAwarenessContext, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2), E_FAIL,
                            L"[IMInitializeEx]: Failed to intialize InputManager due to process DPI awareness context not being PerMonitorV2");
                    }
                }

                LogThrow_IfFailed(UtilitiesRoutineHelper::CheckAndKillServerManager());

                // TODO: Remove this sleep
                // Temporary workaround - input injection does not work reliably if it is the very first test run in the process,
                // due to a timing problem: The CoreWindow Activated event is raised before the DWM has fully
                // setup the visual tree for the application, causing input to be dropped on the floor.
                // By introducing a small delay, this gives the DWM time to setup the tree.
                Sleep(500);

                LogThrow_IfFailed(IMInitializeEx(10, 5));
                LogThrow_IfFailed(IMSetPacketCoalescingOff(true));

                // And also sleep after initialization to ensure the kernel input queue has time to flush
                Sleep(500);

                s_inputManagerInitialized = true;
            }
        }

        void InputRoutineHelper::EnsureMouseInjectionReady()
        {
            if (!s_mouseInjectionReady)
            {
                // Mouse injection does not work reliably just after app launch.
                // See comments above in EnsureInputIsInitialized().
                // The workaround is to add a Sleep.
                Sleep(100);
                s_mouseInjectionReady = true;
            }
        }

        void InputRoutineHelper::Reset()
        {
            // In Win32 mode, we need to wait again after XamlSource has been cleaned up.
            s_mouseInjectionReady = false;
            s_inputManagerInitialized = false;
        }

        //------------------------------------------------------------------------
        // Digitize the distance from screen units to digitizer units.
        // screenDistance: The distance in screen units.
        //------------------------------------------------------------------------
        UINT InputRoutineHelper::DigitizeDistance(UINT digitizerId, UINT screenDistance)
        {
            POINT ptScreenDistance = { static_cast<LONG>(screenDistance), 0 };
            POINT ptDigitizerDistance = { 0, 0 };

            LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, ptScreenDistance, &ptDigitizerDistance));

            return ptDigitizerDistance.x;
        }

        //------------------------------------------------------------------------
        // Mouse coordinates are systematically normalized between 0 and 65535.
        // (0,0) corresponds to the top left corner of the screen.
        // (65535, 65535) corresponds to the bottom right corner of the screen.
        //------------------------------------------------------------------------
        POINT InputRoutineHelper::NormalizeMouseCoordinates(POINT pt)
        {
            POINT ptNormalized = { 0 };
            ptNormalized.x = static_cast<int>(65535 * (static_cast<float>(pt.x) / GetSystemMetrics(SM_CXSCREEN)));
            ptNormalized.y = static_cast<int>(65535 * (static_cast<float>(pt.y) / GetSystemMetrics(SM_CYSCREEN)));
            return ptNormalized;
        }

        void InputRoutineHelper::InjectPress(InputDevice inputDevice, POINT pt, UINT durationMs, UINT pressCount, UINT pressDeltaMs)
        {
            EnsureInputIsInitialized();

            UINT digitizerId = 0;
            INPUT_DEVICE device = static_cast<INPUT_DEVICE>(inputDevice);
            LogThrow_IfFailed(IMGetInjectionDigitizerId(device, &digitizerId));
            Throw::If(digitizerId == s_unknownDigitizerId && !UtilitiesRoutineHelper::IsOneCore(), E_FAIL,
                L"[RpcInjectPress]: Failed to access valid digitizerId. This failure may be due to using a Remote Desktop Connection which is not supported.");
            POINT ptDigitizer = { 0 };

            if (!UtilitiesRoutineHelper::IsOneCore() && device == ID_MOUSE)
            {
                // Mouse coordinates for IMInjectPress are normalized between 0 and 65535.
                // (0, 0) corresponds to the top left corner of the screen.
                // (65535, 65535) corresponds to the bottom right corner of the screen.
                ptDigitizer = NormalizeMouseCoordinates(pt);
            }
            else
            {
                // Note: For multimon with touch this doesn't give the expected result. Rather than mapping to one
                // monitor it maps to the entire desktop spanning all monitors.
                LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, pt, &ptDigitizer));
            }
            LOG_OUTPUT(L"[RpcInjectPress]: Press (%d, %d) %d times. Duration %d, PressDelta %d, digitizer (%d, %d)",
                pt.x, pt.y, pressCount, durationMs, pressDeltaMs, ptDigitizer.x, ptDigitizer.y);
            LogThrow_IfFailed(IMInjectPress(ptDigitizer, durationMs, pressCount /*tapCount*/, pressDeltaMs /*tapDeltaMs*/, s_defaultPacketDeltaMs, digitizerId));
        }

        void InputRoutineHelper::InjectPressAndDrag(InputDevice inputDevice, POINT ptStart, POINT ptEnd, UINT durationInDragMs, UINT durationInHoldMs)
        {
            EnsureInputIsInitialized();
            UINT digitizerId = 0;
            INPUT_DEVICE device = static_cast<INPUT_DEVICE>(inputDevice);
            LogThrow_IfFailed(IMGetInjectionDigitizerId(device, &digitizerId));
            Throw::If(digitizerId == s_unknownDigitizerId && !UtilitiesRoutineHelper::IsOneCore(), E_FAIL,
                L"[RpcInjectPressAndDrag]: Failed to access valid digitizerId. This failure may be due to using a Remote Desktop Connection which is not supported.");
            POINT ptDigitizerStart = { 0 };
            POINT ptDigitizerEnd = { 0 };
            if (device == ID_MOUSE)
            {
                // Mouse coordinates for IMInjectPressAndDrag are normalized between 0 and 65535.
                // (0, 0) corresponds to the top left corner of the screen.
                // (65535, 65535) corresponds to the bottom right corner of the screen.
                ptDigitizerStart = NormalizeMouseCoordinates(ptStart);
                ptDigitizerEnd = NormalizeMouseCoordinates(ptEnd);
            }
            else
            {
                LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, ptStart, &ptDigitizerStart));
                LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, ptEnd, &ptDigitizerEnd));
            }
            LOG_OUTPUT(L"[RpcInjectPressAndDrag]: Press and drag (%d, %d) to (%d, %d). Duration %d, %d.",
                ptDigitizerStart.x, ptDigitizerStart.y, ptDigitizerEnd.x, ptDigitizerEnd.y, durationInDragMs, durationInHoldMs);
            LogThrow_IfFailed(IMInjectPressAndDrag(ptDigitizerStart, ptDigitizerEnd,
                std::max<unsigned int>(s_defaultPacketDeltaMs, durationInDragMs), std::max<unsigned int>(s_defaultPacketDeltaMs, durationInHoldMs),
                s_defaultPacketDeltaMs, digitizerId));
        }

        void InputRoutineHelper::InjectPenBarrelTap(POINT pt)
        {
            // Injection on OneCore SKUs is not supported.
            Throw::If(UtilitiesRoutineHelper::IsOneCore(), E_FAIL,
                L"[RpcInjectPenBarrelTap]: This call is not supported on non-Desktop SKUs.");

            EnsureInputIsInitialized();
            LogThrow_IfFailed(IMInjectPenBarrelTap(pt));
        }

        void InputRoutineHelper::InjectDragWithPenBarrelDown(POINT ptStart, POINT ptEnd, UINT durationInDragMs)
        {
            // Injection on OneCore SKUs is not supported.
            Throw::If(UtilitiesRoutineHelper::IsOneCore(), E_FAIL,
                L"[RpcInjectDragWithPenBarrelDown]: This call is not supported on non-Desktop SKUs.");

            LOG_OUTPUT(L"[RpcInjectDragWithPenBarrelDown]: Press and drag (%d, %d) to (%d, %d). Duration %d.",
                ptStart.x, ptStart.y, ptEnd.x, ptEnd.y, durationInDragMs);

            LogThrow_IfFailed(IMInjectDragWithPenBarrelDown(ptStart, ptEnd, durationInDragMs));
        }

        void InputRoutineHelper::InjectZoom(POINT ptStartFinger1, POINT ptStartFinger2, FLOAT dbDirection, UINT durationMs, UINT distance)
        {
            EnsureInputIsInitialized();
            UINT digitizerId = 0;
            LogThrow_IfFailed(IMGetInjectionDigitizerId(ID_MULTITOUCH, &digitizerId));
            Throw::If(digitizerId == s_unknownDigitizerId && !UtilitiesRoutineHelper::IsOneCore(), E_FAIL,
                L"[RpcInjectZoom]: Failed to access valid digitizerId. This failure may be due to using a Remote Desktop Connection which is not supported.");
            POINT ptDigitizerFinger1 = { 0 };
            POINT ptDigitizerFinger2 = { 0 };
            UINT digitizerDistance = 0;
            LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, ptStartFinger1, &ptDigitizerFinger1));
            LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, ptStartFinger2, &ptDigitizerFinger2));
            digitizerDistance = DigitizeDistance(digitizerId, distance);
            LOG_OUTPUT(L"[RpcInjectZoom]: Zoom (%d, %d) and (%d, %d). Duration %d, Distance %d, Direction %f.",
                ptDigitizerFinger1.x, ptDigitizerFinger1.y, ptDigitizerFinger2.x, ptDigitizerFinger2.y, durationMs, distance, dbDirection);
            LogThrow_IfFailed(IMInjectMTZoom(
                ptDigitizerFinger1,
                ptDigitizerFinger2,
                dbDirection,
                std::max<unsigned int>(s_defaultPacketDeltaMs, durationMs),
                digitizerDistance,
                false /*fPivotZoom*/,
                s_defaultPacketDeltaMs,
                digitizerId));
        }

        void InputRoutineHelper::InjectRotate(POINT startFinger1, POINT startFinger2, FLOAT rotationAngle, UINT durationMs, BOOLEAN pivotRotate)
        {
            EnsureInputIsInitialized();
            UINT digitizerId = 0;
            LogThrow_IfFailed(IMGetInjectionDigitizerId(ID_MULTITOUCH, &digitizerId));
            LogThrow_If(digitizerId == s_unknownDigitizerId && !UtilitiesRoutineHelper::IsOneCore(), E_FAIL,
                L"[RpcInjectRotate]: Failed to access valid digitizerId. This failure may be due to using a Remote Desktop Connection which is not supported.");
            POINT ptDigitizerFinger1 = { 0 };
            POINT ptDigitizerFinger2 = { 0 };
            LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, startFinger1, &ptDigitizerFinger1));
            LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, startFinger2, &ptDigitizerFinger2));
            LOG_OUTPUT(L"[RpcInjectRotate]: Rotate starting at (%d, %d) and (%d, %d). Duration %d, Angle %f.",
                ptDigitizerFinger1.x, ptDigitizerFinger1.y, ptDigitizerFinger2.x, ptDigitizerFinger2.y, durationMs, rotationAngle);
#if TRUE
            LogThrow_IfFailed(E_NOTIMPL);
#else // TODO: Implement InputManager.IMInjectMTRotate and enable this code:
            LogThrow_IfFailed(IMInjectMTRotate(
                ptDigitizerFinger1,
                ptDigitizerFinger2,
                rotationAngle,
                std::max<unsigned int>(s_defaultPacketDeltaMs, durationMs),
                pivotRotate,
                s_defaultPacketDeltaMs,
                digitizerId));
#endif
        }

        void InputRoutineHelper::InjectDynamicPress(InputDevice inputDevice, POINT ptPress, DWORD dwWidth, DWORD dwHeight, UINT contactID)
        {
            EnsureInputIsInitialized();
            UINT digitizerId = 0;
            INPUT_DEVICE device = static_cast<INPUT_DEVICE>(inputDevice);
            LogThrow_IfFailed(IMGetInjectionDigitizerId(device, &digitizerId));
            Throw::If(digitizerId == s_unknownDigitizerId && !UtilitiesRoutineHelper::IsOneCore(), E_FAIL,
                L"[RpcInjectDynamicPress]: Failed to access valid digitizerId. This failure may be due to using a Remote Desktop Connection which is not supported.");
            POINT ptDigitizerFinger1 = { 0 };
            LogThrow_IfFailed(IMConvertCoordinatesToDigitizer(digitizerId, CU_LOGICAL_SCREEN, ptPress, &ptDigitizerFinger1));

            LOG_OUTPUT(L"[RpcInjectDynamicPress]: Dynamic Press (%d, %d) Width %d, Height %d, ContactID %d.",
                ptPress.x, ptPress.y, dwWidth, dwHeight, contactID);
            LogThrow_IfFailed(IMInjectDynamicPress(
                ptDigitizerFinger1,
                dwWidth,
                dwHeight,
                contactID));
        }

        void InputRoutineHelper::InjectDynamicRelease(UINT contactID)
        {
            EnsureInputIsInitialized();

            LOG_OUTPUT(L"[RpcInjectDynamicRelease]: ContactID %d", contactID);

            LogThrow_IfFailed(IMInjectDynamicRelease(contactID));
        }

        void InputRoutineHelper::SendMouseButtonInput(MouseButton button, POINT position, BOOLEAN down)
        {
            EnsureMouseInjectionReady();

            // Although IMInjectMouseButtonInput accepts a position parameter, it doesn't actually
            // set the MOUSEEVENTF_MOVE flag to use that data.  So we will explicitly move it here.
            //
            // TODO: Fix IMInjectMouseButtonInput to move the cursor - then either use the point in
            // IMInjectMouseButtonInput (in which case this move should be removed), or remove the position
            // parameter from IMInjectMouseButtonInput (in which case we should just remove this comment).
            SendMouseMoveInput(position);

            LOG_OUTPUT(L"[RpcSendMouseButtonInput]: %s mouse button %s @ position (%d, %d).",
                (button == MouseButton_Left ? L"LEFT" : button == MouseButton_Right ? L"RIGHT" : L"MIDDLE"),
                (down ? L"DOWN" : L"UP"),
                position.x, position.y
                );

            LogThrow_IfFailed(IMInjectMouseButtonInput((DWORD)button, position, down));
        }

        void InputRoutineHelper::SendMouseDragInput(MouseButton button, POINT positionSrc, POINT positionDest)
        {
            EnsureMouseInjectionReady();

            LOG_OUTPUT(L"[RpcSendMouseDragInput]: %s mouse button drag from positionSrc (%d, %d) to positionDest (%d, %d).",
                (button == MouseButton_Left ? L"LEFT" : button == MouseButton_Right ? L"RIGHT" : L"MIDDLE"),
                positionSrc.x, positionSrc.y,
                positionDest.x, positionDest.y
                );

            LogThrow_IfFailed(IMInjectMouseDragInput((DWORD)button, positionSrc, positionDest));
        }

        void InputRoutineHelper::SendMouseMoveInput(POINT position)
        {
            EnsureMouseInjectionReady();

            LOG_OUTPUT(L"[RpcSendMouseMoveInput]: mouse position (%d, %d).",
                position.x, position.y);

            LogThrow_IfFailed(IMInjectMouseMoveInput(position));
        }

        void InputRoutineHelper::SendMouseWheelInput(POINT position, INT numberOfWheelClicks)
        {
            SendMouseMoveInput(position);

            LOG_OUTPUT(L"[RpcSendMouseWheelInput]: mouse position (%d, %d). numberOfWheelClicks=%d",
                position.x, position.y, numberOfWheelClicks);

            LogThrow_IfFailed(IMInjectMouseWheelInput(position, numberOfWheelClicks));
        }

    }
} } } }
