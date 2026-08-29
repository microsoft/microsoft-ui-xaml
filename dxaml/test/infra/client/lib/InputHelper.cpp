// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>

#include "InputHelper.h"
#include "IslandHelper.h"
#include "WindowHelper.h"
#include "IXamlTestHooks-win.h"
#include "Utilities.h"
#include "Hosting.h"

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private { namespace Infrastructure {

// The default panning & dragging velocity is 1 pixel per ms.
const double InputHelper::s_defaultDragVelocityPxPerMs = 1.0;

// The default zoom-in velocity is 0.005 unit per ms. Thus each increase of the zoom factor
// by one unit takes 200 ms. For example an increase from 1 to 3 takes (3 - 1) * 200 = 400 ms.
const double InputHelper::s_defaultZoomInVelocityUnitPerMs = 0.005;

// The default zoom-out velocity is 0.00125 unit per ms. Thus the decrease of the zoom factor
// from 1 to 0+ takes 800 ms. For example a decrease from 1 to 0.8 takes (1 - 0.8) * 800 = 160 ms.
const double InputHelper::s_defaultZoomOutVelocityUnitPerMs = 0.00125;

// The default contact geometry width/height when using DynamicPressCenter
const DWORD InputHelper::s_defaultDynamicPressWidthLogicalPx = 10;
const DWORD InputHelper::s_defaultDynamicPressHeightLogicalPx = 10;

// Information about the last input event
wil::srwlock InputHelper::s_inputLock;

bool InputHelper::s_suppressGestureStateReset = false;

void InputHelper::EnsureInputReady()
{
    RpcClientEnsureConnected();

    // We have a timing issue with input in that in order for input to be processed, not only do we need to complete a
    // full dcomp commit on the application side (Lifted IXP) but also on the OS side.  However, we don't have way to
    // wait for the commit to complete on the OS side.  So, to work around this, before we do our initial input
    // following a reset of the of the XAML Environment we will push through some dummy mouse moves until one
    // successfuly completes.

    if (WindowHelper::GetTestHooks()->CanFirePointerInputEvent())
    {
        wrl::ComPtr<test_infra::ITestServicesStatics> spTestServicesStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
            &spTestServicesStatics
            ));

        DWORD uiThreadId = 0;
        RunOnUIThread([&]()
        {
            // Because of the way that we create events for the idle/event synchronization, we need to create global handles and
            // so we need to "customize" the global name to include process and thread id.  We could plumb the UI thread id
            // through (we are always in the same process), but rather than do that we will just go to the UI thread to get it.
            uiThreadId = ::GetCurrentThreadId();
        });

        wrl::ComPtr<test_infra::IWindowHelper> spWindowHelper;
        LogThrow_IfFailed(spTestServicesStatics->get_WindowHelper(&spWindowHelper));

        wrl::ComPtr<xaml::IUIElement> spWindowContent;
        wrl::ComPtr<xaml::IFrameworkElement> spWindowContentFE;
        RunOnUIThread([&]()
        {
            // Get the current content.  We have some tests that will attempt to position the pointer before it has set
            // up any content.  When this happens the host window doesn't ever get the input, so there is no way for
            // Xaml to know whether it came through.  We will just let the injection occur since the app isn't expecting
            // Xaml to process it anyway and delay the verification of the roundtrip until we have content.
            LogThrow_IfFailed(spWindowHelper->get_WindowContent(&spWindowContent));
            if (spWindowContent)
            {
                LogThrow_IfFailed(spWindowContent.As(&spWindowContentFE));
            }
        });

        if (!spWindowContent) {
            LOG_OUTPUT(L"Skipping check for pointer input ready due to missing content");
            return;
        }

        IslandHelper::EnsureInputReady(uiThreadId, WindowHelper::GetTestHooks(), spWindowContentFE.Get(), this);
     }
}

POINT InputHelper::ComputeScreenCoordinates(_In_ xaml::IFrameworkElement* elementInIsland, wf::Point point)
{
    auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(point);
    return { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };
}

HRESULT InputHelper::LeftMouseClick(_In_ xaml::IFrameworkElement* pElement)
{
    COM_START_GROUP(L"InputHelper::LeftMouseClick")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);

        auto point = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT pt = { static_cast<LONG>(point.X), static_cast<LONG>(point.Y) };

        LogThrow_IfFailed(RpcInjectPress(InputDevice::Mouse, pt, s_defaultClickDurationMs, 1/*pressCount*/, 0 /* pressDelta*/));
    }
    COM_END
}

HRESULT InputHelper::LeftMouseClickOnPoint(wf::Point position)
{
    COM_START_GROUP(L"InputHelper::LeftMouseClickOnPoint")
    {
        EnsureInputReady();

        auto point = WindowHelper::ConvertToPhysicalDisplayLocationStatic(position);
        POINT pt = { static_cast<LONG>(point.X), static_cast<LONG>(point.Y) };

        LogThrow_IfFailed(RpcInjectPress(InputDevice::Mouse, pt, s_defaultClickDurationMs, 1/*pressCount*/, 0 /* pressDelta*/));
    }
    COM_END
}

HRESULT InputHelper::Tap(_In_ xaml::IFrameworkElement* pElement)
{
    // Tap in the center of the given element
    return TapAtPercent(pElement, 0.5f, 0.5f);
}

HRESULT InputHelper::TapAtPercent(_In_ xaml::IFrameworkElement* pElement, float fractionOfWidth, float fractionOfHeight)
{
    COM_START_GROUP(L"InputHelper::TapAtPercent")
    {
        wf::Point elementCenter = GetElementPosition(pElement, fractionOfWidth, fractionOfHeight);

        LogThrow_IfFailed(TapOnPoint(elementCenter));
    }
    COM_END
}

UINT32 Distance(wf::Point a, wf::Point b)
{
    return static_cast<UINT32>(ceilf(sqrtf(powf(a.X - b.X, 2.0f) + powf(a.Y - b.Y, 2.0f))));
}

HRESULT InputHelper::TapOnPoint(_In_ wf::Point point)
{
    COM_START_GROUP(L"InputHelper::TapOnPoint")
    {
        EnsureInputReady();

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(point);
        POINT ptPosition = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };

        auto inputLock = s_inputLock.lock_shared();

        // To avoid a tap being mistaken for a double-tap, we'll reset the state
        // of our gesture recognizer so it starts fresh with the following injection,
        // unless doing so has been explicitly suppressed (some tests' functionality
        // has taken an implicit dependency on this not happening).
        if (!s_suppressGestureStateReset)
        {
            RunOnUIThread([&]() {
                WindowHelper::GetTestHooks()->StopAllInteractions();
            });
        }

        LogThrow_IfFailed(RpcInjectPress(InputDevice::MultiTouch, ptPosition, s_defaultTapDurationMs, 1 /* pressCount */, 0 /* pressDelta */));
    }
    COM_END
}

HRESULT InputHelper::DoubleTap(_In_ xaml::IFrameworkElement* pElement)
{
    COM_START_GROUP(L"InputHelper::DoubleTap")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);
        LOG_OUTPUT(L"Input point logical:%f,%f", elementCenter.X, elementCenter.Y);

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT pt = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };
        LOG_OUTPUT(L"Input point physical:%ld,%ld", pt.x, pt.y);

        // SPI_GETTOUCHGESTURESETTINGS and TOUCH_GESTURE_SETTINGS are private-only constructs for the SystemParametersInfo API. Apparently there's no public way to access the current double-tap OS settings.
        // That's OK because s_defaultDoubleTapDeltaMs=10ms is real short and works no matter the OS setting.
        LogThrow_IfFailed(RpcInjectPress(InputDevice::MultiTouch, pt, s_defaultTapDurationMs, 2 /* pressCount */, s_defaultDoubleTapDeltaMs /* pressDelta */));
    }
    COM_END
}

HRESULT InputHelper::PenTap(_In_ xaml::IFrameworkElement* pElement)
{
    return HoldForSpecifiedDurationInternal(InputDevice::InternalPen, pElement, s_defaultTapDurationMs);
}

HRESULT InputHelper::PenDoubleTap(_In_ xaml::IFrameworkElement* pElement)
{
    return HoldForSpecifiedDurationInternal(InputDevice::InternalPen, pElement, s_defaultTapDurationMs, 2 /* pressCount*/);
}

HRESULT InputHelper::PenHold(_In_ xaml::IFrameworkElement* pElement)
{
    // SPI_GETTOUCHGESTURESETTINGS and TOUCH_GESTURE_SETTINGS are private-only constructs for the SystemParametersInfo API. Apparently there's no public way to access the current hold-related OS settings.
    // s_defaultHoldDurationMs=1500ms is therefore used - it is large enough that it works no matter the OS setting.
    return HoldForSpecifiedDurationInternal(InputDevice::InternalPen, pElement, s_defaultHoldDurationMs);
}

HRESULT InputHelper::Hold(_In_ xaml::IFrameworkElement* pElement)
{
    // SPI_GETTOUCHGESTURESETTINGS and TOUCH_GESTURE_SETTINGS are private-only constructs for the SystemParametersInfo API. Apparently there's no public way to access the current hold-related OS settings.
    // s_defaultHoldDurationMs=1500ms is therefore used - it is large enough that it works no matter the OS setting.
    return HoldForSpecifiedDurationInternal(InputDevice::MultiTouch, pElement, s_defaultHoldDurationMs);
}

HRESULT InputHelper::HoldOnPoint(_In_ wf::Point point)
{
    // SPI_GETTOUCHGESTURESETTINGS and TOUCH_GESTURE_SETTINGS are private-only constructs for the SystemParametersInfo API. Apparently there's no public way to access the current hold-related OS settings.
    // s_defaultHoldDurationMs=1500ms is therefore used - it is large enough that it works no matter the OS setting.
    COM_START_GROUP(L"InputHelper::HoldOnPoint")
    {
        EnsureInputReady();

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(point);
        POINT pt = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };

        LogThrow_IfFailed(RpcInjectPress(InputDevice::MultiTouch, pt, s_defaultHoldDurationMs, 1 /*pressCount*/, 0 /*pressDelta*/));
    }
    COM_END
}

HRESULT InputHelper::HoldForSpecifiedDuration(_In_ xaml::IFrameworkElement* pElement, UINT32 duration)
{
    return HoldForSpecifiedDurationInternal(InputDevice::MultiTouch, pElement, duration);
}

HRESULT InputHelper::HoldForSpecifiedDurationInternal(InputDevice inputDevice, _In_ xaml::IFrameworkElement* pElement, UINT32 duration, UINT32 pressCount)
{
    COM_START_GROUP(L"InputHelper::HoldForSpecifiedDurationInternal")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT pt = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };

        unsigned int pressDelta = (pressCount == 1) ? 0 : (pressCount*s_defaultDoubleTapDeltaMs);

        LogThrow_IfFailed(RpcInjectPress(inputDevice, pt, duration, pressCount, pressDelta));
    }
    COM_END
}

HRESULT InputHelper::PenBarrelTap(_In_ xaml::IFrameworkElement* pElement)
{
    COM_START_GROUP(L"InputHelper::PenBarrelTap")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT pt = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };

        LogThrow_IfFailed(RpcInjectPenBarrelTap(pt));
    }
    COM_END
}

HRESULT InputHelper::InjectDragWithPenBarrelDown(
    _In_ xaml::IFrameworkElement* pElement,
    _In_ INT32 relX,
    _In_ INT32 relY,
    _In_ DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::InjectDragWithPenBarrelDown")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT pt = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };

        Throw::IfFalse(velocityFactor > 0, E_FAIL, L"velocityFactor must be strictly positive.");

        if (HostingDispatcher::Get()->IsUIThread())
        {
            LOG_OUTPUT(L"Do not call InjectDragWithPenBarrelDown from UI thread!");
            LogThrow_IfFailed(E_UNEXPECTED);
        }

        // s_defaultDragVelocityPxPerMs represents the panning/dragging distance per ms, when velocityFactor is 1.0.
        UINT duration = static_cast<UINT>(std::sqrt(relX * relX + relY * relY) * s_defaultDragVelocityPxPerMs / velocityFactor);
        POINT ptStart = { static_cast<LONG>(elementCenter.X), static_cast<LONG>(elementCenter.Y) };
        POINT ptEnd = ptStart;

        auto pointStart = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        elementCenter.X += static_cast<float>(relX);
        elementCenter.Y += static_cast<float>(relY);
        auto pointEnd = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter, false /* validatePhysicalLocationIsInsideCurrentWindow */);

        ptStart = { static_cast<LONG>(pointStart.X), static_cast<LONG>(pointStart.Y) };
        ptEnd = { static_cast<LONG>(pointEnd.X), static_cast<LONG>(pointEnd.Y) };

        LogThrow_IfFailed(RpcInjectDragWithPenBarrelDown(ptStart, ptEnd, duration));
    }
    COM_END
}

HRESULT InputHelper::Flick(_In_ xaml::IFrameworkElement* pElement, test_infra::FlickDirection direction)
{
    COM_START_GROUP(L"InputHelper::Flick")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT ptStart = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };


        switch (direction)
        {
            case test_infra::FlickDirection_NorthWest:
                elementCenter.X -= s_defaultFlickDistancePx;
                elementCenter.Y -= s_defaultFlickDistancePx;
                break;

            case test_infra::FlickDirection_North:
                elementCenter.Y -= s_defaultFlickDistancePx;
                break;

            case test_infra::FlickDirection_NorthEast:
                elementCenter.X += s_defaultFlickDistancePx;
                elementCenter.Y -= s_defaultFlickDistancePx;
                break;

            case test_infra::FlickDirection_East:
                elementCenter.X += s_defaultFlickDistancePx;
                break;

            case test_infra::FlickDirection_SouthEast:
                elementCenter.X += s_defaultFlickDistancePx;
                elementCenter.Y += s_defaultFlickDistancePx;
                break;

            case test_infra::FlickDirection_South:
                elementCenter.Y += s_defaultFlickDistancePx;
                break;

            case test_infra::FlickDirection_SouthWest:
                elementCenter.X -= s_defaultFlickDistancePx;
                elementCenter.Y += s_defaultFlickDistancePx;
                break;

            case test_infra::FlickDirection_West:
                elementCenter.X -= s_defaultFlickDistancePx;
                break;
        }

        pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter, false /* validatePhysicalLocationIsInsideCurrentWindow */);
        POINT ptEnd = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };
        LogThrow_IfFailed(RpcInjectPressAndDrag(InputDevice::MultiTouch, ptStart, ptEnd, s_defaultFlickDurationMs, 0));
    }
    COM_END
}

//------------------------------------------------------------------------
// Press, hold and pan using a single contact point which starts holding at the center of the
// provided element for holdTime. That contact point then moves by (relX, relY) pixels,
// using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::PressHoldAndPanFromCenter(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor, UINT holdTime)
{
    COM_START_GROUP(L"InputHelper::PressHoldAndPanFromCenter")
    {
        PressHoldAndDrag(InputDevice::MultiTouch, pElement, relX, relY, velocityFactor, holdTime, TRUE /*fromCenter*/);
    }
    COM_END
}

//------------------------------------------------------------------------
// Pan using a single contact point which starts at the center of the
// provided element. That contact point then moves by (relX, relY) pixels,
// using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::PanFromCenter(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::PanFromCenter")
    {
        PressAndDrag(InputDevice::MultiTouch, pElement, relX, relY, velocityFactor, TRUE /*fromCenter*/);
    }
    COM_END
}

//------------------------------------------------------------------------
// Drag using the left mouse button, pressing at the center of the
// provided element. The mouse cursor then moves by (relX, relY) pixels,
// using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::DragFromCenter(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::DragFromCenter")
    {
        PressAndDrag(InputDevice::Mouse, pElement, relX, relY, velocityFactor, TRUE /*fromCenter*/);
    }
    COM_END
}

//------------------------------------------------------------------------
// Drag using the left mouse button, pressing at (relX, relY) offset of the
// center of provided element. The mouse cursor then moves into
// center of element using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::DragToCenter(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::DragToCenter")
    {
        PressAndDrag(InputDevice::Mouse, pElement, relX, relY, velocityFactor, FALSE /*fromCenter*/);
    }
    COM_END
}

//------------------------------------------------------------------------
// Drag using the left mouse button, pressing at the center of the source element
// and dragging to the center of the target element. The mouse cursor moves to the
// center of the target element using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::DragBetweenElements(_In_ xaml::IFrameworkElement* pSourceElement, _In_ xaml::IFrameworkElement* pTargetElement, DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::DragBetweenElements")
    {
        PressAndDrag(InputDevice::Mouse, pSourceElement, pTargetElement, velocityFactor, TRUE /*fromCenter*/);
    }
    COM_END
}

//------------------------------------------------------------------------
// Pen stroke from the center of the provided element. The stroke then moves by (relX, relY) pixels,
// using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::PenStrokeFromCenter(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::PenStrokeFromCenter")
    {
        PressAndDrag(InputDevice::InternalPen, pElement, relX, relY, velocityFactor, TRUE /*fromCenter*/);
    }
    COM_END
}


// Pen strokes to draw the letter T
//------------------------------------------------------------------------
HRESULT InputHelper::PenWriteLetterT(_In_ xaml::IFrameworkElement* pElement)
{
    COM_START_GROUP(L"InputHelper::PenWriteLetterT")
    {
        wf::Point elementCenter = GetCenter(pElement);
        POINT ptTIntersection = { static_cast<LONG>(elementCenter.X + 1), static_cast<LONG>(elementCenter.Y + 3) };
        POINT ptTopbarLeft = { ptTIntersection.x - 5, ptTIntersection.y };
        POINT ptTopbarRight = { ptTIntersection.x + 5, ptTIntersection.y };
        POINT ptPenVerticalBottom = { ptTIntersection.x, ptTIntersection.y + 8 };
        POINT ptPenVerticalTop = { ptTIntersection.x, ptTIntersection.y + 1 };
        DrawPenStroke(ptTopbarLeft, ptTopbarRight); // Drawing the top line of the T
        DrawPenStroke(ptTIntersection, ptPenVerticalBottom); //Drawing the vertical line of the T
    }
    COM_END
}

// Pen strokes to draw a Dash
//------------------------------------------------------------------------
HRESULT InputHelper::PenWriteDash(_In_ xaml::IFrameworkElement* pElement)
{
    COM_START_GROUP(L"InputHelper::PenWriteDash")
    {
        PressAndDrag(InputDevice::InternalPen, pElement, 10, 0, 1.0, TRUE /*fromCenter*/);
    }
    COM_END
}

//------------------------------------------------------------------------
// Zoom in using the center of the provided element as the zoom center.
// Starting contact point positions for orientation==Horizontal:
//     ================================================
//     =                                              =
//     =                                              =
//     =                                              =
//     =      *                                *      =
//     =                                              =
//     =                                              =
//     =                                              =
//     ================================================
// Final contact point positions:
//     ================================================
//     =                                              =
//     =                                              =
//     =                                              =
//     *                                              *
//     =                                              =
//     =                                              =
//     =                                              =
//     ================================================
// The contact points move towards the vertical edges, starting from the
// same distance, using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::ZoomInToEdges(_In_ xaml::IFrameworkElement* pElement, UINT equidistanceFromEdges, xaml::Controls::Orientation orientation, DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::ZoomInToEdges")
    {
        POINT ptStartFinger1 = {};
        POINT ptStartFinger2 = {};
        FLOAT dbDirection = 0;
        UINT physicalEquidistanceFromEdges = 0;
        UINT duration = 0;

        GetZoomInfo(
            pElement,
            equidistanceFromEdges,
            orientation,
            velocityFactor,
            true /*zoomIn*/,
            &physicalEquidistanceFromEdges,
            &duration,
            &dbDirection,
            &ptStartFinger1,
            &ptStartFinger2);

        EnsureInputReady();
        LogThrow_IfFailed(RpcInjectZoom(ptStartFinger1, ptStartFinger2, dbDirection, duration, physicalEquidistanceFromEdges));
    }
    COM_END
}

//------------------------------------------------------------------------
// Zoom out using the center of the provided element as the zoom center.
// Starting contact point positions for orientation==Horizontal:
//     ================================================
//     =                                              =
//     =                                              =
//     =                                              =
//     =*                                            *=
//     =                                              =
//     =                                              =
//     =                                              =
//     ================================================
// Final contact point positions:
//     ================================================
//     =                                              =
//     =                                              =
//     =                                              =
//     =      *                                *      =
//     =                                              =
//     =                                              =
//     =                                              =
//     ================================================
// The contact points move away from the vertical edges, by the same
// amount, using a velocity proportional to velocityFactor.
//------------------------------------------------------------------------
HRESULT InputHelper::ZoomOutFromEdges(_In_ xaml::IFrameworkElement* pElement, UINT equidistanceFromEdges, xaml::Controls::Orientation orientation, DOUBLE velocityFactor)
{
    COM_START_GROUP(L"InputHelper::ZoomOutFromEdges")
    {
        POINT ptStartFinger1 = {};
        POINT ptStartFinger2 = {};
        FLOAT dbDirection = 0;
        UINT physicalEquidistanceFromEdges = 0;
        UINT duration = 0;

        GetZoomInfo(
            pElement,
            equidistanceFromEdges,
            orientation,
            velocityFactor,
            false /*zoomIn*/,
            &physicalEquidistanceFromEdges,
            &duration,
            &dbDirection,
            &ptStartFinger1,
            &ptStartFinger2);

        EnsureInputReady();
        LogThrow_IfFailed(RpcInjectZoom(ptStartFinger1, ptStartFinger2, dbDirection, duration, physicalEquidistanceFromEdges));
    }
    COM_END
}

HRESULT InputHelper::InjectRotate(
    _In_ xaml::IFrameworkElement* element,
    _In_ wf::Point startFinger1,                    // Starting position (relative to element) of the first finger
    _In_ wf::Point startFinger2,                    // Starting position (relative to element) of the second finger
    DOUBLE rotationAngle,                           // Rotation in degrees, counter-clockwise
    UINT32 durationMs,                              // Duration of the gesture in MS
    BOOLEAN pivotRotate)                            // Indicates whether the first finger acts as a pivot or not.
{
    COM_START_GROUP(L"InputHelper::InjectRotate")
    {
        EnsureInputReady();

        POINT ptStartFinger1 = {};
        POINT ptStartFinger2 = {};

        RunOnUIThread([&]()
        {
            wrl::ComPtr<xaml::IUIElement> spElementAsUIE;
            LogThrow_IfFailed((wrl::ComPtr<xaml::IFrameworkElement>(element).As(&spElementAsUIE)));
            wrl::ComPtr<xaml_media::IGeneralTransform> spTransform;
            LogThrow_IfFailed(spElementAsUIE->TransformToVisual(nullptr, &spTransform));
            LogThrow_IfFailed(spTransform->TransformPoint(startFinger1, &startFinger1));
            LogThrow_IfFailed(spTransform->TransformPoint(startFinger2, &startFinger2));

            // Now transform the two points to physical coordinates
            startFinger1 = WindowHelper::ConvertToPhysicalDisplayLocationStatic(startFinger1);
            startFinger2 = WindowHelper::ConvertToPhysicalDisplayLocationStatic(startFinger2);
        });

        const POINT finger1Point { static_cast<int>(startFinger1.X), static_cast<int>(startFinger1.Y) };
        const POINT finger2Point { static_cast<int>(startFinger2.X), static_cast<int>(startFinger2.Y) };

        LogThrow_IfFailed(RpcInjectRotate(finger1Point, finger2Point, static_cast<float>(rotationAngle), durationMs, pivotRotate));
    }
    COM_END
}

HRESULT InputHelper::DynamicPressCenter(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy, test_infra::PointerFinger finger)
{
    COM_START_GROUP(L"InputHelper::DynamicPressCenter")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);
        elementCenter.X += static_cast<float>(dx);
        elementCenter.Y += static_cast<float>(dy);
        auto positionDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT ptPress = { static_cast<int>(positionDisplayCoords.X), static_cast<int>(positionDisplayCoords.Y) };

        LogThrow_IfFailed(RpcInjectDynamicPress(InputDevice::MultiTouch, ptPress, s_defaultDynamicPressWidthLogicalPx, s_defaultDynamicPressHeightLogicalPx, static_cast<UINT>(finger)));
    }
    COM_END
}

HRESULT InputHelper::DynamicRelease(test_infra::PointerFinger finger)
{
    COM_START_GROUP(L"InputHelper::DynamicRelease")
    {
        EnsureInputReady();
        LogThrow_IfFailed(RpcInjectDynamicRelease(static_cast<UINT>(finger)));
    }
    COM_END
}

HRESULT InputHelper::DynamicPenPressCenter(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy)
{
    COM_START_GROUP(L"InputHelper::DynamicPenPressCenter")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);
        elementCenter.X += static_cast<float>(dx);
        elementCenter.Y += static_cast<float>(dy);
        auto positionDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT ptPress = { static_cast<int>(positionDisplayCoords.X), static_cast<int>(positionDisplayCoords.Y) };

        LogThrow_IfFailed(RpcInjectDynamicPress(InputDevice::InternalPen, ptPress, s_defaultDynamicPressWidthLogicalPx, s_defaultDynamicPressHeightLogicalPx, s_penFinger));
    }
    COM_END
}

HRESULT InputHelper::DynamicPenRelease()
{
    COM_START_GROUP(L"InputHelper::DynamicPenRelease")
    {
        EnsureInputReady();
        LogThrow_IfFailed(RpcInjectDynamicRelease(s_penFinger));
    }
    COM_END
}

HRESULT InputHelper::Swipe(wf::Point start, wf::Point end)
{
    COM_START_GROUP(L"InputHelper::Swipe")
    {
        EnsureInputReady();
        auto startDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(start);
        auto endDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(end, false /* validatePhysicalLocationIsInsideCurrentWindow */);

        POINT ptStart = { static_cast<int>(startDisplayCoords.X), static_cast<int>(startDisplayCoords.Y) };
        POINT ptEnd = { static_cast<int>(endDisplayCoords.X), static_cast<int>(endDisplayCoords.Y) };

        LogThrow_IfFailed(RpcInjectPressAndDrag(InputDevice::MultiTouch, ptStart, ptEnd, s_defaultSwipeDurationMs, 0));
    }
    COM_END
}

HRESULT InputHelper::MouseButtonDown(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy, test_infra::MouseButton button)
{
    COM_START_GROUP(L"InputHelper::MouseButtonDown")
    {
        wf::Point elementCenter = GetCenter(pElement);
        elementCenter.X += dx;
        elementCenter.Y += dy;

        LogThrow_IfFailed(MouseButtonDownOnPoint(elementCenter, button));
    }
    COM_END
}

HRESULT InputHelper::MouseButtonDownOnPoint(wf::Point point, test_infra::MouseButton button)
{
    COM_START_GROUP(L"InputHelper::MouseButtonDownOnPoint")
    {
        EnsureInputReady();
        auto positionDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(point);
        POINT ptPosition = { static_cast<int>(positionDisplayCoords.X), static_cast<int>(positionDisplayCoords.Y) };

        LogThrow_IfFailed(RpcSendMouseButtonInput(static_cast<::MouseButton>(button), ptPosition, TRUE /*down*/));
    }
    COM_END
}

HRESULT InputHelper::MouseButtonUp(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy, test_infra::MouseButton button)
{
    COM_START_GROUP(L"InputHelper::MouseButtonUp")
    {
        EnsureInputReady();
        wf::Point elementCenter = GetCenter(pElement);
        elementCenter.X += dx;
        elementCenter.Y += dy;

        LogThrow_IfFailed(MouseButtonUpOnPoint(elementCenter, button));
    }
    COM_END
}

HRESULT InputHelper::MouseButtonUpOnPoint(wf::Point point, test_infra::MouseButton button)
{
    COM_START_GROUP(L"InputHelper::MouseButtonUpOnPoint")
    {
        EnsureInputReady();
        auto positionDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(point);
        POINT ptPosition = { static_cast<int>(positionDisplayCoords.X), static_cast<int>(positionDisplayCoords.Y) };

        LogThrow_IfFailed(RpcSendMouseButtonInput(static_cast<::MouseButton>(button), ptPosition, FALSE /*down*/));
    }
    COM_END
}

HRESULT InputHelper::MouseDrag(_In_ xaml::IFrameworkElement* pElementSrc, INT dxSrc, INT dySrc,
                               _In_ xaml::IFrameworkElement* pElementDest, INT dxDest, INT dyDest,
                               test_infra::MouseButton button)
{
    COM_START_GROUP(L"InputHelper::MouseDrag")
    {
        EnsureInputReady();
        wf::Point elementCenterSrc = GetCenter(pElementSrc);
        elementCenterSrc.X += dxSrc;
        elementCenterSrc.Y += dySrc;

        wf::Point elementCenterDest = GetCenter(pElementDest);
        elementCenterDest.X += dxDest;
        elementCenterDest.Y += dyDest;

        LogThrow_IfFailed(MouseDragOnPoints(elementCenterSrc, elementCenterDest, button));
    }
    COM_END
}

HRESULT InputHelper::MouseDragOnPoints(wf::Point pointSrc, wf::Point pointDest, test_infra::MouseButton button)
{
    COM_START_GROUP(L"InputHelper::MouseDragOnPoints")
    {
        EnsureInputReady();
        auto pointStart = WindowHelper::ConvertToPhysicalDisplayLocationStatic(pointSrc);
        auto pointEnd = WindowHelper::ConvertToPhysicalDisplayLocationStatic(pointDest);
        POINT ptPositionSrc = { static_cast<LONG>(pointStart.X), static_cast<LONG>(pointStart.Y) };
        POINT ptPositionDest = { static_cast<LONG>(pointEnd.X), static_cast<LONG>(pointEnd.Y) };

        LogThrow_IfFailed(RpcSendMouseDragInput(static_cast<::MouseButton>(button), ptPositionSrc, ptPositionDest));
    }
    COM_END
}
HRESULT InputHelper::ClickMouseButton(test_infra::MouseButton button, _In_ xaml::IFrameworkElement* pElement)
{
    COM_START_GROUP(L"InputHelper::ClickMouseButton")
    {
        wf::Point elementCenter = GetCenter(pElement);

        LogThrow_IfFailed(ClickMouseButtonOnPoint(button, elementCenter));
    }
    COM_END
}

HRESULT InputHelper::ClickMouseButtonWithOffset(test_infra::MouseButton button, _In_ xaml::IFrameworkElement* pElement, INT dx, INT dy)
{
    COM_START_GROUP(L"InputHelper::ClickMouseButtonWithOffset")
    {
        wf::Point elementCenter = GetCenter(pElement);
        elementCenter.X += dx;
        elementCenter.Y += dy;

        LogThrow_IfFailed(ClickMouseButtonOnPoint(button, elementCenter));
    }
    COM_END
}

HRESULT InputHelper::ClickMouseButtonOnPoint(test_infra::MouseButton button, wf::Point position)
{
    COM_START_GROUP(L"InputHelper::ClickMouseButtonOnPoint")
    {
        EnsureInputReady();
        auto positionDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(position);
        POINT ptPosition = { static_cast<int>(positionDisplayCoords.X), static_cast<int>(positionDisplayCoords.Y) };

        LogThrow_IfFailed(RpcSendMouseButtonInput(static_cast<::MouseButton>(button), ptPosition, TRUE /*down*/));
        LogThrow_IfFailed(RpcSendMouseButtonInput(static_cast<::MouseButton>(button), ptPosition, FALSE /*down*/));
    }
    COM_END
}

HRESULT InputHelper::MoveMouse(_In_ xaml::IFrameworkElement* pElement)
{
    COM_START_GROUP(L"InputHelper::MoveMouse")
    {
        LogThrow_IfFailed(MoveMouseFromCenter(pElement, 0, 0));
    }
    COM_END
}

HRESULT InputHelper::MoveMouseFromCenter(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy)
{
    COM_START_GROUP(L"InputHelper::MoveMouseFromCenter")
    {
        wf::Point elementCenter = GetCenter(pElement);
        elementCenter.X += dx;
        elementCenter.Y += dy;

        LogThrow_IfFailed(MoveMouseOnPoint(elementCenter));
    }
    COM_END
}

HRESULT InputHelper::MoveMouseOnPoint(_In_ wf::Point point)
{
    COM_START_GROUP(L"InputHelper::MoveMouseOnPoint")
    {
        EnsureInputReady();

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(point);
        POINT ptPosition = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };

        LogThrow_IfFailed(RpcSendMouseMoveInput(ptPosition));
    }
    COM_END
}

// Positive integers mean scroll up
HRESULT InputHelper::ScrollMouseWheel(_In_ xaml::IFrameworkElement* pElement, INT numberOfWheelClicks)
{
    COM_START_GROUP(L"InputHelper::ScrollMouseWheel")
    {
        wf::Point elementCenter = GetCenter(pElement);

        EnsureInputReady();

        auto pointDisplayCoords = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
        POINT ptPosition = { static_cast<LONG>(pointDisplayCoords.X), static_cast<LONG>(pointDisplayCoords.Y) };

        LogThrow_IfFailed(RpcSendMouseWheelInput(ptPosition, numberOfWheelClicks));
    }
    COM_END
}

void InputHelper::PressHoldAndDrag(
    InputDevice inputDevice,
    _In_ xaml::IFrameworkElement* pElement,
    INT32 relX,
    INT32 relY,
    DOUBLE velocityFactor,
    UINT holdTimeBeforeDrag,
    BOOL fromCenter)
{
    Throw::IfFalse(velocityFactor > 0, E_FAIL, L"velocityFactor must be strictly positive.");

    if (HostingDispatcher::Get()->IsUIThread())
    {
        LOG_OUTPUT(L"Do not call PressHoldAndDrag from UI thread!");
        LogThrow_IfFailed(E_UNEXPECTED);
    }

    wf::Point elementCenter = GetCenter(pElement);

    EnsureInputReady();

    // s_defaultDragVelocityPxPerMs represents the panning/dragging distance per ms, when velocityFactor is 1.0.
    UINT duration = static_cast<UINT>(std::sqrt(relX * relX + relY * relY) * s_defaultDragVelocityPxPerMs / velocityFactor);
    POINT ptStart = { static_cast<LONG>(elementCenter.X), static_cast<LONG>(elementCenter.Y) };
    POINT ptEnd = ptStart;

    auto pointStart = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter);
    elementCenter.X += static_cast<float>(relX);
    elementCenter.Y += static_cast<float>(relY);
    auto pointEnd = WindowHelper::ConvertToPhysicalDisplayLocationStatic(elementCenter, false /* validatePhysicalLocationIsInsideCurrentWindow */);

    ptStart = { static_cast<LONG>(pointStart.X), static_cast<LONG>(pointStart.Y) };
    ptEnd = { static_cast<LONG>(pointEnd.X), static_cast<LONG>(pointEnd.Y) };

    if (!fromCenter) // swap start and end points if drag to center
    {
        POINT ptTemp = ptStart;
        ptStart = ptEnd;
        ptEnd = ptTemp;
    }

    LogThrow_IfFailed(RpcInjectPressAndDrag(inputDevice, ptStart, ptEnd, duration, holdTimeBeforeDrag));
}

void InputHelper::PressHoldAndDrag(
    InputDevice inputDevice,
    _In_ xaml::IFrameworkElement* pSourceElement,
    _In_ xaml::IFrameworkElement* pTargetElement,
    DOUBLE velocityFactor,
    UINT holdTimeBeforeDrag,
    BOOL fromCenter)
{
    wf::Point sourceElementCenter = GetCenter(pSourceElement);
    wf::Point targetElementCenter = GetCenter(pTargetElement);

    PressHoldAndDrag(
        inputDevice,
        pSourceElement,
        static_cast<INT32>(targetElementCenter.X - sourceElementCenter.X),
        static_cast<INT32>(targetElementCenter.Y - sourceElementCenter.Y),
        velocityFactor,
        holdTimeBeforeDrag,
        fromCenter);
}

void InputHelper::PressAndDrag(
    InputDevice inputDevice,
    _In_ xaml::IFrameworkElement* pElement,
    INT32 relX,
    INT32 relY,
    DOUBLE velocityFactor,
    BOOL fromCenter)
{
    PressHoldAndDrag(inputDevice, pElement, relX, relY, velocityFactor, s_defaultPanDurationInHoldMs, fromCenter);
}

void InputHelper::PressAndDrag(
    InputDevice inputDevice,
    _In_ xaml::IFrameworkElement* pSourceElement,
    _In_ xaml::IFrameworkElement* pTargetElement,
    DOUBLE velocityFactor,
    BOOL fromCenter)
{
    PressHoldAndDrag(inputDevice, pSourceElement, pTargetElement, velocityFactor, s_defaultPanDurationInHoldMs, fromCenter);
}

void InputHelper::GetZoomInfo(
    _In_ xaml::IFrameworkElement* pElement,
    UINT equidistanceFromEdges,
    xaml::Controls::Orientation orientation,
    DOUBLE velocityFactor,
    BOOL zoomIn,
    _Out_ UINT* pPhysicalEquidistanceFromEdges,
    _Out_ UINT* pDuration,
    _Out_ FLOAT* pDirection,
    _Out_ POINT* pStartFinger1,
    _Out_ POINT* pStartFinger2)
{
    RunOnUIThread([&]()
    {
        Throw::IfFalse(velocityFactor > 0, E_FAIL, L"velocityFactor must be strictly positive.");

        wf::Point pointStartFinger1, pointStartFinger2;

        double width = 0.0;
        double height = 0.0;
        LogThrow_IfFailed(pElement->get_ActualWidth(&width));
        LogThrow_IfFailed(pElement->get_ActualHeight(&height));

        Throw::IfFalse(width != 0.0, E_FAIL, L"Element has no width.");
        Throw::IfFalse(height != 0.0, E_FAIL, L"Element has no height.");

        if (orientation == xaml::Controls::Orientation_Horizontal)
        {
            Throw::IfFalse(2 * equidistanceFromEdges < static_cast<UINT>(width - 1.0), E_FAIL, L"equidistanceFromEdges is too large compared to element's width.");

            pointStartFinger1.Y = static_cast<float>(height / 2.0);
            pointStartFinger2.Y = static_cast<float>(height / 2.0);
            if (zoomIn)
            {
                pointStartFinger1.X = static_cast<float>(equidistanceFromEdges);
                pointStartFinger2.X = static_cast<float>(width - equidistanceFromEdges);

                *pDuration = static_cast<UINT>((width / (width - 2 * equidistanceFromEdges) - 1.0) / s_defaultZoomInVelocityUnitPerMs / velocityFactor);
                *pDirection = 0.0f;
            }
            else
            {
                pointStartFinger1.X = 1.0f;
                pointStartFinger2.X = static_cast<float>(width) -2.0f;

                *pDuration = static_cast<UINT>((1.0 - (width - 2 * equidistanceFromEdges) / width) / s_defaultZoomOutVelocityUnitPerMs / velocityFactor);
                *pDirection = 180.0f;
            }
        }
        else
        {
            Throw::IfFalse(2 * equidistanceFromEdges < static_cast<UINT>(height - 1.0), E_FAIL, L"equidistanceFromEdges is too large compared to element's height.");

            pointStartFinger1.X = static_cast<float>(width / 2.0);
            pointStartFinger2.X = static_cast<float>(width / 2.0);
            if (zoomIn)
            {
                pointStartFinger1.Y = static_cast<float>(equidistanceFromEdges);
                pointStartFinger2.Y = static_cast<float>(height - equidistanceFromEdges);

                *pDuration = static_cast<UINT>((height / (height - 2 * equidistanceFromEdges) - 1.0) / s_defaultZoomInVelocityUnitPerMs / velocityFactor);
                *pDirection = 270.0f;
            }
            else
            {
                pointStartFinger1.Y = 1.0f;
                pointStartFinger2.Y = static_cast<float>(height) -2.0f;

                *pDuration = static_cast<UINT>((1.0 - (height - 2 * equidistanceFromEdges) / height) / s_defaultZoomOutVelocityUnitPerMs / velocityFactor);
                *pDirection = 90.0f;
            }
        }

        // First transform the two points to window logical coordinates
        wrl::ComPtr<xaml::IUIElement> spElementAsUIE;
        LogThrow_IfFailed((wrl::ComPtr<xaml::IFrameworkElement>(pElement).As(&spElementAsUIE)));
        wrl::ComPtr<xaml_media::IGeneralTransform> spTransform;
        LogThrow_IfFailed(spElementAsUIE->TransformToVisual(nullptr, &spTransform));
        LogThrow_IfFailed(spTransform->TransformPoint(pointStartFinger1, &pointStartFinger1));
        LogThrow_IfFailed(spTransform->TransformPoint(pointStartFinger2, &pointStartFinger2));

        // Now transform the two points to physical coordinates
        pointStartFinger1 = WindowHelper::ConvertToPhysicalDisplayLocationStatic(pointStartFinger1);
        pointStartFinger2 = WindowHelper::ConvertToPhysicalDisplayLocationStatic(pointStartFinger2);

        // Now interpret the orientation and adjust the direction for it
        auto adjustment = GetDisplayOrientationAdjustment();
        *pDirection -= adjustment;

        if (*pDirection > 360)
        {
            *pDirection -= 360;
        }
        else if (*pDirection < 0)
        {
            *pDirection += 360;
        }

        pStartFinger1->x = static_cast<LONG>(pointStartFinger1.X);
        pStartFinger1->y = static_cast<LONG>(pointStartFinger1.Y);
        pStartFinger2->x = static_cast<LONG>(pointStartFinger2.X);
        pStartFinger2->y = static_cast<LONG>(pointStartFinger2.Y);

        *pPhysicalEquidistanceFromEdges = static_cast<UINT>(WindowHelper::GetPhysicalPixelsPerLogicalPixelStatic() * equidistanceFromEdges);
    });
}

UINT InputHelper::GetDisplayOrientationAdjustment()
{
    UINT adjustment = 0;
    Hosting::HostingMode hostingMode = Hosting::HostingMode::UAP;
    LogThrow_IfFailed(GetHostingMode(&hostingMode));
    if(hostingMode == Hosting::HostingMode::UAP)
    {
        wgrd::DisplayOrientations nativeOrientation = wgrd::DisplayOrientations_None;
        wgrd::DisplayOrientations orientation = wgrd::DisplayOrientations_None;

        auto spDisplayInformation = GetUWPDisplayInformation();

        LogThrow_IfFailed(spDisplayInformation->get_NativeOrientation(&nativeOrientation));
        LogThrow_IfFailed(spDisplayInformation->get_CurrentOrientation(&orientation));

        if (nativeOrientation == wgrd::DisplayOrientations::DisplayOrientations_Portrait)
        {
            switch (orientation)
            {
                case wgrd::DisplayOrientations_Landscape:
                    adjustment = 270;
                    break;
                case wgrd::DisplayOrientations_LandscapeFlipped:
                    adjustment = 90;
                    break;
                case wgrd::DisplayOrientations_PortraitFlipped:
                    adjustment = 180;
                    break;
            }
        }
        else if (nativeOrientation == wgrd::DisplayOrientations::DisplayOrientations_Landscape)
        {
            switch (orientation)
            {
                case wgrd::DisplayOrientations_Portrait:
                    adjustment = 90;
                    break;
                case wgrd::DisplayOrientations_LandscapeFlipped:
                    adjustment = 180;
                    break;
                case wgrd::DisplayOrientations_PortraitFlipped:
                    adjustment = 270;
                    break;
            }
        }
        else
        {
            LOG_OUTPUT(L"Native orientation %d is unexpected.  Cratering Jupiter.", nativeOrientation);
            CraterJupiterErrorCode error = {};
            error.Error.Flags.InvalidOrientation = true;
            Utilities::CraterJupiter(error);
        }
    }
    else
    {
        auto rotationValue = GetWin32DisplayOrientation();
        // Map to XamlDisplayOrientation type
        switch(rotationValue)
        {
            case DMDO_DEFAULT:
                adjustment = 0;
                break;
            case DMDO_180:
                adjustment = 180;
                break;
            case DMDO_90:
                adjustment = 90;
                break;
            case DMDO_270:
                adjustment = 270;
                break;
            default:
                break;
        }
    }
    return adjustment;
}


UINT InputHelper::GetWin32DisplayOrientation()
{
    UINT rotationValue = DMDO_DEFAULT;
    HWND backingHwnd = NULL;
    RunOnUIThread([&]()
    {
        test_infra::Hosting::IWin32Host* win32Host = nullptr;

        wrl::ComPtr<test_infra::ITestServicesStatics> testServicesStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
            &testServicesStatics
        ));

        LogThrow_IfFailed(testServicesStatics->get_Win32Host(&win32Host));

        uint64_t handle = 0;
        FAIL_FAST_IF_FAILED(win32Host->get_MainWindowHandle(&handle));
        backingHwnd = reinterpret_cast<HWND>(handle);
    });

    if(backingHwnd != NULL)
    {
        HMONITOR myMonitor = MonitorFromWindow(backingHwnd, MONITOR_DEFAULTTONEAREST);
        if(myMonitor != NULL)
        {
            MONITORINFOEX MonitorInfo = {};
            MonitorInfo.cbSize = sizeof(MonitorInfo);
            GetMonitorInfo(myMonitor, &MonitorInfo);

            DEVMODEW DevMode = {};
            DevMode.dmSize = sizeof(DevMode);
            DevMode.dmSpecVersion = DM_SPECVERSION;

            EnumDisplaySettings(MonitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &DevMode);

            if (DevMode.dmFields & DM_DISPLAYORIENTATION)
            {
                rotationValue = DevMode.dmDisplayOrientation;
            }
        }
    }
    return rotationValue;
}

wrl::ComPtr<wgrd::IDisplayInformation> InputHelper::GetUWPDisplayInformation()
{
    wrl::ComPtr<wgrd::IDisplayInformation> spDisplayInformation;
    wrl::ComPtr<wgrd::IDisplayInformationStatics> spDisplayInformationStatics;

    LogThrow_IfFailed(wf::GetActivationFactory(wrl::Wrappers::HStringReference(
            RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
            &spDisplayInformationStatics));
    LogThrow_IfFailed(spDisplayInformationStatics->GetForCurrentView(&spDisplayInformation));
    return spDisplayInformation;
}

wf::Point InputHelper::GetCenter(_In_ xaml::IFrameworkElement* pElement)
{
    return GetElementPosition(pElement, 0.5f, 0.5f);
}

wf::Point InputHelper::GetElementPosition(_In_ xaml::IFrameworkElement* pElement, float fractionOfWidth, float fractionOfHeight)
{
    wf::Point point;

    RunOnUIThread([&]() {
        wrl::ComPtr<xaml::IUIElement> spElementAsUIE;
        LogThrow_IfFailed((wrl::ComPtr<xaml::IFrameworkElement>(pElement).As(&spElementAsUIE)));

        double height = 0;
        double width = 0;
        LogThrow_IfFailed(pElement->get_ActualHeight(&height));
        LogThrow_IfFailed(pElement->get_ActualWidth(&width));

        Throw::IfFalse(height != 0.0, E_FAIL, L"Element has no height.");
        Throw::IfFalse(width != 0.0, E_FAIL, L"Element has no width.");

        // Start with the point at the specified fraction into the element, and then
        // transform that to global coordinates.
        point.X = static_cast<float>(width * fractionOfWidth);
        point.Y = static_cast<float>(height * fractionOfHeight);

        wrl::ComPtr<xaml_media::IGeneralTransform> spTransform;
        LogThrow_IfFailed(spElementAsUIE->TransformToVisual(nullptr, &spTransform));
        LogThrow_IfFailed(spTransform->TransformPoint(point, &point));
    });

    return point;
}

void InputHelper::DrawPenStroke(POINT ptStart, POINT ptEnd)
{
    if (HostingDispatcher::Get()->IsUIThread())
    {
        LOG_OUTPUT(L"Do not call DrawPenStroke from UI thread!");
        LogThrow_IfFailed(E_UNEXPECTED);
    }
    EnsureInputReady();

    LONG dXSquared = (ptEnd.y - ptStart.y) * (ptEnd.y - ptStart.y);
    LONG dYSqured = (ptEnd.y - ptStart.y) * (ptEnd.y - ptStart.y);

    // s_defaultDragVelocityPxPerMs represents the panning/dragging distance per ms, when velocityFactor is 1.0.
    // Here we inject with velocity factor 1.0
    UINT duration = static_cast<UINT>(std::sqrt(dXSquared + dYSqured) * s_defaultDragVelocityPxPerMs);
    LogThrow_IfFailed(RpcInjectPressAndDrag(InputDevice::InternalPen, ptStart, ptEnd, duration, s_defaultPanDurationInHoldMs/2));
}

HRESULT InputHelper::SuppressGestureStateReset(_Out_ wf::IClosable** suppressor)
{
    COM_START_GROUP(L"InputHelper::SuppressGestureStateReset")
    {
        auto suppressorLocal = Microsoft::WRL::Make<GestureStateResetSuppressor>();
        Microsoft::WRL::ComPtr<wf::IClosable> closable;
        LogThrow_IfFailed(suppressorLocal.As(&closable));
        LogThrow_IfFailed(closable.CopyTo(suppressor));
    }
    COM_END
}

HRESULT InputHelper::GestureStateResetSuppressor::Init()
{
    InputHelper::s_suppressGestureStateReset = true;
    return S_OK;
}

HRESULT InputHelper::GestureStateResetSuppressor::Close()
{
    InputHelper::s_suppressGestureStateReset = false;
    return S_OK;
}

} }
