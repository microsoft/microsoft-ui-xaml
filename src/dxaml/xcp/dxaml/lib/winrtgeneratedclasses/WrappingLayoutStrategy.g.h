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


#define __WrappingLayoutStrategy_GUID "34e2aa1d-0d8f-4a10-a3dd-ea7541c4cf94"

namespace DirectUI
{
    class WrappingLayoutStrategy;

    class __declspec(novtable) WrappingLayoutStrategyGenerated:
        public ctl::WeakReferenceSource
        , public ABI::Microsoft::UI::Xaml::Controls::ILayoutStrategy
    {
        friend class DirectUI::WrappingLayoutStrategy;



    public:
        WrappingLayoutStrategyGenerated();
        ~WrappingLayoutStrategyGenerated() override;

        // Event source typedefs.


        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(BeginMeasure)() override;
        IFACEMETHOD(EndMeasure)() override;
        IFACEMETHOD(EstimateElementBounds)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Microsoft::UI::Xaml::Controls::EstimationReference headerReference, ABI::Microsoft::UI::Xaml::Controls::EstimationReference containerReference, ABI::Windows::Foundation::Rect window, _Out_ ABI::Windows::Foundation::Rect* pResult) override;
        IFACEMETHOD(EstimateElementIndex)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, ABI::Microsoft::UI::Xaml::Controls::EstimationReference headerReference, ABI::Microsoft::UI::Xaml::Controls::EstimationReference containerReference, ABI::Windows::Foundation::Rect window, _Out_ ABI::Windows::Foundation::Rect* pTargetRect, _Out_ INT* pResult) override;
        IFACEMETHOD(EstimateIndexFromPoint)(BOOLEAN requestingInsertionIndex, ABI::Windows::Foundation::Point point, ABI::Microsoft::UI::Xaml::Controls::EstimationReference reference, ABI::Windows::Foundation::Rect windowConstraint, _Out_ ABI::Microsoft::UI::Xaml::Controls::IndexSearchHint* pSearchHint, _Out_ ABI::Microsoft::UI::Xaml::Controls::ElementType* pElementType, _Out_ INT* pElementIndex) override;
        IFACEMETHOD(EstimatePanelExtent)(ABI::Microsoft::UI::Xaml::Controls::EstimationReference lastHeaderReference, ABI::Microsoft::UI::Xaml::Controls::EstimationReference lastContainerReference, ABI::Windows::Foundation::Rect windowConstraint, _Out_ ABI::Windows::Foundation::Size* pResult) override;
        IFACEMETHOD(GetElementArrangeBounds)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Windows::Foundation::Rect containerBounds, ABI::Windows::Foundation::Rect windowConstraint, ABI::Windows::Foundation::Size finalSize, _Out_ ABI::Windows::Foundation::Rect* pResult) override;
        IFACEMETHOD(GetElementBounds)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Windows::Foundation::Size containerDesiredSize, ABI::Microsoft::UI::Xaml::Controls::LayoutReference referenceInformation, ABI::Windows::Foundation::Rect windowConstraint, _Out_ ABI::Windows::Foundation::Rect* pResult) override;
        IFACEMETHOD(GetElementMeasureSize)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Windows::Foundation::Rect windowConstraint, _Out_ ABI::Windows::Foundation::Size* pResult) override;
        IFACEMETHOD(GetElementTransitionsBounds)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Windows::Foundation::Rect windowConstraint, _Out_ ABI::Windows::Foundation::Rect* pResult) override;
        IFACEMETHOD(GetIsWrappingStrategy)(_Out_ BOOLEAN* pResult) override;
        IFACEMETHOD(GetPositionOfFirstElement)(_Out_ ABI::Windows::Foundation::Point* pResult) override;
        IFACEMETHOD(GetRegularSnapPoints)(_Out_ FLOAT* pNearOffset, _Out_ FLOAT* pFarOffset, _Out_ FLOAT* pSpacing, _Out_ BOOLEAN* pResult) override;
        IFACEMETHOD(GetTargetIndexFromNavigationAction)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Microsoft::UI::Xaml::Controls::KeyNavigationAction action, ABI::Windows::Foundation::Rect windowConstraint, INT itemIndexHintForHeaderNavigation, _Out_ ABI::Microsoft::UI::Xaml::Controls::ElementType* pTargetElementType, _Out_ INT* pTargetElementIndex) override;
        IFACEMETHOD(GetVirtualizationDirection)(_Out_ ABI::Microsoft::UI::Xaml::Controls::Orientation* pResult) override;
        IFACEMETHOD(HasIrregularSnapPoints)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, _Out_ BOOLEAN* pResult) override;
        IFACEMETHOD(HasSnapPointOnElement)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, _Out_ BOOLEAN* pResult) override;
        IFACEMETHOD(IsIndexLayoutBoundary)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Windows::Foundation::Rect windowConstraint, _Out_ BOOLEAN* pIsLeftBoundary, _Out_ BOOLEAN* pIsTopBoundary, _Out_ BOOLEAN* pIsRightBoundary, _Out_ BOOLEAN* pIsBottomBoundary) override;
        IFACEMETHOD(SetLayoutDataInfoProvider)(_In_ ABI::Microsoft::UI::Xaml::Controls::ILayoutDataInfoProvider* pProvider) override;
        IFACEMETHOD(ShouldContinueFillingUpSpace)(ABI::Microsoft::UI::Xaml::Controls::ElementType elementType, INT elementIndex, ABI::Microsoft::UI::Xaml::Controls::LayoutReference referenceInformation, ABI::Windows::Foundation::Rect windowToFill, _Out_ BOOLEAN* pResult) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "WrappingLayoutStrategy_Partial.h"

