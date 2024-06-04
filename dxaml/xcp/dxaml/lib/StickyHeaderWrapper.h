// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A helper class for managing various data and lifetimes associated with sticky headers

#pragma once

namespace DirectUI
{
    class ScrollViewer;
    class CompositeTransform;

    class StickyHeaderWrapper : public std::enable_shared_from_this<StickyHeaderWrapper>
    {
    public:
        StickyHeaderWrapper(const wf::Rect& desiredBounds, DOUBLE groupBottom, DOUBLE panelOffset, DOUBLE pannableExtent);
        ~StickyHeaderWrapper();

        _Check_return_ HRESULT Initialize(
            _In_ DirectUI::UIElement* pHeader,
            _In_ DirectUI::Panel* pPanel,
            _In_ DirectUI::ScrollViewer* pScrollViewer,
            _In_ DirectUI::UIElement* pLTEParent,
            _In_ xaml::Internal::ISecondaryContentRelationshipStatics* pSecondaryContentRelationshipStatics);

        _Check_return_ HRESULT Clear();

        _Check_return_ HRESULT SetBounds(
            _In_ const wf::Rect& desiredBounds, DOUBLE groupBottom, DOUBLE panelOffset, DOUBLE pannableExtent, _Out_ bool* pCurveUpdated);

        _Check_return_ HRESULT SetDesiredBounds(_In_ const wf::Rect& desiredBounds);

        const ctl::ComPtr<xaml::Internal::ISecondaryContentRelationship>& GetRelationship() const { return m_spRelationship; }
        DOUBLE GetGroupBottom() const { return m_groupBottom; }
        DOUBLE GetPanelOffset() const { return m_panelOffset; }
        DOUBLE GetPannableExtent() const { return m_pannableExtent; }
        const wf::Rect& GetDesiredBounds() const { return m_desiredBounds; }

        _Check_return_ HRESULT NotifyLayoutTransitionStart();
        _Check_return_ HRESULT NotifyLayoutTransitionEnd();

        // we call this method to apply render transform to parent Header to align it with the right position on the screen.
        // note that during direct manipulation parent's position will be off and TransformToVisual API will return wrong data.
        _Check_return_ HRESULT UpdateHeaderPosition();

    private:
        void UpdateLTEPosition();

        // Compensate for overpan compression.
        //
        // When overpan compression is in effect, the default DManip overpan behavior is turned off.
        // Secondary content curves are used to create the overpan compression effect.
        // The sticky header curves are not aware of the secondary content curves for overpan compression.
        // Thus, if we do not compensate for them here, the Sticky Header curves have no knowledge of
        // content boundaries, and will continue to produce a speedbump effect even when we are panning
        // beyond the pannable range.
        //
        // We don't want this - we want to make sure the sticky header curves are only in effect for the pannable range.
        // Thus, we must clamp the sticky header curve coordinates at the pannable range.
        void GetEffectiveDimensions(_Out_ DOUBLE* pEffectiveTop, _Out_ DOUBLE* pEffectiveBottom);

    private:
        ctl::ComPtr<DirectUI::UIElement> m_spLayoutTransitionElement;
        ctl::WeakRefPtr m_wrTarget;
        ctl::WeakRefPtr m_wrTargetHost;
        CUIElement* m_pLTEParentNoRef;

        ctl::ComPtr<DirectUI::CompositeTransform> m_spTransform;
        ctl::ComPtr<xaml::Internal::ISecondaryContentRelationship> m_spRelationship;
        wf::Rect m_desiredBounds;
        DOUBLE m_groupBottom;
        DOUBLE m_panelOffset;
        DOUBLE m_pannableExtent;

        ctl::EventPtr<StoryboardCompletedEventCallback> m_epStoryboardCompleted;
        ctl::ComPtr<xaml_animation::IStoryboard> m_spStoryboard;
    };
}
