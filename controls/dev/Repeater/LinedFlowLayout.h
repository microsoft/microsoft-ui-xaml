// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ElementManager.h"
#include "VirtualizingLayout.h"
#include "LinedFlowLayoutTrace.h"
#include "LinedFlowLayout.g.h"
#include "LinedFlowLayout.properties.h"
#include "LinedFlowLayoutItemAspectRatios.h"

class LinedFlowLayout :
    public ReferenceTracker<LinedFlowLayout, winrt::implementation::LinedFlowLayoutT, VirtualizingLayout>,
    public LinedFlowLayoutProperties
{
public:
    LinedFlowLayout();
    ~LinedFlowLayout();

    // Properties' default values.
    static constexpr winrt::LinedFlowLayoutItemsJustification s_defaultItemsJustification{ winrt::LinedFlowLayoutItemsJustification::Start };
    static constexpr winrt::LinedFlowLayoutItemsStretch s_defaultItemsStretch{ winrt::LinedFlowLayoutItemsStretch::None };
    static constexpr double s_defaultActualLineHeight{ 0.0 };
    static constexpr double s_defaultLineSpacing{ 0.0 };
    static constexpr double s_defaultLineHeight{ std::numeric_limits<double>::quiet_NaN() };
    static constexpr double s_defaultMinItemSpacing{ 0.0 };

#pragma region ILayoutOverrides
    winrt::ItemCollectionTransitionProvider CreateDefaultItemTransitionProvider() override;
#pragma endregion

#pragma region IVirtualizingLayoutOverrides
    void InitializeForContextCore(
        winrt::VirtualizingLayoutContext const& context);
    void UninitializeForContextCore(
        winrt::VirtualizingLayoutContext const& context);

    winrt::Size MeasureOverride(
        winrt::VirtualizingLayoutContext const& context,
        winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(
        winrt::VirtualizingLayoutContext const& context,
        winrt::Size const& finalSize);

    void OnItemsChangedCore(
        winrt::VirtualizingLayoutContext const& context,
        winrt::IInspectable const& source,
        winrt::NotifyCollectionChangedEventArgs const& args);
#pragma endregion

    void SetDesiredAspectRatios(winrt::array_view<double const> const& values)
    {
        m_itemsInfoDesiredAspectRatiosForFastPath = { values.begin(), values.end() };
    }

    void SetMinWidths(winrt::array_view<double const> const& values)
    {
        m_itemsInfoMinWidthsForFastPath = { values.begin(), values.end() };
    }

    void SetMaxWidths(winrt::array_view<double const> const& values)
    {
        m_itemsInfoMaxWidthsForFastPath = { values.begin(), values.end() };
    }

    int RequestedRangeStartIndex();
    int RequestedRangeLength();

    void InvalidateItemsInfo();

    int32_t LockItemToLine(
        int32_t itemIndex);

    void OnPropertyChanged(
        winrt::DependencyPropertyChangedEventArgs const& args);

#pragma region LayoutsTestHooks
    int FirstRealizedItemIndexDbg() const
    {
        if (!m_wasInitializedForContext)
        {
            return -1;
        }

        return m_elementManager.GetFirstRealizedDataIndex();
    };

    int LastRealizedItemIndexDbg() const
    {
        if (!m_wasInitializedForContext)
        {
            return -1;
        }

        const int realizedElementCount = m_elementManager.GetRealizedElementCount();
        return realizedElementCount == 0 ? -1 : m_elementManager.GetFirstRealizedDataIndex() + realizedElementCount - 1;
    };

    int FirstFrozenItemIndexDbg() const
    {
        return m_firstFrozenItemIndex;
    };

    int LastFrozenItemIndexDbg() const
    {
        return m_lastFrozenItemIndex;
    };

    double AverageItemAspectRatioDbg() const
    {
        return m_averageItemAspectRatioDbg;
    };

    double ForcedAverageItemAspectRatioDbg() const
    {
        return m_forcedAverageItemAspectRatioDbg;
    };

    void ForcedAverageItemAspectRatioDbg(
        double forcedAverageItemAspectRatio)
    {
        if (m_forcedAverageItemAspectRatioDbg != forcedAverageItemAspectRatio)
        {
            m_forcedAverageItemAspectRatioDbg = forcedAverageItemAspectRatio;
            InvalidateLayout();
        }
    };

    double ForcedAverageItemsPerLineDividerDbg() const
    {
        return m_forcedAverageItemsPerLineDividerDbg;
    };

    void ForcedAverageItemsPerLineDividerDbg(
        double forcedAverageItemsPerLineDivider)
    {
        if (m_forcedAverageItemsPerLineDividerDbg != forcedAverageItemsPerLineDivider)
        {
            m_forcedAverageItemsPerLineDividerDbg = forcedAverageItemsPerLineDivider;
            InvalidateLayout();
        }
    };

    double ForcedWrapMultiplierDbg() const
    {
        return m_forcedWrapMultiplierDbg;
    };

    // Allows to change LinedFlowLayout::GetItemWidthMultiplierThreshold()'s return value for testing purposes.
    void ForcedWrapMultiplierDbg(
        double forcedWrapMultiplier)
    {
        if (m_forcedWrapMultiplierDbg != forcedWrapMultiplier)
        {
            m_forcedWrapMultiplierDbg = forcedWrapMultiplier;
            InvalidateLayout();
        }
    };

    bool IsFastPathSupportedDbg() const
    {
        return m_isFastPathSupportedDbg;
    };

    // Allows the fall path layout to be turned off for testing purposes.
    // This allows for instance to provide sizing information for an entire
    // small source collection and still exercise the regular path.
    void IsFastPathSupportedDbg(
        bool isFastPathSupported)
    {
        if (m_isFastPathSupportedDbg != isFastPathSupported)
        {
            m_isFastPathSupportedDbg = isFastPathSupported;

            if (UsesFastPathLayout())
            {
                ResetItemsInfo();
            }

            InvalidateLayout();
        }
    };

    void InvalidateLayout(
        bool forceRelayout = true,
        bool resetItemsInfo = false,
        bool invalidateMeasure = true);

    int LogItemIndexDbg() const
    {
        return m_logItemIndexDbg;
    };

    void LogItemIndexDbg(
        int logItemIndex)
    {
        m_logItemIndexDbg = logItemIndex;
    };

    double RawAverageItemsPerLineDbg() const
    {
        return m_averageItemsPerLine.first;
    };

    double SnappedAverageItemsPerLineDbg() const
    {
        return m_averageItemsPerLine.second;
    };

    int GetLineIndexDbg(int itemIndex) const
    {
        return GetLineIndex(itemIndex, UsesFastPathLayout());
    }

    void ClearItemAspectRatios();
    void UnlockItems();
#pragma endregion

private:
    // Structs
    struct ItemsInfo
    {
    public:
        int m_itemsRangeStartIndex{ -1 };
        int m_itemsRangeLength{ -1 };
        double m_minWidth{ -1.0 };
        double m_maxWidth{ -1.0 };
    };

    struct ItemsLayout
    {
    public:
        std::vector<int> m_lineItemCounts{};
        std::vector<double> m_lineItemWidths{};
        double m_availableLineItemsWidth{};
        double m_drawback{};
        double m_smallestHeadItemWidth{};
        double m_smallestTailItemWidth{};
        double m_bestEqualizingHeadItemDrawbackImprovement{};
        double m_bestEqualizingTailItemDrawbackImprovement{};
        int m_smallestHeadItemIndex{};
        int m_smallestTailItemIndex{};
        int m_smallestHeadLineIndex{};
        int m_smallestTailLineIndex{};
        int m_bestEqualizingHeadItemIndex{};
        int m_bestEqualizingTailItemIndex{};
        int m_bestEqualizingHeadLineIndex{};
        int m_bestEqualizingTailLineIndex{};
    };

    // Constants
    static constexpr std::wstring_view s_cannotShareLinedFlowLayout{ L"LinedFlowLayout cannot be shared."sv };
    static constexpr int s_measureCountdownStart{ 5 };

    static constexpr ItemsInfo s_emptyItemsInfo{
        -1 /*itemsRangeStartIndex*/,
        -1 /*itemsRangeLength*/,
        -1.0f /*minWidth*/,
        -1.0f /*maxWidth*/
    };

    // Methods
    void ArrangeConstrainedLines(
        winrt::VirtualizingLayoutContext const& context);

    void ArrangeUnconstrainedLine(
        winrt::VirtualizingLayoutContext const& context);

    bool ComputeFrozenItemsAndLayout(
        winrt::VirtualizingLayoutContext const& context,
        std::vector<int>& oldLineItemCounts,
        std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> oldElementAvailableWidths,
        std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> oldElementDesiredWidths,
        double scrollViewport,
        double scrollOffset,
        double lineSpacing,
        double actualLineHeight,
        float availableWidth,
        float nearSizedItemsRect,
        float farSizedItemsRect,
        float nearRealizationRect,
        float farRealizationRect,
        int lineCount,
        int sizedLineCount,
        int unsizedNearItemCount,
        int sizedItemCount,
        int firstStillSizedLineIndex,
        int lastStillSizedLineIndex,
        int firstStillSizedItemIndex,
        int lastStillSizedItemIndex);

    bool ComputeFrozenItemsRange(
        double scrollViewport,
        double scrollOffset,
        double lineSpacing,
        double actualLineHeight,
        int lineCount,
        int beginSizedLineIndex,
        int endSizedLineIndex,
        int beginSizedItemIndex,
        int endSizedItemIndex,
        int& adjustedBeginSizedItemIndex,
        int& adjustedEndSizedItemIndex);

    float ComputeItemsLayoutFastPath(
        float availableWidth,
        double actualLineHeight);

    void ComputeItemsLayoutRegularPath(
        float availableWidth,
        double scrollViewport,
        double lineSpacing,
        double actualLineHeight,
        int beginSizedLineIndex,
        int endSizedLineIndex,
        int beginSizedItemIndex,
        int endSizedItemIndex,
        int beginLineVectorIndex,
        bool isLastSizedLineStretchEnabled);

    void ComputeItemsLayoutDrawback(
        double availableWidth,
        bool isLastSizedLineStretchEnabled,
        ItemsLayout& itemsLayout) const;

    void ComputeItemsLayoutWithLockedItems(
        ItemsLayout& itemsLayout,
        double availableWidth,
        double minItemSpacing,
        double actualLineHeight,
        double averageAspectRatio,
        int beginSizedLineIndex,
        int endSizedLineIndex,
        int beginSizedItemIndex,
        int endSizedItemIndex,
        int beginLineVectorIndex,
        bool isLastSizedLineStretchEnabled);

    double ComputeLineExpandFactor(
        bool forward,
        int sizedItemIndex,
        int lineItemsCount,
        double lineItemsWidth,
        double availableWidth,
        double minItemSpacings,
        double actualLineHeight,
        double averageAspectRatio);

    double ComputeLineShrinkFactor(
        bool forward,
        int sizedItemIndex,
        int lineItemsCount,
        double lineItemsWidth,
        double availableWidth,
        double minItemSpacings,
        double actualLineHeight,
        double averageAspectRatio);

    void CopyItemsInfo(
        std::vector<double> const& oldItemsInfoDesiredAspectRatios,
        std::vector<double> const& oldItemsInfoMinWidths,
        std::vector<double> const& oldItemsInfoMaxWidths,
        std::vector<float> const& oldItemsInfoArrangeWidths,
        int oldStart,
        int newStart,
        int copyCount);

    void EnsureAndMeasureItemRange(
        winrt::VirtualizingLayoutContext const& context,
        float availableWidth,
        double actualLineHeight,
        bool forward,
        int beginRealizedItemIndex,
        int endRealizedItemIndex);

    void EnsureAndResizeItemAspectRatios(
        double scrollViewport,
        double actualLineHeight,
        double lineSpacing);

    void EnsureElementAvailableWidths();

    void EnsureElementDesiredWidths();

    void EnsureElementRealized(
        bool forward,
        int itemIndex);

    void EnsureItemAspectRatios();

    void EnsureItemRange(
        bool forward,
        int beginRealizedItemIndex,
        int endRealizedItemIndex);

    void EnsureItemRangeFastPath(
        winrt::VirtualizingLayoutContext const& context,
        double actualLineHeight);

    void EnsureItemsInfoDesiredAspectRatios(
        int itemCount);

    void EnsureItemsInfoMinWidths(
        int itemCount);

    void EnsureItemsInfoMaxWidths(
        int itemCount);

    void EnsureItemsInfoArrangeWidths(
        int itemCount);

    void EnsureLineItemCounts(
        int lineCount);

    void ExitRegularPath();

    double GetArrangeWidth(
        double desiredAspectRatio,
        double minWidth,
        double maxWidth,
        double actualLineHeight,
        double scaleFactor) const;

    float GetArrangeWidthFromItemsInfo(
        int sizedItemIndex,
        double actualLineHeight,
        double averageAspectRatio,
        double scaleFactor = 1.0) const;

    double GetAverageAspectRatio(
        float availableWidth,
        double actualLineHeight) const;

    double GetAverageAspectRatioFromStorage() const;

    std::pair<double, double> GetAverageItemsPerLine(
        float availableWidth);

    double GetDesiredAspectRatioFromItemsInfo(
        int itemIndex,
        bool usesFastPathLayout) const;

    winrt::Size GetDesiredSizeForAvailableSize(
        int itemIndex,
        winrt::UIElement const& element,
        winrt::Size availableSize,
        double actualLineHeightToRestore);

    void GetFirstAndLastDisplayedLineIndexes(
        double scrollViewport,
        double scrollOffset,
        double padding,
        double lineSpacing,
        double actualLineHeight,
        int lineCount,
        bool forFullyDisplayedLines,
        int* firstDisplayedLineIndex,
        int* lastDisplayedLineIndex) const;

    void GetFirstFullyRealizedLineIndex(
        int* firstFullyRealizedLineIndex,
        int* firstItemInFullyRealizedLine) const;

    int GetFirstItemIndexInLineIndex(
        int lineVectorIndex) const;

    int GetFrozenLineIndexFromFrozenItemIndex(
        int frozenItemIndex) const;

    double GetItemDrawbackImprovement(
        double movingWidth,
        double availableWidth,
        double currentLineItemsWidth,
        double neighborLineItemsWidth,
        int currentLineIndex,
        int neighborLineIndex);

    int GetItemLineIndex(
        winrt::UIElement const& element);

    void GetItemsInfoRequestedRange(
        winrt::VirtualizingLayoutContext const& context,
        float availableWidth,
        double actualLineHeight,
        int* itemsRangeStartIndex,
        int* itemsRangeRequestedLength);

    double GetItemWidthMultiplierThreshold() const;

    ItemsLayout GetItemsLayout(
        std::map<int, int>* internalLockedItemIndexes,
        double scrollViewport,
        double availableWidth,
        double adjustedAvailableWidth,
        double averageLineItemsWidth,
        double averageAspectRatio,
        double lineSpacing,
        double actualLineHeight,
        int beginSizedLineIndex,
        int endSizedLineIndex,
        int beginSizedItemIndex,
        int endSizedItemIndex,
        int beginLineVectorIndex,
        bool isLastSizedLineStretchEnabled);

    float GetItemsRangeArrangeWidth(
        int beginSizedItemIndex,
        int endSizedItemIndex,
        bool usesArrangeWidthInfo);

    int GetLastItemIndexInLineIndex(
        int lineVectorIndex) const;

    int GetLineCount(
        double averageItemsPerLine) const;

    int GetLineIndex(
        int itemIndex,
        bool usesFastPathLayout) const;

    int GetLineIndexFromAverageItemsPerLine(
        int itemIndex,
        double averageItemsPerLine) const;

    float GetLinesDesiredWidth();

    double GetMaxWidthFromItemsInfo(
        int itemIndex) const;

    double GetMinWidthFromItemsInfo(
        int itemIndex) const;

    void GetNextLockedItem(
        std::map<int, int>* internalLockedItemIndexes,
        bool forward,
        int beginLineIndex,
        int endLineIndex,
        int itemIndex,
        int* lockedItemIndex,
        int* lockedLineIndex) const;

    float GetRealizationRectHeightDeficit(
        winrt::VirtualizingLayoutContext const& context,
        double actualLineHeight,
        double lineSpacing) const;

    void GetRealizedItemsFromSizedItems(
        int realizedLineCount,
        int lineCount,
        int* unrealizedNearItemCount,
        int* realizedItemCount) const;

    double GetRoundedDouble(
        double value) const;

    float GetRoundedFloat(
        float value) const;

    float GetSizedItemsRectHeight(
        winrt::VirtualizingLayoutContext const& context,
        double actualLineHeight,
        double lineSpacing) const;

    void InitializeForRelayout(
        int sizedLineCount,
        int& firstStillSizedLineIndex,
        int& lastStillSizedLineIndex,
        int& firstStillSizedItemIndex,
        int& lastStillSizedItemIndex);

    void InvalidateMeasureAsync();

    void InvalidateMeasureTimerStart(
        int tickCount);
    void InvalidateMeasureTimerStop(
        bool isForDestructor);

    void InvalidateMeasureTimerTick(
        winrt::IInspectable const& sender,
        winrt::IInspectable const& args);

    bool IsItemsLayoutContractionWorthy(
        ItemsLayout const& itemsLayout);

    bool IsItemsLayoutEqualizationWorthy(
        ItemsLayout const& itemsLayout,
        bool withHeadItem);

    bool IsItemsLayoutExpansionWorthy(
        ItemsLayout const& itemsLayout);

    bool IsLockedItem(
        bool forward,
        int beginLineIndex,
        int endLineIndex,
        int itemIndex);

    bool IsVirtualizingContext(
        winrt::VirtualizingLayoutContext const& context) const;

    std::tuple<bool, bool, bool> ItemRangesHaveNewDesiredWidths(
        std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> oldElementDesiredWidths,
        int beginRealizedItemIndex,
        int endRealizedItemIndex);

    int LineItemsCountTotal(
        int expectedTotal);

    bool LineHasLockedItem(
        int lineIndex,
        bool before,
        int itemIndex);

    bool LineHasInternalLockedItem(
        std::map<int, int> const& internalLockedItemIndexes,
        int lineIndex,
        bool before,
        int itemIndex) const;

    bool LockItemToLineInternal(
        std::map<int, int>& internalLockedItemIndexes,
        int beginSizedLineIndex,
        int endSizedLineIndex,
        int beginSizedItemIndex,
        int endSizedItemIndex,
        int lineIndex,
        int itemIndex);

    std::tuple<int, float, ItemsInfo> MeasureConstrainedLinesFastPath(
        winrt::VirtualizingLayoutContext const& context,
        float availableWidth,
        double actualLineHeight,
        bool forceLayoutWithoutItemsInfoRequest);

    int MeasureConstrainedLinesRegularPath(
        winrt::VirtualizingLayoutContext const& context,
        ItemsInfo const& itemsInfo,
        float availableWidth,
        double actualLineHeight);

    int MeasureConstrainedLines(
        winrt::VirtualizingLayoutContext const& context,
        std::set<double> const& averageItemsPerLineProcessed,
        ItemsInfo const& itemsInfo,
        bool forceRelayout,
        float availableWidth,
        float nearRealizationRect,
        float farRealizationRect,
        double scrollViewport,
        double scrollOffset,
        double actualLineHeight,
        std::pair<double, double>* newAverageItemsPerLine);

    void MeasureItemRange(
        double actualLineHeight,
        int beginRealizedItemIndex,
        int endRealizedItemIndex);

    void MeasureItemRangeRegularPath(
        std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> elementAvailableWidths,
        double actualLineHeight,
        int beginRealizedItemIndex,
        int endRealizedItemIndex);

    float MeasureUnconstrainedLine(
        winrt::VirtualizingLayoutContext const& context);

    void NotifyLinedFlowLayoutInvalidatedDbg(
        winrt::LinedFlowLayoutInvalidationTrigger const& invalidationTrigger);

    void NotifyLinedFlowLayoutItemLockedDbg(
        int itemIndex,
        int lineIndex);

    ItemsInfo RaiseItemsInfoRequested(
        int itemsRangeStartIndex,
        int itemsRangeRequestedLength);

    bool RequestItemsInfo(
        ItemsInfo const& itemsInfo,
        int beginSizedItemIndex,
        int endSizedItemIndex);

    void ResetItemsInfo();
    void ResetItemsInfoForFastPath();
    void ResetLinesInfo();
    void ResetSizedLines();

    void SetArrangeWidthFromItemsInfo(
        int itemIndex,
        float arrangeWidth);

    void SetAverageItemsPerLine(
        std::pair<double, double> averageItemsPerLine,
        bool unlockItems = true);

    void SetDesiredAspectRatioFromItemsInfo(
        int itemIndex,
        double desiredAspectRatio);

    float SetItemRangeArrangeWidth(
        int beginItemIndex,
        int endItemIndex,
        double actualLineHeight,
        double averageAspectRatio,
        double scaleFactor = 1.0);

    std::pair<double, double> SnapAverageItemsPerLine(
        double oldAverageItemsPerLineRaw,
        double newAverageItemsPerLineRaw) const;
    double SnapToPower(
        double value,
        double valuePower) const;

    bool UpdateActualLineHeight(
        winrt::VirtualizingLayoutContext const& context,
        winrt::Size const& availableSize);

    void UpdateAspectRatiosWithItemsInfo();

    bool UpdateItemsInfo(
        ItemsInfo const& itemsInfo,
        int oldFirstItemsInfoIndex,
        int oldLastItemsInfoIndex);

    void UpdateRoundingScaleFactor(
        winrt::UIElement const& xamlRootReference);

    bool UsesArrangeWidthInfo() const;
    bool UsesFastPathLayout() const;

#ifdef DBG
    static winrt::hstring DependencyPropertyToStringDbg(
        winrt::IDependencyProperty const& dependencyProperty);
    void LogElementManagerDbg();
    void LogItemsInfoDbg(
        int itemsRangeStartIndex,
        int itemsRangeRequestedLength,
        ItemsInfo const& itemsInfo);
    void LogItemsLayoutDbg(
        ItemsLayout const& itemsLayout,
        double averageLineItemsWidth) const;
    void LogLayoutDbg();
    void LogVirtualizingLayoutContextDbg(
        winrt::VirtualizingLayoutContext const& context) const;
    void VerifyInternalLockedItemsDbg(
        std::map<int, int> const& internalLockedItemIndexes,
        int beginSizedLineIndex,
        int endSizedLineIndex,
        int beginSizedItemIndex,
        int endSizedItemIndex);
    void VerifyLockedItemsDbg();
#endif

    // Fields

    ::ElementManager m_elementManager{ this, false };

    // Data structure for storing item desired aspect ratios.
    std::unique_ptr<LinedFlowLayoutItemAspectRatios> m_aspectRatios{ nullptr };

    // Map keeping track of the locked items.
    // Key:   index of locked item
    // Value: index of line holding the locked item
    std::map<int, int> m_lockedItemIndexes;

    // Element index used in bring-into-view operations to disconnected items.
    int m_anchorIndex{ -1 };
    int m_anchorIndexRetentionCountdown{ 0 };

    // Keep track of the measured items width. The heights are always equal to LineHeight.
    std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> m_elementAvailableWidths;
    std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> m_elementDesiredWidths;

    std::vector<int> m_lineItemCounts;
    std::vector<float> m_itemsInfoArrangeWidths;

    // Items info collected through the ItemsInfoRequested event:
    // - Used only by the regular path layout:
    std::vector<double> m_itemsInfoDesiredAspectRatiosForRegularPath;
    std::vector<double> m_itemsInfoMinWidthsForRegularPath;
    std::vector<double> m_itemsInfoMaxWidthsForRegularPath;
    int m_itemsInfoFirstIndex{ -1 }; // Indicates the index of the first element represented in the 4 vectors above.
    // - Used only by the fast path layout
    winrt::com_array<double> m_itemsInfoDesiredAspectRatiosForFastPath{};
    winrt::com_array<double> m_itemsInfoMinWidthsForFastPath{};
    winrt::com_array<double> m_itemsInfoMaxWidthsForFastPath{};
    // - Used by both layout paths:
    double m_itemsInfoMinWidth{ -1.0 };
    double m_itemsInfoMaxWidth{ -1.0 };
    // Note that the std::vector<double> vectors are used in the regular path because information gathered by successive ItemsInfoRequested occurrences
    // is stitched together in those vectors. This allows to only request information for a fraction of the sized items belonging to 5 viewports
    // in each ItemsInfoRequested event.
    // The fast path is only using cheaper temporary winrt::com_array<double> arrays because no such stitching is performed. Information is gathered for
    // the entire source collection and the arrays are discarded at the end of the measure path.
    
    // This countdown is used during initial loading in order to clamp the average aspect ratio between
    // 2/3 and 3/2 to avoid extranuous item realizations while the first items are still unpopulated.
    // It also allows a progressive expansion of the effective CacheLength to the minimum of 4 when
    // the ItemsRepeater's value is smaller than that.
    int m_measureCountdown{ s_measureCountdownStart };

    int m_itemCount{ 0 };
    int m_unsizedNearLineCount{ -1 };
    int m_unrealizedNearLineCount{ -1 };
    int m_firstSizedLineIndex{ -1 };
    int m_lastSizedLineIndex{ -1 };
    int m_firstSizedItemIndex{ -1 };
    int m_lastSizedItemIndex{ -1 };
    int m_firstFrozenLineIndex{ -1 };
    int m_lastFrozenLineIndex{ -1 };
    int m_firstFrozenItemIndex{ -1 };
    int m_lastFrozenItemIndex{ -1 };
    int m_invalidateMeasureTimerTickCount{ 0 };
    bool m_isVirtualizingContext{ false };
    bool m_wasInitializedForContext{ false };
    bool m_forceRelayout{ false };
    bool m_isFirstOrLastItemLocked{ false };
    float m_previousAvailableWidth{ 0.0f };
    float m_maxLineWidth{ 0.0f };
    double m_roundingScaleFactor{ 1.0 };

    // First double is the 'raw' average-items-per-line value that was snapped to the second double.
    // Second double is the 'snapped' average-items-per-line value starting from the raw first double.
    // The snapping is to a power of 1.1.
    std::pair<double, double> m_averageItemsPerLine;

    // Timer used to trigger an asynchronous measure pass when no items info was provided by the ItemsInfoRequested event.
    // It is started after an item was realized or when the timer expires and m_invalidateMeasureTimerTickCount is still
    // smaller than 7. The interval begins at 100ms and is then increased by 50% each time it is re-started, until
    // m_invalidateMeasureTimerTickCount reaches 7. By then the interval is 1.7s and the total time elapsed is about 5s
    // when the timer is no longer re-started.
    tracker_ref<winrt::DispatcherTimer> m_invalidateMeasureTimer{ this };

    // Fields used to support the LayoutsTestHooks
    int m_logItemIndexDbg{ -1 };
    double m_averageItemAspectRatioDbg{ 0.0 };
    double m_forcedAverageItemAspectRatioDbg{ 0.0 };
    double m_forcedAverageItemsPerLineDividerDbg{ 0.0 };
    double m_forcedWrapMultiplierDbg{ 0.0 };
    bool m_isFastPathSupportedDbg{ true };

#ifdef DBG
    // Other debug fields
    int m_previousFirstDisplayedArrangedLineIndexDbg{ -1 };
    int m_previousLastDisplayedArrangedLineIndexDbg{ -1 };
#endif
};
