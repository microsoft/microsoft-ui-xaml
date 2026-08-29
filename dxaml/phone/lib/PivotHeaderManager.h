// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <PivotCommon.h>
#include <PivotCurveGenerator.h>
#include <PivotStaticContentCurve.h>
#include "PivotHeaderManagerCallbacks.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

class PivotHeaderManager
    : public IPivotHeaderManagerPanelEvents
    , public IPivotHeaderManagerItemEvents
{
public:
    PivotHeaderManager(IPivotHeaderManagerCallbacks* pCallbacks, Private::ReferenceTrackerHelper<Pivot> referenceTrackerHelper);

    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT ApplyTemplateEvent(_In_ xaml_controls::IPanel* pPanel, _In_ xaml_controls::IPanel* pStaticPanel);
    _Check_return_ HRESULT ItemsCollectionChangedEvent(_In_ wfc::IVector<IInspectable*>* pItems,
        _In_ INT32 newItemCount, _In_ INT32 changeIdx, _In_ wfc::CollectionChange changeType);
    _Check_return_ HRESULT HeaderTemplateChangedEvent();
    _Check_return_ HRESULT IsLockedChangedEvent(_In_ BOOLEAN isLocked);
    _Check_return_ HRESULT UnloadedEvent();
    _Check_return_ HRESULT HeaderStateChangedEvent(_In_ bool useStaticHeaders);
    _Check_return_ HRESULT FocusStateChangedEvent(_In_ bool shouldShowFocusStateOnSelectedItem);

    _Check_return_ HRESULT SetSelectedIndex(_In_ INT32 idx, _In_ PivotAnimationDirection animationHint);

    _Check_return_ HRESULT ApplyParallax(
        _In_ xaml_controls::IScrollViewer* pScrollViewer,
        _In_ xaml_controls::IPanel* pPanel,
        _In_ xaml_media::ICompositeTransform* pTransform,
        _In_ DOUBLE sectionOffset,
        _In_ DOUBLE sectionWidth,
        _In_ bool isHeaderPanelInsideLayoutElementTemplatePart,
        _In_ bool isDynamicHeader,
        _In_ float viewportSize);
    _Check_return_ HRESULT SyncParallax();

    _Check_return_ HRESULT ApplyStaticLayoutRelationship(
        _In_ xaml_controls::IScrollViewer* scrollViewer,
        _In_ xaml::IUIElement* element,
        _In_ xaml_media::ICompositeTransform* transform,
        _In_ double sectionWidth);

    _Check_return_ HRESULT ApplyInverseStaticLayoutRelationship(
        _In_ xaml_controls::IScrollViewer* scrollViewer,
        _In_ xaml::IUIElement* element,
        _In_ xaml_media::ICompositeTransform* transform,
        _In_ double sectionWidth);

    _Check_return_ HRESULT GetHeaderWidths(_Out_ std::vector<double> *pHeaderWidths, _Out_opt_ double *pTotalWidth = nullptr);

    _Check_return_ HRESULT Dispose();

#pragma region IPivotHeaderManagerItemEvents
    bool GetIsLocked() const
    {
        return m_isLocked == TRUE;
    }
    _Check_return_ HRESULT PivotHeaderItemTapped(_In_ xaml_primitives::IPivotHeaderItem* pItem) override;
    _Check_return_ HRESULT VsmUnlockedStateChangeCompleteEvent() override;
#pragma endregion

#pragma region IPivotHeaderManagerPanelEvents
    _Check_return_ HRESULT HeaderPanelMeasureEvent(float viewportSize) override;
    _Check_return_ HRESULT HeaderPanelSetLteOffsetEvent(_In_ DOUBLE primaryHorizontalOffset, _In_ DOUBLE ghostHorizontalOffset, _In_ DOUBLE verticalOffset) override;
#pragma endregion

private:
    _Check_return_ HRESULT CreateHeaderItem(_Outptr_ xaml_primitives::IPivotHeaderItem** ppHeaderItem);
    _Check_return_ HRESULT ClearExistingLtes();
    _Check_return_ HRESULT UpdateGhostItem();
    _Check_return_ HRESULT InsertIntoPanel(_In_ xaml_primitives::IPivotHeaderItem* pItem, _In_ INT32 idx);
    _Check_return_ HRESULT RemoveFromPanel(_In_ INT32 idx);
    _Check_return_ HRESULT ClearPanel();
    _Check_return_ HRESULT UpdateItemsStates();

    _Check_return_ HRESULT UpdateCurveHeaderItemSizes();

    _Check_return_ static HRESULT SetBinding(_In_ IInspectable* pItem, _In_ xaml_primitives::IPivotHeaderItem* pHeaderItem);

    template <typename T>
    _Check_return_ HRESULT SetPtrValue(_In_ Private::TrackerPtr<T>& ptr, _In_ T* value)
    {
        RRETURN(m_referenceTrackerHelper.SetPtrValue(ptr, value));
    }
    template <typename T>
    _Check_return_ HRESULT RegisterTrackerPtrVector(_In_ Private::TrackerPtrVector<T>& ptrVector)
    {
        RRETURN(m_referenceTrackerHelper.RegisterTrackerPtrVector(ptrVector));
    }

    _Check_return_ HRESULT ApplyStaticRelationship(
        _In_ xaml_controls::IScrollViewer* pScrollViewer,
        _In_ xaml::IUIElement* pElement,
        _In_ xaml_media::ICompositeTransform* pTransform,
        _In_ bool isInverted,
        _In_ double sectionWidth,
        _Inout_ Private::TrackerPtr<xaml::Internal::ISecondaryContentRelationship> &tpRelationship);

    _Check_return_ HRESULT DetachCallbackPointers();

    Private::ReferenceTrackerHelper<Pivot> m_referenceTrackerHelper;

    PivotCurveGenerator m_curveGenerator;
    PivotStaticContentCurve m_staticContentCurve;
    PivotStaticContentCurve m_staticContentInverseCurve;
    Private::TrackerPtrVector<xaml_primitives::IPivotHeaderItem> m_tpHeaderItems;
    Private::TrackerPtr<wfc::IVector<IInspectable*>> m_tpItems;

    IPivotHeaderManagerCallbacks* m_pCallbackPtr;
    Private::TrackerPtr<xaml_controls::IPanel> m_tpPanel;
    Private::TrackerPtr<xaml_controls::IPanel> m_tpStaticPanel;
    Private::TrackerPtr<xaml::Internal::ISecondaryContentRelationship> m_tpDynamicHeaderRelationship;
    Private::TrackerPtr<xaml::Internal::ISecondaryContentRelationship> m_tpStaticHeaderRelationship;
    Private::TrackerPtr<xaml::Internal::ISecondaryContentRelationship> m_tpStaticLayoutRelationship;
    Private::TrackerPtr<xaml::Internal::ISecondaryContentRelationship> m_tpInverseStaticLayoutRelationship;

#pragma region LayoutTransitionElment State
    // Because LTEs cause the original visual to disappear we create
    // two of them, one to stand where the original was placed and
    // one to be the 'ghost' item to the very left.
    Private::TrackerPtr<xaml::IUIElement> m_tpPrimaryLte;
    Private::TrackerPtr<xaml::IUIElement> m_tpGhostLte;
    Private::TrackerPtr<xaml_media::ITranslateTransform> m_tpPrimaryLteTranslateTransform;
    Private::TrackerPtr<xaml_media::ITranslateTransform> m_tpGhostLteTranslateTransform;

    // The LTE API is a little awkward and requires you
    // to pass in the parent and source of the LTE at
    // destruction. We hold references to those values here
    // for cases when the collection or panel has changed
    // before we update the LTE.
    Private::TrackerPtr<xaml::IUIElement> m_tpLteParent;
    Private::TrackerPtr<xaml::IUIElement> m_tpLteSource;
#pragma endregion

    INT32 m_currentIndex;
    BOOLEAN m_isLocked;
    bool m_useStaticHeaders;
    bool m_shouldShowFocusStateOnSelectedItem;
};

} } } } XAML_ABI_NAMESPACE_END
