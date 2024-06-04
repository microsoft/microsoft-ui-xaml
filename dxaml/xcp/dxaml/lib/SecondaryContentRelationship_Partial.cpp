// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SecondaryContentRelationship.g.h"
#include "SecondaryContentRelationship_Partial.h"
#include "ParametricCurve.g.h"
#include "ParametricCurveCollection.g.h"
#include "ParametricCurveSegment.g.h"
#include "ParametricCurveSegmentCollection.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

namespace
{
    // We'll put some helpers in this anonymous namespace so they aren't visible outside this file
    _Check_return_ HRESULT PopulateCurve(
        _In_ DirectUI::ParametricCurve* pCurve,
        _In_ UINT numPoints,
        _In_reads_(numPoints) const DOUBLE* pPrimaryContentValues,
        _In_reads_(numPoints) const DOUBLE* pSecondaryContentValues)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<ParametricCurveSegmentCollection> spCurveSegments;
        ASSERT(numPoints >= 2);

        IFC(pCurve->get_CurveSegments(&spCurveSegments));
        IFC(spCurveSegments->Clear());

        for (UINT i = 0; i < numPoints; ++i)
        {
            // Don't generate a curve for a zero-width interval.
            if (i < numPoints - 1 && DoubleUtil::AreClose(pPrimaryContentValues[i + 1] - pPrimaryContentValues[i], 0))
            {
                continue;
            }

            ctl::ComPtr<ParametricCurveSegment> spCurveSegment;
            IFC(ctl::make(&spCurveSegment));

            IFC(spCurveSegment->put_BeginOffset(pPrimaryContentValues[i]));
            IFC(spCurveSegment->put_ConstantCoefficient(pSecondaryContentValues[i]));
            IFC(spCurveSegment->put_LinearCoefficient(
                ((i < numPoints - 1) &&
                // accepts duplicated values to make step function
                !DoubleUtil::AreClose(pPrimaryContentValues[i + 1], pPrimaryContentValues[i])) ?
                (pSecondaryContentValues[i + 1] - pSecondaryContentValues[i]) / (pPrimaryContentValues[i + 1] - pPrimaryContentValues[i]) :
                0));
            IFC(spCurveSegment->put_QuadraticCoefficient(0));
            IFC(spCurveSegment->put_CubicCoefficient(0));

            IFC(spCurveSegments->Append(spCurveSegment.Get()));
        }

Cleanup:
        RRETURN(hr);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a secondary content relationship to run a sticky header in a list view.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SecondaryContentRelationshipFactory::CreateStickyHeaderRelationshipImpl(
    _In_ xaml::IUIElement *pScrollViewer,
    _In_ xaml::IUIElement *pPanelObject,
    _In_ xaml::IDependencyObject *pPanelTransform,
    _In_ xaml::IDependencyObject* pHeaderTransform,
    _In_ DOUBLE groupTopY,
    _In_ DOUBLE groupBottomY,
    _In_ DOUBLE headerHeight,
    _Outptr_ xaml::Internal::ISecondaryContentRelationship **ppReturnValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strHorizontalOffsetPropertyName;
    wrl_wrappers::HString strVerticalOffsetPropertyName;
    wrl_wrappers::HString strTranslateXPropertyName;
    wrl_wrappers::HString strTranslateYPropertyName;

    IFC(strHorizontalOffsetPropertyName.Set(STR_LEN_PAIR(L"HorizontalOffset")));
    IFC(strVerticalOffsetPropertyName.Set(STR_LEN_PAIR(L"VerticalOffset")));
    IFC(strTranslateXPropertyName.Set(STR_LEN_PAIR(L"TranslateX")));
    IFC(strTranslateYPropertyName.Set(STR_LEN_PAIR(L"TranslateY")));

    DOUBLE primaryContentValues[] = { DoubleUtil::Min(0.0, groupTopY - 1.0), groupTopY, groupBottomY - headerHeight };
    DOUBLE secondaryContentValues[] = { 0, 0, -(groupBottomY - headerHeight - groupTopY) };

    IFC(CreateNPointSecondaryContentRelationship(
        pScrollViewer,
        pPanelObject,
        pPanelTransform,
        strVerticalOffsetPropertyName.Get(),
        DirectManipulationProperty::TranslationY,
        strTranslateYPropertyName.Get(),
        strHorizontalOffsetPropertyName.Get(),
        DirectManipulationProperty::TranslationX,
        strTranslateXPropertyName.Get(),
        ARRAYSIZE(primaryContentValues),
        primaryContentValues,
        secondaryContentValues,
        ppReturnValue));

    (*ppReturnValue)->SetAuxiliaryDependencyPropertyHolder(pHeaderTransform);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::UpdateStickyHeaderCurve(
    _In_ SecondaryContentRelationship* pRelationship,
    _In_ DOUBLE groupTopY,
    _In_ DOUBLE groupBottomY,
    _In_ DOUBLE headerHeight)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ParametricCurveCollection> spCurveCollection;
    UINT numCurves = 0;

    IFC(pRelationship->get_Curves(&spCurveCollection));
    IFC(spCurveCollection->get_Size(&numCurves));
    ASSERT(numCurves == 2);

    {
        ctl::ComPtr<xaml::IDependencyObject> spCurveAsIDO;
        const DOUBLE primaryContentValues[] = { std::min(0.0, groupTopY - 1.0), groupTopY, groupBottomY - headerHeight };
        const DOUBLE secondaryContentValues[] = { 0, 0, -(groupBottomY - headerHeight - groupTopY) };

        // Assuming that the non-orthogonal curve is at index 0
        IFC(spCurveCollection->GetAt(0, &spCurveAsIDO));
        IFC(PopulateCurve(spCurveAsIDO.Cast<ParametricCurve>(), ARRAYSIZE(primaryContentValues), primaryContentValues, secondaryContentValues));
    }
Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a secondary content relationship to run a clip transform in a list view
//      with sticky headers.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SecondaryContentRelationshipFactory::CreateClipTransformRelationshipImpl(
    _In_ xaml::IUIElement *pScrollViewer,
    _In_ xaml::IUIElement *pClipOwner,
    _In_ xaml::IDependencyObject *pClipTransform,
    _In_ DOUBLE listExtentHeight,
    _In_ DOUBLE listViewportHeight,
    _Outptr_ xaml::Internal::ISecondaryContentRelationship **ppReturnValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strHorizontalOffsetPropertyName;
    wrl_wrappers::HString strVerticalOffsetPropertyName;
    wrl_wrappers::HString strTranslateXPropertyName;
    wrl_wrappers::HString strTranslateYPropertyName;

    IFC(strHorizontalOffsetPropertyName.Set(STR_LEN_PAIR(L"HorizontalOffset")));
    IFC(strVerticalOffsetPropertyName.Set(STR_LEN_PAIR(L"VerticalOffset")));
    IFC(strTranslateXPropertyName.Set(STR_LEN_PAIR(L"TranslateX")));
    IFC(strTranslateYPropertyName.Set(STR_LEN_PAIR(L"TranslateY")));

    DOUBLE primaryContentValues[] = { 0, listExtentHeight - listViewportHeight };
    DOUBLE secondaryContentValues[] = { 0, -(listExtentHeight - listViewportHeight) };

    IFC(CreateNPointSecondaryContentRelationship(
        pScrollViewer,
        pClipOwner,
        pClipTransform,
        strVerticalOffsetPropertyName.Get(),
        DirectManipulationProperty::TranslationY,
        strTranslateYPropertyName.Get(),
        strHorizontalOffsetPropertyName.Get(),
        DirectManipulationProperty::TranslationX,
        strTranslateXPropertyName.Get(),
        ARRAYSIZE(primaryContentValues),
        primaryContentValues,
        secondaryContentValues,
        ppReturnValue));

    IFC(static_cast<DirectUI::SecondaryContentRelationship *>(*ppReturnValue)->put_ShouldTargetClip(TRUE));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a secondary content relationship to run the parallax transform in Hub and Pivot.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SecondaryContentRelationshipFactory::CreateParallaxRelationshipImpl(
    _In_ xaml::IUIElement *pScrollViewer,
    _In_ xaml::IUIElement *pHeaderObject,
    _In_ xaml::IDependencyObject *pHeaderTransform,
    _In_ UINT primaryOffsetsCount,
    _In_reads_(primaryOffsetsCount) DOUBLE primaryOffsets[],
    _In_ UINT secondaryOffsetsCount,
    _In_reads_(secondaryOffsetsCount) DOUBLE secondaryOffsets[],
    _Outptr_ xaml::Internal::ISecondaryContentRelationship **ppReturnValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strHorizontalOffsetPropertyName;
    wrl_wrappers::HString strVerticalOffsetPropertyName;
    wrl_wrappers::HString strTranslateXPropertyName;
    wrl_wrappers::HString strTranslateYPropertyName;

    IFC(strHorizontalOffsetPropertyName.Set(STR_LEN_PAIR(L"HorizontalOffset")));
    IFC(strVerticalOffsetPropertyName.Set(STR_LEN_PAIR(L"VerticalOffset")));
    IFC(strTranslateXPropertyName.Set(STR_LEN_PAIR(L"TranslateX")));
    IFC(strTranslateYPropertyName.Set(STR_LEN_PAIR(L"TranslateY")));

    IFCEXPECT(primaryOffsetsCount == secondaryOffsetsCount);

    IFC(CreateNPointSecondaryContentRelationship(
        pScrollViewer,
        pHeaderObject,
        pHeaderTransform,
        strHorizontalOffsetPropertyName.Get(),
        DirectManipulationProperty::TranslationX,
        strTranslateXPropertyName.Get(),
        strVerticalOffsetPropertyName.Get(),
        DirectManipulationProperty::TranslationY,
        strTranslateYPropertyName.Get(),
        primaryOffsetsCount,
        primaryOffsets,
        secondaryOffsets,
        ppReturnValue));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a secondary content relationship to keep an element in the same position it started in permanently
//      within a ScrollViewer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SecondaryContentRelationshipFactory::CreateStaticElementRelationshipImpl(
    _In_ xaml::IUIElement *pScrollViewer,
    _In_ xaml::IUIElement *pHeaderObject,
    _In_ xaml::IDependencyObject *pHeaderTransform,
    _In_ BOOLEAN isHorizontallyStatic,
    _In_ BOOLEAN isInverted,
    _Outptr_ xaml::Internal::ISecondaryContentRelationship **ppReturnValue)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strHorizontalOffsetPropertyName;
    wrl_wrappers::HString strVerticalOffsetPropertyName;
    wrl_wrappers::HString strTranslateXPropertyName;
    wrl_wrappers::HString strTranslateYPropertyName;

    *ppReturnValue = NULL;

    ctl::ComPtr<SecondaryContentRelationship> spRelationship;
    ctl::ComPtr<ParametricCurveCollection> spCurveCollection;
    ctl::ComPtr<ParametricCurveSegmentCollection> spCurveSegments;
    ctl::ComPtr<ParametricCurveSegment> spCurveSegment;
    ctl::ComPtr<ParametricCurve> spParametricCurve;

    IFC(strHorizontalOffsetPropertyName.Set(STR_LEN_PAIR(L"HorizontalOffset")));
    IFC(strVerticalOffsetPropertyName.Set(STR_LEN_PAIR(L"VerticalOffset")));
    IFC(strTranslateXPropertyName.Set(STR_LEN_PAIR(L"TranslateX")));
    IFC(strTranslateYPropertyName.Set(STR_LEN_PAIR(L"TranslateY")));

    IFC(ctl::make(&spRelationship));

    IFC(spRelationship->SetPrimaryContent(pScrollViewer));
    IFC(spRelationship->SetSecondaryContent(pHeaderObject, pHeaderTransform));

    IFC(spRelationship->get_Curves(&spCurveCollection));

    IFC(ctl::make(&spParametricCurve));

    IFC(spParametricCurve->SetPrimaryContentProperty(
        isHorizontallyStatic ? strHorizontalOffsetPropertyName.Get() : strVerticalOffsetPropertyName.Get()));
    IFC(spParametricCurve->SetSecondaryContentProperty(
        isHorizontallyStatic ? DirectManipulationProperty::TranslationX : DirectManipulationProperty::TranslationY,
        isHorizontallyStatic ? strTranslateXPropertyName.Get() : strTranslateYPropertyName.Get()));

    IFC(spParametricCurve->get_CurveSegments(&spCurveSegments));
    IFC(spCurveSegments->Clear());

    IFC(ctl::make(&spCurveSegment));

    IFC(spCurveSegment->put_BeginOffset(0));
    IFC(spCurveSegment->put_ConstantCoefficient(0));
    IFC(spCurveSegment->put_LinearCoefficient(isInverted ? 1.0 : -1.0));
    IFC(spCurveSegment->put_QuadraticCoefficient(0));
    IFC(spCurveSegment->put_CubicCoefficient(0));

    IFC(spCurveSegments->Append(spCurveSegment.Get()));
    IFC(spCurveCollection->Append(spParametricCurve.Get()));

    IFC(ctl::make(&spParametricCurve));

    IFC(spParametricCurve->SetPrimaryContentProperty(
        isHorizontallyStatic ? strVerticalOffsetPropertyName.Get() : strHorizontalOffsetPropertyName.Get()));
    IFC(spParametricCurve->SetSecondaryContentProperty(
        isHorizontallyStatic ? DirectManipulationProperty::TranslationY : DirectManipulationProperty::TranslationX,
        isHorizontallyStatic ? strTranslateYPropertyName.Get() : strTranslateXPropertyName.Get()));

    IFC(spParametricCurve->get_CurveSegments(&spCurveSegments));

    IFC(ctl::make(&spCurveSegment));

    IFC(spCurveSegment->put_BeginOffset(0));
    IFC(spCurveSegment->put_ConstantCoefficient(0));
    IFC(spCurveSegment->put_LinearCoefficient(0));
    IFC(spCurveSegment->put_QuadraticCoefficient(0));
    IFC(spCurveSegment->put_CubicCoefficient(0));

    IFC(spCurveSegments->Append(spCurveSegment.Get()));
    IFC(spCurveCollection->Append(spParametricCurve.Get()));

    *ppReturnValue = spRelationship.Detach();

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to create a secondary content relationship with
//      a given number of points.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
DirectUI::CreateNPointSecondaryContentRelationship(
    _In_ xaml::IUIElement *pPrimaryContent,
    _In_ xaml::IUIElement *pSecondaryContent,
    _In_ xaml::IDependencyObject *pDependencyPropertyHolder,
    _In_ HSTRING strPrimaryContentProperty,
    _In_ DirectManipulationProperty secondaryContentProperty,
    _In_ HSTRING strAssociatedDependencyProperty,
    _In_opt_ HSTRING strOrthogonalPrimaryContentProperty,
    _In_opt_ DirectManipulationProperty orthogonalSecondaryContentProperty,
    _In_opt_ HSTRING strOrthogonalAssociatedDependencyProperty,
    _In_ UINT numPoints,
    _In_reads_(numPoints) const DOUBLE *pPrimaryContentValues,
    _In_reads_(numPoints) const DOUBLE *pSecondaryContentValues,
    _Outptr_result_maybenull_ xaml::Internal::ISecondaryContentRelationship **ppSecondaryContentRelationship)
{
    HRESULT hr = S_OK;

    *ppSecondaryContentRelationship = nullptr;

    ctl::ComPtr<SecondaryContentRelationship> spRelationship;
    ctl::ComPtr<ParametricCurveCollection> spCurveCollection;
    ctl::ComPtr<ParametricCurve> spParametricCurve;

    IFCPTR(pPrimaryContent);
    IFCPTR(pSecondaryContent);
    IFCPTR(pDependencyPropertyHolder);
    IFCPTR(ppSecondaryContentRelationship);
    ASSERT(numPoints >= 2);

    IFC(ctl::make(&spRelationship));

    IFC(spRelationship->SetPrimaryContent(pPrimaryContent));
    IFC(spRelationship->SetSecondaryContent(pSecondaryContent, pDependencyPropertyHolder));

    IFC(spRelationship->get_Curves(&spCurveCollection));

    IFC(ctl::make(&spParametricCurve));

    IFC(spParametricCurve->SetPrimaryContentProperty(strPrimaryContentProperty));
    IFC(spParametricCurve->SetSecondaryContentProperty(secondaryContentProperty, strAssociatedDependencyProperty));

    IFC(PopulateCurve(spParametricCurve.Get(), numPoints, pPrimaryContentValues, pSecondaryContentValues));
    IFC(spCurveCollection->Append(spParametricCurve.Get()));

    // If we have orthogonal properties, peg those to always be zero.
    if (strOrthogonalPrimaryContentProperty &&
        orthogonalSecondaryContentProperty != DirectManipulationProperty::None &&
        strOrthogonalAssociatedDependencyProperty)
    {
        ctl::ComPtr<ParametricCurveSegmentCollection> spCurveSegmentCollection;
        ctl::ComPtr<ParametricCurveSegment> spCurveSegment;

        IFC(ctl::make(&spParametricCurve));

        IFC(spParametricCurve->SetPrimaryContentProperty(strOrthogonalPrimaryContentProperty));
        IFC(spParametricCurve->SetSecondaryContentProperty(orthogonalSecondaryContentProperty, strOrthogonalAssociatedDependencyProperty));

        IFC(spParametricCurve->get_CurveSegments(&spCurveSegmentCollection));

        IFC(ctl::make(&spCurveSegment));

        IFC(spCurveSegment->put_BeginOffset(0));
        IFC(spCurveSegment->put_ConstantCoefficient(0));
        IFC(spCurveSegment->put_LinearCoefficient(0));
        IFC(spCurveSegment->put_QuadraticCoefficient(0));
        IFC(spCurveSegment->put_CubicCoefficient(0));

        IFC(spCurveSegmentCollection->Append(spCurveSegment.Get()));
        IFC(spCurveCollection->Append(spParametricCurve.Get()));
    }

    *ppSecondaryContentRelationship = spRelationship.Detach();

Cleanup:
    RRETURN(hr);
}


