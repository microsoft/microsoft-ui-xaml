// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "ScrollContentControl.g.h"

#define __ScrollViewer_GUID "71d529d6-3a4e-4a1f-8b40-6f1e6cdce52c"

namespace DirectUI
{
    class ScrollViewer;
    class UIElement;

    class __declspec(novtable) ScrollViewerGenerated:
        public DirectUI::ScrollContentControl
        , public ABI::Microsoft::UI::Xaml::Controls::IScrollViewer
        , public ABI::Microsoft::UI::Xaml::Controls::IScrollAnchorProvider
        , public ABI::Microsoft::UI::Xaml::Controls::IScrollViewerPrivate
    {
        friend class DirectUI::ScrollViewer;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.ScrollViewer");

        BEGIN_INTERFACE_MAP(ScrollViewerGenerated, DirectUI::ScrollContentControl)
            INTERFACE_ENTRY(ScrollViewerGenerated, ABI::Microsoft::UI::Xaml::Controls::IScrollViewer)
            INTERFACE_ENTRY(ScrollViewerGenerated, ABI::Microsoft::UI::Xaml::Controls::IScrollAnchorProvider)
            INTERFACE_ENTRY(ScrollViewerGenerated, ABI::Microsoft::UI::Xaml::Controls::IScrollViewerPrivate)
        END_INTERFACE_MAP(ScrollViewerGenerated, DirectUI::ScrollContentControl)

    public:
        ScrollViewerGenerated();
        ~ScrollViewerGenerated() override;

        // Event source typedefs.
        typedef CEventSource<ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::ScrollViewer*, ABI::Microsoft::UI::Xaml::Controls::AnchorRequestedEventArgs*>, ABI::Microsoft::UI::Xaml::Controls::IScrollViewer, ABI::Microsoft::UI::Xaml::Controls::IAnchorRequestedEventArgs> AnchorRequestedEventSourceType;
        typedef CEventSource<ABI::Windows::Foundation::IEventHandler<ABI::Microsoft::UI::Xaml::Controls::ScrollViewerViewChangingEventArgs*>, IInspectable, ABI::Microsoft::UI::Xaml::Controls::IScrollViewerViewChangingEventArgs> ViewChangingEventSourceType;
        typedef CEventSource<ABI::Windows::Foundation::IEventHandler<ABI::Microsoft::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs*>, IInspectable, ABI::Microsoft::UI::Xaml::Controls::IScrollViewerViewChangedEventArgs> ViewChangedEventSourceType;
        typedef CEventSource<ABI::Windows::Foundation::IEventHandler<IInspectable*>, IInspectable, IInspectable> DirectManipulationStartedEventSourceType;
        typedef CEventSource<ABI::Windows::Foundation::IEventHandler<IInspectable*>, IInspectable, IInspectable> DirectManipulationCompletedEventSourceType;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ScrollViewer;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ScrollViewer;
        }

        // Properties.
        IFACEMETHOD(get_ArePointerWheelEventsIgnored)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_ArePointerWheelEventsIgnored)(BOOLEAN value) override;
        IFACEMETHOD(get_BringIntoViewOnFocusChange)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_BringIntoViewOnFocusChange)(BOOLEAN value) override;
        IFACEMETHOD(get_CanContentRenderOutsideBounds)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_CanContentRenderOutsideBounds)(BOOLEAN value) override;
        IFACEMETHOD(get_ComputedHorizontalScrollBarVisibility)(_Out_ ABI::Microsoft::UI::Xaml::Visibility* pValue) override;
        _Check_return_ HRESULT put_ComputedHorizontalScrollBarVisibility(ABI::Microsoft::UI::Xaml::Visibility value);
        IFACEMETHOD(get_ComputedVerticalScrollBarVisibility)(_Out_ ABI::Microsoft::UI::Xaml::Visibility* pValue) override;
        _Check_return_ HRESULT put_ComputedVerticalScrollBarVisibility(ABI::Microsoft::UI::Xaml::Visibility value);
        IFACEMETHOD(get_CurrentAnchor)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue) override;
        IFACEMETHOD(get_ExtentHeight)(_Out_ DOUBLE* pValue) override;
        virtual _Check_return_ HRESULT put_ExtentHeight(DOUBLE value);
        IFACEMETHOD(get_ExtentWidth)(_Out_ DOUBLE* pValue) override;
        virtual _Check_return_ HRESULT put_ExtentWidth(DOUBLE value);
        IFACEMETHOD(get_HorizontalAnchorRatio)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_HorizontalAnchorRatio)(DOUBLE value) override;
        IFACEMETHOD(get_HorizontalOffset)(_Out_ DOUBLE* pValue) override;
        virtual _Check_return_ HRESULT put_HorizontalOffset(DOUBLE value);
        IFACEMETHOD(get_HorizontalScrollBarVisibility)(_Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility* pValue) override;
        IFACEMETHOD(put_HorizontalScrollBarVisibility)(ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility value) override;
        IFACEMETHOD(get_HorizontalScrollMode)(_Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollMode* pValue) override;
        IFACEMETHOD(put_HorizontalScrollMode)(ABI::Microsoft::UI::Xaml::Controls::ScrollMode value) override;
        IFACEMETHOD(get_HorizontalSnapPointsAlignment)(_Out_ ABI::Microsoft::UI::Xaml::Controls::Primitives::SnapPointsAlignment* pValue) override;
        IFACEMETHOD(put_HorizontalSnapPointsAlignment)(ABI::Microsoft::UI::Xaml::Controls::Primitives::SnapPointsAlignment value) override;
        IFACEMETHOD(get_HorizontalSnapPointsType)(_Out_ ABI::Microsoft::UI::Xaml::Controls::SnapPointsType* pValue) override;
        IFACEMETHOD(put_HorizontalSnapPointsType)(ABI::Microsoft::UI::Xaml::Controls::SnapPointsType value) override;
        IFACEMETHOD(get_IsDeferredScrollingEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsDeferredScrollingEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsHorizontalRailEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsHorizontalRailEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsHorizontalScrollChainingEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsHorizontalScrollChainingEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsInActiveDirectManipulation)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_IsInDirectManipulation)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(get_IsRequestBringIntoViewIgnored)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsRequestBringIntoViewIgnored)(BOOLEAN value) override;
        IFACEMETHOD(get_IsScrollInertiaEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsScrollInertiaEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsVerticalRailEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsVerticalRailEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsVerticalScrollChainingEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsVerticalScrollChainingEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsZoomChainingEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsZoomChainingEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_IsZoomInertiaEnabled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsZoomInertiaEnabled)(BOOLEAN value) override;
        IFACEMETHOD(get_LeftHeader)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue) override;
        IFACEMETHOD(put_LeftHeader)(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue) override;
        IFACEMETHOD(get_MaxZoomFactor)(_Out_ FLOAT* pValue) override;
        IFACEMETHOD(put_MaxZoomFactor)(FLOAT value) override;
        IFACEMETHOD(get_MinZoomFactor)(_Out_ FLOAT* pValue) override;
        IFACEMETHOD(put_MinZoomFactor)(FLOAT value) override;
        IFACEMETHOD(get_ReduceViewportForCoreInputViewOcclusions)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_ReduceViewportForCoreInputViewOcclusions)(BOOLEAN value) override;
        IFACEMETHOD(get_ScrollableHeight)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_ScrollableHeight(DOUBLE value);
        IFACEMETHOD(get_ScrollableWidth)(_Out_ DOUBLE* pValue) override;
        _Check_return_ HRESULT put_ScrollableWidth(DOUBLE value);
        IFACEMETHOD(get_TopHeader)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue) override;
        IFACEMETHOD(put_TopHeader)(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue) override;
        IFACEMETHOD(get_TopLeftHeader)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IUIElement** ppValue) override;
        IFACEMETHOD(put_TopLeftHeader)(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pValue) override;
        IFACEMETHOD(get_VerticalAnchorRatio)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_VerticalAnchorRatio)(DOUBLE value) override;
        IFACEMETHOD(get_VerticalOffset)(_Out_ DOUBLE* pValue) override;
        virtual _Check_return_ HRESULT put_VerticalOffset(DOUBLE value);
        IFACEMETHOD(get_VerticalScrollBarVisibility)(_Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility* pValue) override;
        IFACEMETHOD(put_VerticalScrollBarVisibility)(ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility value) override;
        IFACEMETHOD(get_VerticalScrollMode)(_Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollMode* pValue) override;
        IFACEMETHOD(put_VerticalScrollMode)(ABI::Microsoft::UI::Xaml::Controls::ScrollMode value) override;
        IFACEMETHOD(get_VerticalSnapPointsAlignment)(_Out_ ABI::Microsoft::UI::Xaml::Controls::Primitives::SnapPointsAlignment* pValue) override;
        IFACEMETHOD(put_VerticalSnapPointsAlignment)(ABI::Microsoft::UI::Xaml::Controls::Primitives::SnapPointsAlignment value) override;
        IFACEMETHOD(get_VerticalSnapPointsType)(_Out_ ABI::Microsoft::UI::Xaml::Controls::SnapPointsType* pValue) override;
        IFACEMETHOD(put_VerticalSnapPointsType)(ABI::Microsoft::UI::Xaml::Controls::SnapPointsType value) override;
        IFACEMETHOD(get_ViewportHeight)(_Out_ DOUBLE* pValue) override;
        virtual _Check_return_ HRESULT put_ViewportHeight(DOUBLE value);
        IFACEMETHOD(get_ViewportWidth)(_Out_ DOUBLE* pValue) override;
        virtual _Check_return_ HRESULT put_ViewportWidth(DOUBLE value);
        IFACEMETHOD(get_ZoomFactor)(_Out_ FLOAT* pValue) override;
        virtual _Check_return_ HRESULT put_ZoomFactor(FLOAT value);
        IFACEMETHOD(get_ZoomMode)(_Out_ ABI::Microsoft::UI::Xaml::Controls::ZoomMode* pValue) override;
        IFACEMETHOD(put_ZoomMode)(ABI::Microsoft::UI::Xaml::Controls::ZoomMode value) override;
        IFACEMETHOD(get_ZoomSnapPoints)(_Outptr_result_maybenull_ ABI::Windows::Foundation::Collections::IVector<FLOAT>** ppValue) override;
        IFACEMETHOD(get_ZoomSnapPointsType)(_Out_ ABI::Microsoft::UI::Xaml::Controls::SnapPointsType* pValue) override;
        IFACEMETHOD(put_ZoomSnapPointsType)(ABI::Microsoft::UI::Xaml::Controls::SnapPointsType value) override;

        // Events.
        _Check_return_ HRESULT GetAnchorRequestedEventSourceNoRef(_Outptr_ AnchorRequestedEventSourceType** ppEventSource);
        IFACEMETHOD(add_AnchorRequested)(_In_ ABI::Windows::Foundation::ITypedEventHandler<ABI::Microsoft::UI::Xaml::Controls::ScrollViewer*, ABI::Microsoft::UI::Xaml::Controls::AnchorRequestedEventArgs*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_AnchorRequested)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetDirectManipulationCompletedEventSourceNoRef(_Outptr_ DirectManipulationCompletedEventSourceType** ppEventSource);
        IFACEMETHOD(add_DirectManipulationCompleted)(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_DirectManipulationCompleted)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetDirectManipulationStartedEventSourceNoRef(_Outptr_ DirectManipulationStartedEventSourceType** ppEventSource);
        IFACEMETHOD(add_DirectManipulationStarted)(_In_ ABI::Windows::Foundation::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_DirectManipulationStarted)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetViewChangedEventSourceNoRef(_Outptr_ ViewChangedEventSourceType** ppEventSource);
        IFACEMETHOD(add_ViewChanged)(_In_ ABI::Windows::Foundation::IEventHandler<ABI::Microsoft::UI::Xaml::Controls::ScrollViewerViewChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_ViewChanged)(EventRegistrationToken token) override;
        _Check_return_ HRESULT GetViewChangingEventSourceNoRef(_Outptr_ ViewChangingEventSourceType** ppEventSource);
        IFACEMETHOD(add_ViewChanging)(_In_ ABI::Windows::Foundation::IEventHandler<ABI::Microsoft::UI::Xaml::Controls::ScrollViewerViewChangingEventArgs*>* pValue, _Out_ EventRegistrationToken* pToken) override;
        IFACEMETHOD(remove_ViewChanging)(EventRegistrationToken token) override;

        // Methods.
        IFACEMETHOD(ChangeView)(ABI::Windows::Foundation::IReference<DOUBLE>* pHorizontalOffset, ABI::Windows::Foundation::IReference<DOUBLE>* pVerticalOffset, ABI::Windows::Foundation::IReference<FLOAT>* pZoomFactor, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(ChangeViewWithOptionalAnimation)(ABI::Windows::Foundation::IReference<DOUBLE>* pHorizontalOffset, ABI::Windows::Foundation::IReference<DOUBLE>* pVerticalOffset, ABI::Windows::Foundation::IReference<FLOAT>* pZoomFactor, BOOLEAN disableAnimation, _Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(DisableOverpan)() override;
        IFACEMETHOD(EnableOverpan)() override;
        IFACEMETHOD(InvalidateScrollInfo)() override;
        IFACEMETHOD(RegisterAnchorCandidate)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement) override;
        IFACEMETHOD(ScrollToHorizontalOffset)(DOUBLE offset) override;
        IFACEMETHOD(ScrollToVerticalOffset)(DOUBLE offset) override;
        IFACEMETHOD(SetIsNearVerticalAlignmentForced)(BOOLEAN enabled) override;
        IFACEMETHOD(UnregisterAnchorCandidate)(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement) override;
        IFACEMETHOD(ZoomToFactor)(FLOAT factor) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        _Check_return_ HRESULT EventAddHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo) override;
        _Check_return_ HRESULT EventRemoveHandlerByIndex(_In_ KnownEventIndex nEventIndex, _In_ IInspectable* pHandler) override;

    private:

        // Fields.
    };
}

#include "ScrollViewer_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) ScrollViewerFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Controls::IScrollViewerStatics
    {
        BEGIN_INTERFACE_MAP(ScrollViewerFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(ScrollViewerFactory, ABI::Microsoft::UI::Xaml::Controls::IScrollViewerStatics)
        END_INTERFACE_MAP(ScrollViewerFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_HorizontalSnapPointsAlignmentProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_VerticalSnapPointsAlignmentProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_HorizontalSnapPointsTypeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_VerticalSnapPointsTypeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ZoomSnapPointsTypeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_HorizontalOffsetProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ViewportWidthProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ScrollableWidthProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ComputedHorizontalScrollBarVisibilityProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ExtentWidthProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_VerticalOffsetProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ViewportHeightProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ScrollableHeightProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ComputedVerticalScrollBarVisibilityProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ExtentHeightProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_MinZoomFactorProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_MaxZoomFactorProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ZoomFactorProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ZoomSnapPointsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TopLeftHeaderProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_LeftHeaderProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_TopHeaderProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_ReduceViewportForCoreInputViewOcclusionsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_HorizontalAnchorRatioProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(get_VerticalAnchorRatioProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.
        static _Check_return_ HRESULT GetHorizontalScrollBarVisibilityStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility* pHorizontalScrollBarVisibility);
        static _Check_return_ HRESULT SetHorizontalScrollBarVisibilityStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility horizontalScrollBarVisibility);
        IFACEMETHOD(get_HorizontalScrollBarVisibilityProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetHorizontalScrollBarVisibility)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility* pHorizontalScrollBarVisibility);
        IFACEMETHOD(SetHorizontalScrollBarVisibility)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility horizontalScrollBarVisibility);
        static _Check_return_ HRESULT GetVerticalScrollBarVisibilityStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility* pVerticalScrollBarVisibility);
        static _Check_return_ HRESULT SetVerticalScrollBarVisibilityStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility verticalScrollBarVisibility);
        IFACEMETHOD(get_VerticalScrollBarVisibilityProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetVerticalScrollBarVisibility)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility* pVerticalScrollBarVisibility);
        IFACEMETHOD(SetVerticalScrollBarVisibility)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollBarVisibility verticalScrollBarVisibility);
        static _Check_return_ HRESULT GetIsHorizontalRailEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsHorizontalRailEnabled);
        static _Check_return_ HRESULT SetIsHorizontalRailEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isHorizontalRailEnabled);
        IFACEMETHOD(get_IsHorizontalRailEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsHorizontalRailEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsHorizontalRailEnabled);
        IFACEMETHOD(SetIsHorizontalRailEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isHorizontalRailEnabled);
        static _Check_return_ HRESULT GetIsVerticalRailEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsVerticalRailEnabled);
        static _Check_return_ HRESULT SetIsVerticalRailEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isVerticalRailEnabled);
        IFACEMETHOD(get_IsVerticalRailEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsVerticalRailEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsVerticalRailEnabled);
        IFACEMETHOD(SetIsVerticalRailEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isVerticalRailEnabled);
        static _Check_return_ HRESULT GetIsHorizontalScrollChainingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsHorizontalScrollChainingEnabled);
        static _Check_return_ HRESULT SetIsHorizontalScrollChainingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isHorizontalScrollChainingEnabled);
        IFACEMETHOD(get_IsHorizontalScrollChainingEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsHorizontalScrollChainingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsHorizontalScrollChainingEnabled);
        IFACEMETHOD(SetIsHorizontalScrollChainingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isHorizontalScrollChainingEnabled);
        static _Check_return_ HRESULT GetIsVerticalScrollChainingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsVerticalScrollChainingEnabled);
        static _Check_return_ HRESULT SetIsVerticalScrollChainingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isVerticalScrollChainingEnabled);
        IFACEMETHOD(get_IsVerticalScrollChainingEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsVerticalScrollChainingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsVerticalScrollChainingEnabled);
        IFACEMETHOD(SetIsVerticalScrollChainingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isVerticalScrollChainingEnabled);
        static _Check_return_ HRESULT GetIsZoomChainingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsZoomChainingEnabled);
        static _Check_return_ HRESULT SetIsZoomChainingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isZoomChainingEnabled);
        IFACEMETHOD(get_IsZoomChainingEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsZoomChainingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsZoomChainingEnabled);
        IFACEMETHOD(SetIsZoomChainingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isZoomChainingEnabled);
        static _Check_return_ HRESULT GetIsScrollInertiaEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsScrollInertiaEnabled);
        static _Check_return_ HRESULT SetIsScrollInertiaEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isScrollInertiaEnabled);
        IFACEMETHOD(get_IsScrollInertiaEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsScrollInertiaEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsScrollInertiaEnabled);
        IFACEMETHOD(SetIsScrollInertiaEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isScrollInertiaEnabled);
        static _Check_return_ HRESULT GetIsZoomInertiaEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsZoomInertiaEnabled);
        static _Check_return_ HRESULT SetIsZoomInertiaEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isZoomInertiaEnabled);
        IFACEMETHOD(get_IsZoomInertiaEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsZoomInertiaEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsZoomInertiaEnabled);
        IFACEMETHOD(SetIsZoomInertiaEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isZoomInertiaEnabled);
        static _Check_return_ HRESULT GetHorizontalScrollModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollMode* pHorizontalScrollMode);
        static _Check_return_ HRESULT SetHorizontalScrollModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollMode horizontalScrollMode);
        IFACEMETHOD(get_HorizontalScrollModeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetHorizontalScrollMode)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollMode* pHorizontalScrollMode);
        IFACEMETHOD(SetHorizontalScrollMode)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollMode horizontalScrollMode);
        static _Check_return_ HRESULT GetVerticalScrollModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollMode* pVerticalScrollMode);
        static _Check_return_ HRESULT SetVerticalScrollModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollMode verticalScrollMode);
        IFACEMETHOD(get_VerticalScrollModeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetVerticalScrollMode)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ScrollMode* pVerticalScrollMode);
        IFACEMETHOD(SetVerticalScrollMode)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ScrollMode verticalScrollMode);
        static _Check_return_ HRESULT GetZoomModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ZoomMode* pZoomMode);
        static _Check_return_ HRESULT SetZoomModeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ZoomMode zoomMode);
        IFACEMETHOD(get_ZoomModeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetZoomMode)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ ABI::Microsoft::UI::Xaml::Controls::ZoomMode* pZoomMode);
        IFACEMETHOD(SetZoomMode)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, ABI::Microsoft::UI::Xaml::Controls::ZoomMode zoomMode);
        static _Check_return_ HRESULT GetIsDeferredScrollingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsDeferredScrollingEnabled);
        static _Check_return_ HRESULT SetIsDeferredScrollingEnabledStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isDeferredScrollingEnabled);
        IFACEMETHOD(get_IsDeferredScrollingEnabledProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsDeferredScrollingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pIsDeferredScrollingEnabled);
        IFACEMETHOD(SetIsDeferredScrollingEnabled)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN isDeferredScrollingEnabled);
        static _Check_return_ HRESULT GetBringIntoViewOnFocusChangeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pBringIntoViewOnFocusChange);
        static _Check_return_ HRESULT SetBringIntoViewOnFocusChangeStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN bringIntoViewOnFocusChange);
        IFACEMETHOD(get_BringIntoViewOnFocusChangeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetBringIntoViewOnFocusChange)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pBringIntoViewOnFocusChange);
        IFACEMETHOD(SetBringIntoViewOnFocusChange)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN bringIntoViewOnFocusChange);
        static _Check_return_ HRESULT GetCanContentRenderOutsideBoundsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pCanContentRenderOutsideBounds);
        static _Check_return_ HRESULT SetCanContentRenderOutsideBoundsStatic(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN canContentRenderOutsideBounds);
        IFACEMETHOD(get_CanContentRenderOutsideBoundsProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetCanContentRenderOutsideBounds)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pCanContentRenderOutsideBounds);
        IFACEMETHOD(SetCanContentRenderOutsideBounds)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, BOOLEAN canContentRenderOutsideBounds);

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ScrollViewer;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
