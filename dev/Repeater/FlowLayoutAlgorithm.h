// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementManager.h"
#include "IFlowLayoutAlgorithmDelegates.h"
#include "OrientationBasedMeasures.h"

class FlowLayoutAlgorithm : OrientationBasedMeasures
{
public:
    // Types
    enum class LineAlignment
    {
        Start,
        Center,
        End,
        SpaceAround,
        SpaceBetween,
        SpaceEvenly
    };

    FlowLayoutAlgorithm(const ITrackerHandleManager* owner) :
        m_owner(owner),
        m_elementManager(owner),
        m_context(owner)
    { }

    // Methods
    winrt::Rect LastExtent() const { return m_lastExtent; }

    void InitializeForContext(const winrt::VirtualizingLayoutContext& context, IFlowLayoutAlgorithmDelegates* callbacks);
    void UninitializeForContext(const winrt::VirtualizingLayoutContext& context);

    winrt::Size Measure(
        const winrt::Size& availableSize,
        const winrt::VirtualizingLayoutContext& context,
        bool isWrapping,
        double minItemSpacing,
        double lineSpacing,
        unsigned int maxItemsPerLine,
        const ScrollOrientation& orientation,
        const bool disableVirtualization,
        const wstring_view& layoutId);
    winrt::Size Arrange(
        const winrt::Size& finalSize,
        const winrt::VirtualizingLayoutContext& context,
        bool isWrapping,
        FlowLayoutAlgorithm::LineAlignment lineAlignment,
        const wstring_view& layoutId);
    void OnItemsSourceChanged(
        const winrt::IInspectable& source,
        winrt::NotifyCollectionChangedEventArgs const& args,
        const winrt::IVirtualizingLayoutContext& context);

    winrt::Size MeasureElement(
        const winrt::UIElement& element,
        int index,
        const winrt::Size& availableSize,
        const winrt::VirtualizingLayoutContext& context);

    winrt::UIElement GetElementIfRealized(int dataindex);
    bool TryAddElement0(winrt::UIElement const& element);

private:
    // Types
    enum class GenerateDirection
    {
        Forward,
        Backward
    };

    // Methods
#pragma region Measure related private methods
    int GetAnchorIndex(
        const winrt::Size& availableSize,
        bool isWrapping,
        double minItemSpacing,
        const bool disableVirtualization,
        const wstring_view& layoutId);
    void Generate(
        GenerateDirection direction,
        int anchorIndex,
        const winrt::Size& availableSize,
        double minItemSpacing,
        double lineSpacing,
        unsigned int maxItemsPerLine,
        const bool disableVirtualization,
        const wstring_view& layoutId);
    void MakeAnchor(
        const winrt::VirtualizingLayoutContext& context,
        int index,
        const winrt::Size& availableSize);
    bool IsReflowRequired() const;
    bool ShouldContinueFillingUpSpace(
        int index,
        GenerateDirection direction);
    winrt::Rect EstimateExtent(const winrt::Size& availableSize, const wstring_view& layoutId);
    void RaiseLineArranged();
#pragma endregion

#pragma region Arrange related private methods
    void ArrangeVirtualizingLayout(
        const winrt::Size& finalSize,
        FlowLayoutAlgorithm::LineAlignment lineAlignment,
        bool isWrapping,
        const wstring_view& layoutId);
    void PerformLineAlignment(
        int lineStartIndex,
        int countInLine,
        float spaceAtLineStart,
        float spaceAtLineEnd,
        float lineSize,
        FlowLayoutAlgorithm::LineAlignment lineAlignment,
        bool isWrapping,
        const winrt::Size& finalSize,
        const wstring_view& layoutId);
#pragma endregion

#pragma region Layout Context Helpers
    winrt::Rect RealizationRect();
    void SetLayoutOrigin();
#pragma endregion

    bool IsVirtualizingContext();

    // Fields
    const ITrackerHandleManager* m_owner;
    ::ElementManager m_elementManager;
    winrt::Size m_lastAvailableSize{};
    double m_lastItemSpacing{};
    bool m_collectionChangePending{};
    tracker_ref<winrt::VirtualizingLayoutContext> m_context;
    IFlowLayoutAlgorithmDelegates* m_algorithmCallbacks{ nullptr };
    winrt::Rect m_lastExtent{};
    int m_firstRealizedDataIndexInsideRealizationWindow{ -1 };
    int m_lastRealizedDataIndexInsideRealizationWindow{ -1 };

    // If the scroll orientation is the same as the folow orientation
    // we will only have one line since we will never wrap. In that case
    // we do not want to align the line. We could potentially switch the
    // meaning of line alignment in this case, but I'll hold off on that
    // feature until someone asks for it - This is not a common scenario
    // anyway.
    bool m_scrollOrientationSameAsFlow{ false };
};
