// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Contains common logic between the stacking and wrapping layout
//      strategies.

#pragma once

#include <EnumDefs.g.h>

namespace DirectUI { namespace Components { namespace Moco {

    class LayoutStrategyBase
    {
    public:

        LayoutStrategyBase(_In_ bool useFullWidthHeaders, _In_ bool isWrapping)
            : m_useFullWidthHeaders(useFullWidthHeaders)
            , m_isWrapping(isWrapping)
            , m_pDataInfoProviderNoRef(nullptr)
            , m_virtualizationDirection(xaml_controls::Orientation_Horizontal)
            , m_groupHeaderStrategy()
            , m_groupPadding()
        { }

        bool IsGrouping() const { return m_groupHeaderStrategy != GroupHeaderStrategy::None; }
        bool GetIsWrappingStrategy() const { return m_isWrapping; }
        
        xaml::Thickness GetGroupPadding() const { return m_groupPadding; }
        void SetGroupPadding(xaml::Thickness padding) { m_groupPadding = padding; }

        GroupHeaderStrategy GetGroupHeaderStrategy() const { return m_groupHeaderStrategy; }
        void SetGroupHeaderStrategy(GroupHeaderStrategy strategy) { m_groupHeaderStrategy = strategy; }

        xaml_controls::Orientation GetVirtualizationDirection() const { return m_virtualizationDirection; }
        void SetVirtualizationDirection(xaml_controls::Orientation direction) { m_virtualizationDirection = direction; }

        bool UseFullWidthHeaders() const { return m_useFullWidthHeaders; }

        xaml_controls::ILayoutDataInfoProvider* GetLayoutDataInfoProviderNoRef() const { return m_pDataInfoProviderNoRef; }
        void SetLayoutDataInfoProviderNoRef(_In_ xaml_controls::ILayoutDataInfoProvider *pDataInfoProvider) { m_pDataInfoProviderNoRef = pDataInfoProvider; }

        // Used by unit tests only.
        void SetUseFullWidthHeaders(_In_ bool useFullWidthHeaders) { m_useFullWidthHeaders = useFullWidthHeaders; }

    protected:

        // a redirection to allow us to abstract the direction we care about
        // basically this will return a function to a member variable, switched on the orientation
        float wf::Point::* PointInNonVirtualizingDirection () const;
        float wf::Point::* PointInVirtualizingDirection () const;

        float wf::Size::* SizeInNonVirtualizingDirection () const;
        float wf::Size::* SizeInVirtualizingDirection () const;

        float wf::Rect::* PointFromRectInNonVirtualizingDirection () const;
        float wf::Rect::* PointFromRectInVirtualizingDirection () const;
        float wf::Rect::* SizeFromRectInNonVirtualizingDirection () const;
        float wf::Rect::* SizeFromRectInVirtualizingDirection () const;

        wf::Size GetGroupPaddingAtStart() const;
        wf::Size GetGroupPaddingAtEnd() const;

        // Determine if a point is inside the window, or is before or after it in the virtualizing direction.
        RelativePosition GetReferenceDirectionFromWindow(_In_ wf::Rect referenceRect, _In_ wf::Rect window) const;

        static int GetRemainingGroups(_In_ int referenceGroupIndex, _In_ int totalGroups, _In_ RelativePosition positionOfReference);
        static int GetRemainingItems(_In_ int referenceItemIndex, _In_ int totalItems, _In_ RelativePosition positionOfReference);

        static const int c_specialGroupIndex = 0;
        static const int c_specialItemIndex = 0;

    private:
        // direction in which we layout out items
        xaml_controls::Orientation m_virtualizationDirection;

        // extra space we surround every group with
        // When we give back a size for measure/arrange, we will include this padding around top & bottom.
        // then during container/header layout we will just ensure that a padding has been applied in between
        xaml::Thickness m_groupPadding;

        GroupHeaderStrategy m_groupHeaderStrategy;
        bool m_isWrapping;
        bool m_useFullWidthHeaders;

        xaml_controls::ILayoutDataInfoProvider *m_pDataInfoProviderNoRef;
    };

} } }
