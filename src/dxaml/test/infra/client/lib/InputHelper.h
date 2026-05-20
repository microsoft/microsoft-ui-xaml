// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RpcClient.h"
#include "ICoordinateTransformer.h"

namespace Private { namespace Infrastructure {

    class InputHelper
        : public Microsoft::WRL::RuntimeClass<test_infra::IInputHelper>
        , public ICoordinateTransformer
    {
        friend class GestureStateResetSuppressor;

        InspectableClass(RuntimeClass_Private_Infrastructure_InputHelper, TrustLevel::BaseTrust);

    public:
        IFACEMETHOD(LeftMouseClick)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(LeftMouseClickOnPoint)(wf::Point position) override;
        IFACEMETHOD(Tap)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(TapAtPercent)(_In_ xaml::IFrameworkElement* pElement, float fractionOfWidth, float fractionOfHeight) override;
        IFACEMETHOD(TapOnPoint)(_In_ wf::Point point) override;
        IFACEMETHOD(DoubleTap)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(Hold)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(HoldOnPoint)(_In_ wf::Point point) override;
        IFACEMETHOD(HoldForSpecifiedDuration)(_In_ xaml::IFrameworkElement* pElement, UINT32 holdDuration) override;
        IFACEMETHOD(Flick)(_In_ xaml::IFrameworkElement* pElement, test_infra::FlickDirection direction) override;
        IFACEMETHOD(PressHoldAndPanFromCenter)(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor, UINT holdTime) override;
        IFACEMETHOD(PanFromCenter)(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor) override;
        IFACEMETHOD(DragFromCenter)(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor) override;
        IFACEMETHOD(DragToCenter)(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor) override;
        IFACEMETHOD(DragBetweenElements)(_In_ xaml::IFrameworkElement* pSourceElement, _In_ xaml::IFrameworkElement* pTargetElement, DOUBLE velocityFactor) override;
        IFACEMETHOD(PenStrokeFromCenter)(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY, DOUBLE velocityFactor) override;
        IFACEMETHOD(PenHold)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(PenTap)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(PenDoubleTap)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(PenBarrelTap)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(InjectDragWithPenBarrelDown)(_In_ xaml::IFrameworkElement* pElement, _In_ INT32 relX, _In_ INT32 relY, _In_ DOUBLE velocityFactor) override;
        IFACEMETHOD(PenWriteLetterT)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(PenWriteDash)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(ZoomInToEdges)(_In_ xaml::IFrameworkElement* pElement, UINT equidistanceFromEdges, xaml::Controls::Orientation orientation, DOUBLE velocityFactor) override;
        IFACEMETHOD(ZoomOutFromEdges)(_In_ xaml::IFrameworkElement* pElement, UINT equidistanceFromEdges, xaml::Controls::Orientation orientation, DOUBLE velocityFactor) override;
        IFACEMETHOD(InjectRotate)(_In_ xaml::IFrameworkElement* element, _In_ wf::Point startFinger1, _In_ wf::Point startFinger2, DOUBLE rotationAngle, UINT32 durationMs, BOOLEAN pivotRotate) override;
        IFACEMETHOD(MoveMouse)(_In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(MoveMouseFromCenter)(_In_ xaml::IFrameworkElement* pElement, INT32 relX, INT32 relY) override;
        IFACEMETHOD(MoveMouseOnPoint)(_In_ wf::Point point) override;
        IFACEMETHOD(ScrollMouseWheel)(_In_ xaml::IFrameworkElement* pElement, INT numberOfWheel) override;

        // Methods not relative to xaml elements.
        IFACEMETHOD(Swipe)(wf::Point start, wf::Point end) override;

        // More advanced "dynamic press" APIs, allow you to separately inject pointer down/up events
        IFACEMETHOD(DynamicPressCenter)(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy, test_infra::PointerFinger finger) override;
        IFACEMETHOD(DynamicRelease)(test_infra::PointerFinger finger) override;
        IFACEMETHOD(DynamicPenPressCenter)(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy) override;
        IFACEMETHOD(DynamicPenRelease)() override;

        // Allows to press a mouse button, drag and release
        IFACEMETHOD(MouseButtonDown)(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy, test_infra::MouseButton button) override;
        IFACEMETHOD(MouseButtonDownOnPoint)(wf::Point point, test_infra::MouseButton button) override;
        IFACEMETHOD(MouseButtonUp)(_In_ xaml::IFrameworkElement* pElement, INT dx, INT dy, test_infra::MouseButton button) override;
        IFACEMETHOD(MouseButtonUpOnPoint)(wf::Point point, test_infra::MouseButton button) override;
        IFACEMETHOD(MouseDrag)(_In_ xaml::IFrameworkElement* pElementSrc, INT dxSrc, INT dySrc, _In_ xaml::IFrameworkElement* pElementDest, INT dxDest, INT dyDest, test_infra::MouseButton button) override;
        IFACEMETHOD(MouseDragOnPoints)(wf::Point pointSrc, wf::Point pointDest, test_infra::MouseButton button) override;

        // Allows clicking
        IFACEMETHOD(ClickMouseButtonOnPoint)(test_infra::MouseButton button, wf::Point position) override;
        IFACEMETHOD(ClickMouseButton)(test_infra::MouseButton button, _In_ xaml::IFrameworkElement* pElement) override;
        IFACEMETHOD(ClickMouseButtonWithOffset)(test_infra::MouseButton button, _In_ xaml::IFrameworkElement* pElement, INT dx, INT dy) override;

        IFACEMETHOD(SuppressGestureStateReset)(_Out_ wf::IClosable** suppressor) override;

        // ICoordinateTransformer
        POINT ComputeScreenCoordinates(_In_ xaml::IFrameworkElement* elementInIsland, wf::Point point) override;

    private:
        // Some tests relied on us not resetting the state of the gesture recognizer before injecting a tap,
        // likely due to timing or something similarly.  This provides the ability to suppress that behavior
        // in tests that have that reliance for passing.
        class GestureStateResetSuppressor : public Microsoft::WRL::RuntimeClass<wf::IClosable>
        {
        public:
            IFACEMETHOD(Init)();
            IFACEMETHOD(Close)();
        };

        void EnsureInputReady();

        HRESULT HoldForSpecifiedDurationInternal(InputDevice inputDevice, _In_ xaml::IFrameworkElement* pElement, UINT32 holdDuration, UINT32 pressCount = 1);
        void PressHoldAndDrag(
            InputDevice inputDevice,
            _In_ xaml::IFrameworkElement* pElement,
            INT32 relX,
            INT32 relY,
            DOUBLE velocityFactor,
            UINT holdTimeBeforeDrag,
            BOOL fromCenter);

        void PressHoldAndDrag(
            InputDevice inputDevice,
            _In_ xaml::IFrameworkElement* pSourceElement,
            _In_ xaml::IFrameworkElement* pTargetElement,
            DOUBLE velocityFactor,
            UINT holdTimeBeforeDrag,
            BOOL fromCenter);

        void PressAndDrag(
                InputDevice inputDevice,
                _In_ xaml::IFrameworkElement* pElement,
                INT32 relX,
                INT32 relY,
                DOUBLE velocityFactor,
                BOOL fromCenter);

        void PressAndDrag(
                InputDevice inputDevice,
                _In_ xaml::IFrameworkElement* pSourceElement,
                _In_ xaml::IFrameworkElement* pTargetElement,
                DOUBLE velocityFactor,
                BOOL fromCenter);

        wf::Point GetCenter(
                _In_ xaml::IFrameworkElement* pElement);

        wf::Point GetElementPosition(
                _In_ xaml::IFrameworkElement* pElement,
                float fractionOfWidth,
                float fractionOfHeight);

        void GetZoomInfo(
                _In_ xaml::IFrameworkElement* pElement,
                UINT equidistanceFromEdges,
                xaml::Controls::Orientation orientation,
                DOUBLE velocityFactor,
                BOOL zoomIn,
                _Out_ UINT* pPhysicalEquidistanceFromEdges,
                _Out_ UINT* pDuration,
                _Out_ FLOAT* pDirection,
                _Out_ POINT* pStartFinger1,
                _Out_ POINT* pStartFinger2);

        UINT GetDisplayOrientationAdjustment();
        UINT GetWin32DisplayOrientation();
        wrl::ComPtr<wgrd::IDisplayInformation> GetUWPDisplayInformation();

        void DrawPenStroke(POINT ptStart, POINT ptEnd);

        static const unsigned int s_defaultTapDurationMs = 32;
        static const unsigned int s_defaultClickDurationMs = 32;

        // Even in the worse case scenario where the interval required to recognize a double-tap is set to the lowest OS setting,
        // 10 msec is short enough to always trigger the double-tap.
        static const unsigned int s_defaultDoubleTapDeltaMs = 10;

        // Even in the worse case scenario where the amount of time required to recognize a hold gesture is set to the highest OS setting,
        // 1.5 sec is long enough to always trigger the gesture.
        static const unsigned int s_defaultHoldDurationMs = 1500;
        static const unsigned int s_defaultFlickDurationMs = 250;
        static const unsigned int s_defaultFlickDistancePx = 150;
        static const unsigned int s_defaultSwipeDurationMs = 250;

        // Time between pointer down and drag start
        static const unsigned int s_defaultPanDurationInHoldMs = 10;

        // The default panning/dragging velocity is 1 pixel per ms.
        static const double s_defaultDragVelocityPxPerMs;

        // The default zoom-in velocity is 0.005 unit per ms.
        // Thus each increase of the zoom factor by one unit takes 200 ms.
        static const double s_defaultZoomInVelocityUnitPerMs;

        // The default zoom-out velocity is 0.00125 unit per ms.
        // Thus the decrease of the zoom factor from 1 to 0+ takes 800 ms.
        static const double s_defaultZoomOutVelocityUnitPerMs;

        // The default contact geometry width/height when using DynamicPressCenter
        static const DWORD s_defaultDynamicPressWidthLogicalPx;
        static const DWORD s_defaultDynamicPressHeightLogicalPx;

        static wil::srwlock s_inputLock;

        //The Id we pass to rpcinjections for pen injections, like the PointerFinger enum
        static const UINT InputHelper::s_penFinger = 5;

        static bool s_suppressGestureStateReset;
    };

} }
