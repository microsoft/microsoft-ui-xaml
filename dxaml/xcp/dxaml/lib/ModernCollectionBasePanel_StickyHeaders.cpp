// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ModernCollectionBasePanel.g.h"
#include "CompositeTransform.g.h"
#include "SecondaryContentRelationship_Partial.h"
#include "ScrollViewer.g.h"
#include "RectangleGeometry.g.h"
#include "CompositeTransform.g.h"
#include "ParametricCurve.g.h"
#include "ParametricCurveSegment.g.h"
#include "ListViewBaseHeaderItem.g.h"
#include "ParametricCurveCollection.g.h"
#include "ParametricCurveSegmentCollection.g.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using xaml_controls::EstimationReference;
using namespace xaml_primitives;

// Work around disruptive max/min macros
#undef max
#undef min

// Configure or update the sticky behavior of a header
_Check_return_ HRESULT
ModernCollectionBasePanel::ConfigureStickyHeader(
    _In_ const ctl::ComPtr<IUIElement>& spHeader,
    _In_ INT groupDataIndex,
    _In_ const wf::Rect& desiredBounds)
{
    HRESULT hr = S_OK;
    auto spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();

    // If we don't have a ScrollViewer here, we just cannot stick
    if (spScrollViewer)
    {
        ctl::ComPtr<IDependencyObject> spParentAsIDO;
        ctl::ComPtr<IUIElement> spParentAsUIElement;

        IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsIDO));
        IFC(spParentAsIDO.As(&spParentAsUIElement));

        if (spParentAsUIElement)
        {
            ctl::ComPtr<IUIElement> spNextHeader;
            bool curveUpdated = false;
            // Estimate bottom of the group if not given
            DOUBLE groupBottomY = 0;
            IFC(m_containerManager.GetHeaderAtGroupIndex(groupDataIndex + 1, &spNextHeader));
            if (spNextHeader)
            {
                // Note: if the header has been realized, its VirtualizationInformation is never NULL
                UIElement::VirtualizationInformation* pNextVirtualizationInfo = GetVirtualizationInformationFromElement(spNextHeader);
                groupBottomY = pNextVirtualizationInfo->GetBounds().Y;
            }
            else
            {
                // If next header is not realized, take the size of the buffer
                // the curves will be updated later with correct values : the bottom only has an impact on the end of the translation
                // which means that next group will always be visible at that time
                groupBottomY = desiredBounds.Y + (1.0 + m_cacheLength) * m_windowState.GetVisibleWindow().Height;
            }

            IFC(ConfigureStickyHeader(spHeader, spScrollViewer.Cast<ScrollViewer>(), spParentAsUIElement.Cast<UIElement>(), groupBottomY, desiredBounds, false, &curveUpdated));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::ConfigureStickyHeader(
    _In_ const ctl::ComPtr<IUIElement>& spHeader,
    _In_ ScrollViewer* pScrollViewer,
    _In_ DirectUI::UIElement* pLTEParent,
    _In_ DOUBLE groupBottomY,
    _In_ const wf::Rect& desiredBounds,
    _In_ bool manuallyUpdatePosition,
    _Out_ bool* pUpdated)
{
    HRESULT hr = S_OK;
    DOUBLE pannableExtent = 0.0;
    *pUpdated = false;
    UIElement::VirtualizationInformation* pVirtualizationInfo = GetVirtualizationInformationFromElement(spHeader);
    StickyHeaderWrapper* pWrapper = pVirtualizationInfo->m_stickyHeaderWrapper.get();

    if (!m_tpSecondaryContentRelationshipStatics)
    {
        ctl::ComPtr<IActivationFactory> spActivationFactory;
        ctl::ComPtr<xaml::Internal::ISecondaryContentRelationshipStatics> spSecondaryContentRelationshipStatics;

        spActivationFactory.Attach(ctl::ActivationFactoryCreator<SecondaryContentRelationshipFactory>::CreateActivationFactory());
        IFC(spActivationFactory.As(&spSecondaryContentRelationshipStatics));

        SetPtrValue(m_tpSecondaryContentRelationshipStatics, spSecondaryContentRelationshipStatics);
    }

    if (!pWrapper)
    {

        pVirtualizationInfo->m_stickyHeaderWrapper = std::make_shared<StickyHeaderWrapper>(desiredBounds, groupBottomY, m_currentHeaderHeight, pannableExtent);
        pWrapper = pVirtualizationInfo->m_stickyHeaderWrapper.get();
        IFC(pWrapper->Initialize(
            spHeader.Cast<UIElement>(),
            this,
            pScrollViewer,
            pLTEParent,
            m_tpSecondaryContentRelationshipStatics.Get()));
    }

    IFC(pWrapper->SetBounds(desiredBounds, groupBottomY, m_currentHeaderHeight, pannableExtent, pUpdated));

    if (manuallyUpdatePosition)
    {
        // DManip wasn't responsible for the ScrollViewer change, so we need to manually evaluate the secondary content curve and update translate transform on Headers
        IFC(pWrapper->UpdateHeaderPosition());
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
ModernCollectionBasePanel::UpdateStickyHeaders(_In_ const wf::Size& finalSize)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ScrollViewer> spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>().Cast<ScrollViewer>();
    if (spScrollViewer)
    {
        ctl::ComPtr<IDependencyObject> spParentAsIDO;
        ctl::ComPtr<IUIElement> spParentAsUIElement;
        bool wasStickyCurveUpdated = false;

        IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsIDO));
        IFC(spParentAsIDO.As(&spParentAsUIElement));

        if (spParentAsUIElement)
        {
            bool manuallyUpdatePosition = !spScrollViewer->IsInDirectManipulation();

            // Update the size of the header
            IFC(spScrollViewer->get_HeaderHeight(&m_currentHeaderHeight));

            // It is better to give a size too large for a group than too small as other groups will be realized once the scroll is stating
            // That's why we put at least one buffer's height even for a group at the very bottom of the buffer
            DOUBLE groupBottomY = 0.0;
            BOOLEAN lastGroup = TRUE;
            bool headerCurveUpdated = false;
            for (auto validHeaderIndex = m_containerManager.GetValidHeaderCount() - 1;
                validHeaderIndex >= 0;
                --validHeaderIndex)
            {
                const INT groupIndex = m_containerManager.GetGroupIndexFromValidIndex(validHeaderIndex);
                ctl::ComPtr<xaml::IUIElement> spHeader;
                IFC(m_containerManager.GetHeaderAtGroupIndex(groupIndex, &spHeader));

                if (spHeader)
                {
                    UIElement::VirtualizationInformation* pVirtualizationInfo = GetVirtualizationInformationFromElement(spHeader);
                    wf::Rect bounds = pVirtualizationInfo->GetBounds();
                    if (lastGroup)
                    {
                        INT itemCount = 0;
                        INT firstItemDataIndex = 0;
                        IFC(m_cacheManager.GetGroupInformationFromGroupIndex(groupIndex, &firstItemDataIndex, &itemCount));
                        if (itemCount == 0)
                        {
                            // This is an empty group
                            groupBottomY = bounds.Y + bounds.Height;
                        }
                        else if (m_bLastItemRealized)
                        {
                            // We have realized the last item of the list,
                            // it means that this is the last group and also
                            // that we can compute the exact size of the group
                            ctl::ComPtr<IUIElement> spContainer;
                            IFC(m_containerManager.GetContainerAtValidIndex(m_containerManager.GetValidContainerCount() - 1, &spContainer));
                            ASSERT(spContainer.Get());
                            VirtualizationInformation *pLastVirtualizationInformation = spContainer.Cast<UIElement>()->GetVirtualizationInformation();
                            ASSERT(pLastVirtualizationInformation);
                            wf::Rect itemBounds = pLastVirtualizationInformation->GetBounds();
                            groupBottomY = itemBounds.Y + itemBounds.Height;
                        }
                        else
                        {
                            // we need to get an estimation of the last item's position
                            // We will provide the header estimaton reference but not
                            // the container estimation reference as EstimateContainerLocation
                            // should be able to handle that.
                            if (m_containerManager.GetValidContainerCount() > 0)
                            {
                                EstimationReference headerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_GroupHeader);
                                EstimationReference containerRef = CreateDefaultEstimationReference(xaml_controls::ElementType_ItemContainer);
                                wf::Rect lastContainerBounds = {};

                                headerRef.ElementIndex = m_cacheManager.DataIndexToLayoutIndex(xaml_controls::ElementType_GroupHeader, groupIndex);
                                headerRef.ElementBounds = bounds;

                                IFC(m_spLayoutStrategy->EstimateElementBounds(
                                    xaml_controls::ElementType_ItemContainer,
                                    firstItemDataIndex + itemCount - 1,
                                    headerRef,
                                    containerRef,
                                    m_windowState.GetRealizationWindow(),
                                    &lastContainerBounds));
                                groupBottomY = lastContainerBounds.Y + lastContainerBounds.Height;
                            }
                            else
                            {
                                // No container realized but we know that the group
                                // is not empty, let's put an estimation which will be corrected
                                // as soon as an item is realized
                                groupBottomY =  bounds.Y + bounds.Height + (1.0 + m_cacheLength) * m_windowState.GetVisibleWindow().Height;
                            }
                        }
                        lastGroup = FALSE;
                    }
                    else
                    {
                        if (groupBottomY < bounds.Y + bounds.Height)
                        {
                            groupBottomY = bounds.Y + bounds.Height;
                        }
                    }

                    IFC(ConfigureStickyHeader(spHeader,
                                             spScrollViewer.Get(),
                                             spParentAsUIElement.Cast<UIElement>(),
                                             groupBottomY,
                                             bounds,
                                             manuallyUpdatePosition,
                                             &headerCurveUpdated));
                    groupBottomY = bounds.Y;
                    wasStickyCurveUpdated |= headerCurveUpdated;
                }
            }
            IFC(UpdateItemClippingForStickyHeaders(finalSize, manuallyUpdatePosition, wasStickyCurveUpdated /* updateClip */));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ModernCollectionBasePanel::UpdateItemClippingForStickyHeaders(
    _In_ const wf::Size& finalSize,
    _In_ bool manuallyUpdatePosition,
    _In_ bool updateClip) noexcept
{
    HRESULT hr = S_OK;
    bool updateClipAnimation = updateClip;
    wf::Rect clipRect = { 0, 0, std::max(finalSize.Width, m_estimatedSize.Width), std::max(finalSize.Height, m_estimatedSize.Height) };
    ctl::ComPtr<RectangleGeometry> spClip;

    ASSERT(m_cacheManager.IsGrouping());

    {
        ctl::ComPtr<IRectangleGeometry> spClipAsI;
        IFC(get_Clip(&spClipAsI));
        spClip.Attach(static_cast<RectangleGeometry*>(spClipAsI.Detach()));
    }

    // Ensure the clip is created and populated
    if (!spClip)
    {
        updateClipAnimation = true;

        IFC(ctl::make(&spClip));
        IFC(put_Clip(spClip.Get()));
    }

    IFC(spClip->put_Rect(clipRect));

    // Ensure we have a workable transform
    if (!m_tpItemClippingTransform)
    {
        updateClipAnimation = true;

        ctl::ComPtr<CompositeTransform> spTransform;
        IFC(ctl::make(&spTransform));
        SetPtrValue(m_tpItemClippingTransform, spTransform);
    }
    else
    {
        ctl::ComPtr<ITransform> spITransform;
        IFC(spClip->get_Transform(&spITransform));
        if (!ctl::are_equal(spITransform.Get(), ctl::iinspectable_cast(m_tpItemClippingTransform.Get())))
        {
            updateClipAnimation = true;
        }
    }

    m_newStickyHeaderLocations.clear();
    m_newStickyHeaderLocations.reserve(m_containerManager.GetValidHeaderCount());
    for (auto validHeaderIndex = 0; validHeaderIndex < m_containerManager.GetValidHeaderCount(); ++validHeaderIndex)
    {
        ctl::ComPtr<IUIElement> spHeader;
        IFC(m_containerManager.GetHeaderAtValidIndex(validHeaderIndex, &spHeader));
        if (spHeader)
        {
            auto bounds = GetBoundsFromElement(spHeader);
            m_newStickyHeaderLocations.emplace_back(bounds.Y, bounds.Height);
        }
    }

    if (m_currentStickyHeaderLocations != m_newStickyHeaderLocations)
    {
        updateClipAnimation = true;
        m_currentStickyHeaderLocations.swap(m_newStickyHeaderLocations);
    }

    if (!m_tpItemClippingRelationship)
    {
        updateClipAnimation = true;

        ctl::ComPtr<SecondaryContentRelationship> spRelationship;
        ctl::ComPtr<IScrollViewer> spScrollViewer;
        ctl::ComPtr<ParametricCurveCollection> spParametricCurveCollection;

        IFC(m_wrScrollViewer.As(&spScrollViewer));

        IFC(ctl::make(&spRelationship));

        IFC(spRelationship->SetPrimaryContent(spScrollViewer.Cast<ScrollViewer>()));
        ASSERT(m_tpItemClippingTransform);
        IFC(spRelationship->SetSecondaryContent(this, m_tpItemClippingTransform.Get()));
        IFC(spRelationship->get_Curves(&spParametricCurveCollection));
        IFC(spRelationship->put_ShouldTargetClip(TRUE));

        // We'll create the curve here, but populate the segments later
        {
            ctl::ComPtr<ParametricCurve> spParametricCurve;
            wrl_wrappers::HString strPanningOffsetPropertyName;
            wrl_wrappers::HString strPanningTranslatePropertyName;

            IFC(ctl::make(&spParametricCurve));
            IFC(strPanningOffsetPropertyName.Set(STR_LEN_PAIR(L"VerticalOffset")));
            IFC(strPanningTranslatePropertyName.Set(STR_LEN_PAIR(L"TranslateY")));
            IFC(spParametricCurve->SetPrimaryContentProperty(strPanningOffsetPropertyName.Get()));
            IFC(spParametricCurve->SetSecondaryContentProperty(DirectManipulationProperty::TranslationY, strPanningTranslatePropertyName.Get()));

            IFC(spParametricCurveCollection->Append(spParametricCurve.Get()));
        }

        // Now that we've got the relationship and its curve collection, let's also add a curve to keep
        // the clipping region stationary in the non-panning direction.
        {
            ctl::ComPtr<ParametricCurve> spParametricCurve;
            ctl::ComPtr<ParametricCurveSegmentCollection> spCurveSegmentCollection;
            ctl::ComPtr<ParametricCurveSegment> spCurveSegment;
            wrl_wrappers::HString strOrthogonalOffsetPropertyName;
            wrl_wrappers::HString strOrthogonalTranslatePropertyName;

            IFC(ctl::make(&spParametricCurve));
            IFC(strOrthogonalOffsetPropertyName.Set(STR_LEN_PAIR(L"HorizontalOffset")));
            IFC(strOrthogonalTranslatePropertyName.Set(STR_LEN_PAIR(L"TranslateX")));
            IFC(spParametricCurve->SetPrimaryContentProperty(strOrthogonalOffsetPropertyName.Get()));
            IFC(spParametricCurve->SetSecondaryContentProperty(DirectManipulationProperty::TranslationX, strOrthogonalTranslatePropertyName.Get()));

            IFC(spParametricCurve->get_CurveSegments(&spCurveSegmentCollection));
            IFC(ctl::make(&spCurveSegment)); // Default init to all zeroes
            IFC(spCurveSegmentCollection->Append(spCurveSegment.Get()));

            IFC(spParametricCurveCollection->Append(spParametricCurve.Get()));
        }

        SetPtrValue(m_tpItemClippingRelationship, spRelationship);
    }

    if (updateClipAnimation)
    {
        ctl::ComPtr<ParametricCurveSegmentCollection> spCurveSegmentCollection;

        // First, ensure we're all referring to the correct transform object
        IFC(spClip->put_Transform(m_tpItemClippingTransform.Get()));

        // Extract the target curve and its collection of curve segments
        {
            ctl::ComPtr<IDependencyObject> spParametricCurveAsIDO;
            ctl::ComPtr<ParametricCurveCollection> spParametricCurveCollection;
            UINT curveCollectionSize;

            IFC(m_tpItemClippingRelationship->get_Curves(&spParametricCurveCollection));
            IFC(spParametricCurveCollection->get_Size(&curveCollectionSize));

            // We should only have a curve for panning and an orthogonal curve
            // The curve we are modifying is the last one in the collection
            ASSERT(curveCollectionSize == 2);
            IFC(spParametricCurveCollection->GetAt(0, &spParametricCurveAsIDO));

            IFC(spParametricCurveAsIDO.Cast<ParametricCurve>()->get_CurveSegments(&spCurveSegmentCollection));
        }
        IFC(spCurveSegmentCollection->Clear());

        if (!m_currentStickyHeaderLocations.empty())
        {
            // Make sure we account for the listview/ScrollViewer header. If it exists
            // we want this function animation to begin with a region that's not animating
            if (m_currentHeaderHeight + m_currentStickyHeaderLocations[0].m_offset > 0)
            {
                ctl::ComPtr<ParametricCurveSegment> spCurveSegment;
                IFC(ctl::make(&spCurveSegment));
                IFC(spCurveSegment->put_BeginOffset(0));
                IFC(spCurveSegment->put_ConstantCoefficient(0));
                IFC(spCurveSegment->put_LinearCoefficient(0));
                IFC(spCurveSegmentCollection->Append(spCurveSegment.Get()));
            }

            // Now we walk through each header, and ensure the clipping region is in the right place for each
            DOUBLE previousHeaderSize = 0;
            DOUBLE previousLinearCurveBeginOffset = 0;
            DOUBLE previousConstantCurveCoefficient = 0;
            for (const auto& currentHeaderLocation : m_currentStickyHeaderLocations)
            {
                ctl::ComPtr<ParametricCurveSegment> spCurveSegment;

                DOUBLE constantCurveBeginOffset = currentHeaderLocation.m_offset - previousHeaderSize + m_currentHeaderHeight;
                DOUBLE constantCurveCoefficient = currentHeaderLocation.m_offset + currentHeaderLocation.m_size;

                IFC(ctl::make(&spCurveSegment));
                IFC(spCurveSegment->put_BeginOffset(constantCurveBeginOffset));
                IFC(spCurveSegment->put_ConstantCoefficient(-constantCurveCoefficient));
                IFC(spCurveSegment->put_LinearCoefficient(0));
                IFC(spCurveSegmentCollection->Append(spCurveSegment.Get()));

                DOUBLE linearCurveBeginOffset = currentHeaderLocation.m_offset + m_currentHeaderHeight;

                IFC(ctl::make(&spCurveSegment));
                IFC(spCurveSegment->put_BeginOffset(linearCurveBeginOffset));
                IFC(spCurveSegment->put_ConstantCoefficient(-constantCurveCoefficient));
                IFC(spCurveSegment->put_LinearCoefficient(-1));
                IFC(spCurveSegmentCollection->Append(spCurveSegment.Get()));

                previousHeaderSize = currentHeaderLocation.m_size;
                previousLinearCurveBeginOffset = linearCurveBeginOffset;
                previousConstantCurveCoefficient = constantCurveCoefficient;
            }
        }

        // Synchronize the change of the clip manipulation curve.
        // See extensive banner comments on CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
        IFC(m_tpItemClippingRelationship->PrepareForCurveUpdate());
        IFC(m_tpItemClippingRelationship->Apply());
    }

    if (manuallyUpdatePosition)
    {
        // DManip wasn't responsible for the ScrollViewer change, so we need to manually evaluate the secondary content curve
        IFC(m_tpItemClippingRelationship->UpdateDependencyProperties());
    }

Cleanup:
    RRETURN(hr);
}

// Remove sticky header transformation
void ModernCollectionBasePanel::RemoveStickyHeader(_In_ ctl::ComPtr<IUIElement>& spHeader)
{
    GetVirtualizationInformationFromElement(spHeader)->m_stickyHeaderWrapper.reset();
}

// Remove all sticky headers transformations
_Check_return_ HRESULT ModernCollectionBasePanel::RemoveStickyHeaders()
{
    HRESULT hr = S_OK;

    for (INT groupIndex = m_containerManager.EndOfHeaderVisualSection() - 1;
            groupIndex >= m_containerManager.StartOfHeaderVisualSection();
            groupIndex--)
    {
        ctl::ComPtr<xaml::IUIElement> spHeader;
        IFC(m_containerManager.GetHeaderAtValidIndex(groupIndex, &spHeader));

        if (spHeader)
        {
            RemoveStickyHeader(spHeader);
        }
    }

    m_tpItemClippingTransform.Clear();
    if (m_tpItemClippingRelationship)
    {
        IFC(m_tpItemClippingRelationship->Remove());
    }
    m_tpItemClippingRelationship.Clear();

    IFC(ClearValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_Clip)));

Cleanup:
    RRETURN(hr);
}

// Computes the vertical position of a sticky header
DOUBLE ModernCollectionBasePanel::GetStickyHeaderPositionY(
    _In_ DOUBLE groupTopY,
    _In_ DOUBLE groupInnerHeight,
    _In_ DOUBLE visibleTopY)
{
    if (groupTopY > visibleTopY)
    {
        // When the group top is below visible top, it does not stick
        return groupTopY;
    }
    else
    {
        // This is the maximum position of the header (i.e. just above next group)
        DOUBLE maxTop = groupTopY + groupInnerHeight;
        // Therefore the constrained value is either the top of visible window
        // or this one.
        return DoubleUtil::Min(visibleTopY, maxTop);
    }
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetRealHeaderExtentOutsideVisibleWindow(_Out_ FLOAT *headerExtentOutsideVisibleWindow)
{
    ASSERT(m_bUseStickyHeaders);
    *headerExtentOutsideVisibleWindow = 0;

    if (m_containerManager.IsGroupIndexWithinValidRange(m_firstVisibleGroupIndexBase))
    {
        ctl::ComPtr<IUIElement> spRealHeader;
        IFC_RETURN(m_containerManager.GetHeaderAtGroupIndex(m_firstVisibleGroupIndexBase, &spRealHeader));
        // This method could be invoked between a collection change and the following layout, which
        // means there could be sentinels in the realized range.
        if (spRealHeader)
        {
            const wf::Rect realHeaderBounds = GetBoundsFromElement(spRealHeader);
            wf::Rect visibleWindow = m_windowState.GetVisibleWindow();

            // calculate the part of the real (not sticky) header that is outside the visible window.
            // It could be completely above the visible window or a part of it could be within
            // the visible window. We need the part that is outside the visible window
            // since that space will be pushed down to show the full sticky header.
            auto realHeaderDistanceFromVisibleWindow = DoubleUtil::Max(visibleWindow.Y - realHeaderBounds.Y, 0.0f);
            *headerExtentOutsideVisibleWindow = static_cast<FLOAT>(DoubleUtil::Min(realHeaderDistanceFromVisibleWindow, realHeaderBounds.Height));
        }
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT ModernCollectionBasePanel::GetVerticalViewportPadding(_Out_ FLOAT *verticalViewportPadding)
{
    *verticalViewportPadding = 0;

    if (m_bUseStickyHeaders)
    {
        IFC_RETURN(GetRealHeaderExtentOutsideVisibleWindow(verticalViewportPadding));
    }

    RRETURN(S_OK);
}

// Computes the offsets resulting from Sticky Headers
// Note: we take as a prerequisite here that all sticky headers are correcly configured and we are just
// applying the same computation than the Parametric Curve does
_Check_return_ HRESULT ModernCollectionBasePanel::CoerceStickyHeaderOffsets(
    _In_ ListViewBaseHeaderItem *pHeaderItem,
    _In_ INT cOffsets,
    _Inout_updates_(cOffsets) DOUBLE *pOffsets)
{
    HRESULT hr = S_OK;

    if (m_bUseStickyHeaders && (cOffsets > 0))
    {
        VirtualizationInformation *pVirtualizationInformation = pHeaderItem->GetVirtualizationInformation();
        ctl::ComPtr<IScrollViewer> spScrollViewer = m_wrScrollViewer.AsOrNull<IScrollViewer>();
        if (spScrollViewer && pVirtualizationInformation && pVirtualizationInformation->GetIsSticky())
        {
            auto pWrapper = pVirtualizationInformation->m_stickyHeaderWrapper.get();
            DOUBLE visibleTopY = 0.0;
            DOUBLE groupBottomY = pWrapper->GetGroupBottom();
            DOUBLE groupTopY = pWrapper->GetDesiredBounds().Y;
            DOUBLE groupHeaderHeight = pWrapper->GetDesiredBounds().Height;
            DOUBLE groupInnerHeight = groupBottomY - groupTopY - groupHeaderHeight;

            IFC(spScrollViewer->get_VerticalOffset(&visibleTopY));

            for (INT i = 0; i < cOffsets; i++)
            {
                DOUBLE newTopY = GetStickyHeaderPositionY(groupTopY + pOffsets[i], groupInnerHeight, visibleTopY);
                pOffsets[i] = newTopY - groupTopY;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ModernCollectionBasePanel::NotifyLayoutTransitionStart(
    _In_ ListViewBaseHeaderItem* pHeaderItem)
{
    if (m_bUseStickyHeaders && pHeaderItem->GetVirtualizationInformation()->GetIsSticky())
    {
        IFC_RETURN(pHeaderItem->GetVirtualizationInformation()->m_stickyHeaderWrapper->NotifyLayoutTransitionStart());
    }
    return S_OK;
}

_Check_return_ HRESULT ModernCollectionBasePanel::NotifyLayoutTransitionEnd(
    _In_ ListViewBaseHeaderItem* pHeaderItem)
{
    if (m_bUseStickyHeaders && pHeaderItem->GetVirtualizationInformation()->GetIsSticky())
    {
        IFC_RETURN(pHeaderItem->GetVirtualizationInformation()->m_stickyHeaderWrapper->NotifyLayoutTransitionEnd());
    }
    return S_OK;
}
