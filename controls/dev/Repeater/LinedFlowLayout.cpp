// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "LinedFlowLayout.h"
#include "LinedFlowLayoutItemsInfoRequestedEventArgs.h"
#include "LinedFlowLayoutItemCollectionTransitionProvider.h"
#include "RuntimeProfiler.h"
#include "VirtualizingLayoutContext.h"
#include "LayoutsTestHooks.h"
#include "SharedHelpers.h"

// Change to 'true' to turn on debugging outputs in Output window
bool LinedFlowLayoutTrace::s_IsDebugOutputEnabled{ false };
bool LinedFlowLayoutTrace::s_IsVerboseDebugOutputEnabled{ false };

// Maximum difference for scroll offsets to be considered equal. Used in GetFirstAndLastDisplayedLineIndexes for the moment.
const double c_offsetEqualityEpsilon{ 0.01 };

// The desired aspect ratio associated with an item has a weight between 1 and 16. Each time an item's aspect ratio is evaluated, its weight is incremented up to 16.
// The average aspect ratio is computed using those weights. This is to avoid giving too much importance to newly realized items which are still loading their content
// and temporarily provide an aspect ratio that would throw off the average. This stabilizes the m_averageItemsPerLine field which leads to fewer total re-layouts.
const int c_maxAspectRatioWeight{ 16 };

// See m_anchorIndexRetentionCountdown usage below. Represents the measure pass count after which a RecommendedAnchorIndex reset to -1 takes effect.
const int c_maxAnchorIndexRetentionCount{ 10 };

// An area sized 80% of a scroll viewport made of frozen lines is placed before the displayed lines. An identically sized area is also placed after the displayed lines.
const double c_frozenLinesRatio{ 0.8 };

#pragma region ILinedFlowLayout

LinedFlowLayout::LinedFlowLayout()
{
    LINEDFLOWLAYOUT_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_LinedFlowLayout);
    LayoutId(L"LinedFlowLayout");

    EnsureProperties();

    SetIndexBasedLayoutOrientation(winrt::IndexBasedLayoutOrientation::LeftToRight);
}

LinedFlowLayout::~LinedFlowLayout()
{
    LINEDFLOWLAYOUT_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    InvalidateMeasureTimerStop(true /*isForDestructor*/);
}

// Locks the 'itemIndex' into its line until the m_averageItemsPerLine or collection changes,
// at which point the ItemsUnlocked event is raised.
int32_t LinedFlowLayout::LockItemToLine(
    int32_t itemIndex)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, itemIndex);

    if (itemIndex < 0 || itemIndex >= m_itemCount)
    {
        throw winrt::hresult_out_of_bounds();
    }

    if (m_averageItemsPerLine.second == 0.0)
    {
        // LockItemToLine is called before the first measure pass or when ActualLineHeight <= 0.
        // Returning -1 as an indication that LockItemToLine must be called later, after the measure pass has established an average items per line.
        return -1;
    }

    const bool usesFastPathLayout = UsesFastPathLayout();
    int lockedLineIndex{ -1 };

    if (itemIndex == 0)
    {
        // First item is implicitly locked on the first line - no need to update the m_lockedItemIndexes map.
        // Setting the m_isFirstOrLastItemLocked flag though so that ItemsUnlocked is raised when only the first and/or last item was locked.
        m_isFirstOrLastItemLocked = true;
        lockedLineIndex = 0;
    }
    else if (itemIndex == m_itemCount - 1)
    {
        // Last item is implicitly locked on the last line - no need to update the m_lockedItemIndexes map.
        // Setting the m_isFirstOrLastItemLocked flag though so that ItemsUnlocked is raised when only the first and/or last item was locked.
        m_isFirstOrLastItemLocked = true;
        lockedLineIndex = usesFastPathLayout ? GetLineIndex(itemIndex, true /*usesFastPathLayout*/) : GetLineIndexFromAverageItemsPerLine(itemIndex, m_averageItemsPerLine.second);
    }

    if (m_lockedItemIndexes.find(itemIndex) != m_lockedItemIndexes.end())
    {
        // Item is already locked - return its current lock line index.
        return m_lockedItemIndexes[itemIndex];
    }

    if (lockedLineIndex == -1)
    {
        if (usesFastPathLayout)
        {
            lockedLineIndex = GetLineIndex(itemIndex, true /*usesFastPathLayout*/);
        }
        else
        {
            if (m_firstFrozenItemIndex != -1 &&
                m_lastFrozenItemIndex != -1 &&
                itemIndex >= m_firstFrozenItemIndex &&
                itemIndex <= m_lastFrozenItemIndex)
            {
                // Item is among the frozen items - Return the current frozen line index for the item.
                lockedLineIndex = GetFrozenLineIndexFromFrozenItemIndex(itemIndex);
            }
            else
            {
                // Else return the line index based on the average items per line.
                lockedLineIndex = GetLineIndexFromAverageItemsPerLine(itemIndex, m_averageItemsPerLine.second);
            }
        }
    }

    MUX_ASSERT(lockedLineIndex >= 0);

    if (m_lockedItemIndexes.size() > 0)
    {
        // Check if that line already has locked items. Any unlocked item squeezed between two locked items is declared locked.
        for (const auto& lockedItemIterator : m_lockedItemIndexes)
        {
            if (lockedItemIterator.second == lockedLineIndex)
            {
                // Found a locked item on the same line
                const int lockedItemIndex = lockedItemIterator.first;

                if (lockedItemIndex < itemIndex)
                {
                    // It is before the newly locked one. Ensure all items in between them are locked.
                    for (int lockedItemIndexTmp = lockedItemIndex + 1; lockedItemIndexTmp < itemIndex; lockedItemIndexTmp++)
                    {
                        if (m_lockedItemIndexes.find(lockedItemIndexTmp) == m_lockedItemIndexes.end())
                        {
                            m_lockedItemIndexes.insert(std::pair<int, int>(lockedItemIndexTmp, lockedLineIndex));

                            NotifyLinedFlowLayoutItemLockedDbg(lockedItemIndexTmp, lockedLineIndex);
                        }
                    }

                    // No other item can possibly be locked on the same line.
                    break;
                }
                else
                {
                    // It is after the newly locked one. Ensure all items in between them are locked.
                    MUX_ASSERT(lockedItemIndex > itemIndex);

                    for (int lockedItemIndexTmp = lockedItemIndex - 1; lockedItemIndexTmp > itemIndex; lockedItemIndexTmp--)
                    {
                        if (m_lockedItemIndexes.find(lockedItemIndexTmp) == m_lockedItemIndexes.end())
                        {
                            m_lockedItemIndexes.insert(std::pair<int, int>(lockedItemIndexTmp, lockedLineIndex));

                            NotifyLinedFlowLayoutItemLockedDbg(lockedItemIndexTmp, lockedLineIndex);
                        }
                    }

                    // No other item can possibly be locked on the same line.
                    break;
                }
            }
        }
    }

    // Finally lock the provided item itself, unless it's the first or last item.
    MUX_ASSERT(m_lockedItemIndexes.find(itemIndex) == m_lockedItemIndexes.end());

    if (itemIndex != 0 && itemIndex != m_itemCount - 1)
    {
        m_lockedItemIndexes.insert(std::pair<int, int>(itemIndex, lockedLineIndex));
    }

    NotifyLinedFlowLayoutItemLockedDbg(itemIndex, lockedLineIndex);

    return lockedLineIndex;
}

void LinedFlowLayout::OnPropertyChanged(
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto dependencyProperty = args.Property();

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(nullptr, L"%s(property: %s)\n", METH_NAME, DependencyPropertyToStringDbg(dependencyProperty).c_str());

    if (dependencyProperty != s_ActualLineHeightProperty)
    {
        InvalidateLayout();
    }
}

#pragma endregion

#pragma region ILayoutOverrides

winrt::ItemCollectionTransitionProvider LinedFlowLayout::CreateDefaultItemTransitionProvider()
{
    return winrt::make<LinedFlowLayoutItemCollectionTransitionProvider>();
}

#pragma endregion

#pragma region IVirtualizingLayoutOverrides

void LinedFlowLayout::InitializeForContextCore(
    winrt::VirtualizingLayoutContext const& context)
{
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (m_wasInitializedForContext)
    {
        throw winrt::hresult_error(E_INVALID_OPERATION, s_cannotShareLinedFlowLayout);
    }

    __super::InitializeForContextCore(context);

    m_wasInitializedForContext = true;
    m_isVirtualizingContext = IsVirtualizingContext(context);
    m_itemCount = context.ItemCount();
    m_elementManager.SetContext(context);
}

void LinedFlowLayout::UninitializeForContextCore(
    winrt::VirtualizingLayoutContext const& context)
{
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(m_wasInitializedForContext);
    MUX_ASSERT(m_isVirtualizingContext == IsVirtualizingContext(context));

    __super::UninitializeForContextCore(context);

    m_wasInitializedForContext = false;
    m_itemCount = 0;
    if (m_isVirtualizingContext)
    {
        m_isVirtualizingContext = false;
        m_elementManager.ClearRealizedRange();
    }

    InvalidateMeasureTimerStop(false /*isForDestructor*/);
    UnlockItems();
}

winrt::Size LinedFlowLayout::MeasureOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& availableSize)
{
    const float availableWidth = availableSize.Width;

#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"availableSize", availableWidth, availableSize.Height);

    MUX_ASSERT(m_isVirtualizingContext == IsVirtualizingContext(context));
    LogVirtualizingLayoutContextDbg(context);
    LogElementManagerDbg();
#endif

    if (!m_isVirtualizingContext)
    {
        // LinedFlowLayout is expected to be hosted by a panel like LayoutPanel. Always perform a complete re-layout in non-virtualizing scenarios.
        // No measure passes occur on simple scrolling, and realization window is infinite.
        m_forceRelayout = true;

        const int newItemCount = context.ItemCount();

        if (!UsesArrangeWidthInfo())
        {
            if (m_itemCount < newItemCount)
            {
                // In the non-virtualizing case, the asynchronous measure passes are started only when the item count increased.
                // Ideally InvalidateMeasureTimerStart should be called instead whenever an item is added or replaced, or when
                // the collection is reset, but LinedFlowLayout::OnItemsChangedCore is not invoked in the non-virtualizing case.
                InvalidateMeasureTimerStart(0 /*tickCount*/);
            }
            else if (m_itemCount > 0 && newItemCount == 0)
            {
                // Stop the potential timer as the collection is empty now.
                InvalidateMeasureTimerStop(false /*isForDestructor*/);
            }
        }

        // LayoutPanel.Children.Count may have changed. LinedFlowLayout::OnItemsChangedCore is not invoked in non-virtualizing scenarios.
        m_itemCount = newItemCount;

        // Item locks are systematically invalidated since the measure pass must be the result of a child or Children collection change.
        UnlockItems();
    }

    MUX_ASSERT(m_itemCount == context.ItemCount());

    if (m_measureCountdown > 1)
    {
        m_measureCountdown--;
    }

    const bool actualLineHeightChanged = UpdateActualLineHeight(context, availableSize);
    const double actualLineHeight = ActualLineHeight();
    float desiredWidth = 0.0f;
    double linesSpacing = 0.0;
    int lineCount = 0;

    if (m_itemCount > 0 && actualLineHeight > 0.0)
    {
        winrt::Point layoutOrigin{};

        context.LayoutOrigin(layoutOrigin);

        if (std::isinf(availableWidth))
        {
            // Clearing the items info that may have been gathered during a previous pass while availableSize.Width != infinity.
            ResetItemsInfo();

            // Set desiredWidth to total desired width of items, plus the MinItemSpacing between them.
            desiredWidth = MeasureUnconstrainedLine(context);
            lineCount = 1;
        }
        else
        {
            // Desired width beyond the available width.
            float desiredExtraWidth = 0.0f;

            // Do not even attempt to perform the fast path when there is no listener for the ItemsInfoRequested event.
            // Instead, fall back to the regular path right away.
            if (IsFastPathSupportedDbg() && m_itemsInfoRequestedEventSource)
            {
                if (actualLineHeightChanged && !m_forceRelayout)
                {
                    // Because ActualLineHeight changed above, MeasureConstrainedLinesFastPath below needs to call 
                    // ComputeItemsLayoutFastPath to perform a re-layout of all items when the fast path is enabled.
                    // The ItemsInfoRequest event needs to be raised though because of a prior ResetItemsInfoForFastPath call,
                    // so forceLayoutWithoutItemsInfoRequest cannot simply be used as True.
                    m_forceRelayout = true;
                }

                std::tuple<int /*lineCount*/, float /*maxLineWidth*/, ItemsInfo> fastPathLayoutResults = MeasureConstrainedLinesFastPath(
                    context,
                    availableWidth,
                    actualLineHeight,
                    false /*forceLayoutWithoutItemsInfoRequest*/);

                lineCount = std::get<0>(fastPathLayoutResults);

                if (lineCount == -1)
                {
                    // The items info gathered during the fast path attempt above is handed off
                    // to the regular path since in many cases it is enough to not require a
                    // new ItemsInfoRequested event to be raised.
                    lineCount = MeasureConstrainedLinesRegularPath(
                        context,
                        std::get<2>(fastPathLayoutResults) /*itemsInfo*/,
                        availableWidth,
                        actualLineHeight);

                    if (lineCount == -1)
                    {
                        // The fast path was enabled by the ItemsInfoRequested handler. This case can occur when
                        // (m_forceRelayout || m_previousAvailableWidth != availableWidth) evaluated to False in the
                        // MeasureConstrainedLinesFastPath call above, and a transition out of the regular path is
                        // enabled in MeasureConstrainedLinesRegularPath.

                        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"MeasureConstrainedLinesRegularPath enabled fast path");

                        fastPathLayoutResults = MeasureConstrainedLinesFastPath(
                            context,
                            availableWidth,
                            actualLineHeight,
                            true /*forceLayoutWithoutItemsInfoRequest*/);

                        lineCount = std::get<0>(fastPathLayoutResults);

                        MUX_ASSERT(lineCount > 0);

                        m_maxLineWidth = std::get<1>(fastPathLayoutResults);
                    }
                    else
                    {
                        m_maxLineWidth = GetLinesDesiredWidth();
                    }

#ifdef DBG
                    if (m_aspectRatios != nullptr && !m_aspectRatios->IsEmpty())
                    {
                        // Uncomment for verbose tracing off all stored aspect ratios & weights.
                        //m_aspectRatios->LogItemAspectRatiosDbg();
                        const int firstRealizedItemIndexDbg = m_isVirtualizingContext ? m_elementManager.GetFirstRealizedDataIndex() : 0;
                        const int lastRealizedItemIndexDbg = firstRealizedItemIndexDbg + m_elementManager.GetRealizedElementCount() - 1;
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"averageAspectRatio (1)", m_aspectRatios->GetAverageAspectRatio(firstRealizedItemIndexDbg, lastRealizedItemIndexDbg, c_maxAspectRatioWeight));
                    }
#endif
                }
                else
                {
                    m_maxLineWidth = std::get<1>(fastPathLayoutResults);

                    // Reset the fields specific to the regular path.
                    ExitRegularPath();
                }
            }
            else
            {
                if (UsesArrangeWidthInfo())
                {
                    // Necessary for cases where a prior fast path is turned off because the ItemsInfoRequested handler was removed.
                    m_itemsInfoArrangeWidths.clear();
                }

                lineCount = MeasureConstrainedLinesRegularPath(
                    context,
                    s_emptyItemsInfo,
                    availableWidth,
                    actualLineHeight);

                m_maxLineWidth = GetLinesDesiredWidth();

#ifdef DBG
                if (m_aspectRatios != nullptr && !m_aspectRatios->IsEmpty())
                {
                    // Uncomment for verbose tracing off all stored aspect ratios & weights.
                    //m_aspectRatios->LogItemAspectRatiosDbg();
                    const int firstRealizedItemIndexDbg = m_isVirtualizingContext ? m_elementManager.GetFirstRealizedDataIndex() : 0;
                    const int lastRealizedItemIndexDbg = firstRealizedItemIndexDbg + m_elementManager.GetRealizedElementCount() - 1;
                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"averageAspectRatio (2)", m_aspectRatios->GetAverageAspectRatio(firstRealizedItemIndexDbg, lastRealizedItemIndexDbg, c_maxAspectRatioWeight));
                }
#endif
            }

            desiredWidth = availableWidth;
            desiredExtraWidth = m_maxLineWidth - availableWidth;

#ifdef DBG
            if (desiredExtraWidth != 0.0f)
            {
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"desiredExtraWidth used", desiredExtraWidth);
            }
#endif

            // Only account for the extra desired width if it goes beyond pixel snapping rounding.
            if (desiredExtraWidth > 0.5f / static_cast<float>(m_roundingScaleFactor) * m_averageItemsPerLine.second)
            {
                desiredWidth += desiredExtraWidth;
            }

#ifdef DBG_VERBOSE
            LogLayoutDbg();
#endif
        }

        linesSpacing = (lineCount - 1) * LineSpacing();

        if (m_isVirtualizingContext && m_itemsInfoFirstIndex == -1 && m_aspectRatios != nullptr)
        {
            const int firstRealizedItemIndex = m_elementManager.GetFirstRealizedDataIndex();
            const int lastRealizedItemIndex = firstRealizedItemIndex + m_elementManager.GetRealizedElementCount() - 1;

            MUX_ASSERT(firstRealizedItemIndex >= 0);
            MUX_ASSERT(lastRealizedItemIndex >= 0);

            if (m_aspectRatios->HasLowerWeight(firstRealizedItemIndex, lastRealizedItemIndex, c_maxAspectRatioWeight))
            {
                LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Forcing layout pass for low aspect ratio weight");

                // No items info was provided in the ItemsInfoRequest handler and there is at least an aspect ratio with a growing weight. 
                // Making sure there is another layout coming so that weights below c_maxAspectRatioWeight can be increased and ultimately
                // reach c_maxAspectRatioWeight. Otherwise scrolling could trigger weight increases, an average-items-per-line change and re-layout.
                InvalidateMeasureAsync();
            }
        }
    }
    else
    {
        if (m_isVirtualizingContext)
        {
            m_elementManager.ClearRealizedRange();
        }
        m_elementAvailableWidths = nullptr;
        m_elementDesiredWidths = nullptr;
        SetAverageItemsPerLine(std::pair<double, double>(0.0 /*averageItemsPerLineRaw*/, 0.0 /*averageItemsPerLineSnapped*/));
        ClearItemAspectRatios();
        m_averageItemAspectRatioDbg = 0.0f;
        m_unsizedNearLineCount = -1;
        m_unrealizedNearLineCount = -1;
        m_maxLineWidth = 0.0f;
        ResetItemsInfo();
        ResetLinesInfo();
        ResetSizedLines();
    }

    m_previousAvailableWidth = availableSize.Width;

    winrt::Size desiredSize{ desiredWidth, static_cast<float>(lineCount * actualLineHeight + linesSpacing) };

#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"desiredSize", desiredSize.Width, desiredSize.Height);
    LogElementManagerDbg();
#endif

    return desiredSize;
}

winrt::Size LinedFlowLayout::ArrangeOverride(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& finalSize)
{
#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"finalSize", finalSize.Width, finalSize.Height);

    MUX_ASSERT(m_itemCount == context.ItemCount());
    LogVirtualizingLayoutContextDbg(context);
    LogElementManagerDbg();
#endif

    if (std::isinf(m_previousAvailableWidth))
    {
        ArrangeUnconstrainedLine(context);
    }
    else
    {
        ArrangeConstrainedLines(context);
    }

    return finalSize;
}

void LinedFlowLayout::OnItemsChangedCore(
    winrt::VirtualizingLayoutContext const& context,
    winrt::IInspectable const& source,
    winrt::NotifyCollectionChangedEventArgs const& args)
{
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_elementManager.DataSourceChanged(source, args);

    m_itemCount = context.ItemCount();

    if (m_itemCount == 0 && !UsesArrangeWidthInfo())
    {
        // Stop the potential timer as the collection is empty now.
        InvalidateMeasureTimerStop(false /*isForDestructor*/);
    }

    // Discard any potential item sizing info previously collected through the ItemsInfoRequested event.
    // Perform a complete re-layout after the data source change.
    InvalidateLayout(true /*forceRelayout*/, true /*resetItemsInfo*/, false /*invalidateMeasure*/);

    if (args.Action() == winrt::NotifyCollectionChangedAction::Reset)
    {
        // Discard any aspect ratio previously recorded since the data source has significantly changed.
        ClearItemAspectRatios();

        // Reset the measure pass countdown to avoid extreme aspect ratio again.
        m_measureCountdown = s_measureCountdownStart;

        m_averageItemAspectRatioDbg = 0.0f;
    }

    // Calls InvalidateMeasure() to ensure the layout reflects the data source change.
    __super::OnItemsChangedCore(context, source, args);

    // Item locks are invalidated by the data source change.
    UnlockItems();
}

#pragma endregion

#pragma region private helpers

// Items arrangement when the non-scrolling dimension is constrained.
void LinedFlowLayout::ArrangeConstrainedLines(
    winrt::VirtualizingLayoutContext const& context)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    const double actualLineHeight = ActualLineHeight();

    if (m_lineItemCounts.size() == 0 || actualLineHeight <= 0.0)
    {
        return;
    }

    MUX_ASSERT(m_isVirtualizingContext == IsVirtualizingContext(context));

    const bool usesArrangeWidthInfo = UsesArrangeWidthInfo();
    const float minItemSpacing = static_cast<float>(MinItemSpacing());
    const double lineSpacing = LineSpacing();
    const int firstSizedLineIndex = m_firstSizedLineIndex == -1 ? 0 : m_firstSizedLineIndex;
    const int lineCount = m_firstSizedLineIndex == -1 ? static_cast<int>(m_lineItemCounts.size()) : GetLineCount(m_averageItemsPerLine.second);
    const auto itemsStretch = ItemsStretch();
    const auto itemsJustification = ItemsJustification();

#ifdef DBG
    const int sizedItemCountDbg = LineItemsCountTotal(m_firstSizedItemIndex == -1 ? 0 : m_lastSizedItemIndex - m_firstSizedItemIndex + 1);

    if (UsesFastPathLayout())
    {
        MUX_ASSERT(m_unsizedNearLineCount == -1);
    }
    else
    {
        MUX_ASSERT(m_firstSizedItemIndex < m_itemCount);
    }

    if (m_isVirtualizingContext)
    {
        const double scrollViewport = context.VisibleRect().Height;
        const double scrollOffset = context.VisibleRect().Y;

        GetFirstAndLastDisplayedLineIndexes(
            scrollViewport,
            scrollOffset,
            0 /*padding*/,
            lineSpacing,
            actualLineHeight,
            lineCount,
            false /*forFullyDisplayedLines*/,
            &m_previousFirstDisplayedArrangedLineIndexDbg,
            &m_previousLastDisplayedArrangedLineIndexDbg);
    }
#endif

    int firstFullyRealizedLineIndex{ -1 };
    int firstItemInFullyRealizedLine{ -1 };

    GetFirstFullyRealizedLineIndex(
        &firstFullyRealizedLineIndex,
        &firstItemInFullyRealizedLine);

    if (firstFullyRealizedLineIndex == -1)
    {
        // This situation can occur when context.VisibleRect().Height is 0.
        MUX_ASSERT(firstItemInFullyRealizedLine == -1);

        return;
    }

    const int itemsInfoArrangeWidthsOffset = m_itemsInfoFirstIndex == -1 ? 0 : m_itemsInfoFirstIndex;
    const int lastSizedLineVectorIndex = static_cast<int>(m_lineItemCounts.size()) - 1;
    int sizedItemIndex = firstItemInFullyRealizedLine;

    for (int sizedLineVectorIndex = firstFullyRealizedLineIndex - firstSizedLineIndex; sizedLineVectorIndex <= lastSizedLineVectorIndex; sizedLineVectorIndex++)
    {
        float cumulatedOffset = 0.0f;
        float itemSpacing = minItemSpacing;
        const int lineItemsCount = m_lineItemCounts[sizedLineVectorIndex];
        const float lineArrangeWidth = GetItemsRangeArrangeWidth(sizedItemIndex /*beginSizedItemIndex*/, sizedItemIndex + lineItemsCount - 1 /*endSizedItemIndex*/, usesArrangeWidthInfo);
        const double lineExtraAvailableWidth = m_previousAvailableWidth - lineArrangeWidth - itemSpacing * (lineItemsCount - 1);

#ifdef DBG
        float cumulatedWidthDbg = 0.0;
#endif

        if (itemsStretch == winrt::LinedFlowLayoutItemsStretch::None)
        {
            if (lineExtraAvailableWidth > 0.0)
            {
                switch (itemsJustification)
                {
                case winrt::LinedFlowLayoutItemsJustification::Start:
#ifdef DBG
                    cumulatedWidthDbg = static_cast<float>(lineExtraAvailableWidth);
#endif
                    break;
                case winrt::LinedFlowLayoutItemsJustification::Center:
                    cumulatedOffset = static_cast<float>(lineExtraAvailableWidth / 2.0);
#ifdef DBG
                    cumulatedWidthDbg = 2.0f * cumulatedOffset;
#endif
                    break;
                case winrt::LinedFlowLayoutItemsJustification::End:
                    cumulatedOffset = static_cast<float>(lineExtraAvailableWidth);
#ifdef DBG
                    cumulatedWidthDbg = cumulatedOffset;
#endif
                    break;
                case winrt::LinedFlowLayoutItemsJustification::SpaceEvenly:
                    cumulatedOffset = static_cast<float>(lineExtraAvailableWidth / (lineItemsCount + 1));
                    itemSpacing += cumulatedOffset;
#ifdef DBG
                    cumulatedWidthDbg = 2.0f * cumulatedOffset;
#endif
                    break;
                case winrt::LinedFlowLayoutItemsJustification::SpaceAround:
                    cumulatedOffset = static_cast<float>(lineExtraAvailableWidth / lineItemsCount / 2.0);
                    itemSpacing += cumulatedOffset * 2.0f;
#ifdef DBG
                    cumulatedWidthDbg = 2.0f * cumulatedOffset;
#endif
                    break;
                case winrt::LinedFlowLayoutItemsJustification::SpaceBetween:
                    if (lineItemsCount > 1)
                    {
                        itemSpacing += static_cast<float>(lineExtraAvailableWidth / (lineItemsCount - 1));
                    }
                    break;
                }
            }
            else if (lineExtraAvailableWidth < 0.0)
            {
                if (lineExtraAvailableWidth >= -0.5 / m_roundingScaleFactor * lineItemsCount)
                {
                    // Because of pixel snapping based on m_roundingScaleFactor, lineExtraAvailableWidth can be slightly different from 0.0.
                    // In that case we adjust the minItemSpacing by distributing the small delta equally.
                    itemSpacing += static_cast<float>(lineExtraAvailableWidth / (lineItemsCount - 1));
                }
#ifdef DBG
                else
                {
                    // Because of items MinWidth, items could not be scaled down to make them fit in the line.
                    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"lineExtraAvailableWidth", lineExtraAvailableWidth);
                }
#endif
            }
        }
        else if (lineExtraAvailableWidth != 0.0)
        {
            // Condition #1. Because of pixel snapping based on m_roundingScaleFactor, lineExtraAvailableWidth can be slightly different from 0.0.
            // Condition #2. Because of MaxWidth constraining the expansion of line items, lineExtraAvailableWidth can be much larger than 0.0.
            // Condition #2. Because Image elements do not expand beyond MinWidth when their natural width is smaller than MinWidth, lineExtraAvailableWidth can be much larger than 0.0.
            // In those cases we adjust the minItemSpacing by distributing the small delta equally.
            const bool itemSpacingAdjustedForRounding = std::abs(lineExtraAvailableWidth) <= 0.5 / m_roundingScaleFactor * lineItemsCount;
            const bool itemSpacingAdjustedForMinMaxWidth = !itemSpacingAdjustedForRounding && lineExtraAvailableWidth > 0.0 && lineCount != firstSizedLineIndex + sizedLineVectorIndex + 1;

            if (itemSpacingAdjustedForRounding || itemSpacingAdjustedForMinMaxWidth)
            {
#ifdef DBG
                if (itemSpacingAdjustedForMinMaxWidth)
                {
                    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"lineExtraAvailableWidth", lineExtraAvailableWidth);
                }
#endif

                itemSpacing += static_cast<float>(lineExtraAvailableWidth / (lineItemsCount - 1));
            }
#ifdef DBG
            else
            {
                // In addition to those three cases above, the line items' MinWidth may prevent them from being shrunk sufficiently to fit in the available width. In those cases nothing is done.
                LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"lineExtraAvailableWidth", lineExtraAvailableWidth);
            }
#endif
        }

        const int lineIndex = sizedLineVectorIndex + (m_unsizedNearLineCount == -1 ? 0 : m_unsizedNearLineCount);

        MUX_ASSERT(lineIndex < lineCount);

        for (int sizedLineItemIndex = 0; sizedLineItemIndex < lineItemsCount; sizedLineItemIndex++)
        {
            if (m_elementManager.IsDataIndexRealized(sizedItemIndex))
            {
                if (const auto element = m_elementManager.GetRealizedElement(sizedItemIndex /*dataIndex*/))
                {
                    float arrangeWidth{};

                    if (usesArrangeWidthInfo)
                    {
                        // Use the item info provided by the ItemsInfoRequested handler since the DesiredSize
                        // may be different from the computed arrange width when the item content failed to load properly.
                        arrangeWidth = m_itemsInfoArrangeWidths[sizedItemIndex - itemsInfoArrangeWidthsOffset];
                    }
                    else
                    {
                        arrangeWidth = element.DesiredSize().Width;
                    }

#ifdef DBG
                    if (arrangeWidth == 0.0f)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, usesArrangeWidthInfo ? L"Unexpected nil arrange width" : L"Unexpected nil desired width", sizedItemIndex);
                    }
#endif

                    const winrt::Rect elementRect{ cumulatedOffset, static_cast<float>(lineIndex * (actualLineHeight + lineSpacing)), arrangeWidth, static_cast<float>(actualLineHeight)};

                    element.Arrange(elementRect);

#ifdef DBG
                    if (sizedItemIndex == m_logItemIndexDbg)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_PTR_STR, METH_NAME, this, element, L"Arrange element");
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Arrange line", lineIndex);
                    }

                    if (m_elementAvailableWidths && m_elementDesiredWidths)
                    {
                        MUX_ASSERT(!UsesFastPathLayout());

                        const auto elementTracker = tracker_ref<winrt::UIElement>(this, element);
                        auto recordedSizeIterator = m_elementAvailableWidths->find(elementTracker);

                        if (recordedSizeIterator == m_elementAvailableWidths->end())
                        {
                            recordedSizeIterator = m_elementDesiredWidths->find(elementTracker);

                            if (recordedSizeIterator != m_elementDesiredWidths->end())
                            {
                                const float desiredWidth = recordedSizeIterator->second;

                                if ((std::abs(desiredWidth - element.DesiredSize().Width) >= 1.0f || std::abs(actualLineHeight - element.DesiredSize().Height) >= 1.0f) ||
                                    sizedItemIndex == m_logItemIndexDbg)
                                {
                                    if (sizedItemIndex != m_logItemIndexDbg)
                                    {
                                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"sizedItemIndex", sizedItemIndex);
                                    }
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"desiredWidth", desiredWidth);
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"element.DesiredSize.Width", element.DesiredSize().Width);
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"desiredHeight", actualLineHeight);
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"element.DesiredSize.Height", element.DesiredSize().Height);
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"arrangeWidth", arrangeWidth);
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"renderWidth", element.RenderSize().Width);
                                }
                            }
                        }
                        else
                        {
                            const float arrangeWidthDbg = recordedSizeIterator->second;

                            if (std::abs(arrangeWidthDbg - element.DesiredSize().Width) >= 1.0f ||
                                sizedItemIndex == m_logItemIndexDbg)
                            {
                                if (sizedItemIndex != m_logItemIndexDbg)
                                {
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"sizedItemIndex", sizedItemIndex);
                                }
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"element.DesiredSize.Width", element.DesiredSize().Width);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"arrangeWidthDbg", arrangeWidthDbg);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"arrangeWidth", arrangeWidth);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"renderWidth", element.RenderSize().Width);
                            }
                        }
                    }
                    else if (sizedItemIndex == m_logItemIndexDbg)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"desiredWidth", element.DesiredSize().Width);
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"arrangeWidth", arrangeWidth);
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"renderWidth", element.RenderSize().Width);
                    }

                    if (sizedLineItemIndex > 0)
                    {
                        cumulatedWidthDbg += itemSpacing;
                    }

                    cumulatedWidthDbg += elementRect.Width;
#endif

                    cumulatedOffset += elementRect.Width + itemSpacing;
                }
            }
            else
            {
                // The unrealized area was reached.
                return;
            }

            sizedItemIndex++;
        }

#ifdef DBG
        if (std::abs(context.VisibleRect().Width - cumulatedWidthDbg) > 1.0f)
        {
            LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"cumulatedWidth", cumulatedWidthDbg);
        }
#endif
    }
}

// Items arrangement when the non-scrolling dimension is unconstrained.
void LinedFlowLayout::ArrangeUnconstrainedLine(
    winrt::VirtualizingLayoutContext const& context)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (ActualLineHeight() <= 0.0)
    {
        return;
    }

    const float minItemSpacing = static_cast<float>(MinItemSpacing());
    float cumulatedOffset = 0.0f;

    if (ItemsStretch() == winrt::LinedFlowLayoutItemsStretch::None)
    {
        switch (ItemsJustification())
        {
        case winrt::LinedFlowLayoutItemsJustification::SpaceEvenly:
            cumulatedOffset = minItemSpacing;
            break;
        case winrt::LinedFlowLayoutItemsJustification::SpaceAround:
            cumulatedOffset = minItemSpacing / 2.0f;
            break;
        }
    }

    for (int itemIndex = 0; itemIndex < context.ItemCount(); itemIndex++)
    {
        const auto element = m_elementManager.GetAt(itemIndex);

        if (element)
        {
            const auto desiredSize = element.DesiredSize();
            const auto itemWidth = desiredSize.Width;
            const winrt::Rect arrangeRect{ cumulatedOffset, 0.0f, itemWidth, desiredSize.Height };

            element.Arrange(arrangeRect);
            cumulatedOffset += itemWidth + minItemSpacing;
        }
    }
}

void LinedFlowLayout::ClearItemAspectRatios()
{
    if (m_aspectRatios)
    {
        m_aspectRatios->Clear();
    }
}

// Last major phase of a measure pass:
// - updates the range of frozen lines and items (ComputeFrozenItemsRange calls).
// - breaks down all the sized lines into blocks on lines that require an items distribution (ComputeItemsLayout calls).
//   * when a full re-layout is required, a single block of all sized lines produces a single items distribution.
//   * otherwise two unfrozen blocks before and after the frozen lines can produce up to two items distributions.
//     These two distributions are getting lines at the edges of the unfrozen blocks ready to become frozen due to a scroll viewport size or scroll offset change.
// Returns True when a still sized item has a new desired width.
bool LinedFlowLayout::ComputeFrozenItemsAndLayout(
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
    int lastStillSizedItemIndex)
{
#ifdef DBG
    MUX_ASSERT(m_isVirtualizingContext == IsVirtualizingContext(context));
    MUX_ASSERT(nearSizedItemsRect <= nearRealizationRect);
    MUX_ASSERT(farSizedItemsRect >= farRealizationRect);
    MUX_ASSERT(sizedLineCount <= lineCount);
    MUX_ASSERT(sizedItemCount >= m_elementManager.GetRealizedElementCount());
#endif

    bool itemHasNewDesiredWidth{};

    if (firstStillSizedLineIndex != -1)
    {
        // Some line indexes are present in the previous measure pass and this new one.
#ifdef DBG
        MUX_ASSERT(m_isVirtualizingContext);
        MUX_ASSERT(lastStillSizedLineIndex != -1);
        MUX_ASSERT(m_firstSizedLineIndex <= firstStillSizedLineIndex);
        MUX_ASSERT(m_lastSizedLineIndex >= lastStillSizedLineIndex);
        MUX_ASSERT(m_firstSizedItemIndex <= firstStillSizedItemIndex);
        MUX_ASSERT(m_lastSizedItemIndex >= lastStillSizedItemIndex);
        MUX_ASSERT(m_lastSizedItemIndex - m_firstSizedItemIndex + 1 == sizedItemCount);
        MUX_ASSERT(lastStillSizedItemIndex - firstStillSizedItemIndex + 1 <= sizedItemCount);

        const int stillSizedItemCountDbg = LineItemsCountTotal(0);
        const int preStillSizedItemCountDbg = firstStillSizedItemIndex - m_firstSizedItemIndex;
        const int postStillSizedItemCountDbg = m_firstSizedItemIndex + sizedItemCount - lastStillSizedItemIndex - 1;

        MUX_ASSERT(preStillSizedItemCountDbg + stillSizedItemCountDbg + postStillSizedItemCountDbg == sizedItemCount);
#endif

        const int firstRealizedItemIndex = m_elementManager.GetFirstRealizedDataIndex();
        const int lastRealizedItemIndex = firstRealizedItemIndex + m_elementManager.GetRealizedElementCount() - 1;

        MUX_ASSERT(firstRealizedItemIndex >= 0);
        MUX_ASSERT(lastRealizedItemIndex >= 0);

        int adjustedFirstStillSizedItemIndex;
        int adjustedLastStillSizedItemIndex;
        bool preFrozenItemHasNewDesiredWidth = false;
        bool postFrozenItemHasNewDesiredWidth = false;

        bool forceRelayout = ComputeFrozenItemsRange(
            scrollViewport,
            scrollOffset,
            lineSpacing,
            actualLineHeight,
            lineCount,
            firstStillSizedLineIndex /*beginSizedLineIndex*/,
            lastStillSizedLineIndex  /*endSizedLineIndex*/,
            firstStillSizedItemIndex /*beginSizedItemIndex*/,
            lastStillSizedItemIndex  /*endSizedItemIndex*/,
            adjustedFirstStillSizedItemIndex,
            adjustedLastStillSizedItemIndex);

        if (!forceRelayout)
        {
            // m_firstFrozenItemIndex & m_lastFrozenItemIndex may still be -1 for example when scrollViewport==0.0.

            if (m_itemsInfoFirstIndex == -1)
            {
                if (oldElementDesiredWidths && oldElementAvailableWidths)
                {
                    const int clampedAdjustedFirstStillSizedItemIndex = std::clamp(adjustedFirstStillSizedItemIndex, firstRealizedItemIndex, lastRealizedItemIndex);
                    const int clampedAdjustedLastStillSizedItemIndex = std::clamp(adjustedLastStillSizedItemIndex, firstRealizedItemIndex, lastRealizedItemIndex);

                    EnsureAndMeasureItemRange(
                        context,
                        availableWidth,
                        actualLineHeight,
                        true /*forward*/,
                        clampedAdjustedFirstStillSizedItemIndex /*beginRealizedItemIndex*/,
                        clampedAdjustedLastStillSizedItemIndex /*endRealizedItemIndex*/);

                    // Check if any of the elements from m_firstFrozenItemIndex to m_lastFrozenItemIndex has a new or modified DesiredSize.
                    // In that case, a full re-layout is required.
                    // When a sized item before the frozen ones has a modified DesiredSize, it does not cause a full re-layout, but
                    // the pre-frozen-range is laid out in isolation. The same is applicable to the post-frozen-range.
                    std::tuple<bool /*preFrozenItemHasNewDesiredWidth*/, bool /*frozenItemHasNewDesiredWidth*/, bool /*postFrozenItemHasNewDesiredWidth*/> itemRangesHaveNewDesiredWidths =
                        ItemRangesHaveNewDesiredWidths(
                            oldElementDesiredWidths,
                            std::min(firstStillSizedItemIndex, m_firstFrozenItemIndex),
                            std::max(lastStillSizedItemIndex, m_lastFrozenItemIndex));

                    // forceRelayout is set to True when a frozen item has a new or modified DesiredSize.
                    forceRelayout = std::get<1>(itemRangesHaveNewDesiredWidths);

                    itemHasNewDesiredWidth = forceRelayout || std::get<0>(itemRangesHaveNewDesiredWidths) || std::get<2>(itemRangesHaveNewDesiredWidths);

                    if (!forceRelayout)
                    {
                        // preFrozenItemHasNewDesiredWidth is set to True when a sized item before the frozen ones has a modified DesiredSize.
                        preFrozenItemHasNewDesiredWidth = std::get<0>(itemRangesHaveNewDesiredWidths);

                        // postFrozenItemHasNewDesiredWidth is set to True when a sized item after the frozen ones has a modified DesiredSize.
                        postFrozenItemHasNewDesiredWidth = std::get<2>(itemRangesHaveNewDesiredWidths);

                        if (!preFrozenItemHasNewDesiredWidth && !postFrozenItemHasNewDesiredWidth)
                        {
                            // Measure realized items based on previously computed available widths 'oldElementAvailableWidths'.
                            // Re-populate m_elementAvailableWidths with the old & re-applied available widths.
                            MeasureItemRangeRegularPath(
                                oldElementAvailableWidths,
                                actualLineHeight,
                                clampedAdjustedFirstStillSizedItemIndex /*beginRealizedItemIndex*/,
                                clampedAdjustedLastStillSizedItemIndex /*endRealizedItemIndex*/);
                        }
                        else
                        {
                            MUX_ASSERT(clampedAdjustedFirstStillSizedItemIndex <= m_firstFrozenItemIndex);
                            MUX_ASSERT(clampedAdjustedLastStillSizedItemIndex >= m_lastFrozenItemIndex);

                            // Measure frozen items based on previously computed available widths 'oldElementAvailableWidths'.
                            // Re-populate m_elementAvailableWidths with the old & re-applied available widths.
                            MeasureItemRangeRegularPath(
                                oldElementAvailableWidths,
                                actualLineHeight,
                                m_firstFrozenItemIndex /*beginRealizedItemIndex*/,
                                m_lastFrozenItemIndex /*endRealizedItemIndex*/);

                            if (!preFrozenItemHasNewDesiredWidth && clampedAdjustedFirstStillSizedItemIndex <= m_firstFrozenItemIndex - 1)
                            {
                                // Measure pre-frozen items based on previously computed available widths 'oldElementAvailableWidths'.
                                // Re-populate m_elementAvailableWidths with the old & re-applied available widths.
                                MeasureItemRangeRegularPath(
                                    oldElementAvailableWidths,
                                    actualLineHeight,
                                    clampedAdjustedFirstStillSizedItemIndex /*beginRealizedItemIndex*/,
                                    m_firstFrozenItemIndex - 1 /*endRealizedItemIndex*/);
                            }

                            if (!postFrozenItemHasNewDesiredWidth && m_lastFrozenItemIndex + 1 <= clampedAdjustedLastStillSizedItemIndex)
                            {
                                // Measure post-frozen items based on previously computed available widths 'oldElementAvailableWidths'.
                                // Re-populate m_elementAvailableWidths with the old & re-applied available widths.
                                MeasureItemRangeRegularPath(
                                    oldElementAvailableWidths,
                                    actualLineHeight,
                                    m_lastFrozenItemIndex + 1 /*beginRealizedItemIndex*/,
                                    clampedAdjustedLastStillSizedItemIndex /*endRealizedItemIndex*/);
                            }
                        }
                    }
                }
            }
            else
            {
                MeasureItemRange(
                    actualLineHeight,
                    firstRealizedItemIndex /*beginRealizedItemIndex*/,
                    lastRealizedItemIndex  /*endRealizedItemIndex*/);
            }
        }

        if (forceRelayout)
        {
            // Layout all the sized lines.
            InitializeForRelayout(
                sizedLineCount,
                firstStillSizedLineIndex,
                lastStillSizedLineIndex,
                firstStillSizedItemIndex,
                lastStillSizedItemIndex);
            oldLineItemCounts.clear();

            ComputeItemsLayoutRegularPath(
                availableWidth,
                scrollViewport,
                lineSpacing,
                actualLineHeight,
                m_firstSizedLineIndex /*beginSizedLineIndex*/,
                m_lastSizedLineIndex  /*endSizedLineIndex*/,
                m_firstSizedItemIndex /*beginSizedItemIndex*/,
                m_lastSizedItemIndex  /*endSizedItemIndex*/,
                0 /*beginLineVectorIndex*/,
                m_lastSizedLineIndex < lineCount - 1 /*isLastSizedLineStretchEnabled*/);

            int adjustedFirstSizedItemIndex;
            int adjustedLastSizedItemIndex;

            ComputeFrozenItemsRange(
                scrollViewport,
                scrollOffset,
                lineSpacing,
                actualLineHeight,
                lineCount,
                m_firstSizedLineIndex /*beginSizedLineIndex*/,
                m_lastSizedLineIndex  /*endSizedLineIndex*/,
                m_firstSizedItemIndex /*beginSizedItemIndex*/,
                m_lastSizedItemIndex  /*endSizedItemIndex*/,
                adjustedFirstSizedItemIndex,
                adjustedLastSizedItemIndex);
        }
        else
        {
            MUX_ASSERT(adjustedFirstStillSizedItemIndex != -1 || m_firstSizedLineIndex == firstStillSizedLineIndex);
            MUX_ASSERT(adjustedLastStillSizedItemIndex != -1 || m_lastSizedLineIndex == lastStillSizedLineIndex);

            if (m_firstSizedLineIndex < firstStillSizedLineIndex)
            {
                if (m_firstFrozenLineIndex > firstStillSizedLineIndex)
                {
                    MUX_ASSERT(adjustedFirstStillSizedItemIndex - 1 >= firstStillSizedItemIndex);

                    EnsureAndMeasureItemRange(
                        context,
                        availableWidth,
                        actualLineHeight,
                        false /*forward*/,
                        std::clamp(adjustedFirstStillSizedItemIndex - 1, firstRealizedItemIndex, lastRealizedItemIndex) /*beginRealizedItemIndex*/,
                        std::clamp(firstStillSizedItemIndex, firstRealizedItemIndex, lastRealizedItemIndex) /*endRealizedItemIndex*/);
                }

                // Layout unfrozen & sized lines before the frozen ones.
                MUX_ASSERT(firstStillSizedLineIndex < lineCount);
                MUX_ASSERT(m_firstFrozenLineIndex > 0);
                MUX_ASSERT(m_firstFrozenLineIndex - 1 >= m_firstSizedLineIndex);
                MUX_ASSERT(adjustedFirstStillSizedItemIndex - 1 >= m_firstSizedItemIndex);

                ComputeItemsLayoutRegularPath(
                    availableWidth,
                    scrollViewport,
                    lineSpacing,
                    actualLineHeight,
                    m_firstFrozenLineIndex - 1 /*beginSizedLineIndex*/,
                    m_firstSizedLineIndex /*endSizedLineIndex*/,
                    adjustedFirstStillSizedItemIndex - 1 /*beginSizedItemIndex*/,
                    m_firstSizedItemIndex /*endSizedItemIndex*/,
                    0 /*beginLineVectorIndex*/,
                    true /*isLastSizedLineStretchEnabled*/);
            }
            else if (preFrozenItemHasNewDesiredWidth)
            {
                MUX_ASSERT(m_firstFrozenLineIndex > 0);
                MUX_ASSERT(m_firstFrozenItemIndex > 0);
                MUX_ASSERT(m_firstSizedLineIndex <= m_firstFrozenLineIndex - 1);
                MUX_ASSERT(m_firstSizedItemIndex <= m_firstFrozenItemIndex - 1);

                // Re-layout the pre-frozen range since at least one of the items in there has a modified DesiredSize.
                ComputeItemsLayoutRegularPath(
                    availableWidth,
                    scrollViewport,
                    lineSpacing,
                    actualLineHeight,
                    m_firstFrozenLineIndex - 1 /*beginSizedLineIndex*/,
                    m_firstSizedLineIndex /*endSizedLineIndex*/,
                    m_firstFrozenItemIndex - 1 /*beginSizedItemIndex*/,
                    m_firstSizedItemIndex /*endSizedItemIndex*/,
                    0 /*beginLineVectorIndex*/,
                    true /*isLastSizedLineStretchEnabled*/);
            }

            if (m_lastSizedLineIndex > lastStillSizedLineIndex)
            {
                if (m_lastFrozenLineIndex < lastStillSizedLineIndex)
                {
                    MUX_ASSERT(adjustedLastStillSizedItemIndex + 1 <= lastStillSizedItemIndex);

                    EnsureAndMeasureItemRange(
                        context,
                        availableWidth,
                        actualLineHeight,
                        true /*forward*/,
                        std::clamp(adjustedLastStillSizedItemIndex + 1, firstRealizedItemIndex, lastRealizedItemIndex) /*beginRealizedItemIndex*/,
                        std::clamp(lastStillSizedItemIndex, firstRealizedItemIndex, lastRealizedItemIndex) /*endRealizedItemIndex*/);
                }

                // Layout unfrozen & sized lines after the frozen ones.
                MUX_ASSERT(lastStillSizedLineIndex < lineCount);
                MUX_ASSERT(m_lastFrozenLineIndex < lineCount - 1);
                MUX_ASSERT(m_lastFrozenLineIndex + 1 <= m_lastSizedLineIndex);
                MUX_ASSERT(adjustedLastStillSizedItemIndex + 1 <= m_lastSizedItemIndex);

                ComputeItemsLayoutRegularPath(
                    availableWidth,
                    scrollViewport,
                    lineSpacing,
                    actualLineHeight,
                    m_lastFrozenLineIndex + 1 /*beginSizedLineIndex*/,
                    m_lastSizedLineIndex /*endSizedLineIndex*/,
                    adjustedLastStillSizedItemIndex + 1 /*beginSizedItemIndex*/,
                    m_lastSizedItemIndex /*endSizedItemIndex*/,
                    static_cast<int>(m_lineItemCounts.size()) - m_lastSizedLineIndex + m_lastFrozenLineIndex /*beginLineVectorIndex*/,
                    m_lastSizedLineIndex < lineCount - 1 /*isLastSizedLineStretchEnabled*/);
            }
            else if (postFrozenItemHasNewDesiredWidth)
            {
                MUX_ASSERT(m_lastFrozenLineIndex + 1 <= m_lastSizedLineIndex);
                MUX_ASSERT(m_lastFrozenItemIndex + 1 <= m_lastSizedItemIndex);

                // Re-layout the post-frozen range since at least one of the items in there has a modified DesiredSize.
                ComputeItemsLayoutRegularPath(
                    availableWidth,
                    scrollViewport,
                    lineSpacing,
                    actualLineHeight,
                    m_lastFrozenLineIndex + 1 /*beginSizedLineIndex*/,
                    m_lastSizedLineIndex /*endSizedLineIndex*/,
                    m_lastFrozenItemIndex + 1 /*beginSizedItemIndex*/,
                    m_lastSizedItemIndex /*endSizedItemIndex*/,
                    static_cast<int>(m_lineItemCounts.size()) - m_lastSizedLineIndex + m_lastFrozenLineIndex /*beginLineVectorIndex*/,
                    m_lastSizedLineIndex < lineCount - 1 /*isLastSizedLineStretchEnabled*/);
            }
        }
    }
    else
    {
        // No recurring sized lines. Layout all the new sized lines.
        ComputeItemsLayoutRegularPath(
            availableWidth,
            scrollViewport,
            lineSpacing,
            actualLineHeight,
            m_firstSizedLineIndex /*beginSizedLineIndex*/,
            m_lastSizedLineIndex  /*endSizedLineIndex*/,
            unsizedNearItemCount  /*beginSizedItemIndex*/,
            unsizedNearItemCount + sizedItemCount - 1 /*endSizedItemIndex*/,
            0 /*beginLineVectorIndex*/,
            m_lastSizedLineIndex < lineCount - 1 /*isLastSizedLineStretchEnabled*/);

        int adjustedFirstSizedItemIndex;
        int adjustedLastSizedItemIndex;

        ComputeFrozenItemsRange(
            scrollViewport,
            scrollOffset,
            lineSpacing,
            actualLineHeight,
            lineCount,
            m_firstSizedLineIndex /*beginSizedLineIndex*/,
            m_lastSizedLineIndex  /*endSizedLineIndex*/,
            unsizedNearItemCount  /*beginSizedItemIndex*/,
            unsizedNearItemCount + sizedItemCount - 1 /*endSizedItemIndex*/,
            adjustedFirstSizedItemIndex,
            adjustedLastSizedItemIndex);
    }

#ifdef DBG
    if (itemHasNewDesiredWidth)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns True");
    }
#endif

    return itemHasNewDesiredWidth;
}

// Updates the m_firstFrozenLineIndex, m_lastFrozenLineIndex, m_firstFrozenItemIndex and m_lastFrozenItemIndex fields.
// Returns 'True' when still sized lines must be ignored & a full re-layout must be performed.
// beginSizedLineIndex:
//  - m_firstSizedLineIndex when no scrolling occurred since the last measure pass, or when doing a full re-layout.
//  - or first sized line index common among the previous and current measure passes. Slightly different from m_firstSizedLineIndex because of scrolling.
// endSizedLineIndex:
//  - m_lastSizedLineIndex when no scrolling occurred since the last measure pass, or when doing a full re-layout.
//  - or last sized line index common among the previous and current measure passes. Slightly different from m_lastSizedLineIndex because of scrolling.
// beginSizedItemIndex:
//  - first sized item index when no scrolling occurred since the last measure pass, or when doing a full re-layout.
//  - or first sized item index common among the previous and current measure passes. Slightly different from previously first sized item index because of scrolling.
// endSizedItemIndex:
//  - last sized item index when no scrolling occurred since the last measure pass, or when doing a full re-layout.
//  - or last sized item index common among the previous and current measure passes. Slightly different from previously last sized item index because of scrolling.
// adjustedBeginSizedItemIndex:
//  returned as a variation of beginSizedItemIndex. For example, when the scroll offset decreased, the m_firstFrozenLineIndex and m_firstFrozenItemIndex can decrease
//  and adjustedBeginSizedItemIndex becomes beginSizedItemIndex minus the number of items that have become frozen.
// adjustedEndSizedItemIndex:
//  returned as a variation of endSizedItemIndex. For example, when the scroll offset increased, the m_lastFrozenLineIndex and m_lastFrozenItemIndex can increase
//  and adjustedEndSizedItemIndex becomes endSizedItemIndex plus the number of items that have become frozen.
bool LinedFlowLayout::ComputeFrozenItemsRange(
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
    int& adjustedEndSizedItemIndex)
{
    MUX_ASSERT(!UsesFastPathLayout());
    MUX_ASSERT(m_isVirtualizingContext == (scrollViewport != std::numeric_limits<double>::infinity()));
    MUX_ASSERT(!(beginSizedLineIndex < endSizedLineIndex && beginSizedItemIndex >= endSizedItemIndex));
    MUX_ASSERT(!(beginSizedLineIndex > endSizedLineIndex && beginSizedItemIndex <= endSizedItemIndex));
    MUX_ASSERT(std::abs(beginSizedLineIndex - endSizedLineIndex) <= std::abs(beginSizedItemIndex - endSizedItemIndex));

    adjustedBeginSizedItemIndex = -1;
    adjustedEndSizedItemIndex = -1;

    if (!m_isVirtualizingContext)
    {
        // Layout operating in non-virtualizing mode. All lines and items are frozen.
        MUX_ASSERT(m_firstSizedLineIndex == 0);
        MUX_ASSERT(m_itemCount > 0);

        m_firstFrozenLineIndex = 0;
        m_lastFrozenLineIndex = m_lastSizedLineIndex;
        m_firstFrozenItemIndex = 0;
        m_lastFrozenItemIndex = m_itemCount - 1;
        return false;
    }

    int firstDisplayedLineIndex = -1;
    int lastDisplayedLineIndex = -1;

    GetFirstAndLastDisplayedLineIndexes(
        scrollViewport,
        scrollOffset,
        0 /*padding*/,
        lineSpacing,
        actualLineHeight,
        lineCount,
        false /*forFullyDisplayedLines*/,
        &firstDisplayedLineIndex,
        &lastDisplayedLineIndex);

    if (firstDisplayedLineIndex == -1)
    {
        // This situation occurs when actualLineHeight + lineSpacing or context.VisibleRect().Height is 0.
        m_firstFrozenLineIndex = -1;
        m_lastFrozenLineIndex = -1;
        m_firstFrozenItemIndex = -1;
        m_lastFrozenItemIndex = -1;
        return false;
    }

    MUX_ASSERT(lastDisplayedLineIndex != -1);
    MUX_ASSERT(firstDisplayedLineIndex >= m_firstSizedLineIndex);
    MUX_ASSERT(lastDisplayedLineIndex <= m_lastSizedLineIndex);

    const int linesPerScrollViewport = static_cast<int>(scrollViewport / (actualLineHeight + lineSpacing));

    // Frozen lines before the first displayed line use 80% of a scroll viewport or 40% of the sized area whichever is largest.
    int nearFrozenLinesCount = std::min(static_cast<int>(c_frozenLinesRatio * linesPerScrollViewport) + 1, firstDisplayedLineIndex - m_firstSizedLineIndex);
    nearFrozenLinesCount = std::max(nearFrozenLinesCount, static_cast<int>(c_frozenLinesRatio / 2.0 * (firstDisplayedLineIndex - m_firstSizedLineIndex)));

    // Frozen lines after the last displayed line use 80% of a scroll viewport or 40% of the sized area whichever is largest.
    int farFrozenLinesCount = std::min(static_cast<int>(c_frozenLinesRatio * linesPerScrollViewport) + 1, m_lastSizedLineIndex - lastDisplayedLineIndex);
    farFrozenLinesCount = std::max(farFrozenLinesCount, static_cast<int>(c_frozenLinesRatio / 2.0 * (m_lastSizedLineIndex - lastDisplayedLineIndex)));
    
    m_firstFrozenLineIndex = firstDisplayedLineIndex - nearFrozenLinesCount;
    m_lastFrozenLineIndex = lastDisplayedLineIndex + farFrozenLinesCount;

    MUX_ASSERT(m_firstFrozenLineIndex >= m_firstSizedLineIndex);
    MUX_ASSERT(m_lastFrozenLineIndex <= m_lastSizedLineIndex);

#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_firstFrozenLineIndex", m_firstFrozenLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_lastFrozenLineIndex", m_lastFrozenLineIndex);

    // Check if the previously first displayed & arranged line is still frozen
    if (m_previousFirstDisplayedArrangedLineIndexDbg != -1 && (m_previousFirstDisplayedArrangedLineIndexDbg < m_firstFrozenLineIndex || m_previousFirstDisplayedArrangedLineIndexDbg > m_lastFrozenLineIndex))
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_previousFirstDisplayedArrangedLineIndexDbg", m_previousFirstDisplayedArrangedLineIndexDbg);
    }

    // Check if the previously last displayed & arranged line is still frozen
    if (m_previousLastDisplayedArrangedLineIndexDbg != -1 && (m_previousLastDisplayedArrangedLineIndexDbg < m_firstFrozenLineIndex || m_previousLastDisplayedArrangedLineIndexDbg > m_lastFrozenLineIndex))
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_previousLastDisplayedArrangedLineIndexDbg", m_previousLastDisplayedArrangedLineIndexDbg);
    }
#endif

    const int nearUnfrozenLinesCount = m_firstFrozenLineIndex - beginSizedLineIndex;
    const int farUnfrozenLinesCount = endSizedLineIndex - m_lastFrozenLineIndex;

    if (nearUnfrozenLinesCount < 0 || farUnfrozenLinesCount < 0)
    {
        // nearUnfrozenLinesCount < 0 occurs when the scrolling offset has decreased too fast and the first frozen line has caught up and crossed the previous first sized line
        // farUnfrozenLinesCount < 0 occurs when the scrolling offset has increased too fast and the last frozen line has caught up and crossed the previous last sized line
        // These conditions can also occur when performing a large programmatic offset jump via ScrollView.ScrollTo/ScrollBy.  Trigger a full re-layout.

        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"nearUnfrozenLinesCount", nearUnfrozenLinesCount);
        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"farUnfrozenLinesCount", farUnfrozenLinesCount);

        return true;
    }

    int firstFrozenItemIndex = beginSizedItemIndex;
    int lastFrozenItemIndex = endSizedItemIndex;

    adjustedBeginSizedItemIndex = beginSizedItemIndex;
    adjustedEndSizedItemIndex = endSizedItemIndex;

    if (m_firstSizedLineIndex < beginSizedLineIndex)
    {
        MUX_ASSERT(beginSizedItemIndex > 0);
        MUX_ASSERT(m_firstSizedItemIndex <= beginSizedItemIndex);

        if (nearUnfrozenLinesCount > 0)
        {
            MUX_ASSERT(beginSizedLineIndex - m_firstSizedLineIndex + nearUnfrozenLinesCount <= static_cast<int>(m_lineItemCounts.size()));

            for (int frozenLineVectorIndex = beginSizedLineIndex - m_firstSizedLineIndex;
                frozenLineVectorIndex < beginSizedLineIndex - m_firstSizedLineIndex + nearUnfrozenLinesCount;
                frozenLineVectorIndex++)
            {
                MUX_ASSERT(m_lineItemCounts[frozenLineVectorIndex] > 0);

                adjustedBeginSizedItemIndex += m_lineItemCounts[frozenLineVectorIndex];
                m_lineItemCounts[frozenLineVectorIndex] = 0;
            }

            firstFrozenItemIndex = adjustedBeginSizedItemIndex;
        }
    }
    else if (nearUnfrozenLinesCount > 0)
    {
        MUX_ASSERT(m_firstSizedLineIndex == beginSizedLineIndex);
        MUX_ASSERT(nearUnfrozenLinesCount <= static_cast<int>(m_lineItemCounts.size()));

        for (int frozenLineVectorIndex = 0; frozenLineVectorIndex < nearUnfrozenLinesCount; frozenLineVectorIndex++)
        {
            MUX_ASSERT(m_lineItemCounts[frozenLineVectorIndex] > 0);

            firstFrozenItemIndex += m_lineItemCounts[frozenLineVectorIndex];
        }
    }

    if (endSizedLineIndex < m_lastSizedLineIndex)
    {
        if (farUnfrozenLinesCount > 0)
        {
            for (int frozenLineVectorIndex = static_cast<int>(m_lineItemCounts.size()) - farUnfrozenLinesCount - m_lastSizedLineIndex + endSizedLineIndex;
                frozenLineVectorIndex < static_cast<int>(m_lineItemCounts.size()) - m_lastSizedLineIndex + endSizedLineIndex;
                frozenLineVectorIndex++)
            {
                MUX_ASSERT(m_lineItemCounts[frozenLineVectorIndex] > 0);

                adjustedEndSizedItemIndex -= m_lineItemCounts[frozenLineVectorIndex];
                m_lineItemCounts[frozenLineVectorIndex] = 0;
            }

            lastFrozenItemIndex = adjustedEndSizedItemIndex;
        }
    }
    else if (farUnfrozenLinesCount > 0)
    {
        MUX_ASSERT(endSizedLineIndex == m_lastSizedLineIndex);
        MUX_ASSERT(farUnfrozenLinesCount <= static_cast<int>(m_lineItemCounts.size()));

        for (int frozenLineVectorIndex = static_cast<int>(m_lineItemCounts.size()) - farUnfrozenLinesCount;
            frozenLineVectorIndex < static_cast<int>(m_lineItemCounts.size());
            frozenLineVectorIndex++)
        {
            MUX_ASSERT(m_lineItemCounts[frozenLineVectorIndex] > 0);

            lastFrozenItemIndex -= m_lineItemCounts[frozenLineVectorIndex];
        }
    }

    m_firstFrozenItemIndex = firstFrozenItemIndex;
    m_lastFrozenItemIndex = lastFrozenItemIndex;

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_firstFrozenItemIndex", firstFrozenItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_lastFrozenItemIndex", lastFrozenItemIndex);

    return false;
}

// Computes an drawback for the provided ItemsLayout. That drawback is stored in ItemsLayout.m_drawback, the smaller the better.
// The drawback is the sum of distances between the available width and the line widths.
void LinedFlowLayout::ComputeItemsLayoutDrawback(
    double availableWidth,
    bool isLastSizedLineStretchEnabled,
    ItemsLayout& itemsLayout) const
{
    const int sizedLineCount = static_cast<int>(itemsLayout.m_lineItemWidths.size());
    const int lineVectorCount = static_cast<int>(isLastSizedLineStretchEnabled ? sizedLineCount : sizedLineCount - 1);

    if (lineVectorCount == 0)
    {
        itemsLayout.m_drawback = 0;
        return;
    }

    const double c_lineTooSmallExponent = 2.0;
    // Be careful when using std::pow(..., 3) as a negative input results in a negative output.
    // For the moment, c_lineTooLargeExponent is only applied to positive numbers, so its use is safe.
    const double c_lineTooLargeExponent = 3.0;
    double drawback = 0.0;

    for (int lineVectorIndex = 0; lineVectorIndex < lineVectorCount; lineVectorIndex++)
    {
        const double lineWidthDelta = itemsLayout.m_lineItemWidths[lineVectorIndex] - availableWidth;

        if (lineWidthDelta != 0.0)
        {
            // To minimize the occurrences of lines that exceed the available width, they are graded worse than lines that have room to spare.
            drawback += std::pow(lineWidthDelta, lineWidthDelta > 0.0 ? c_lineTooLargeExponent : c_lineTooSmallExponent);
        }
    }

    // The last line does not participate in the grading if it's smaller than the available width, whether ItemsStretch is Fill or not.
    if (!isLastSizedLineStretchEnabled) 
    {
        const double lineWidthDelta = itemsLayout.m_lineItemWidths[sizedLineCount - 1] - availableWidth;

        if (lineWidthDelta > 0.0)
        {
            // The last line exceeds the available width, grade it as such.
            drawback += std::pow(lineWidthDelta, c_lineTooLargeExponent);
        }
    }

    itemsLayout.m_drawback = drawback;
}

// Computes the final item widths based on the best ItemsLayout, available width and stretching mode.
// Items are potentially shrunk to accommodate a smaller available line width, or stretched to accommodate a larger available line width.
void LinedFlowLayout::ComputeItemsLayoutWithLockedItems(
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
    bool isLastSizedLineStretchEnabled)
{
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginSizedLineIndex", beginSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endSizedLineIndex", endSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginSizedItemIndex", beginSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endSizedItemIndex", endSizedItemIndex);

    MUX_ASSERT(!UsesFastPathLayout());
    MUX_ASSERT(!(beginSizedLineIndex < endSizedLineIndex && beginSizedItemIndex >= endSizedItemIndex));
    MUX_ASSERT(!(beginSizedLineIndex > endSizedLineIndex && beginSizedItemIndex <= endSizedItemIndex));
    MUX_ASSERT(std::abs(beginSizedLineIndex - endSizedLineIndex) <= std::abs(beginSizedItemIndex - endSizedItemIndex));

    const bool forward = beginSizedItemIndex <= endSizedItemIndex;
    const int sizedLineCount = static_cast<int>(itemsLayout.m_lineItemCounts.size());

#ifdef DBG
    MUX_ASSERT(sizedLineCount == static_cast<int>(forward ? endSizedLineIndex - beginSizedLineIndex + 1 : beginSizedLineIndex - endSizedLineIndex + 1));

    const int sizedLineCountDbg = forward ? endSizedItemIndex - beginSizedItemIndex + 1 : beginSizedItemIndex - endSizedItemIndex + 1;

    MUX_ASSERT(sizedLineCountDbg >= sizedLineCount);
#endif

    int lineVectorIndex;

    for (lineVectorIndex = 0; lineVectorIndex < sizedLineCount; lineVectorIndex++)
    {
        MUX_ASSERT(itemsLayout.m_lineItemCounts[lineVectorIndex] > 0);

        m_lineItemCounts[beginLineVectorIndex + lineVectorIndex] = itemsLayout.m_lineItemCounts[lineVectorIndex];
    }

    const bool itemsAreStretched = ItemsStretch() == winrt::LinedFlowLayoutItemsStretch::Fill;
    int sizedItemIndex = beginSizedItemIndex;

    for (lineVectorIndex = forward ? 0 : sizedLineCount - 1; ; lineVectorIndex = forward ? lineVectorIndex + 1 : lineVectorIndex - 1)
    {
        const int lineItemsCount = itemsLayout.m_lineItemCounts[lineVectorIndex];
        const double lineItemsWidth = itemsLayout.m_lineItemWidths[lineVectorIndex];
        const double minItemSpacings = lineItemsCount > 1 ? (lineItemsCount - 1) * minItemSpacing : 0.0;
        double scaleFactor = 1.0;

        if (lineItemsWidth - minItemSpacings > 0.0)
        {
            if (lineItemsWidth > availableWidth)
            {
                scaleFactor = ComputeLineShrinkFactor(
                    forward,
                    sizedItemIndex,
                    lineItemsCount,
                    lineItemsWidth,
                    availableWidth,
                    minItemSpacings,
                    actualLineHeight,
                    averageAspectRatio);
                MUX_ASSERT(scaleFactor >= 0.0);
                MUX_ASSERT(scaleFactor < 1.0);
            }
            else if ((!forward || lineVectorIndex < (isLastSizedLineStretchEnabled ? sizedLineCount : sizedLineCount - 1)) &&
                lineItemsWidth < availableWidth &&
                itemsAreStretched)
            {
                scaleFactor = ComputeLineExpandFactor(
                    forward,
                    sizedItemIndex,
                    lineItemsCount,
                    lineItemsWidth,
                    availableWidth,
                    minItemSpacings,
                    actualLineHeight,
                    averageAspectRatio);
                MUX_ASSERT(scaleFactor > 1.0);
            }
        }

        if (scaleFactor != 1.0)
        {
            for (int itemIndex = 0; itemIndex < lineItemsCount; itemIndex++)
            {
                MUX_ASSERT((forward && sizedItemIndex <= endSizedItemIndex) || (!forward && sizedItemIndex >= endSizedItemIndex));

                double minWidth{};

                if (m_itemsInfoFirstIndex == -1)
                {
                    MUX_ASSERT(m_elementManager.IsDataIndexRealized(sizedItemIndex));

                    if (const auto element = m_elementManager.GetRealizedElement(sizedItemIndex /*dataIndex*/))
                    {
                        if (scaleFactor < 1.0)
                        {
                            if (const auto frameworkElement = element.try_as<winrt::FrameworkElement>())
                            {
                                minWidth = frameworkElement.MinWidth();
                            }
                        }

                        auto elementAvailableSize = winrt::Size(
                            std::max(static_cast<float>(minWidth), static_cast<float>(element.DesiredSize().Width * scaleFactor)),
                            static_cast<float>(actualLineHeight));

                        element.Measure(elementAvailableSize);

                        const float itemWidth = element.DesiredSize().Width;

                        elementAvailableSize.Width = itemWidth;

                        MUX_ASSERT(m_elementAvailableWidths);

                        const auto elementTracker = tracker_ref<winrt::UIElement>(this, element);

                        if (m_elementAvailableWidths->find(elementTracker) == m_elementAvailableWidths->end())
                        {
                            m_elementAvailableWidths->insert(std::pair<tracker_ref<winrt::UIElement>, float>(elementTracker, itemWidth));
                        }
                        else
                        {
                            m_elementAvailableWidths->emplace(elementTracker, itemWidth);
                        }

#ifdef DBG
                        if (sizedItemIndex == m_logItemIndexDbg)
                        {
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"measured with elementAvailableSize (1)",
                                elementAvailableSize.Width,
                                elementAvailableSize.Height);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"element.DesiredSize",
                                element.DesiredSize().Width,
                                element.DesiredSize().Height);
                        }
#endif
                    }
                }
                else
                {
                    MUX_ASSERT(sizedItemIndex - m_itemsInfoFirstIndex < static_cast<int>(m_itemsInfoArrangeWidths.size()));

                    const float arrangeWidth = GetArrangeWidthFromItemsInfo(sizedItemIndex, actualLineHeight, averageAspectRatio, scaleFactor);

                    SetArrangeWidthFromItemsInfo(sizedItemIndex, arrangeWidth);

                    if (m_elementManager.IsDataIndexRealized(sizedItemIndex))
                    {
                        if (const auto element = m_elementManager.GetRealizedElement(sizedItemIndex /*dataIndex*/))
                        {
                            const auto elementAvailableSize = winrt::Size(
                                arrangeWidth,
                                static_cast<float>(actualLineHeight));

                            element.Measure(elementAvailableSize);

#ifdef DBG
                            if (sizedItemIndex == m_logItemIndexDbg)
                            {
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"measured with elementAvailableSize (2)",
                                    elementAvailableSize.Width,
                                    elementAvailableSize.Height);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"element.DesiredSize",
                                    element.DesiredSize().Width,
                                    element.DesiredSize().Height);
                            }
#endif
                        }
                    }
                }

                sizedItemIndex = forward ? sizedItemIndex + 1 : sizedItemIndex - 1;
            }
        }
        else
        {
            // Measuring items even when no scale factor is applied so that they fill their available width (otherwise background-colored bands appear on the items' left/right edges).
            for (int itemIndex = 0; itemIndex < lineItemsCount; itemIndex++)
            {
                MUX_ASSERT((forward && sizedItemIndex <= endSizedItemIndex) || (!forward && sizedItemIndex >= endSizedItemIndex));
                MUX_ASSERT(m_itemsInfoFirstIndex == -1 || sizedItemIndex - m_itemsInfoFirstIndex < static_cast<int>(m_itemsInfoArrangeWidths.size()));

                float arrangeWidth{};

                if (m_itemsInfoFirstIndex != -1)
                {
                    // Case where LinedFlowLayout.ItemsInfoRequested returned partial sizing information.
                    // Retrieve the item's arrange width computed from the m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath,
                    // m_itemsInfoMinWidth, m_itemsInfoMaxWidth fields.
                    arrangeWidth = GetArrangeWidthFromItemsInfo(sizedItemIndex, actualLineHeight, averageAspectRatio);

                    // Store that arrange width in the m_itemsInfoArrangeWidths vector.
                    SetArrangeWidthFromItemsInfo(sizedItemIndex, arrangeWidth);
                }

                if (m_elementManager.IsDataIndexRealized(sizedItemIndex))
                {
                    if (const auto element = m_elementManager.GetRealizedElement(sizedItemIndex /*dataIndex*/))
                    {
                        if (m_itemsInfoFirstIndex == -1)
                        {
                            // Case where LinedFlowLayout.ItemsInfoRequested returned no sizing information.
                            // Retrieve the item's arrange width as its desired width.
                            arrangeWidth = element.DesiredSize().Width;
                        }

                        const auto elementAvailableSize = winrt::Size(
                            arrangeWidth,
                            static_cast<float>(actualLineHeight));

                        element.Measure(elementAvailableSize);

#ifdef DBG
                        if (sizedItemIndex == m_logItemIndexDbg)
                        {
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"measured with elementAvailableSize (3)",
                                elementAvailableSize.Width,
                                elementAvailableSize.Height);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"element.DesiredSize",
                                element.DesiredSize().Width,
                                element.DesiredSize().Height);
                        }
#endif
                    }
                }

                sizedItemIndex = forward ? sizedItemIndex + 1 : sizedItemIndex - 1;
            }

            // Making sure the items fill the line when ItemsStretch is Fill.
            MUX_ASSERT(!(itemsAreStretched && availableWidth - lineItemsWidth > lineItemsCount && lineItemsWidth > minItemSpacings && lineVectorIndex != sizedLineCount - 1));
        }

        if (forward && lineVectorIndex == sizedLineCount - 1)
        {
            break;
        }
        if (!forward && lineVectorIndex == 0)
        {
            break;
        }
    }
}

// Computes the best layout by assigning items to lines - for the fast path.
// This method populates the m_itemsInfoArrangeWidths and m_lineItemCounts vectors successively, using the
// m_itemsInfoDesiredAspectRatiosForFastPath, m_itemsInfoMinWidthsForFastPath, m_itemsInfoMaxWidthsForFastPath,
// m_itemsInfoMinWidth, m_itemsInfoMaxWidth fields.
// Returns the largest line width among all the lines.
float LinedFlowLayout::ComputeItemsLayoutFastPath(
    float availableWidth,
    double actualLineHeight)
{
    MUX_ASSERT(m_itemCount > 0);

    EnsureItemsInfoArrangeWidths(m_itemCount);

    MUX_ASSERT(UsesFastPathLayout());

    // The existing average aspect ratio is used as a fallback aspect ratio for items
    // given an aspect ratio <= 0 by the ItemsInfoRequested handler.
    const double averageAspectRatio = GetAverageAspectRatio(availableWidth, actualLineHeight);

    for (int itemIndex = 0; itemIndex < m_itemCount; itemIndex++)
    {
        const float arrangeWidth = GetArrangeWidthFromItemsInfo(itemIndex, actualLineHeight, averageAspectRatio);

        SetArrangeWidthFromItemsInfo(itemIndex, arrangeWidth);
    }

    // Final line count is originally unknown. m_lineItemCounts is originally allocated to accommodate 5 items per line.
    // The vector is progressively grown as needed when that assumption is too optimistic.
    constexpr int c_itemsPerLineAllocation = 5;
    // The vector capacity is increased by 25% each time its size becomes too small.
    constexpr double c_lineCountGrowthFactor = 1.25;
    int allocatedLineCount{ m_itemCount / c_itemsPerLineAllocation + 1 };

    EnsureLineItemCounts(allocatedLineCount);

    const float minItemSpacing{ static_cast<float>(MinItemSpacing()) };
    const bool itemsAreStretched{ ItemsStretch() == winrt::LinedFlowLayoutItemsStretch::Fill };
    int lineIndex{ -1 };
    int lineItemCount{ 0 };
    float lineWidth{ 0.0f };
    float maxLineWidth{ 0.0f };

    for (int itemIndex = 0; itemIndex < m_itemCount; itemIndex++)
    {
        const float arrangeWidth{ m_itemsInfoArrangeWidths[itemIndex] };
        double scaleFactor{ 1.0 };

        if (lineWidth == 0.0f || lineWidth + minItemSpacing + arrangeWidth > availableWidth)
        {
            bool cumulate{ lineWidth != 0.0f };

            if (cumulate)
            {
                // Additional item goes beyond the available width.
                MUX_ASSERT(lineWidth > 0.0);
                MUX_ASSERT(lineWidth + minItemSpacing + arrangeWidth > availableWidth);
                MUX_ASSERT(lineItemCount > 0);

                // Determine whether it is better to create a new line or not.

                const double shrinkScaleFactor = ComputeLineShrinkFactor(
                    true /*forward*/,
                    itemIndex - lineItemCount /*sizedItemIndex*/,
                    lineItemCount + 1,
                    lineWidth + minItemSpacing + arrangeWidth /*lineItemsWidth*/,
                    availableWidth,
                    lineItemCount * minItemSpacing /*minItemSpacings*/,
                    actualLineHeight,
                    averageAspectRatio);

                MUX_ASSERT(shrinkScaleFactor >= 0.0);
                MUX_ASSERT(shrinkScaleFactor < 1.0);

                const double expandScaleFactor = ComputeLineExpandFactor(
                    true /*forward*/,
                    itemIndex - lineItemCount /*sizedItemIndex*/,
                    lineItemCount,
                    lineWidth /*lineItemsWidth*/,
                    availableWidth,
                    (lineItemCount - 1) * minItemSpacing /*minItemSpacings*/,
                    actualLineHeight,
                    averageAspectRatio);

                MUX_ASSERT(expandScaleFactor > 1.0);

                if (expandScaleFactor - 1.0 < 1.0 - shrinkScaleFactor || shrinkScaleFactor == 0.0)
                {
                    // Creating a new line and leaving a gap in the current one requires a smaller
                    // expansion than the shrinkage required to add this item.
                    // Or the items' min widths prevent the shrinkage required to fit them in the line.
                    cumulate = false;

                    if (itemsAreStretched)
                    {
                        // Only expand the items when ItemsStretch is LinedFlowLayoutItemsStretch::Fill.
                        scaleFactor = expandScaleFactor;
                    }
                }
                else
                {
                    // The line items need to shrink less to accommodate this additional item than expand to fill the gap,
                    // let it belong to the current line (cumulate == true).
                    scaleFactor = shrinkScaleFactor;
                }
            }

            if (cumulate)
            {
                lineWidth += minItemSpacing + arrangeWidth;
                lineItemCount++;
            }
            else
            {
                if (lineIndex >= 0 && m_lineItemCounts[lineIndex] == 0)
                {
                    MUX_ASSERT(lineIndex < allocatedLineCount);
                    MUX_ASSERT(lineItemCount > 0);
                    MUX_ASSERT(lineWidth > 0);

                    m_lineItemCounts[lineIndex] = lineItemCount;

                    if (lineIndex + 1 == allocatedLineCount)
                    {
                        allocatedLineCount = std::max(allocatedLineCount + 1, static_cast<int>(c_lineCountGrowthFactor * allocatedLineCount));
                        m_lineItemCounts.resize(allocatedLineCount, 0);
                    }

                    if (scaleFactor == 1.0)
                    {
                        maxLineWidth = std::max(lineWidth, maxLineWidth);
                    }
                    else
                    {
                        // Apply shrinking or expanding scale factor to line's m_itemsInfoArrangeWidths.
                        const float totalArrangeWidth = SetItemRangeArrangeWidth(
                            itemIndex - lineItemCount /*beginItemIndex*/,
                            itemIndex - 1 /*endItemIndex*/,
                            actualLineHeight,
                            averageAspectRatio,
                            scaleFactor);

                        maxLineWidth = std::max(totalArrangeWidth + (lineItemCount - 1) * minItemSpacing, maxLineWidth);
                    }
                }

                lineWidth = arrangeWidth;
                lineItemCount = 1;
                lineIndex++;
            }
        }
        else
        {
            lineWidth += minItemSpacing + arrangeWidth;
            lineItemCount++;
        }

        if (lineWidth >= availableWidth)
        {
            MUX_ASSERT(lineIndex >= 0);
            MUX_ASSERT(lineIndex < allocatedLineCount);
            MUX_ASSERT(lineItemCount > 0);
            MUX_ASSERT(lineWidth > 0);

            m_lineItemCounts[lineIndex] = lineItemCount;

            if (lineIndex + 1 == allocatedLineCount)
            {
                allocatedLineCount = std::max(allocatedLineCount + 1, static_cast<int>(c_lineCountGrowthFactor * allocatedLineCount));
                m_lineItemCounts.resize(allocatedLineCount, 0);
            }

            if (lineItemCount == 1 && lineWidth != availableWidth)
            {
                // The single item on the line is bigger than the available width.
                scaleFactor = ComputeLineShrinkFactor(
                    true /*forward*/,
                    itemIndex /*sizedItemIndex*/,
                    1 /*lineItemCount*/,
                    lineWidth /*lineItemsWidth*/,
                    availableWidth,
                    0.0 /*minItemSpacings*/,
                    actualLineHeight,
                    averageAspectRatio);

                MUX_ASSERT(scaleFactor >= 0.0);
                MUX_ASSERT(scaleFactor < 1.0);
            }

            if (scaleFactor != 0.0 && scaleFactor != 1.0)
            {
                // Apply shrinking scale factor to line's m_itemsInfoArrangeWidths.
                const float totalArrangeWidth = SetItemRangeArrangeWidth(
                    itemIndex - lineItemCount + 1 /*beginItemIndex*/,
                    itemIndex /*endItemIndex*/,
                    actualLineHeight,
                    averageAspectRatio,
                    scaleFactor);

                maxLineWidth = std::max(totalArrangeWidth + (lineItemCount - 1) * minItemSpacing, maxLineWidth);
            }
            else
            {
                maxLineWidth = std::max(lineWidth, maxLineWidth);
            }

            lineWidth = 0.0f;
            lineItemCount = 0;
        }
    }

    MUX_ASSERT(lineIndex >= 0);
    MUX_ASSERT(lineIndex < allocatedLineCount);

    if (lineItemCount > 0)
    {
        MUX_ASSERT(lineWidth > 0);
        MUX_ASSERT(availableWidth >= lineWidth);

        maxLineWidth = std::max(lineWidth, maxLineWidth);
        m_lineItemCounts[lineIndex] = lineItemCount;
    }

#ifdef DBG
    for (int lineIndexTmp = 0; lineIndexTmp <= lineIndex; lineIndexTmp++)
    {
        MUX_ASSERT(m_lineItemCounts[lineIndexTmp] > 0);
    }
#endif

    m_lineItemCounts.resize(lineIndex + 1, 0);

#ifdef DBG
    for (int lineIndexTmp = 0; lineIndexTmp <= lineIndex; lineIndexTmp++)
    {
        MUX_ASSERT(m_lineItemCounts[lineIndexTmp] > 0);
    }
#endif

    return maxLineWidth;
}

// Computes the ItemsLayout with the best drawback by assigning items to lines - for the regular path.
// This method populates the m_itemsInfoArrangeWidths / m_lineItemCounts vectors and the m_elementAvailableWidths map,
// using the m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath,
// m_itemsInfoMinWidth, m_itemsInfoMaxWidth fields.
// The work is broken down into 6 successive phases:
// Phase 1: Compute an initial layout and its drawback using an available width coming from LinedFlowLayout::MeasureOverride.
//          This phase provides an average line width ALW for the following phases which successively try to compute layouts with a lower drawback.
// Phase 2: Compute another layout and drawback using the resulting average line width ALW from Phase 1 as the available width.
// Phase 3: Compute layouts and drawbacks with an available width within 30% of the average line width ALW from Phase 1 using the smallest head and smallest tail items of prior layouts.
//          The layout with the lowest drawback is kept - it corresponds to a best available width BAW.
// Phase 4: Compute a layout using the average of ALW and BAW for the available width, which may result in an even better drawback. 
// Phase 5: Keeping the available width from Phase 4, fine tune that layout by moving best equalizing head and best equalizing tail items into their neighboring lines using internal locks.
//          Whichever item among the best equalizing head and best equalizing tail is present and provides the largest drawback improvement is moved.
// Phase 6: Items are finally measured based on the best outcome of Phase 5.
void LinedFlowLayout::ComputeItemsLayoutRegularPath(
    float availableWidth,
    double scrollViewport,
    double lineSpacing,
    double actualLineHeight,
    int beginSizedLineIndex,
    int endSizedLineIndex,
    int beginSizedItemIndex,
    int endSizedItemIndex,
    int beginLineVectorIndex,
    bool isLastSizedLineStretchEnabled)
{
#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginSizedLineIndex", beginSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endSizedLineIndex", endSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginSizedItemIndex", beginSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endSizedItemIndex", endSizedItemIndex);

    MUX_ASSERT(m_isVirtualizingContext == (scrollViewport != std::numeric_limits<double>::infinity()));

    const int sizedItemCountDbg = m_lastSizedItemIndex - m_firstSizedItemIndex + 1;
    const int realizedItemCountDbg = m_elementManager.GetRealizedElementCount();
    const int unrealizedNearItemCountDbg = m_isVirtualizingContext ? m_elementManager.GetFirstRealizedDataIndex() : 0;

    MUX_ASSERT(!(beginSizedLineIndex < endSizedLineIndex && beginSizedItemIndex >= endSizedItemIndex));
    MUX_ASSERT(!(beginSizedLineIndex > endSizedLineIndex && beginSizedItemIndex <= endSizedItemIndex));
    MUX_ASSERT(std::abs(beginSizedLineIndex - endSizedLineIndex) <= std::abs(beginSizedItemIndex - endSizedItemIndex));
    MUX_ASSERT(beginSizedItemIndex > endSizedItemIndex || beginSizedItemIndex >= m_firstSizedItemIndex);
    MUX_ASSERT(beginSizedItemIndex > endSizedItemIndex || endSizedItemIndex <= m_firstSizedItemIndex + sizedItemCountDbg - 1);
    MUX_ASSERT(beginSizedItemIndex <= endSizedItemIndex || endSizedItemIndex >= m_firstSizedItemIndex);
    MUX_ASSERT(beginSizedItemIndex <= endSizedItemIndex || beginSizedItemIndex <= m_firstSizedItemIndex + sizedItemCountDbg - 1);
#endif

    const double minItemSpacing = MinItemSpacing();
    const double averageAspectRatio = GetAverageAspectRatio(availableWidth, actualLineHeight);
    double averageLineItemsWidth = std::numeric_limits<double>::max();
    std::vector<ItemsLayout> itemsLayouts;

    // Internal locks are used to force an item to belong to a particular line.
    std::map<int /*itemIndex*/, int /*lineIndex*/> internalLockedItemIndexes;

    // Phase 1: evaluate layout for an available width equal to the MeasureOverride's available width.
    ItemsLayout itemsLayout = GetItemsLayout(
        &internalLockedItemIndexes,
        scrollViewport,
        availableWidth,
        availableWidth /*adjustedAvailableWidth*/,
        -1.0 /*averageLineItemsWidth*/,
        averageAspectRatio,
        lineSpacing,
        actualLineHeight,
        beginSizedLineIndex,
        endSizedLineIndex,
        beginSizedItemIndex,
        endSizedItemIndex,
        beginLineVectorIndex,
        isLastSizedLineStretchEnabled);

    MUX_ASSERT(itemsLayout.m_availableLineItemsWidth > 0.0);
    MUX_ASSERT(itemsLayout.m_lineItemCounts.size() > 0);
    MUX_ASSERT(itemsLayout.m_lineItemWidths.size() > 0);

    itemsLayouts.push_back(itemsLayout);

#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"End-of-Phase-1 layout for MeasureOverride's availableWidth", availableWidth);
    LogItemsLayoutDbg(itemsLayout, 0);
#endif

    std::set<double> availableWidthsProcessed;

    if (beginSizedLineIndex != endSizedLineIndex)
    {
        availableWidthsProcessed.insert(availableWidth);

        // Phase 2: evaluate layout for an available width equal to the resulting average desired line items width of Phase 1.
        std::set<double> availableWidthsToProcess;

        double totalLineItemWidths = 0.0;

        for (const auto& lineItemWidthsIterator : itemsLayout.m_lineItemWidths)
        {
            totalLineItemWidths += lineItemWidthsIterator;
        }

        averageLineItemsWidth = totalLineItemWidths / itemsLayout.m_lineItemWidths.size();

        availableWidthsToProcess.insert(averageLineItemsWidth);

        double availableWidthLowerbound = averageLineItemsWidth;
        double availableWidthUpperbound = averageLineItemsWidth;

        while (availableWidthsToProcess.size() > 0)
        {
            const double adjustedAvailableWidth = *availableWidthsToProcess.begin();

            MUX_ASSERT(adjustedAvailableWidth >= 0.0);

            itemsLayout = GetItemsLayout(
                &internalLockedItemIndexes,
                scrollViewport,
                availableWidth,
                adjustedAvailableWidth,
                averageLineItemsWidth,
                averageAspectRatio,
                lineSpacing,
                actualLineHeight,
                beginSizedLineIndex,
                endSizedLineIndex,
                beginSizedItemIndex,
                endSizedItemIndex,
                beginLineVectorIndex,
                isLastSizedLineStretchEnabled);

#ifdef DBG
            LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"Phases-2&3 layout for adjustedAvailableWidth", adjustedAvailableWidth);
            LogItemsLayoutDbg(itemsLayout, averageLineItemsWidth);
#endif

            itemsLayouts.push_back(itemsLayout);
            availableWidthsProcessed.insert(adjustedAvailableWidth);

            // Phase 3: evaluate layouts with an available width within 30% of the average desired line items width using the smallest head and smallest tail items of prior layouts.
            // Any layouts with an available width further away is not likely to produce the best layout. Pick the layout with the lowest drawback.
            const double availableWidthClampingPercent = 0.3;
            const bool isExpansionWorthy = IsItemsLayoutExpansionWorthy(itemsLayout);
            const bool isContractionWorthy = IsItemsLayoutContractionWorthy(itemsLayout);

            if (isExpansionWorthy)
            {
                MUX_ASSERT(itemsLayout.m_smallestHeadItemWidth > 0);

                const double availableWidthToProcess = adjustedAvailableWidth + minItemSpacing + itemsLayout.m_smallestHeadItemWidth;

                if (std::abs(availableWidthToProcess - averageLineItemsWidth) < availableWidthClampingPercent * averageLineItemsWidth &&
                    availableWidthToProcess > availableWidthUpperbound &&
                    availableWidthToProcess != availableWidth &&
                    availableWidthsProcessed.find(availableWidthToProcess) == availableWidthsProcessed.end() &&
                    availableWidthsToProcess.find(availableWidthToProcess) == availableWidthsToProcess.end())
                {
                    availableWidthUpperbound = availableWidthToProcess;
                    availableWidthsToProcess.insert(availableWidthToProcess);
                }
            }

            if (isContractionWorthy)
            {
                MUX_ASSERT(itemsLayout.m_smallestTailItemWidth > 0);

                const double availableWidthToProcess = adjustedAvailableWidth - minItemSpacing - itemsLayout.m_smallestTailItemWidth;

                if (availableWidthToProcess > 0.0 &&
                    std::abs(availableWidthToProcess - averageLineItemsWidth) < availableWidthClampingPercent * averageLineItemsWidth &&
                    availableWidthToProcess < availableWidthLowerbound &&
                    availableWidthToProcess != availableWidth &&
                    availableWidthsProcessed.find(availableWidthToProcess) == availableWidthsProcessed.end() &&
                    availableWidthsToProcess.find(availableWidthToProcess) == availableWidthsToProcess.end())
                {
                    availableWidthLowerbound = availableWidthToProcess;
                    availableWidthsToProcess.insert(availableWidthToProcess);
                }
            }

            availableWidthsToProcess.erase(adjustedAvailableWidth);
        }
    }

    ItemsLayout bestItemsLayout = itemsLayouts[0];

    if (itemsLayouts.size() > 1)
    {
        // Figure out the layout with the lowest drawback so far.
        double bestDrawback = std::numeric_limits<double>::max();

        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemsLayouts.size", itemsLayouts.size());

        // Find the layout with the smallest drawback so far.
        for (const auto& itemsLayoutIterator : itemsLayouts)
        {
            ItemsLayout itemsLayoutTmp = itemsLayoutIterator;

            if (itemsLayoutTmp.m_drawback < bestDrawback)
            {
                bestDrawback = itemsLayoutTmp.m_drawback;
                bestItemsLayout = itemsLayoutTmp;
            }
        }

#ifdef DBG
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"End-of-Phase-4 best layout");
        LogItemsLayoutDbg(bestItemsLayout, averageLineItemsWidth);

        MUX_ASSERT(beginSizedLineIndex != endSizedLineIndex);
#endif

        // Phase 4: try a layout with adjustedAvailableWidth = (bestItemsLayout.m_availableLineItemsWidth + averageLineItemsWidth) / 2.0,
        // i.e. the average of the best available width so far and the average line width, as it sometimes results in a lower drawback.
        // A layout for averageLineItemsWidth was performed in Phase 2, a layout for bestItemsLayout.m_availableLineItemsWidth was performed
        // in Phase 3 - the optimum available width is likely to be in the neighborhood of those two numbers and trying a layout with their
        // average has proven worthy through experimentations.
        const double adjustedAvailableWidth = (bestItemsLayout.m_availableLineItemsWidth + averageLineItemsWidth) / 2.0;

        // Make sure adjustedAvailableWidth has not been processed already in Phase 3.
        if (availableWidthsProcessed.find(adjustedAvailableWidth) == availableWidthsProcessed.end())
        {
            itemsLayout = GetItemsLayout(
                &internalLockedItemIndexes,
                scrollViewport,
                availableWidth,
                adjustedAvailableWidth,
                averageLineItemsWidth,
                averageAspectRatio,
                lineSpacing,
                actualLineHeight,
                beginSizedLineIndex,
                endSizedLineIndex,
                beginSizedItemIndex,
                endSizedItemIndex,
                beginLineVectorIndex,
                isLastSizedLineStretchEnabled);

#ifdef DBG
            LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"Phase-5 layout for adjustedAvailableWidth", adjustedAvailableWidth);
            LogItemsLayoutDbg(itemsLayout, averageLineItemsWidth);
#endif

            if (itemsLayout.m_drawback < bestItemsLayout.m_drawback)
            {
                bestItemsLayout = itemsLayout;

                LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"End-of-Phase-5 picking phase 5's layout");
            }
        }

        ItemsLayout itemsLayoutToImprove = bestItemsLayout;
        // noDrawbackDecreaseCount indicates how many successive attempts at decreasing the drawback have failed.
        int noDrawbackDecreaseCount = 0;
        const bool forward = beginSizedLineIndex < endSizedLineIndex;

        // Phase 5: keeping the available width from Phase 4, fine tune that layout by moving best equalizing head and best equalizing tail items into their neighboring lines using internal locks.
        // Whichever item among the best equalizing head and best equalizing tail is present and provides the largest drawback improvement is moved.

        // Incrementally improve the drawback by moving the best equalizing head to its previous line, or the best equalizing tail to its next line.
        // The move with the best anticipated outcome is performed until several attempts produce no drawback improvements. 
        do
        {
            if (itemsLayoutToImprove.m_bestEqualizingHeadItemDrawbackImprovement != 0.0 &&
                (itemsLayoutToImprove.m_bestEqualizingTailItemDrawbackImprovement == 0.0 ||
                 itemsLayoutToImprove.m_bestEqualizingHeadItemDrawbackImprovement > itemsLayoutToImprove.m_bestEqualizingTailItemDrawbackImprovement))
            {
                if (IsItemsLayoutEqualizationWorthy(itemsLayoutToImprove, true /*withHeadItem*/))
                {
                    if (LockItemToLineInternal(
                            internalLockedItemIndexes,
                            beginSizedLineIndex,
                            endSizedLineIndex,
                            beginSizedItemIndex,
                            endSizedItemIndex,
                            forward ? itemsLayoutToImprove.m_bestEqualizingHeadLineIndex - 1 : itemsLayoutToImprove.m_bestEqualizingHeadLineIndex + 1,
                            itemsLayoutToImprove.m_bestEqualizingHeadItemIndex))
                    {
                        itemsLayout = GetItemsLayout(
                            &internalLockedItemIndexes,
                            scrollViewport,
                            availableWidth,
                            itemsLayoutToImprove.m_availableLineItemsWidth,
                            averageLineItemsWidth,
                            averageAspectRatio,
                            lineSpacing,
                            actualLineHeight,
                            beginSizedLineIndex,
                            endSizedLineIndex,
                            beginSizedItemIndex,
                            endSizedItemIndex,
                            beginLineVectorIndex,
                            isLastSizedLineStretchEnabled);

#ifdef DBG
                        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"Phase-6 layout after equalization attempt with head item", itemsLayout.m_drawback < bestItemsLayout.m_drawback ? L"improvement" : L"no improvement");
                        LogItemsLayoutDbg(itemsLayout, averageLineItemsWidth);
#endif

                        if (itemsLayout.m_drawback < bestItemsLayout.m_drawback)
                        {
                            bestItemsLayout = itemsLayout;
                            noDrawbackDecreaseCount = 0;
                        }
                        else
                        {
                            noDrawbackDecreaseCount++;
                        }

                        itemsLayoutToImprove = itemsLayout;

                        continue;
                    }
                }
            }

            if (itemsLayoutToImprove.m_bestEqualizingTailItemDrawbackImprovement != 0.0 &&
                (itemsLayoutToImprove.m_bestEqualizingHeadItemDrawbackImprovement == 0.0 ||
                 itemsLayoutToImprove.m_bestEqualizingTailItemDrawbackImprovement > itemsLayoutToImprove.m_bestEqualizingHeadItemDrawbackImprovement))
            {
                if (IsItemsLayoutEqualizationWorthy(itemsLayoutToImprove, false /*withHeadItem*/))
                {
                    if (LockItemToLineInternal(
                            internalLockedItemIndexes,
                            beginSizedLineIndex,
                            endSizedLineIndex,
                            beginSizedItemIndex,
                            endSizedItemIndex,
                            forward ? itemsLayoutToImprove.m_bestEqualizingTailLineIndex + 1 : itemsLayoutToImprove.m_bestEqualizingTailLineIndex - 1,
                            itemsLayoutToImprove.m_bestEqualizingTailItemIndex))
                    {
                        itemsLayout = GetItemsLayout(
                            &internalLockedItemIndexes,
                            scrollViewport,
                            availableWidth,
                            itemsLayoutToImprove.m_availableLineItemsWidth,
                            averageLineItemsWidth,
                            averageAspectRatio,
                            lineSpacing,
                            actualLineHeight,
                            beginSizedLineIndex,
                            endSizedLineIndex,
                            beginSizedItemIndex,
                            endSizedItemIndex,
                            beginLineVectorIndex,
                            isLastSizedLineStretchEnabled);

#ifdef DBG
                        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR, METH_NAME, this, L"Phase-6 layout after equalization attempt with tail item", itemsLayout.m_drawback < bestItemsLayout.m_drawback ? L"improvement" : L"no improvement");
                        LogItemsLayoutDbg(itemsLayout, averageLineItemsWidth);
#endif

                        if (itemsLayout.m_drawback < bestItemsLayout.m_drawback)
                        {
                            bestItemsLayout = itemsLayout;
                            noDrawbackDecreaseCount = 0;
                        }
                        else
                        {
                            noDrawbackDecreaseCount++;
                        }

                        itemsLayoutToImprove = itemsLayout;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        while (noDrawbackDecreaseCount < static_cast<int>(bestItemsLayout.m_lineItemCounts.size() / 2));
        // TODO: Task 38031375 - Performance optimizations.
        // Given a line count, investigate what the typical number of iterations without drawback improvement
        // leads to no improvements no matter the number of subsequent tries.
    }

#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"End-of-Phase-6 best layout");
    LogItemsLayoutDbg(bestItemsLayout, averageLineItemsWidth);
#endif

    // Phase 6: Final item measurements.
    ComputeItemsLayoutWithLockedItems(
        bestItemsLayout,
        availableWidth,
        minItemSpacing,
        actualLineHeight,
        averageAspectRatio,
        beginSizedLineIndex,
        endSizedLineIndex,
        beginSizedItemIndex,
        endSizedItemIndex,
        beginLineVectorIndex,
        isLastSizedLineStretchEnabled);

#ifdef DBG
    const bool forwardDbg = beginSizedItemIndex <= endSizedItemIndex;

    MUX_ASSERT(!(!forwardDbg && beginSizedLineIndex < endSizedLineIndex));

    const int sizedLineCountDbg = forwardDbg ? endSizedLineIndex - beginSizedLineIndex + 1 :
        beginSizedLineIndex - endSizedLineIndex + 1;
    const int sizedItemCountExpectedDbg = forwardDbg ? endSizedItemIndex - beginSizedItemIndex + 1 :
        beginSizedItemIndex - endSizedItemIndex + 1;

    MUX_ASSERT(sizedItemCountExpectedDbg >= sizedLineCountDbg);

    int sizedItemCountActualDbg = 0;

    for (int sizedLineVectorIndexDbg = 0; sizedLineVectorIndexDbg < sizedLineCountDbg; sizedLineVectorIndexDbg++)
    {
        const int lineItemsCountDbg = m_lineItemCounts[beginLineVectorIndex + sizedLineVectorIndexDbg /*lineVectorIndex*/];

        MUX_ASSERT(lineItemsCountDbg > 0);

        sizedItemCountActualDbg += lineItemsCountDbg;
    }

    MUX_ASSERT(sizedItemCountActualDbg == sizedItemCountExpectedDbg);
#endif
}

// Computes the expanding factor for a line with a desired width smaller than the available width.
double LinedFlowLayout::ComputeLineExpandFactor(
    bool forward,
    int sizedItemIndex,
    int lineItemsCount,
    double lineItemsWidth,
    double availableWidth,
    double minItemSpacings,
    double actualLineHeight,
    double averageAspectRatio)
{
    const bool usesFastPathLayout = UsesFastPathLayout();
    double scaleFactor;
    bool largerScaleFactorNeeded;
    std::set<int> ignoredItemIndexes;

    MUX_ASSERT(lineItemsWidth < availableWidth);

    availableWidth -= minItemSpacings;
    lineItemsWidth -= minItemSpacings;

    MUX_ASSERT(availableWidth > 0.0);
    MUX_ASSERT(lineItemsWidth > 0.0);

    do
    {
        int sizedItemIndexTmp = sizedItemIndex;

        largerScaleFactorNeeded = false;
        scaleFactor = availableWidth / lineItemsWidth;

        for (int itemIndex = 0; itemIndex < lineItemsCount; itemIndex++)
        {
            if (ignoredItemIndexes.find(itemIndex) == ignoredItemIndexes.end())
            {
                double minWidth{}, maxWidth{ std::numeric_limits<double>::infinity() }, desiredWidth{};
                winrt::UIElement element = nullptr;

                if (!usesFastPathLayout && m_itemsInfoFirstIndex == -1)
                {
                    MUX_ASSERT(m_elementManager.IsDataIndexRealized(sizedItemIndexTmp));

                    if (element = m_elementManager.GetRealizedElement(sizedItemIndexTmp /*dataIndex*/))
                    {
                        if (const auto frameworkElement = element.try_as<winrt::FrameworkElement>())
                        {
                            minWidth = frameworkElement.MinWidth();
                            maxWidth = frameworkElement.MaxWidth();
                            desiredWidth = element.DesiredSize().Width;
                        }
                    }
                }
                else
                {
                    double desiredAspectRatio = GetDesiredAspectRatioFromItemsInfo(sizedItemIndexTmp, usesFastPathLayout);

                    if (desiredAspectRatio <= 0)
                    {
                        // The average aspect ratio is used as a fallback value for items that were given a negative ratio
                        // by the ItemsInfoRequested handler.
                        desiredAspectRatio = averageAspectRatio;
                    }

                    desiredWidth = desiredAspectRatio * actualLineHeight;

                    const double desiredMinWidth = GetMinWidthFromItemsInfo(sizedItemIndexTmp);

                    if (desiredMinWidth >= 0.0)
                    {
                        minWidth = desiredMinWidth;
                        desiredWidth = std::max(minWidth, desiredWidth);
                    }

                    const double desiredMaxWidth = GetMaxWidthFromItemsInfo(sizedItemIndexTmp);

                    if (desiredMaxWidth >= 0.0)
                    {
                        maxWidth = desiredMaxWidth;
                        desiredWidth = std::min(maxWidth, desiredWidth);
                    }
                }

                if (maxWidth < desiredWidth * scaleFactor)
                {
                    availableWidth = std::max(0.0, availableWidth - maxWidth);
                    lineItemsWidth = std::max(0.0, lineItemsWidth - desiredWidth);

                    ignoredItemIndexes.insert(itemIndex);
                    largerScaleFactorNeeded = true;
                    break;
                }

                if (element != nullptr &&                                               // ItemsInfoRequested event did not provide sizing information.
                    (scaleFactor - 1.0) * minWidth > 1.0 / m_roundingScaleFactor &&     // scaleFactor is large enough that the rounded scaled desired width is expected to be larger than the rounded min width.
                    std::abs(minWidth - desiredWidth) <= 1.0 / m_roundingScaleFactor)   // element's DesiredSize.Width and element.MinWidth have the same rounded value.
                {
                    const auto scaledAvailableSize = winrt::Size(static_cast<float>(desiredWidth * scaleFactor), static_cast<float>(actualLineHeight));
                    const auto scaledDesiredSize = GetDesiredSizeForAvailableSize(sizedItemIndexTmp, element, scaledAvailableSize, actualLineHeight);

                    if (scaledDesiredSize.Width != -1.0f && std::abs(minWidth - scaledDesiredSize.Width) <= 1.0 / m_roundingScaleFactor)
                    {
                        // These are cases where the desired width matches the minimum width even though the element is given a larger available width.
                        availableWidth = std::max(0.0, availableWidth - minWidth);
                        lineItemsWidth = std::max(0.0, lineItemsWidth - minWidth);

                        ignoredItemIndexes.insert(itemIndex);
                        largerScaleFactorNeeded = true;
                        break;
                    }
                }
            }

            sizedItemIndexTmp = forward ? sizedItemIndexTmp + 1 : sizedItemIndexTmp - 1;
        }
    }
    while (availableWidth > 0.0 && lineItemsWidth > 0.0 && largerScaleFactorNeeded);

    MUX_ASSERT(scaleFactor > 1.0);

    return scaleFactor;
}

// Computes the shrinking factor for a line with a desired width larger than the available width.
// Returns 0 when the items' minimum width prevent enough shrinking to accommodate the available width.
double LinedFlowLayout::ComputeLineShrinkFactor(
    bool forward,
    int sizedItemIndex,
    int lineItemsCount,
    double lineItemsWidth,
    double availableWidth,
    double minItemSpacings,
    double actualLineHeight,
    double averageAspectRatio)
{
    const bool usesFastPathLayout = UsesFastPathLayout();
    double scaleFactor;
    bool smallerScaleFactorNeeded;
    std::set<int> ignoredItemIndexes;

    MUX_ASSERT(lineItemsWidth > availableWidth);

    availableWidth -= minItemSpacings;
    lineItemsWidth -= minItemSpacings;

    MUX_ASSERT(availableWidth > 0.0);
    MUX_ASSERT(lineItemsWidth > 0.0);

    do
    {
        int sizedItemIndexTmp = sizedItemIndex;

        smallerScaleFactorNeeded = false;
        scaleFactor = availableWidth / lineItemsWidth;

        for (int itemIndex = 0; itemIndex < lineItemsCount; itemIndex++)
        {
            if (ignoredItemIndexes.find(itemIndex) == ignoredItemIndexes.end())
            {
                double minWidth{}, desiredWidth{};

                if (!usesFastPathLayout && m_itemsInfoFirstIndex == -1)
                {
                    MUX_ASSERT(m_elementManager.IsDataIndexRealized(sizedItemIndexTmp));

                    if (const auto element = m_elementManager.GetRealizedElement(sizedItemIndexTmp /*dataIndex*/))
                    {
                        if (const auto frameworkElement = element.try_as<winrt::FrameworkElement>())
                        {
                            minWidth = frameworkElement.MinWidth();
                        }

                        desiredWidth = element.DesiredSize().Width;
                    }
                }
                else
                {
                    double desiredAspectRatio = GetDesiredAspectRatioFromItemsInfo(sizedItemIndexTmp, usesFastPathLayout);

                    if (desiredAspectRatio <= 0)
                    {
                        // The average aspect ratio is used as a fallback value for items that were given a negative ratio
                        // by the ItemsInfoRequested handler.
                        desiredAspectRatio = averageAspectRatio;
                    }

                    desiredWidth = desiredAspectRatio * actualLineHeight;

                    const double desiredMaxWidth = GetMaxWidthFromItemsInfo(sizedItemIndexTmp);

                    if (desiredMaxWidth >= 0.0)
                    {
                        desiredWidth = std::min(desiredMaxWidth, desiredWidth);
                    }

                    const double desiredMinWidth = GetMinWidthFromItemsInfo(sizedItemIndexTmp);

                    if (desiredMinWidth >= 0.0)
                    {
                        minWidth = desiredMinWidth;
                        desiredWidth = std::max(minWidth, desiredWidth);
                    }
                }

                if (minWidth > desiredWidth * scaleFactor)
                {
                    availableWidth = std::max(0.0, availableWidth - minWidth);
                    lineItemsWidth = std::max(0.0, lineItemsWidth - desiredWidth);

                    ignoredItemIndexes.insert(itemIndex);
                    smallerScaleFactorNeeded = true;
                    break;
                }
            }

            sizedItemIndexTmp = forward ? sizedItemIndexTmp + 1 : sizedItemIndexTmp - 1;
        }
    }
    while (availableWidth > 0.0 && lineItemsWidth > 0.0 && smallerScaleFactorNeeded);

    MUX_ASSERT(scaleFactor > 0.0);
    MUX_ASSERT(scaleFactor < 1.0);

    return smallerScaleFactorNeeded ? 0.0 : scaleFactor;
}

// Copies a section of the provided oldItemsInfo vector into m_itemsInfoDesiredAspectRatiosForRegularPath.
void LinedFlowLayout::CopyItemsInfo(
    std::vector<double> const& oldItemsInfoDesiredAspectRatios,
    std::vector<double> const& oldItemsInfoMinWidths,
    std::vector<double> const& oldItemsInfoMaxWidths,
    std::vector<float> const& oldItemsInfoArrangeWidths,
    int oldStart,
    int newStart,
    int copyCount)
{
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"oldStart", oldStart);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"newStart", newStart);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"copyCount", copyCount);

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, oldStart + copyCount, oldItemsInfoDesiredAspectRatios.size());
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, newStart + copyCount, m_itemsInfoDesiredAspectRatiosForRegularPath.size());

    MUX_ASSERT(oldStart >= 0);
    MUX_ASSERT(newStart >= 0);
    MUX_ASSERT(copyCount > 0);
    MUX_ASSERT(oldStart + copyCount <= static_cast<int>(oldItemsInfoDesiredAspectRatios.size()));
    MUX_ASSERT(newStart + copyCount <= static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size()));

    for (int index = 0; index < copyCount; index++)
    {
        SetDesiredAspectRatioFromItemsInfo(m_itemsInfoFirstIndex + newStart + index, oldItemsInfoDesiredAspectRatios[oldStart + index]);

        MUX_ASSERT(m_itemsInfoDesiredAspectRatiosForRegularPath[newStart + index] == oldItemsInfoDesiredAspectRatios[oldStart + index]);

        if (oldItemsInfoMinWidths.size() > 0)
        {
            m_itemsInfoMinWidthsForRegularPath[newStart + index] = oldItemsInfoMinWidths[oldStart + index];
        }

        if (oldItemsInfoMaxWidths.size() > 0)
        {
            m_itemsInfoMaxWidthsForRegularPath[newStart + index] = oldItemsInfoMaxWidths[oldStart + index];
        }

        SetArrangeWidthFromItemsInfo(m_itemsInfoFirstIndex + newStart + index, oldItemsInfoArrangeWidths[oldStart + index]);

        MUX_ASSERT(m_itemsInfoArrangeWidths[newStart + index] == oldItemsInfoArrangeWidths[oldStart + index]);
    }
}

// Ensures items are realized and measured to determine their desired width.
// Returns True when there is at least one stored aspect ratio with a weight
// lower than c_maxAspectRatioWeight. In those cases the caller must trigger
// an additional UI thread tick so that those low weights can be increased
// and ultimately hit the c_maxAspectRatioWeight value.
void LinedFlowLayout::EnsureAndMeasureItemRange(
    winrt::VirtualizingLayoutContext const& context,
    float availableWidth,
    double actualLineHeight,
    bool forward,
    int beginRealizedItemIndex,
    int endRealizedItemIndex)
{
#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginRealizedItemIndex", beginRealizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endRealizedItemIndex", endRealizedItemIndex);

    MUX_ASSERT(!(forward && beginRealizedItemIndex > endRealizedItemIndex));
    MUX_ASSERT(!(!forward && beginRealizedItemIndex < endRealizedItemIndex));
    MUX_ASSERT(beginRealizedItemIndex < context.ItemCount());
    MUX_ASSERT(endRealizedItemIndex < context.ItemCount());
    MUX_ASSERT(m_elementDesiredWidths);

    int beginNewRealizedItemIndexDbg = -1, endNewRealizedItemIndexDbg = -1;
#endif

    bool realizedNewElement = false;

    for (int realizedItemIndex = beginRealizedItemIndex; ; realizedItemIndex = forward ? realizedItemIndex + 1 : realizedItemIndex - 1)
    {
        bool isElementNewlyRealized = false;

        if (!m_elementManager.IsDataIndexRealized(realizedItemIndex))
        {
            EnsureElementRealized(forward, realizedItemIndex);

            realizedNewElement = isElementNewlyRealized = true;

#ifdef DBG
            if (beginNewRealizedItemIndexDbg == -1)
            {
                beginNewRealizedItemIndexDbg = realizedItemIndex;
            }
            else
            {
                endNewRealizedItemIndexDbg = realizedItemIndex;
            }
#endif
        }

        if (m_itemsInfoFirstIndex == -1)
        {
            if (const auto element = m_elementManager.GetRealizedElement(realizedItemIndex /*dataIndex*/))
            {
                const auto elementTracker = tracker_ref<winrt::UIElement>(this, element);

                if (m_elementDesiredWidths->find(elementTracker) == m_elementDesiredWidths->end())
                {
                    const auto elementAvailableSize = winrt::Size(std::numeric_limits<float>::infinity(), static_cast<float>(actualLineHeight));

                    element.Measure(elementAvailableSize);

                    const auto desiredSize = element.DesiredSize();

                    bool desiredWidthGreaterThanMinWidth{};

                    if (const auto frameworkElement = element.try_as<winrt::FrameworkElement>())
                    {
                        const float minWidth = static_cast<float>(frameworkElement.MinWidth());

                        if (desiredSize.Width > minWidth)
                        {
                            // Items for which the desired width is greater than the min width are immediately
                            // assigned the c_maxAspectRatioWeight weight. For the other items, the weight is
                            // incremented from 1 to c_maxAspectRatioWeight as the min width may be due to the 
                            // item content not being loaded yet.
                            desiredWidthGreaterThanMinWidth = true;
                        }
                    }

                    m_elementDesiredWidths->insert(std::pair<tracker_ref<winrt::UIElement>, float>(elementTracker, desiredSize.Width));

                    if (desiredSize.Height != 0.0f && m_aspectRatios != nullptr)
                    {
                        LinedFlowLayoutItemAspectRatios::ItemAspectRatio itemAspectRatio = m_aspectRatios->GetAt(realizedItemIndex);
                        const float aspectRatio = desiredSize.Width / desiredSize.Height;

                        if (itemAspectRatio.m_weight == 0)
                        {
                            itemAspectRatio.m_aspectRatio = aspectRatio;
                            itemAspectRatio.m_weight = desiredWidthGreaterThanMinWidth ? c_maxAspectRatioWeight : 1;
                        
#ifdef DBG
                            if (realizedItemIndex == m_logItemIndexDbg)
                            {
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"initial aspect ratio", aspectRatio);
                            }
#endif
                        }
                        else
                        {
                            if (isElementNewlyRealized)
                            {
                                itemAspectRatio.m_weight = desiredWidthGreaterThanMinWidth ? c_maxAspectRatioWeight : 1;
                            }
                            else
                            {
                                const int aspectRatioWeight = itemAspectRatio.m_weight;

                                MUX_ASSERT(itemAspectRatio.m_aspectRatio >= 0.0f);
                                MUX_ASSERT(aspectRatioWeight >= 1);
                                MUX_ASSERT(aspectRatioWeight <= c_maxAspectRatioWeight);

                                if (aspectRatioWeight < c_maxAspectRatioWeight)
                                {
                                    if (desiredWidthGreaterThanMinWidth)
                                    {
                                        itemAspectRatio.m_weight = c_maxAspectRatioWeight;
                                    }
                                    else
                                    {
                                        itemAspectRatio.m_weight++;
                                    }
                                }
                            }
#ifdef DBG
                            if (realizedItemIndex == m_logItemIndexDbg)
                            {
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this,
                                    itemAspectRatio.m_aspectRatio == aspectRatio ? L"same aspect ratio" : L"new aspect ratio", aspectRatio);
                                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"weight", itemAspectRatio.m_weight);
                            }
#endif
                            if (itemAspectRatio.m_aspectRatio != aspectRatio)
                            {
                                itemAspectRatio.m_aspectRatio = aspectRatio;
                            }
                        }

                        m_aspectRatios->SetAt(realizedItemIndex, itemAspectRatio);
                    }

#ifdef DBG
                    if (realizedItemIndex == m_logItemIndexDbg)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"measured with elementAvailableSize",
                            elementAvailableSize.Width,
                            elementAvailableSize.Height);
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"element.DesiredSize",
                            desiredSize.Width,
                            desiredSize.Height);
                    }
#endif
                }
            }
        }

        if (realizedItemIndex == endRealizedItemIndex)
        {
            break;
        }
    }

#ifdef DBG
    if (beginNewRealizedItemIndexDbg != -1)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"newly realized item index range", beginNewRealizedItemIndexDbg, endNewRealizedItemIndexDbg);
    }
#endif

    if (realizedNewElement && !UsesArrangeWidthInfo())
    {
        InvalidateMeasureTimerStart(0 /*tickCount*/);
    }
}

// Allocates the storage for tracking weighted aspect ratios. Widths that storage to track about 10 viewports worth of items at the most.
void LinedFlowLayout::EnsureAndResizeItemAspectRatios(
    double scrollViewport,
    double actualLineHeight,
    double lineSpacing)
{
    MUX_ASSERT(m_averageItemsPerLine.second > 0.0);
    MUX_ASSERT(m_firstSizedItemIndex >= 0);
    MUX_ASSERT(m_firstSizedItemIndex <= m_lastSizedItemIndex);

    EnsureItemAspectRatios();

    constexpr int c_scrollViewportCount = 10; // m_aspectRatios is sized large enough to hold aspect ratio information for 10 viewport worth of items.
    const int referenceItemIndex = static_cast<int>((m_lastSizedItemIndex - m_firstSizedItemIndex) / 2);
    const double c_minItemsPerScrollViewport = 20.0; // Avoid small aspect ratio sampling that is not statistically significant by forcing at least 20 items per viewport.
    const double linesPerScrollViewport = scrollViewport / (actualLineHeight + lineSpacing);
    const double averageItemsPerScrollViewport = std::max(c_minItemsPerScrollViewport, linesPerScrollViewport * m_averageItemsPerLine.second);
    int itemAspectRatiosStorageSize = std::max(static_cast<int>(averageItemsPerScrollViewport * c_scrollViewportCount), m_lastSizedItemIndex - m_firstSizedItemIndex + 1);

    itemAspectRatiosStorageSize = std::min(itemAspectRatiosStorageSize, m_itemCount);

    m_aspectRatios->Resize(itemAspectRatiosStorageSize, referenceItemIndex);
}

void LinedFlowLayout::EnsureElementAvailableWidths()
{
    if (!m_elementAvailableWidths)
    {
        m_elementAvailableWidths = std::make_shared<std::map<tracker_ref<winrt::UIElement>, float>>();
    }
}

void LinedFlowLayout::EnsureElementDesiredWidths()
{
    if (!m_elementDesiredWidths)
    {
        m_elementDesiredWidths = std::make_shared<std::map<tracker_ref<winrt::UIElement>, float>>();
    }
}

void LinedFlowLayout::EnsureElementRealized(
    bool forward,
    int itemIndex)
{
    MUX_ASSERT(m_isVirtualizingContext);
    MUX_ASSERT(!m_elementManager.IsDataIndexRealized(itemIndex));

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemIndex", itemIndex);

    m_elementManager.EnsureElementRealized(forward, itemIndex, LayoutId());
}

void LinedFlowLayout::EnsureItemAspectRatios()
{
    if (!m_aspectRatios)
    {
        m_aspectRatios = std::make_unique<LinedFlowLayoutItemAspectRatios>();
    }
}

// Ensures the items in the provided range are realized.
void LinedFlowLayout::EnsureItemRange(
    bool forward,
    int beginRealizedItemIndex,
    int endRealizedItemIndex)
{
#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginRealizedItemIndex", beginRealizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endRealizedItemIndex", endRealizedItemIndex);

    MUX_ASSERT(!(forward && beginRealizedItemIndex > endRealizedItemIndex));
    MUX_ASSERT(!(!forward && beginRealizedItemIndex < endRealizedItemIndex));
    MUX_ASSERT(beginRealizedItemIndex < m_itemCount);
    MUX_ASSERT(endRealizedItemIndex < m_itemCount);

    int beginNewRealizedItemIndexDbg = -1, endNewRealizedItemIndexDbg = -1;
#endif

    bool realizedNewElement = false;

    for (int realizedItemIndex = beginRealizedItemIndex; ; realizedItemIndex = forward ? realizedItemIndex + 1 : realizedItemIndex - 1)
    {
        if (!m_elementManager.IsDataIndexRealized(realizedItemIndex))
        {
            EnsureElementRealized(forward, realizedItemIndex);

#ifdef DBG
            if (beginNewRealizedItemIndexDbg == -1)
            {
                beginNewRealizedItemIndexDbg = realizedItemIndex;
            }
            else
            {
                endNewRealizedItemIndexDbg = realizedItemIndex;
            }
#endif
            realizedNewElement = true;
        }

        if (realizedItemIndex == endRealizedItemIndex)
        {
            break;
        }
    }

#ifdef DBG
    if (beginNewRealizedItemIndexDbg != -1)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"newly realized item index range", beginNewRealizedItemIndexDbg, endNewRealizedItemIndexDbg);
    }
#endif

    if (realizedNewElement && !UsesArrangeWidthInfo())
    {
        InvalidateMeasureTimerStart(0 /*tickCount*/);
    }
}

// Ensures items are realized exactly for the context's realization window - for the fast path layout.
void LinedFlowLayout::EnsureItemRangeFastPath(
    winrt::VirtualizingLayoutContext const& context,
    double actualLineHeight)
{
    const int newRecommendedAnchorIndex{ context.RecommendedAnchorIndex() };

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"entry", actualLineHeight);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_anchorIndex", m_anchorIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"newRecommendedAnchorIndex", newRecommendedAnchorIndex);

    MUX_ASSERT(actualLineHeight > 0.0);
    MUX_ASSERT(m_itemCount > 0);

    const double scrollViewport{ context.VisibleRect().Height };
    int newFirstRealizedItemIndex{ 0 };
    int newLastRealizedItemIndex{ 0 };

    MUX_ASSERT(m_isVirtualizingContext == (scrollViewport != std::numeric_limits<double>::infinity()));

    if (m_isVirtualizingContext)
    {
        const float realizationRectHeight{ context.RealizationRect().Height };

        if (realizationRectHeight == 0.0)
        {
            return;
        }

        const int lineCount{ static_cast<int>(m_lineItemCounts.size()) };

        MUX_ASSERT(lineCount >= 0);

        const double lineSpacing{ LineSpacing() };
        float nearRealizationRect{ GetRoundedFloat(context.RealizationRect().Y) };
        int firstRealizedLineIndex{ -1 };
        int lastRealizedLineIndex{ -1 };

        GetFirstAndLastDisplayedLineIndexes(
            realizationRectHeight /*scrollViewport*/,
            nearRealizationRect,
            0 /*padding*/,
            lineSpacing,
            actualLineHeight,
            lineCount,
            false /*forFullyDisplayedLines*/,
            &firstRealizedLineIndex,
            &lastRealizedLineIndex);

        MUX_ASSERT(firstRealizedLineIndex >= 0);
        MUX_ASSERT(lastRealizedLineIndex >= 0);
        MUX_ASSERT(firstRealizedLineIndex < lineCount);
        MUX_ASSERT(lastRealizedLineIndex < lineCount);

        newFirstRealizedItemIndex = GetFirstItemIndexInLineIndex(firstRealizedLineIndex);
        newLastRealizedItemIndex = GetLastItemIndexInLineIndex(lastRealizedLineIndex);

        MUX_ASSERT(newFirstRealizedItemIndex >= 0);
        MUX_ASSERT(newLastRealizedItemIndex >= 0);
        MUX_ASSERT(newFirstRealizedItemIndex < m_itemCount);
        MUX_ASSERT(newLastRealizedItemIndex < m_itemCount);

        bool realizationRectCenteredAroundAnchor{ false };

        if (m_anchorIndex != -1 || newRecommendedAnchorIndex != -1)
        {
            const int anchorLineIndex = GetLineIndex(newRecommendedAnchorIndex == -1 ? m_anchorIndex : newRecommendedAnchorIndex, true /*usesFastPathLayout*/);

            MUX_ASSERT(anchorLineIndex >= 0);
            MUX_ASSERT(anchorLineIndex < lineCount);

            if (anchorLineIndex < firstRealizedLineIndex || anchorLineIndex > lastRealizedLineIndex)
            {
                // The anchor line is disconnected. Centering the realization window around its center.
                const float anchorLineNearEdge = static_cast<float>(anchorLineIndex * (actualLineHeight + lineSpacing));
                const float anchorLineMiddle = static_cast<float>(anchorLineNearEdge + actualLineHeight / 2.0);

                nearRealizationRect = GetRoundedFloat(anchorLineMiddle - realizationRectHeight / 2.0f);
                realizationRectCenteredAroundAnchor = true;

                // Same comments and treatment as in LinedFlowLayout::MeasureConstrainedLinesRegularPath.
                // Sometimes during a bring-into-view operation the RecommendedAnchorIndex momentarily switches from the target index N to -1 and then back to the target index N.
                // To avoid momentarily setting m_anchorIndex to -1, its value is preserved c_maxAnchorIndexRetentionCount times, based purely on experimentations.
                // Without this momentary retention the bring-into-view operation lands on incorrect offsets. If m_anchorIndex were to be preserved without a countdown, large offset changes
                // through ScrollView.ScrollTo calls would not complete.
                // TODO: Bug 41896418 - Investigate why the occurrences of -1 sometimes come up - even when the ScrollView anchoring is turned on.
                //                      Does such a countdown need to be used? What is a reliable countdown starting value?
                if (newRecommendedAnchorIndex == -1)
                {
                    MUX_ASSERT(m_anchorIndexRetentionCountdown >= 1 && m_anchorIndexRetentionCountdown <= c_maxAnchorIndexRetentionCount);

                    m_anchorIndexRetentionCountdown--;

                    if (m_anchorIndexRetentionCountdown == 0)
                    {
                        m_anchorIndex = -1;
                    }
                }
                else
                {
                    m_anchorIndexRetentionCountdown = c_maxAnchorIndexRetentionCount;
                    m_anchorIndex = newRecommendedAnchorIndex;
                }

                LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Forcing layout pass to decrement m_anchorIndexRetentionCountdown");

                // Layout is invalidated to ensure that m_anchorIndexRetentionCountdown is decremented from c_maxAnchorIndexRetentionCount all the way to 0 (a few lines above).
                // Otherwise m_anchorIndex could be stuck to a value that prevents the realization window from moving to the actual scroller's offset.
                InvalidateMeasureAsync();
            }
        }

        if (realizationRectCenteredAroundAnchor)
        {
            // Re-evaluate realization range after centering around the anchor line.
            GetFirstAndLastDisplayedLineIndexes(
                realizationRectHeight /*scrollViewport*/,
                nearRealizationRect,
                0 /*padding*/,
                lineSpacing,
                actualLineHeight,
                lineCount,
                false /*forFullyDisplayedLines*/,
                &firstRealizedLineIndex,
                &lastRealizedLineIndex);

            MUX_ASSERT(firstRealizedLineIndex >= 0);
            MUX_ASSERT(lastRealizedLineIndex >= 0);
            MUX_ASSERT(firstRealizedLineIndex < lineCount);
            MUX_ASSERT(lastRealizedLineIndex < lineCount);

            newFirstRealizedItemIndex = GetFirstItemIndexInLineIndex(firstRealizedLineIndex);
            newLastRealizedItemIndex = GetLastItemIndexInLineIndex(lastRealizedLineIndex);

            MUX_ASSERT(newFirstRealizedItemIndex >= 0);
            MUX_ASSERT(newLastRealizedItemIndex >= 0);
            MUX_ASSERT(newFirstRealizedItemIndex < m_itemCount);
            MUX_ASSERT(newLastRealizedItemIndex < m_itemCount);
        }
        else if (m_anchorIndex != -1)
        {
            m_anchorIndex = -1;
        }
    }
    else
    {
        // All items are realized in non-virtualizing mode.
        MUX_ASSERT(context.VisibleRect().Y == 0.0f);

        newLastRealizedItemIndex = m_itemCount - 1;
    }

    const int newRealizedItemCount = newLastRealizedItemIndex - newFirstRealizedItemIndex + 1;

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"newFirstRealizedItemIndex", newFirstRealizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"newLastRealizedItemIndex", newLastRealizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"newRealizedItemCount", newRealizedItemCount);

    const int oldFirstRealizedItemIndex = m_elementManager.GetFirstRealizedDataIndex(); // -1 and unused in non-virtualizing mode.
    const int oldLastRealizedItemIndex = oldFirstRealizedItemIndex == -1 ? -1 : oldFirstRealizedItemIndex + m_elementManager.GetRealizedElementCount() - 1;

    // Discard the first realized items fallen off the realization window.
    if (oldFirstRealizedItemIndex != -1 && oldFirstRealizedItemIndex < newFirstRealizedItemIndex)
    {
        MUX_ASSERT(m_isVirtualizingContext);

        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"discarded realized head range",
            oldFirstRealizedItemIndex, newFirstRealizedItemIndex - 1);

        m_elementManager.DiscardElementsOutsideWindow(
            false /*forward*/,
            std::min(newFirstRealizedItemIndex, oldFirstRealizedItemIndex + m_elementManager.GetRealizedElementCount()) - 1 /*startIndex*/);
    }

    MUX_ASSERT(newFirstRealizedItemIndex <= m_elementManager.GetFirstRealizedDataIndex() || -1 == m_elementManager.GetFirstRealizedDataIndex());

    int firstStillRealizedItemIndex{ -1 };
    int lastStillRealizedItemIndex{ -1 };

    if (oldFirstRealizedItemIndex != -1 && oldLastRealizedItemIndex != -1)
    {
        if (newFirstRealizedItemIndex >= oldFirstRealizedItemIndex && newFirstRealizedItemIndex <= oldLastRealizedItemIndex)
        {
            firstStillRealizedItemIndex = newFirstRealizedItemIndex;
            lastStillRealizedItemIndex = std::min(oldLastRealizedItemIndex, newLastRealizedItemIndex);
        }
        else if (newLastRealizedItemIndex >= oldFirstRealizedItemIndex && newLastRealizedItemIndex <= oldLastRealizedItemIndex)
        {
            lastStillRealizedItemIndex = newLastRealizedItemIndex;
            firstStillRealizedItemIndex = std::max(oldFirstRealizedItemIndex, newFirstRealizedItemIndex);
        }
    }

    if (m_elementManager.GetFirstRealizedDataIndex() != -1 && newFirstRealizedItemIndex < m_elementManager.GetFirstRealizedDataIndex())
    {
        MUX_ASSERT(oldFirstRealizedItemIndex > 0);

        // Ensure and measure the realized items before the old first realized item.
        EnsureItemRange(
            false /*forward*/,
            std::min(oldFirstRealizedItemIndex - 1, newFirstRealizedItemIndex + newRealizedItemCount - 1) /*beginRealizedItemIndex*/,
            newFirstRealizedItemIndex /*endRealizedItemIndex*/);

        // Ensure and measure the realized items after the old first realized item.
        MUX_ASSERT(newFirstRealizedItemIndex == m_elementManager.GetFirstRealizedDataIndex());

        if (firstStillRealizedItemIndex != -1)
        {
            if (firstStillRealizedItemIndex > 0 && oldFirstRealizedItemIndex <= firstStillRealizedItemIndex - 1)
            {
                EnsureItemRange(
                    true /*forward*/,
                    oldFirstRealizedItemIndex /*beginRealizedItemIndex*/,
                    firstStillRealizedItemIndex - 1 /*endRealizedItemIndex*/);
            }

            if (lastStillRealizedItemIndex + 1 <= newFirstRealizedItemIndex + newRealizedItemCount - 1)
            {
                EnsureItemRange(
                    true /*forward*/,
                    lastStillRealizedItemIndex + 1 /*beginRealizedItemIndex*/,
                    newFirstRealizedItemIndex + newRealizedItemCount - 1 /*endRealizedItemIndex*/);
            }
        }
        else if (newFirstRealizedItemIndex + newRealizedItemCount - 1 >= oldFirstRealizedItemIndex)
        {
            EnsureItemRange(
                true /*forward*/,
                oldFirstRealizedItemIndex /*beginRealizedItemIndex*/,
                newFirstRealizedItemIndex + newRealizedItemCount - 1 /*endRealizedItemIndex*/);
        }
    }
    else
    {
        // Ensure and measure the realized items.
        MUX_ASSERT(newFirstRealizedItemIndex == m_elementManager.GetFirstRealizedDataIndex() || m_elementManager.GetFirstRealizedDataIndex() == -1);

        if (firstStillRealizedItemIndex != -1)
        {
            if (firstStillRealizedItemIndex > 0 && newFirstRealizedItemIndex <= firstStillRealizedItemIndex - 1)
            {
                EnsureItemRange(
                    true /*forward*/,
                    newFirstRealizedItemIndex /*beginRealizedItemIndex*/,
                    firstStillRealizedItemIndex - 1 /*endRealizedItemIndex*/);
            }

            if (lastStillRealizedItemIndex + 1 <= newFirstRealizedItemIndex + newRealizedItemCount - 1)
            {
                EnsureItemRange(
                    true /*forward*/,
                    lastStillRealizedItemIndex + 1 /*beginRealizedItemIndex*/,
                    newFirstRealizedItemIndex + newRealizedItemCount - 1 /*endRealizedItemIndex*/);
            }
        }
        else
        {
            EnsureItemRange(
                true /*forward*/,
                newFirstRealizedItemIndex /*beginRealizedItemIndex*/,
                newFirstRealizedItemIndex + newRealizedItemCount - 1 /*endRealizedItemIndex*/);
        }
    }

    // Discard the last realized items fallen off the realization window.
    if (oldLastRealizedItemIndex != -1 && oldLastRealizedItemIndex > newLastRealizedItemIndex)
    {
        MUX_ASSERT(m_isVirtualizingContext);
        MUX_ASSERT(m_elementManager.GetRealizedElementCount() > newRealizedItemCount);

        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"discarded realized tail range",
            newFirstRealizedItemIndex + newRealizedItemCount, m_elementManager.GetFirstRealizedDataIndex() + m_elementManager.GetRealizedElementCount() - 1);

        m_elementManager.DiscardElementsOutsideWindow(
            true /*forward*/,
            newLastRealizedItemIndex + 1 /*startIndex*/);
    }

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"exit");
}

void LinedFlowLayout::EnsureItemsInfoDesiredAspectRatios(
    int itemCount)
{
    m_itemsInfoDesiredAspectRatiosForRegularPath.clear();
    m_itemsInfoDesiredAspectRatiosForRegularPath.resize(itemCount, -1.0);
}

void LinedFlowLayout::EnsureItemsInfoMinWidths(
    int itemCount)
{
    m_itemsInfoMinWidthsForRegularPath.clear();
    m_itemsInfoMinWidthsForRegularPath.resize(itemCount, -1.0);
}

void LinedFlowLayout::EnsureItemsInfoMaxWidths(
    int itemCount)
{
    m_itemsInfoMaxWidthsForRegularPath.clear();
    m_itemsInfoMaxWidthsForRegularPath.resize(itemCount, -1.0);
}

void LinedFlowLayout::EnsureItemsInfoArrangeWidths(
    int itemCount)
{
    m_itemsInfoArrangeWidths.clear();
    m_itemsInfoArrangeWidths.resize(itemCount, -1.0f);
}

void LinedFlowLayout::EnsureLineItemCounts(
    int lineCount)
{
    m_lineItemCounts.clear();
    m_lineItemCounts.resize(lineCount, 0);
}

void LinedFlowLayout::ExitRegularPath()
{
    m_itemsInfoFirstIndex = -1;
    m_unsizedNearLineCount = -1;
    m_unrealizedNearLineCount = -1;
    m_elementAvailableWidths = nullptr;
    m_elementDesiredWidths = nullptr;
    ResetSizedLines();
}

double LinedFlowLayout::GetArrangeWidth(
    double desiredAspectRatio,
    double minWidth,
    double maxWidth,
    double actualLineHeight,
    double scaleFactor) const
{
    MUX_ASSERT(desiredAspectRatio > 0.0);

    double arrangeWidth = desiredAspectRatio * actualLineHeight;

    minWidth = std::max(0.0, minWidth);

    arrangeWidth = std::max(minWidth, arrangeWidth);

    if (maxWidth >= 0.0)
    {
        arrangeWidth = std::min(maxWidth, arrangeWidth);
    }

    if (scaleFactor != 1.0)
    {
        arrangeWidth *= scaleFactor;

        if (scaleFactor < 1.0)
        {
            arrangeWidth = std::max(minWidth, arrangeWidth);
        }

        if (maxWidth >= 0.0 && scaleFactor > 1.0)
        {
            arrangeWidth = std::min(maxWidth, arrangeWidth);
        }
    }

    return arrangeWidth;
}

// Fast path layout:
//   Returns the arrange width computed from the m_itemsInfoDesiredAspectRatiosForFastPath, m_itemsInfoMinWidthsForFastPath, m_itemsInfoMaxWidthsForFastPath,
//   m_itemsInfoMinWidth, m_itemsInfoMaxWidth fields.
// Regular path layout:
//   Returns the arrange width computed from the m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath,
//   m_itemsInfoMinWidth, m_itemsInfoMaxWidth fields.
// The desired aspect ratio is combined with the provided scaleFactor.
float LinedFlowLayout::GetArrangeWidthFromItemsInfo(
    int sizedItemIndex,
    double actualLineHeight,
    double averageAspectRatio,
    double scaleFactor) const
{
    double desiredAspectRatio = GetDesiredAspectRatioFromItemsInfo(sizedItemIndex, UsesFastPathLayout());

    if (desiredAspectRatio <= 0)
    {
        // The average aspect ratio is used as a fallback value for items that were given a negative ratio
        // by the ItemsInfoRequested handler.
        desiredAspectRatio = averageAspectRatio;
    }

    return static_cast<float>(GetArrangeWidth(
        desiredAspectRatio,
        GetMinWidthFromItemsInfo(sizedItemIndex),
        GetMaxWidthFromItemsInfo(sizedItemIndex),
        actualLineHeight,
        scaleFactor));
}

// Returns the current average aspect ratio, either
// based on the m_aspectRatios storage when it's
// populated, or m_averageItemsPerLine when set, or
// finally the default defaultAspectRatio == 1.0.
double LinedFlowLayout::GetAverageAspectRatio(
    float availableWidth,
    double actualLineHeight) const
{
    if (m_aspectRatios == nullptr || m_aspectRatios->IsEmpty())
    {
        if (m_averageItemsPerLine.second == 0.0 || actualLineHeight == 0.0)
        {
            constexpr double defaultAspectRatio{ 1.0 };

            return defaultAspectRatio;
        }
        else
        {
            return availableWidth / m_averageItemsPerLine.second / actualLineHeight;
        }
    }

    return GetAverageAspectRatioFromStorage();
}

// Returns the average aspect ratio from m_aspectRatios or 1.0 when it is empty.
double LinedFlowLayout::GetAverageAspectRatioFromStorage() const
{
    constexpr double defaultAspectRatio{ 1.0 };

    if (m_aspectRatios == nullptr || m_aspectRatios->IsEmpty())
    {
        return defaultAspectRatio;
    }

    const int firstRealizedItemIndex = m_isVirtualizingContext ? m_elementManager.GetFirstRealizedDataIndex() : 0;
    const int lastRealizedItemIndex = firstRealizedItemIndex + m_elementManager.GetRealizedElementCount() - 1;
    // Items outside the current realized range must have a weight equal to c_maxAspectRatioWeight to be taken into account.
    // Smaller weights may not reflect the real aspect ratio and negatively influence the average.
    float averageItemAspectRatio = m_aspectRatios->GetAverageAspectRatio(firstRealizedItemIndex, lastRealizedItemIndex, c_maxAspectRatioWeight);

    return averageItemAspectRatio == 0.0f ? defaultAspectRatio : averageItemAspectRatio;
}

std::pair<double, double> LinedFlowLayout::GetAverageItemsPerLine(
    float availableWidth)
{
    double averageWidth;
    const double minItemSpacing = MinItemSpacing();
    const double actualLineHeight = ActualLineHeight();

    // During initial loading, in cases no sizing information is provided by an ItemsInfoRequested event handler,
    // the average item aspect ratio is set no lower than 1.5, 1.25 & 1.0 in the first 3 measure passes.
    // This is to avoid the unpopulated items' small MinWidth to artificially result in a small average aspect ratio
    // causing a momentary item realization that gets thrown away as soon as the real average aspect ratio is set.
    const double minAverageItemAspectRatio = m_measureCountdown * 0.25 + 0.5;
    
    if (m_aspectRatios != nullptr && !m_aspectRatios->IsEmpty())
    {
        double averageItemAspectRatio = GetAverageAspectRatioFromStorage();

        // During initial load, avoid small aspect ratios while the first items are still loading
        // to avoid unnecessary item realizations. This is only done when the ItemsInfoRequested event handler did not provide sizing information.
        if (m_itemsInfoFirstIndex == -1 && m_measureCountdown > 1 && averageItemAspectRatio < minAverageItemAspectRatio)
        {
            LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Forcing layout pass for low initial aspect ratio average");
            LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"minAverageItemAspectRatio", minAverageItemAspectRatio);
            LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"averageItemAspectRatio", averageItemAspectRatio);

            averageItemAspectRatio = minAverageItemAspectRatio;

            // Making sure there is another layout coming with a decreased m_measureCountdown value to stop using an adjusted averageItemAspectRatio.
            // Layout invalidation is done asynchronously to have an effect since this method, GetAverageItemsPerLine, is called during measure passes.
            InvalidateMeasureAsync();
        }

        m_averageItemAspectRatioDbg = averageItemAspectRatio;

        averageWidth = averageItemAspectRatio * actualLineHeight;
    }
    else
    {
        m_averageItemAspectRatioDbg = minAverageItemAspectRatio;

        // When no item is realized, minAverageItemAspectRatio which starts at 1.5 is used.
        averageWidth = minAverageItemAspectRatio * actualLineHeight;
    }

    if (m_forcedAverageItemAspectRatioDbg != 0.0)
    {
        averageWidth = m_forcedAverageItemAspectRatioDbg * actualLineHeight;
    }

    // Account for the minimum spacing between items by adding minItemSpacing
    // minItemSpacing is over-counted once for the last item.
    averageWidth = std::max(1.0, averageWidth + minItemSpacing);

    // minItemSpacing is added to the availableWidth to account for the over-counting above.
    double averageItemsPerLineRaw = (availableWidth + minItemSpacing) / averageWidth;

    averageItemsPerLineRaw = std::max(1.0, averageItemsPerLineRaw);

    std::pair<double, double> averageItemsPerLine = SnapAverageItemsPerLine(
        m_averageItemsPerLine.first /*oldAverageItemsPerLineRaw*/,
        averageItemsPerLineRaw /*newAverageItemsPerLineRaw*/);

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_DBL, METH_NAME, this, averageItemsPerLine.second);

    MUX_ASSERT(averageItemsPerLine.second >= 1.0);

    return averageItemsPerLine;
}

double LinedFlowLayout::GetDesiredAspectRatioFromItemsInfo(
    int itemIndex,
    bool usesFastPathLayout) const
{
    MUX_ASSERT(itemIndex >= 0);
    MUX_ASSERT(itemIndex < m_itemCount);

    double desiredAspectRatio{};

    if (usesFastPathLayout)
    {
        MUX_ASSERT(m_itemsInfoFirstIndex == -1);
        MUX_ASSERT(itemIndex < static_cast<int>(m_itemsInfoDesiredAspectRatiosForFastPath.size()));

        desiredAspectRatio = m_itemsInfoDesiredAspectRatiosForFastPath[itemIndex];
    }
    else
    {
        MUX_ASSERT(m_itemsInfoFirstIndex >= 0);
        MUX_ASSERT(itemIndex - m_itemsInfoFirstIndex < static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size()));

        desiredAspectRatio = m_itemsInfoDesiredAspectRatiosForRegularPath[itemIndex - m_itemsInfoFirstIndex];
    }

    return desiredAspectRatio;
}

// Measures the element with the provided availableSize if there is a recorded width from a prior measure call.
// Returns the resulting desired size, or -1,-1 otherwise.
winrt::Size LinedFlowLayout::GetDesiredSizeForAvailableSize(
    int itemIndex,
    winrt::UIElement const& element,
    winrt::Size availableSize,
    double actualLineHeightToRestore)
{
    MUX_ASSERT(element != nullptr);
    MUX_ASSERT(itemIndex >= 0);

    // When m_itemsInfoArrangeWidths is populated, 
    // - m_itemsInfoFirstIndex != -1: Regular path layout case, with items info available (m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath).
    // - m_itemsInfoFirstIndex == -1: Fast path layout case, without items info available.
    const int itemsInfoArrangeWidthsIndex = itemIndex - (m_itemsInfoFirstIndex == -1 ? 0 : m_itemsInfoFirstIndex);
    float measureWidthToRestore{};

    if (itemsInfoArrangeWidthsIndex < static_cast<int>(m_itemsInfoArrangeWidths.size()))
    {
        const float arrangeWidth = m_itemsInfoArrangeWidths[itemsInfoArrangeWidthsIndex];

        if (arrangeWidth < 0.0f)
        {
            // The m_itemsInfoArrangeWidths vector has not been set for the provided 'itemIndex' yet.
            // The effects of measuring 'element' with 'availableSize' below could not be undone.
            // Returning -1, -1 to indicate no desired size could be measured.
            return winrt::Size{ -1.0f, -1.0f };
        }
        else
        {
            measureWidthToRestore = arrangeWidth;
        }
    }
    else if (m_elementAvailableWidths != nullptr && m_elementDesiredWidths != nullptr)
    {
        // Check if the element has a recorded desired or available width
        // to restore after measurement with the provided 'availableSize'.
        const auto elementTracker = tracker_ref<winrt::UIElement>(this, element);
        auto recordedSizeIterator = m_elementAvailableWidths->find(elementTracker);
        
        if (recordedSizeIterator == m_elementAvailableWidths->end())
        {
            recordedSizeIterator = m_elementDesiredWidths->find(elementTracker);

            if (recordedSizeIterator == m_elementDesiredWidths->end())
            {
                // No recorded width to use to undo effects of a measure with 'availableSize'.
                return winrt::Size{ -1.0f, -1.0f };
            }
            else
            {
                // Use the previous desired width to undo the effects of the measure with 'availableSize'.
                measureWidthToRestore = recordedSizeIterator->second;
            }
        }
        else
        {
            // Use the previous available width to undo the effects of the measure with 'availableSize'.
            measureWidthToRestore = recordedSizeIterator->second;
        }
    }

    element.Measure(availableSize);

    winrt::Size desiredSize = element.DesiredSize();

    element.Measure(winrt::Size{ measureWidthToRestore, static_cast<float>(actualLineHeightToRestore) });

    return desiredSize;
}

void LinedFlowLayout::GetFirstAndLastDisplayedLineIndexes(
    double scrollViewport,
    double scrollOffset,
    double padding,
    double lineSpacing,
    double actualLineHeight,
    int lineCount,
    bool forFullyDisplayedLines,
    int* firstDisplayedLineIndex,
    int* lastDisplayedLineIndex) const
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"scrollViewport", scrollViewport);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"scrollOffset", scrollOffset);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"padding", padding);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"lineSpacing", lineSpacing);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"actualLineHeight", actualLineHeight);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lineCount", lineCount);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"forFullyDisplayedLines", forFullyDisplayedLines);

    MUX_ASSERT(m_isVirtualizingContext);
    MUX_ASSERT(scrollViewport != std::numeric_limits<double>::infinity());
    MUX_ASSERT(lineCount > 0);

    *firstDisplayedLineIndex = *lastDisplayedLineIndex = -1;

    const double lineHeight = actualLineHeight + lineSpacing;

    if (lineHeight == 0.0 || scrollViewport == 0.0)
    {
        return;
    }

    const double linesSpacing = lineCount == 0 ? 0.0 : (lineCount - 1) * lineSpacing;
    const double scrollExtent = lineCount * actualLineHeight + linesSpacing;
    const double maxScrollOffset = std::max(0.0, scrollExtent - scrollViewport);
    const double lineSpacingPortion = lineSpacing / lineHeight;

    scrollOffset = std::min(maxScrollOffset, scrollOffset);

    if (std::abs(scrollOffset) < c_offsetEqualityEpsilon)
    {
        scrollOffset = 0.0;
    }

    padding = std::min(padding, scrollViewport / 2.0);

    const double nearPadding = std::min(scrollOffset, padding);
    const double farPadding = std::min(maxScrollOffset - scrollOffset, padding);

    if (!(forFullyDisplayedLines && actualLineHeight + nearPadding + farPadding > scrollViewport))
    {
        double nearDisplayed = scrollOffset + nearPadding;
        double farDisplayed = scrollOffset + scrollViewport - farPadding;

        if (farDisplayed > 0.0 && nearDisplayed < scrollExtent)
        {
            nearDisplayed = std::max(0.0, nearDisplayed);
            farDisplayed = std::min(scrollExtent, farDisplayed);

            const double fractionalFirstDisplayedLineIndex = nearDisplayed / lineHeight;
            const int roundedFirstDisplayedLineIndex = static_cast<int>(fractionalFirstDisplayedLineIndex);

            if (roundedFirstDisplayedLineIndex + 1 - fractionalFirstDisplayedLineIndex <= lineSpacingPortion &&
                roundedFirstDisplayedLineIndex + 1 < lineCount)
            {
                // The portion displayed before the first displayed item is smaller than the line spacing - it can be ignored.
                *firstDisplayedLineIndex = roundedFirstDisplayedLineIndex + 1;
            }
            else
            {
                *firstDisplayedLineIndex = roundedFirstDisplayedLineIndex;
            }

            if (forFullyDisplayedLines &&
                fractionalFirstDisplayedLineIndex > roundedFirstDisplayedLineIndex &&
                roundedFirstDisplayedLineIndex + 1 < lineCount)
            {
                *firstDisplayedLineIndex = roundedFirstDisplayedLineIndex + 1;
            }

            MUX_ASSERT(*firstDisplayedLineIndex >= 0);
            MUX_ASSERT(*firstDisplayedLineIndex < lineCount);

            const double fractionalLastDisplayedLineIndex = (farDisplayed + lineSpacing) / lineHeight;
            const int roundedLastDisplayedLineIndex = static_cast<int>(fractionalLastDisplayedLineIndex);

            *lastDisplayedLineIndex = roundedLastDisplayedLineIndex;

            if (forFullyDisplayedLines)
            {
                if (fractionalLastDisplayedLineIndex >= 1.0)
                {
                    *lastDisplayedLineIndex = std::min(roundedLastDisplayedLineIndex, lineCount) - 1;
                }
            }
            else
            {
                if (fractionalLastDisplayedLineIndex - roundedLastDisplayedLineIndex <= lineSpacingPortion)
                {
                    *lastDisplayedLineIndex = std::max(roundedLastDisplayedLineIndex - 1, 0);
                }
            }

            MUX_ASSERT(*lastDisplayedLineIndex >= 0);
            MUX_ASSERT(*lastDisplayedLineIndex < lineCount);

            if (*firstDisplayedLineIndex > *lastDisplayedLineIndex)
            {
                *firstDisplayedLineIndex = *lastDisplayedLineIndex = -1;
            }
        }
    }
    // else actualLineHeight is large enough that no line can be fully displayed in the scroll viewport.

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, forFullyDisplayedLines ? L"fullyDisplayedLines" : L"displayedLines", *firstDisplayedLineIndex, *lastDisplayedLineIndex);
}

// Returns the index of the first fully realized line, as well as the first item index in that line.
void LinedFlowLayout::GetFirstFullyRealizedLineIndex(
    int* firstFullyRealizedLineIndex,
    int* firstItemInFullyRealizedLine) const
{
    const int firstRealizedItemIndex = m_isVirtualizingContext ? m_elementManager.GetFirstRealizedDataIndex() : 0;

    if (firstRealizedItemIndex == -1)
    {
        *firstFullyRealizedLineIndex = -1;
        *firstItemInFullyRealizedLine = -1;
        return;
    }

    int sizedItemIndex = m_firstSizedItemIndex == -1 ? 0 : m_firstSizedItemIndex;
    int sizedLineIndex = m_firstSizedLineIndex == -1 ? 0 : m_firstSizedLineIndex;
    int sizedLineVectorIndex = 0;

    MUX_ASSERT(firstRealizedItemIndex >= 0);
    MUX_ASSERT(sizedItemIndex >= 0);
    MUX_ASSERT(sizedLineIndex >= 0);

    while (sizedItemIndex < firstRealizedItemIndex)
    {
        sizedItemIndex += m_lineItemCounts[sizedLineVectorIndex];
        sizedLineIndex++;
        sizedLineVectorIndex++;
    }

    MUX_ASSERT(m_lastSizedLineIndex == -1 || sizedLineIndex <= m_lastSizedLineIndex);
    MUX_ASSERT(m_lastSizedItemIndex == -1 || sizedItemIndex <= m_lastSizedItemIndex);

    *firstFullyRealizedLineIndex = sizedLineIndex;
    *firstItemInFullyRealizedLine = sizedItemIndex;
}

// Uses the m_lineItemCounts vector to return the index of the first item in the provided line index.
int LinedFlowLayout::GetFirstItemIndexInLineIndex(
    int lineVectorIndex) const
{
    MUX_ASSERT(lineVectorIndex >= 0);
    MUX_ASSERT(lineVectorIndex < static_cast<int>(m_lineItemCounts.size()));

    int itemIndex = 0;

    for (int lineVectorIndexTmp = 0; lineVectorIndexTmp < lineVectorIndex; lineVectorIndexTmp++)
    {
        itemIndex += m_lineItemCounts[lineVectorIndexTmp];
    }

    return itemIndex;
}

int LinedFlowLayout::GetFrozenLineIndexFromFrozenItemIndex(
    int frozenItemIndex) const
{
    MUX_ASSERT(m_firstSizedLineIndex != -1);
    MUX_ASSERT(m_lastSizedLineIndex != -1);
    MUX_ASSERT(frozenItemIndex >= m_firstFrozenItemIndex);
    MUX_ASSERT(frozenItemIndex <= m_lastFrozenItemIndex);
    MUX_ASSERT(m_firstFrozenLineIndex != -1);
    MUX_ASSERT(m_lastFrozenLineIndex != -1);

    int frozenItemVectorIndex = frozenItemIndex - m_firstFrozenItemIndex;

    for (int frozenLineVectorIndex = m_firstFrozenLineIndex - m_firstSizedLineIndex;
        frozenLineVectorIndex <= m_lastFrozenLineIndex - m_firstSizedLineIndex;
        frozenLineVectorIndex++)
    {
        MUX_ASSERT(frozenLineVectorIndex >= 0);
        MUX_ASSERT(frozenLineVectorIndex < static_cast<int>(m_lineItemCounts.size()));

        if (frozenItemVectorIndex < m_lineItemCounts[frozenLineVectorIndex])
        {
            return frozenLineVectorIndex + m_firstSizedLineIndex;
        }
        frozenItemVectorIndex -= m_lineItemCounts[frozenLineVectorIndex];
    }

    MUX_ASSERT(false);
    return -1;
}

// Returns the drawback improvement when an item moves from a line to a neighboring one.
// The drawback is nil for the last line when it is smaller than the available width.
double LinedFlowLayout::GetItemDrawbackImprovement(
    double movingWidth,
    double availableWidth,
    double currentLineItemsWidth,
    double neighborLineItemsWidth,
    int currentLineIndex,
    int neighborLineIndex)
{
    MUX_ASSERT(movingWidth - MinItemSpacing() <= currentLineItemsWidth);
    MUX_ASSERT(std::abs(currentLineIndex - neighborLineIndex) == 1);

    const int lastLineIndex = GetLineCount(m_averageItemsPerLine.second) - 1;
    double currentDrawback = 0.0;

    if (currentLineIndex != lastLineIndex || currentLineItemsWidth > availableWidth)
    {
        currentDrawback = std::pow(currentLineItemsWidth - availableWidth, 2.0);
    }

    if (neighborLineIndex != lastLineIndex || neighborLineItemsWidth > availableWidth)
    {
        currentDrawback += std::pow(neighborLineItemsWidth - availableWidth, 2.0);
    }

    double newDrawback = 0.0;

    if (currentLineIndex != lastLineIndex || currentLineItemsWidth - movingWidth > availableWidth)
    {
        newDrawback = std::pow(currentLineItemsWidth - movingWidth - availableWidth, 2.0);
    }

    if (neighborLineIndex != lastLineIndex || neighborLineItemsWidth + movingWidth > availableWidth)
    {
        newDrawback += std::pow(neighborLineItemsWidth + movingWidth - availableWidth, 2.0);
    }

    return currentDrawback - newDrawback;
}

// Returns the line index the provided element belongs to, or -1 if not found.
int LinedFlowLayout::GetItemLineIndex(
    winrt::UIElement const& element)
{
    const int sizedLineVectorCount = static_cast<int>(m_lineItemCounts.size());
    int sizedItemIndex = m_firstSizedItemIndex;

    for (int sizedLineVectorIndex = 0; sizedLineVectorIndex < sizedLineVectorCount; sizedLineVectorIndex++)
    {
        const int lineItemsCount = m_lineItemCounts[sizedLineVectorIndex];

        for (int itemVectorIndex = 0; itemVectorIndex < lineItemsCount; itemVectorIndex++)
        {
            if (m_elementManager.IsDataIndexRealized(sizedItemIndex) &&
                m_elementManager.GetRealizedElement(sizedItemIndex /*dataIndex*/) == element)
            {
                return sizedLineVectorIndex + m_firstSizedLineIndex;
            }

            sizedItemIndex++;
        }
    }

    return -1;
}

// Returns the item range for the ItemsInfoRequested event based on the average items per line, the realization rect size and offset.
void LinedFlowLayout::GetItemsInfoRequestedRange(
    winrt::VirtualizingLayoutContext const& context,
    float availableWidth,
    double actualLineHeight,
    int* itemsRangeStartIndex,
    int* itemsRangeRequestedLength)
{
    if (m_isVirtualizingContext)
    {
        const float nearRealizationRect = GetRoundedFloat(context.RealizationRect().Y);
        const float farRealizationRect = nearRealizationRect + context.RealizationRect().Height;

        const double lineSpacing = LineSpacing();
        float sizedItemsRectHeight = GetSizedItemsRectHeight(context, actualLineHeight, lineSpacing);

        const float realizationRectHeightDeficit = std::max(0.0f, sizedItemsRectHeight - farRealizationRect + nearRealizationRect);
        float nearSizedItemsRect = nearRealizationRect - realizationRectHeightDeficit / 2.0f;
        float farSizedItemsRect = farRealizationRect + realizationRectHeightDeficit / 2.0f;

        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"sizedItemsRectHeight", sizedItemsRectHeight);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"nearRealizationRect", nearRealizationRect);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"farRealizationRect", farRealizationRect);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"realizationRectHeightDeficit", realizationRectHeightDeficit);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"nearSizedItemsRect", nearSizedItemsRect);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"farSizedItemsRect", farSizedItemsRect);

        const double minItemSpacing = MinItemSpacing();
        // Either an average was recorded in a prior measure pass, or the items are considered square for an initial rough estimation.
        const double averageItemsPerLine = m_averageItemsPerLine.second == 0.0 ? ((availableWidth + minItemSpacing) / (actualLineHeight + minItemSpacing)) : m_averageItemsPerLine.second;

        MUX_ASSERT(averageItemsPerLine > 0.0);

        const int lineCount = GetLineCount(averageItemsPerLine);

        MUX_ASSERT(lineCount >= 0);

        const int newRecommendedAnchorIndex = context.RecommendedAnchorIndex();

        sizedItemsRectHeight = farSizedItemsRect - nearSizedItemsRect;

        if (m_anchorIndex != -1 || newRecommendedAnchorIndex != -1)
        {
            const int anchorLineIndex = static_cast<int>((newRecommendedAnchorIndex == -1 ? m_anchorIndex : newRecommendedAnchorIndex) / averageItemsPerLine);

            MUX_ASSERT(anchorLineIndex >= 0);
            MUX_ASSERT(anchorLineIndex < lineCount);

            const float anchorLineNearEdge = static_cast<float>(anchorLineIndex * (actualLineHeight + lineSpacing));

            // Check if the sized items rect overlaps the anticipated anchor line, with 1 line of margin of error in the line index approximation.
            if (anchorLineNearEdge - 2 * actualLineHeight - lineSpacing <= nearSizedItemsRect || anchorLineNearEdge + actualLineHeight + lineSpacing >= farSizedItemsRect)
            {
                // Ensure the anchor index is sized by using a non-inflated sizing rect centered around the middle of the anticipated anchor line.
                const float anchorLineMiddle = static_cast<float>(anchorLineNearEdge + actualLineHeight / 2.0);

                sizedItemsRectHeight = farRealizationRect - nearRealizationRect;
                nearSizedItemsRect = GetRoundedFloat(anchorLineMiddle - sizedItemsRectHeight / 2.0f);
                farSizedItemsRect = nearSizedItemsRect + sizedItemsRectHeight;

                LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"nearSizedItemsRect", nearSizedItemsRect);
                LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"farSizedItemsRect", farSizedItemsRect);
            }
        }

        const double linesSpacing = lineCount == 0 ? 0.0 : (lineCount - 1) * lineSpacing;
        const double linesHeight = lineCount * actualLineHeight + linesSpacing;
        const double scrollViewport = context.VisibleRect().Height;
        const double scrollableSize = std::max(0.0, linesHeight - scrollViewport);

        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"linesHeight", linesHeight);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"scrollViewport", scrollViewport);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"scrollableSize", scrollableSize);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"sizedItemsRectHeight", sizedItemsRectHeight);

        // nearSizedItemsRect must not be negative.
        if (nearSizedItemsRect < 0.0)
        {
            sizedItemsRectHeight += nearSizedItemsRect;
            nearSizedItemsRect = 0.0;

            MUX_ASSERT(sizedItemsRectHeight >= 0.0f);
        }

        // farSizedItemsRect must not exceed the extent value.
        if (farSizedItemsRect > linesHeight)
        {
            sizedItemsRectHeight += static_cast<float>(linesHeight - farSizedItemsRect);
            farSizedItemsRect = static_cast<float>(linesHeight);

            MUX_ASSERT(sizedItemsRectHeight >= 0.0f);
        }

        // clampedNearSizedItemsRect must not exceed the max offset value.
        double clampedNearSizedItemsRect = std::min(scrollableSize, static_cast<double>(nearSizedItemsRect));

        // clampedFarSizedItemsRect must not exceed the extent value.
        double clampedFarSizedItemsRect = std::max(clampedNearSizedItemsRect + sizedItemsRectHeight, static_cast<double>(farSizedItemsRect));
        clampedFarSizedItemsRect = std::min(linesHeight, clampedFarSizedItemsRect);

        clampedNearSizedItemsRect = std::min(clampedFarSizedItemsRect - sizedItemsRectHeight, clampedNearSizedItemsRect);
        clampedNearSizedItemsRect = std::max(0.0, clampedNearSizedItemsRect);

        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"clampedNearSizedItemsRect", clampedNearSizedItemsRect);
        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"clampedFarSizedItemsRect", clampedFarSizedItemsRect);

        MUX_ASSERT(clampedFarSizedItemsRect - clampedNearSizedItemsRect <= sizedItemsRectHeight + 0.001);
        MUX_ASSERT(clampedNearSizedItemsRect >= 0.0);
        MUX_ASSERT(clampedNearSizedItemsRect <= scrollableSize);
        MUX_ASSERT(clampedFarSizedItemsRect >= 0.0);
        MUX_ASSERT(clampedFarSizedItemsRect <= linesHeight);

        int firstSizedLineIndex{ -1 };
        int lastSizedLineIndex{ -1 };

        GetFirstAndLastDisplayedLineIndexes(
            clampedFarSizedItemsRect - clampedNearSizedItemsRect /*scrollViewport*/,
            clampedNearSizedItemsRect /*scrollOffset*/,
            0 /*padding*/,
            lineSpacing,
            actualLineHeight,
            lineCount,
            false /*forFullyDisplayedLines*/,
            &firstSizedLineIndex,
            &lastSizedLineIndex);

        MUX_ASSERT(firstSizedLineIndex >= 0);
        MUX_ASSERT(lastSizedLineIndex >= 0);
        MUX_ASSERT(firstSizedLineIndex < lineCount);
        MUX_ASSERT(lastSizedLineIndex < lineCount);

        int newFirstSizedItemIndex = std::max(0, static_cast<int>(firstSizedLineIndex * averageItemsPerLine) - 1);
        int newLastSizedItemIndex = std::min(m_itemCount - 1, static_cast<int>((lastSizedLineIndex + 1) * averageItemsPerLine));

        MUX_ASSERT(newFirstSizedItemIndex >= 0);
        MUX_ASSERT(newLastSizedItemIndex >= 0);
        MUX_ASSERT(newFirstSizedItemIndex < m_itemCount);
        MUX_ASSERT(newLastSizedItemIndex < m_itemCount);

        *itemsRangeStartIndex = newFirstSizedItemIndex;
        *itemsRangeRequestedLength = newLastSizedItemIndex - newFirstSizedItemIndex + 1;
    }
    else
    {
        // All items are realized in non-virtualizing mode, and the fast path requests items info for the entire source collection.
        MUX_ASSERT(context.VisibleRect().Y == 0.0f);

        *itemsRangeStartIndex = 0;
        *itemsRangeRequestedLength = m_itemCount;
    }
}

// Used by LinedFlowLayout::GetItemsLayout to determine whether an item is wrapping to a new line or appended
// to the current line. The current item's width is multiplied by this 'item-size-multiplier-threshold' and then
// compared to the expected cumulated line widths and actual cumulated line widths.
double LinedFlowLayout::GetItemWidthMultiplierThreshold() const
{
    // If the discrepancy between the actual and expected total line width is greater than twice the processed item width,
    // the default wrapping behavior is reversed.
    constexpr double c_itemWidthMultiplierThreshold = 2.0;

    const double forcedWrapMultiplierDbg = ForcedWrapMultiplierDbg();

    return forcedWrapMultiplierDbg != 0.0 ? forcedWrapMultiplierDbg : c_itemWidthMultiplierThreshold;
}

// Computes an ItemsLayout for the provided available width.
// availableWidth: available width as provided by MeasureOverride.
// adjustedAvailableWidth: variation of availableWidth to equalize the items' layout on the lines.
LinedFlowLayout::ItemsLayout LinedFlowLayout::GetItemsLayout(
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
    bool isLastSizedLineStretchEnabled)
{
    MUX_ASSERT(!(beginSizedLineIndex < endSizedLineIndex && beginSizedItemIndex >= endSizedItemIndex));
    MUX_ASSERT(!(beginSizedLineIndex > endSizedLineIndex && beginSizedItemIndex <= endSizedItemIndex));
    MUX_ASSERT(std::abs(beginSizedLineIndex - endSizedLineIndex) <= std::abs(beginSizedItemIndex - endSizedItemIndex));

    const bool forward = beginSizedItemIndex <= endSizedItemIndex;
    const int sizedLineCount = forward ? endSizedLineIndex - beginSizedLineIndex + 1 : beginSizedLineIndex - endSizedLineIndex + 1;
    const double minItemSpacing = MinItemSpacing();
    int lineVectorIndex;
    int lineItemsCount = 0;
    int lastItemIndex = std::numeric_limits<int>::max();
    double cumulatedLinesWidth = 0.0;
    double cumulatedWidth = 0.0;
    double lastItemWidth = std::numeric_limits<double>::max();
    std::vector<double> headItemWidths(sizedLineCount);
    std::vector<double> tailItemWidths(sizedLineCount);
    std::vector<double> headItemDrawbackImprovements(sizedLineCount);
    std::vector<double> tailItemDrawbackImprovements(sizedLineCount);
    std::vector<int> headItemIndexes(sizedLineCount, std::numeric_limits<int>::max());
    std::vector<int> tailItemIndexes(sizedLineCount, std::numeric_limits<int>::max());

#ifdef DBG
    MUX_ASSERT(m_isVirtualizingContext == (scrollViewport != std::numeric_limits<double>::infinity()));
    MUX_ASSERT(internalLockedItemIndexes != nullptr);
    MUX_ASSERT(!forward || beginSizedItemIndex >= m_firstSizedItemIndex);
    MUX_ASSERT(!forward || endSizedItemIndex <= m_lastSizedItemIndex);
    MUX_ASSERT(forward || endSizedItemIndex >= m_firstSizedItemIndex);
    MUX_ASSERT(forward || beginSizedItemIndex <= m_lastSizedItemIndex);

    VerifyInternalLockedItemsDbg(
        *internalLockedItemIndexes,
        beginSizedLineIndex,
        endSizedLineIndex,
        beginSizedItemIndex,
        endSizedItemIndex);
#endif

    ItemsLayout itemsLayout;

    itemsLayout.m_availableLineItemsWidth = adjustedAvailableWidth;
    itemsLayout.m_lineItemCounts.resize(sizedLineCount);
    itemsLayout.m_lineItemWidths.resize(sizedLineCount);

    lineVectorIndex = forward ? 0 : sizedLineCount - 1;

#ifdef DBG
    const int lineCountDbg = GetLineCount(m_averageItemsPerLine.second);
    const int sizedItemCountDbg = forward ? endSizedItemIndex - beginSizedItemIndex + 1 : beginSizedItemIndex - endSizedItemIndex + 1;

    MUX_ASSERT(sizedItemCountDbg >= sizedLineCount);
#endif

    for (int sizedItemIndex = beginSizedItemIndex; ; sizedItemIndex = forward ? sizedItemIndex + 1 : sizedItemIndex - 1)
    {
        double itemWidth{};

        if (m_itemsInfoFirstIndex == -1)
        {
            MUX_ASSERT(m_elementManager.IsDataIndexRealized(sizedItemIndex));

            if (const auto element = m_elementManager.GetRealizedElement(sizedItemIndex /*dataIndex*/))
            {
                itemWidth = element.DesiredSize().Width;

#ifdef DBG
                const auto frameworkElementDbg = element.try_as<winrt::FrameworkElement>();

                if (frameworkElementDbg)
                {
                    if (itemWidth < frameworkElementDbg.MinWidth() - 1.0 / m_roundingScaleFactor)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, itemWidth, frameworkElementDbg.MinWidth());
                    }
                    if (itemWidth > frameworkElementDbg.MaxWidth() + 1.0 / m_roundingScaleFactor)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, itemWidth, frameworkElementDbg.MaxWidth());
                    }
                    MUX_ASSERT(itemWidth >= frameworkElementDbg.MinWidth() - 1.0 / m_roundingScaleFactor);
                    MUX_ASSERT(itemWidth <= frameworkElementDbg.MaxWidth() + 1.0 / m_roundingScaleFactor);
                }
#endif
            }
        }
        else
        {
            MUX_ASSERT(sizedItemIndex - m_itemsInfoFirstIndex < static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size()));

            const double desiredMinWidth = GetMinWidthFromItemsInfo(sizedItemIndex);
            const double desiredMaxWidth = GetMaxWidthFromItemsInfo(sizedItemIndex);
            double desiredAspectRatio = m_itemsInfoDesiredAspectRatiosForRegularPath[sizedItemIndex - m_itemsInfoFirstIndex];

            if (desiredAspectRatio <= 0.0)
            {
                // The average aspect ratio is used as a fallback value for items that were given a negative ratio
                // by the ItemsInfoRequested handler.
                desiredAspectRatio = averageAspectRatio;
            }

            itemWidth = desiredAspectRatio * actualLineHeight;

            if (desiredMinWidth >= 0.0)
            {
                itemWidth = std::max(desiredMinWidth, itemWidth);
            }

            if (desiredMaxWidth >= 0.0)
            {
                itemWidth = std::min(desiredMaxWidth, itemWidth);
            }
        }

        const auto& internalLockedItemIndexesIterator = internalLockedItemIndexes->find(sizedItemIndex);
        const bool isInternalLockedItem = internalLockedItemIndexesIterator != internalLockedItemIndexes->end();
        const int internalLockedLineIndex = isInternalLockedItem ? internalLockedItemIndexesIterator->second : -1;

        MUX_ASSERT(!forward || beginSizedLineIndex + lineVectorIndex < lineCountDbg);
        MUX_ASSERT(forward || endSizedLineIndex + lineVectorIndex < lineCountDbg);

        const bool isLockedItem = IsLockedItem(
            forward,
            beginSizedLineIndex,
            endSizedLineIndex,
            sizedItemIndex);

        const int lockedLineIndex = isLockedItem ? m_lockedItemIndexes[sizedItemIndex] : -1;

        int nextLockedItemIndex, nextLockedLineIndex;

        GetNextLockedItem(
            internalLockedItemIndexes,
            forward,
            beginSizedLineIndex,
            endSizedLineIndex,
            sizedItemIndex,
            &nextLockedItemIndex,
            &nextLockedLineIndex);

        MUX_ASSERT(!(nextLockedItemIndex == -1 && nextLockedLineIndex != -1));
        MUX_ASSERT(!(nextLockedItemIndex != -1 && nextLockedLineIndex == -1));

        // mustCumulate is set to True when 'element' must be appended to the current line.
        const bool mustCumulate =
            lineItemsCount == 0 ||                                                                                                                            // No item is present on the current line - it must at least contain one item
            (forward && LineHasLockedItem(beginSizedLineIndex + lineVectorIndex, false /*before*/, sizedItemIndex)) ||                                        // A publicly locked item is ahead going forward, and must be on the current line
            (!forward && LineHasLockedItem(endSizedLineIndex + lineVectorIndex, true /*before*/, sizedItemIndex)) ||                                          // A publicly locked item is ahead going backward, and must be on the current line
            (forward && LineHasInternalLockedItem(*internalLockedItemIndexes, beginSizedLineIndex + lineVectorIndex, false /*before*/, sizedItemIndex)) ||    // An internally locked item is ahead going forward, and must be on the current line
            (!forward && LineHasInternalLockedItem(*internalLockedItemIndexes, endSizedLineIndex + lineVectorIndex, true /*before*/, sizedItemIndex)) ||      // An internally locked item is ahead going backward, and must be on the current line
            (forward && lineVectorIndex == sizedLineCount - 1) ||                                                                                             // The current line is the last one processed, going forward
            (!forward && lineVectorIndex == 0);                                                                                                               // The current line is the last one processed, going backward
        // canCumulate is set to True when 'element' has enough room to fit in the current line.
        const bool canCumulate = cumulatedWidth + (lineItemsCount == 0.0 ? 0.0 : minItemSpacing) + itemWidth <= adjustedAvailableWidth;
        // mustNotCumulate is set to True when 'element' must wrap to the next line.
        bool mustNotCumulate = false;
        // shouldNotCumulate is set to True when there is no mandatory assignment for 'element', there is still room for it on the current line,
        // but in order to equalize lines, wrapping is preferred over not wrapping,
        //   - because of an upcoming publicly locked item,
        //   - because the cumulated item widths significantly exceeds the expected one given the line index.
        bool shouldNotCumulate = false;
        // shouldCumulate is set to True when there is no mandatory assignment for 'element', there is no room for it on the current line,
        // but in order to equalize lines, wrapping is skipped because the cumulated item widths is significantly lower than the expected one given the line index.
        bool shouldCumulate = false;

#ifdef DBG
        // mustNotCumulateDbg is a replica of mustNotCumulate below. It is only evaluated in debug mode for the purpose of the assertion
        // in order to detect an expected situation where the current item must and must not belong to the current line at the same time.
        // This avoids the evaluation of mustNotCumulate when not needed in retail mode.
        const bool mustNotCumulateDbg =
            (forward && endSizedItemIndex - sizedItemIndex < sizedLineCount - 1 - lineVectorIndex) ||
            (!forward && sizedItemIndex - endSizedItemIndex < lineVectorIndex) ||
            (forward && isLockedItem && lockedLineIndex != beginSizedLineIndex + lineVectorIndex) ||
            (!forward && isLockedItem && lockedLineIndex != endSizedLineIndex + lineVectorIndex) ||
            (forward && isInternalLockedItem && internalLockedLineIndex != beginSizedLineIndex + lineVectorIndex) ||
            (!forward && isInternalLockedItem && internalLockedLineIndex != endSizedLineIndex + lineVectorIndex) ||
            (forward && nextLockedItemIndex != -1 && nextLockedItemIndex - sizedItemIndex < nextLockedLineIndex - (beginSizedLineIndex + lineVectorIndex)) ||
            (!forward && nextLockedItemIndex != -1 && sizedItemIndex - nextLockedItemIndex < endSizedLineIndex + lineVectorIndex - nextLockedLineIndex);

        MUX_ASSERT(!(mustCumulate && mustNotCumulateDbg));
#endif

        if (!mustCumulate)
        {
            mustNotCumulate =
                (forward && endSizedItemIndex - sizedItemIndex < sizedLineCount - 1 - lineVectorIndex) ||                                                         // There are as many items left as there are lines, going forward
                (!forward && sizedItemIndex - endSizedItemIndex < lineVectorIndex) ||                                                                             // There are as many items left as there are lines, going backward
                (forward && isLockedItem && lockedLineIndex != beginSizedLineIndex + lineVectorIndex) ||                                                          // 'element' is a publicly locked item for the next line, going forward
                (!forward && isLockedItem && lockedLineIndex != endSizedLineIndex + lineVectorIndex) ||                                                           // 'element' is a publicly locked item for the next line, going backward
                (forward && isInternalLockedItem && internalLockedLineIndex != beginSizedLineIndex + lineVectorIndex) ||                                          // 'element' is an internally locked item for the next line, going forward
                (!forward && isInternalLockedItem && internalLockedLineIndex != endSizedLineIndex + lineVectorIndex) ||                                           // 'element' is an internally locked item for the next line, going backward
                (forward && nextLockedItemIndex != -1 && nextLockedItemIndex - sizedItemIndex < nextLockedLineIndex - (beginSizedLineIndex + lineVectorIndex)) || // A locked item is ahead, going forward, just far enough to impose the minimum of one item per line
                (!forward && nextLockedItemIndex != -1 && sizedItemIndex - nextLockedItemIndex < endSizedLineIndex + lineVectorIndex - nextLockedLineIndex);      // A locked item is ahead, going backward, just far enough to impose the minimum of one item per line

            if (!mustNotCumulate)
            {
                if (canCumulate)
                {
                    if (nextLockedItemIndex != -1)
                    {
                        // Avoiding under-filled lines around publicly locked items.
                        int nextPublicLockedItemIndex, nextPublicLockedLineIndex;

                        GetNextLockedItem(
                            nullptr,
                            forward,
                            beginSizedLineIndex,
                            endSizedLineIndex,
                            sizedItemIndex,
                            &nextPublicLockedItemIndex,
                            &nextPublicLockedLineIndex);

                        if (nextPublicLockedItemIndex != -1)
                        {
                            MUX_ASSERT(nextPublicLockedLineIndex != -1);

                            const int linesToFill = forward ? nextPublicLockedLineIndex - beginSizedLineIndex - lineVectorIndex : endSizedLineIndex + lineVectorIndex - nextPublicLockedLineIndex;
                            const int linesPerScrollViewport = static_cast<int>(scrollViewport / (actualLineHeight + lineSpacing));

                            if (linesToFill <= linesPerScrollViewport)
                            {
                                MUX_ASSERT(lineItemsCount > 0);

                                const int itemsAvailable = forward ? nextPublicLockedItemIndex - sizedItemIndex + lineItemsCount : sizedItemIndex - nextPublicLockedItemIndex + lineItemsCount;
                                const int averageItemsToFill = static_cast<int>(m_averageItemsPerLine.second * linesToFill);

                                // shouldNotCumulate is set to true to wrap earlier than necessary to avoid under-filled lines around the publicly locked item.
                                shouldNotCumulate = itemsAvailable < averageItemsToFill;
                            }
                        }
                    }
                }

                if (averageLineItemsWidth > 0.0 && ((canCumulate && !shouldNotCumulate) || !canCumulate))
                {
                    MUX_ASSERT(!mustCumulate);
                    MUX_ASSERT(!mustNotCumulate);

                    // Check if cumulated total line widths is much larger or smaller than expected total based on average line width.
                    const int lineCount = forward ? (lineVectorIndex + 1) : (sizedLineCount - lineVectorIndex);
                    MUX_ASSERT(lineCount > 0);
                    const double itemWidthMultiplierThreshold = GetItemWidthMultiplierThreshold();
                    const double expectedCumulatedLinesWidth = averageLineItemsWidth * lineCount;
                    const double actualCumulatedLinesWidth = cumulatedLinesWidth + cumulatedWidth + itemWidth;

                    if (canCumulate && !shouldNotCumulate)
                    {
                        if (actualCumulatedLinesWidth - expectedCumulatedLinesWidth > itemWidthMultiplierThreshold * itemWidth)
                        {
                            // In order to equalize lines, wrapping is preferred over not wrapping because the cumulated item widths significantly exceeds the expected one given the line index.
                            shouldNotCumulate = true;
                        }
                    }
                    else
                    {
                        MUX_ASSERT(!canCumulate);
                        MUX_ASSERT(!shouldNotCumulate);

                        if (expectedCumulatedLinesWidth - actualCumulatedLinesWidth > itemWidthMultiplierThreshold * itemWidth)
                        {
                            // In order to equalize lines, not wrapping is preferred over wrapping because the cumulated item widths is significantly lower than the expected one given the line index.
                            shouldCumulate = true;
                        }
                    }
                }
            }
        }

        MUX_ASSERT(!shouldNotCumulate || !shouldCumulate);

        if (mustCumulate || ((canCumulate || shouldCumulate) && !mustNotCumulate && !shouldNotCumulate))
        {
            // The current item sizedItemIndex is appended on the current line.
            if (isLockedItem ||
                isInternalLockedItem ||
                sizedItemIndex == beginSizedItemIndex ||
                sizedItemIndex == endSizedItemIndex)
            {
                lastItemWidth = std::numeric_limits<double>::max();
                lastItemIndex = std::numeric_limits<int>::max();
            }
            else
            {
                lastItemWidth = itemWidth;
                lastItemIndex = sizedItemIndex;
            }

            if (lineItemsCount != 0.0)
            {
                cumulatedWidth += minItemSpacing;
            }

            cumulatedWidth += itemWidth;
            lineItemsCount++;
        }
        else
        {
            // The current item sizedItemIndex creates a brand new line.
            MUX_ASSERT(lineItemsCount > 0);

            if (lastItemWidth != std::numeric_limits<double>::max())
            {
                tailItemWidths[lineVectorIndex] = lastItemWidth;
                tailItemIndexes[lineVectorIndex] = lastItemIndex;

                lastItemWidth = std::numeric_limits<double>::max();
                lastItemIndex = std::numeric_limits<int>::max();
            }

            cumulatedLinesWidth += cumulatedWidth;
            cumulatedWidth = itemWidth;
            lineItemsCount = 1;

            if (isLockedItem)
            {
                lineVectorIndex = lockedLineIndex - (forward ? beginSizedLineIndex : endSizedLineIndex);
                MUX_ASSERT(lineVectorIndex >= 0);
                MUX_ASSERT(lineVectorIndex <= sizedLineCount - 1);
            }
            else if (isInternalLockedItem)
            {
                lineVectorIndex = internalLockedLineIndex - (forward ? beginSizedLineIndex : endSizedLineIndex);
                MUX_ASSERT(lineVectorIndex >= 0);
                MUX_ASSERT(lineVectorIndex <= sizedLineCount - 1);
            }
            else if (sizedItemIndex == endSizedItemIndex)
            {
                lineVectorIndex = forward ? sizedLineCount - 1 : 0;
#ifdef DBG
                if (forward)
                {
                    MUX_ASSERT(lineVectorIndex == 0 || itemsLayout.m_lineItemCounts[lineVectorIndex - 1] > 0);
                }
                else
                {
                    MUX_ASSERT(lineVectorIndex == sizedLineCount - 1 || itemsLayout.m_lineItemCounts[lineVectorIndex + 1] > 0);
                }
#endif
            }
            else
            {
                MUX_ASSERT(itemsLayout.m_lineItemCounts[lineVectorIndex] > 0);
                lineVectorIndex = forward ? lineVectorIndex + 1 : lineVectorIndex - 1;
                MUX_ASSERT(lineVectorIndex >= 0);
                MUX_ASSERT(lineVectorIndex <= sizedLineCount - 1);
            }

            if (!isLockedItem &&
                !isInternalLockedItem &&
                sizedItemIndex != beginSizedItemIndex &&
                sizedItemIndex != endSizedItemIndex &&
                ((forward && endSizedItemIndex - sizedItemIndex >= sizedLineCount - lineVectorIndex) ||
                    (!forward && sizedItemIndex - endSizedItemIndex > lineVectorIndex)))
            {
                headItemWidths[lineVectorIndex] = itemWidth;
                headItemIndexes[lineVectorIndex] = sizedItemIndex;
            }
        }

        itemsLayout.m_lineItemCounts[lineVectorIndex] = lineItemsCount;
        itemsLayout.m_lineItemWidths[lineVectorIndex] = cumulatedWidth;

        if (sizedItemIndex == endSizedItemIndex)
        {
            break;
        }
    }

#ifdef DBG
    if (forward)
    {
        MUX_ASSERT(headItemWidths[0] == 0.0);
        MUX_ASSERT(headItemIndexes[0] == std::numeric_limits<int>::max());
        MUX_ASSERT(tailItemWidths[sizedLineCount - 1] == 0.0);
        MUX_ASSERT(tailItemIndexes[sizedLineCount - 1] == std::numeric_limits<int>::max());
    }
    else
    {
        MUX_ASSERT(tailItemWidths[0] == 0.0);
        MUX_ASSERT(tailItemIndexes[0] == std::numeric_limits<int>::max());
        MUX_ASSERT(headItemWidths[sizedLineCount - 1] == 0.0);
        MUX_ASSERT(headItemIndexes[sizedLineCount - 1] == std::numeric_limits<int>::max());
    }

    for (lineVectorIndex = 1; lineVectorIndex < sizedLineCount; lineVectorIndex++)
    {
        MUX_ASSERT(headItemIndexes[lineVectorIndex - 1] == std::numeric_limits<int>::max() ||
            headItemIndexes[lineVectorIndex] == std::numeric_limits<int>::max() ||
            headItemIndexes[lineVectorIndex - 1] < headItemIndexes[lineVectorIndex]);
        MUX_ASSERT(tailItemIndexes[lineVectorIndex - 1] == std::numeric_limits<int>::max() ||
            tailItemIndexes[lineVectorIndex] == std::numeric_limits<int>::max() ||
            tailItemIndexes[lineVectorIndex - 1] < tailItemIndexes[lineVectorIndex]);
    }
#endif

    for (lineVectorIndex = 0; lineVectorIndex < sizedLineCount; lineVectorIndex++)
    {
        if (headItemWidths[lineVectorIndex] != 0.0 && itemsLayout.m_lineItemCounts[lineVectorIndex] > 1)
        {
            MUX_ASSERT(!forward || lineVectorIndex - 1 >= 0);
            MUX_ASSERT(forward || lineVectorIndex + 1 < sizedLineCount);

            headItemDrawbackImprovements[lineVectorIndex] = GetItemDrawbackImprovement(
                headItemWidths[lineVectorIndex] + minItemSpacing,
                adjustedAvailableWidth,
                itemsLayout.m_lineItemWidths[lineVectorIndex],
                itemsLayout.m_lineItemWidths[forward ? lineVectorIndex - 1 : lineVectorIndex + 1],
                beginLineVectorIndex + lineVectorIndex,
                beginLineVectorIndex + (forward ? lineVectorIndex - 1 : lineVectorIndex + 1));
        }

        if (tailItemWidths[lineVectorIndex] != 0.0 && itemsLayout.m_lineItemCounts[lineVectorIndex] > 1)
        {
            MUX_ASSERT(!forward || lineVectorIndex + 1 < sizedLineCount);
            MUX_ASSERT(forward || lineVectorIndex - 1 >= 0);

            tailItemDrawbackImprovements[lineVectorIndex] = GetItemDrawbackImprovement(
                tailItemWidths[lineVectorIndex] + minItemSpacing,
                adjustedAvailableWidth,
                itemsLayout.m_lineItemWidths[lineVectorIndex],
                itemsLayout.m_lineItemWidths[forward ? lineVectorIndex + 1 : lineVectorIndex - 1],
                beginLineVectorIndex + lineVectorIndex,
                beginLineVectorIndex + (forward ? lineVectorIndex + 1 : lineVectorIndex - 1));
        }
    }

#ifdef DBG
    if (forward)
    {
        MUX_ASSERT(headItemDrawbackImprovements[0] == 0.0);
        MUX_ASSERT(tailItemDrawbackImprovements[sizedLineCount - 1] == 0.0);
    }
    else
    {
        MUX_ASSERT(tailItemDrawbackImprovements[0] == 0.0);
        MUX_ASSERT(headItemDrawbackImprovements[sizedLineCount - 1] == 0.0);
    }
#endif

    double smallestHeadItemWidth = std::numeric_limits<double>::max();
    double smallestTailItemWidth = std::numeric_limits<double>::max();
    double bestEqualizingHeadItemDrawbackImprovement = 0.0;
    double bestEqualizingTailItemDrawbackImprovement = 0.0;
    int smallestHeadItemIndex = std::numeric_limits<int>::max();
    int smallestTailItemIndex = std::numeric_limits<int>::max();
    int smallestHeadLineIndex = std::numeric_limits<int>::max();
    int smallestTailLineIndex = std::numeric_limits<int>::max();
    int bestEqualizingHeadItemIndex = std::numeric_limits<int>::max();
    int bestEqualizingTailItemIndex = std::numeric_limits<int>::max();
    int bestEqualizingHeadLineIndex = std::numeric_limits<int>::max();
    int bestEqualizingTailLineIndex = std::numeric_limits<int>::max();

    for (lineVectorIndex = 0; lineVectorIndex < sizedLineCount; lineVectorIndex++)
    {
        if (smallestHeadItemWidth > headItemWidths[lineVectorIndex] &&
            headItemWidths[lineVectorIndex] > 0.0)
        {
            smallestHeadItemWidth = headItemWidths[lineVectorIndex];
            smallestHeadItemIndex = headItemIndexes[lineVectorIndex];
            smallestHeadLineIndex = forward ? beginSizedLineIndex + lineVectorIndex : endSizedLineIndex + lineVectorIndex;
        }

        if (bestEqualizingHeadItemDrawbackImprovement < headItemDrawbackImprovements[lineVectorIndex])
        {
            bestEqualizingHeadItemDrawbackImprovement = headItemDrawbackImprovements[lineVectorIndex];
            bestEqualizingHeadItemIndex = headItemIndexes[lineVectorIndex];
            bestEqualizingHeadLineIndex = forward ? beginSizedLineIndex + lineVectorIndex : endSizedLineIndex + lineVectorIndex;
        }

        if (itemsLayout.m_lineItemWidths[lineVectorIndex] <= adjustedAvailableWidth &&
            tailItemWidths[lineVectorIndex] > 0.0 &&
            smallestTailItemWidth > tailItemWidths[lineVectorIndex] + adjustedAvailableWidth - itemsLayout.m_lineItemWidths[lineVectorIndex])
        {
            smallestTailItemWidth = tailItemWidths[lineVectorIndex] + adjustedAvailableWidth - itemsLayout.m_lineItemWidths[lineVectorIndex];
            smallestTailItemIndex = tailItemIndexes[lineVectorIndex];
            smallestTailLineIndex = forward ? beginSizedLineIndex + lineVectorIndex : endSizedLineIndex + lineVectorIndex;
        }

        if (bestEqualizingTailItemDrawbackImprovement < tailItemDrawbackImprovements[lineVectorIndex])
        {
            bestEqualizingTailItemDrawbackImprovement = tailItemDrawbackImprovements[lineVectorIndex];
            bestEqualizingTailItemIndex = tailItemIndexes[lineVectorIndex];
            bestEqualizingTailLineIndex = forward ? beginSizedLineIndex + lineVectorIndex : endSizedLineIndex + lineVectorIndex;
        }
    }

#ifdef DBG
    if (forward)
    {
        MUX_ASSERT(smallestHeadLineIndex == std::numeric_limits<int>::max() || smallestHeadLineIndex > beginSizedLineIndex);
        MUX_ASSERT(smallestHeadLineIndex == std::numeric_limits<int>::max() || smallestHeadLineIndex <= endSizedLineIndex);
        MUX_ASSERT(smallestTailLineIndex == std::numeric_limits<int>::max() || smallestTailLineIndex >= beginSizedLineIndex);
        MUX_ASSERT(smallestTailLineIndex == std::numeric_limits<int>::max() || smallestTailLineIndex < endSizedLineIndex);

        MUX_ASSERT(bestEqualizingHeadLineIndex == std::numeric_limits<int>::max() || bestEqualizingHeadLineIndex > beginSizedLineIndex);
        MUX_ASSERT(bestEqualizingHeadLineIndex == std::numeric_limits<int>::max() || bestEqualizingHeadLineIndex <= endSizedLineIndex);
        MUX_ASSERT(bestEqualizingTailLineIndex == std::numeric_limits<int>::max() || bestEqualizingTailLineIndex >= beginSizedLineIndex);
        MUX_ASSERT(bestEqualizingTailLineIndex == std::numeric_limits<int>::max() || bestEqualizingTailLineIndex < endSizedLineIndex);
    }
    else
    {
        MUX_ASSERT(smallestHeadLineIndex == std::numeric_limits<int>::max() || smallestHeadLineIndex >= endSizedLineIndex);
        MUX_ASSERT(smallestHeadLineIndex == std::numeric_limits<int>::max() || smallestHeadLineIndex < beginSizedLineIndex);
        MUX_ASSERT(smallestTailLineIndex == std::numeric_limits<int>::max() || smallestTailLineIndex > endSizedLineIndex);
        MUX_ASSERT(smallestTailLineIndex == std::numeric_limits<int>::max() || smallestTailLineIndex <= beginSizedLineIndex);

        MUX_ASSERT(bestEqualizingHeadLineIndex == std::numeric_limits<int>::max() || bestEqualizingHeadLineIndex >= endSizedLineIndex);
        MUX_ASSERT(bestEqualizingHeadLineIndex == std::numeric_limits<int>::max() || bestEqualizingHeadLineIndex < beginSizedLineIndex);
        MUX_ASSERT(bestEqualizingTailLineIndex == std::numeric_limits<int>::max() || bestEqualizingTailLineIndex > endSizedLineIndex);
        MUX_ASSERT(bestEqualizingTailLineIndex == std::numeric_limits<int>::max() || bestEqualizingTailLineIndex <= beginSizedLineIndex);
    }

    MUX_ASSERT(smallestHeadItemIndex != beginSizedItemIndex);
    MUX_ASSERT(smallestHeadItemIndex != endSizedItemIndex);
    MUX_ASSERT(smallestTailItemIndex != beginSizedItemIndex);
    MUX_ASSERT(smallestTailItemIndex != endSizedItemIndex);

    MUX_ASSERT(bestEqualizingHeadItemIndex != beginSizedItemIndex);
    MUX_ASSERT(bestEqualizingHeadItemIndex != endSizedItemIndex);
    MUX_ASSERT(bestEqualizingTailItemIndex != beginSizedItemIndex);
    MUX_ASSERT(bestEqualizingTailItemIndex != endSizedItemIndex);
#endif

    itemsLayout.m_smallestHeadItemWidth = smallestHeadItemWidth;
    itemsLayout.m_smallestTailItemWidth = smallestTailItemWidth;
    itemsLayout.m_smallestHeadItemIndex = smallestHeadItemIndex;
    itemsLayout.m_smallestTailItemIndex = smallestTailItemIndex;
    itemsLayout.m_smallestHeadLineIndex = smallestHeadLineIndex;
    itemsLayout.m_smallestTailLineIndex = smallestTailLineIndex;

    itemsLayout.m_bestEqualizingHeadItemDrawbackImprovement = bestEqualizingHeadItemDrawbackImprovement;
    itemsLayout.m_bestEqualizingTailItemDrawbackImprovement = bestEqualizingTailItemDrawbackImprovement;
    itemsLayout.m_bestEqualizingHeadItemIndex = bestEqualizingHeadItemIndex;
    itemsLayout.m_bestEqualizingTailItemIndex = bestEqualizingTailItemIndex;
    itemsLayout.m_bestEqualizingHeadLineIndex = bestEqualizingHeadLineIndex;
    itemsLayout.m_bestEqualizingTailLineIndex = bestEqualizingTailLineIndex;

    // When itemsLayout.m_smallestTailItemWidth == std::numeric_limits<double>::max(), i.e. when all itemsLayout.m_lineItemWidths[lineVectorIndex] are greater
    // than adjustedAvailableWidth, it's not worth collapsing the adjustedAvailableWidth.
    // When all itemsLayout.m_lineItemWidths[lineVectorIndex] are smaller than adjustedAvailableWidth, it's not worth expanding the adjustedAvailableWidth.

    ComputeItemsLayoutDrawback(availableWidth, isLastSizedLineStretchEnabled, itemsLayout);

#ifdef DBG
    MUX_ASSERT(itemsLayout.m_lineItemCounts[0] > 0);
    MUX_ASSERT(itemsLayout.m_lineItemCounts[sizedLineCount - 1] > 0);

    int lineItemCountsDbg = 0;

    for (lineVectorIndex = 0; lineVectorIndex < static_cast<int>(itemsLayout.m_lineItemCounts.size()); lineVectorIndex++)
    {
        MUX_ASSERT(itemsLayout.m_lineItemCounts[lineVectorIndex] > 0);

        lineItemCountsDbg += itemsLayout.m_lineItemCounts[lineVectorIndex];
    }

    MUX_ASSERT(sizedItemCountDbg == lineItemCountsDbg);
#endif

    return itemsLayout;
}

// Returns the total arrange width of the items within the provided range.
float LinedFlowLayout::GetItemsRangeArrangeWidth(
    int beginSizedItemIndex,
    int endSizedItemIndex,
    bool usesArrangeWidthInfo)
{
    MUX_ASSERT(beginSizedItemIndex >= 0);
    MUX_ASSERT(endSizedItemIndex < m_itemCount);
    MUX_ASSERT(beginSizedItemIndex <= endSizedItemIndex);
    MUX_ASSERT(!UsesFastPathLayout() || usesArrangeWidthInfo);
    MUX_ASSERT(!UsesFastPathLayout() || static_cast<int>(m_itemsInfoArrangeWidths.size()) > endSizedItemIndex);

    const int itemsInfoArrangeWidthsOffset = m_itemsInfoFirstIndex == -1 ? 0 : m_itemsInfoFirstIndex;
    float cumulatedArrangeWidth{};

    for (int sizedItemIndex = beginSizedItemIndex; sizedItemIndex <= endSizedItemIndex; sizedItemIndex++)
    {
        if (usesArrangeWidthInfo)
        {
            // Use the item info provided by the ItemsInfoRequested handler since the DesiredSize.Width 
            // may still be the MinWidth because the content is loading.
            // The use of m_itemsInfoArrangeWidths must be consistent with the one in LinedFlowLayout::ArrangeConstrainedLines.
            cumulatedArrangeWidth += m_itemsInfoArrangeWidths[sizedItemIndex - itemsInfoArrangeWidthsOffset];
        }
        else if (m_elementManager.IsDataIndexRealized(sizedItemIndex))
        {
            if (const auto element = m_elementManager.GetRealizedElement(sizedItemIndex /*dataIndex*/))
            {
                cumulatedArrangeWidth += element.DesiredSize().Width;
            }
        }
    }

    return cumulatedArrangeWidth;
}

// Uses the m_lineItemCounts vector to return the index of the last item in the provided line index.
int LinedFlowLayout::GetLastItemIndexInLineIndex(
    int lineVectorIndex) const
{
    MUX_ASSERT(lineVectorIndex >= 0);
    MUX_ASSERT(lineVectorIndex < static_cast<int>(m_lineItemCounts.size()));

    int firstItemIndexInLineIndex = GetFirstItemIndexInLineIndex(lineVectorIndex);

    return firstItemIndexInLineIndex + m_lineItemCounts[lineVectorIndex] - 1;
}

int LinedFlowLayout::GetLineCount(
    double averageItemsPerLine) const
{
    if (m_itemCount == 0)
    {
        return 0;
    }

    MUX_ASSERT(m_itemCount > 0);

    const int lineCount = GetLineIndexFromAverageItemsPerLine(m_itemCount - 1, averageItemsPerLine) + 1;

#ifdef DBG_VERBOSE
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, lineCount);
#endif

    return lineCount;
}

// Returns the line index for the provided item index.
int LinedFlowLayout::GetLineIndex(
    int itemIndex,
    bool usesFastPathLayout) const
{
    MUX_ASSERT(m_isVirtualizingContext);
    MUX_ASSERT(itemIndex >= 0);
    MUX_ASSERT(itemIndex < m_itemCount);

    if (itemIndex == 0)
    {
        // May occur while m_averageItemsPerLine.second is still 0.
        return 0;
    }

    int lineIndex = 0;

    if (usesFastPathLayout)
    {
        int lineItemCounts{};

        while (lineItemCounts + m_lineItemCounts[lineIndex] <= itemIndex)
        {
            lineItemCounts += m_lineItemCounts[lineIndex];
            lineIndex++;
        }
    }
    else
    {
        MUX_ASSERT(m_averageItemsPerLine.second >= 1.0);

        const auto& lockedItemIterator = m_lockedItemIndexes.find(itemIndex);

        // Determine the element's line index depending on its locked status.
        if (lockedItemIterator != m_lockedItemIndexes.end())
        {
            // Since the item is locked, use the line index it's locked on.
            lineIndex = lockedItemIterator->second;
        }
        else
        {
            int sizedItemIndex = m_firstSizedItemIndex;
            const int sizedItemCount = m_lastSizedItemIndex - m_firstSizedItemIndex + 1;
            const int sizedLineVectorCount = static_cast<int>(m_lineItemCounts.size());

            // If the item is sized, use its sized line index.
            if (sizedItemIndex != -1 &&
                itemIndex >= sizedItemIndex &&
                itemIndex < sizedItemIndex + sizedItemCount &&
                m_lastSizedLineIndex - m_firstSizedLineIndex + 1 == sizedLineVectorCount)
            {
                MUX_ASSERT(m_firstSizedLineIndex >= 0);

                int sizedLineVectorIndex = 0;

                while (sizedItemIndex + m_lineItemCounts[sizedLineVectorIndex] - 1 < itemIndex)
                {
                    sizedItemIndex += m_lineItemCounts[sizedLineVectorIndex];
                    sizedLineVectorIndex++;

                    MUX_ASSERT(sizedLineVectorIndex < sizedLineVectorCount);
                }

                lineIndex = m_firstSizedLineIndex + sizedLineVectorIndex;
            }
            else
            {
                // As a last resort, use the estimated line index based on the average items per line.
                lineIndex = GetLineIndexFromAverageItemsPerLine(itemIndex, m_averageItemsPerLine.second);
            }
        }
    }

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, L"lineIndex", lineIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, L"itemIndex", itemIndex);

    return lineIndex;
}

int LinedFlowLayout::GetLineIndexFromAverageItemsPerLine(
    int itemIndex,
    double averageItemsPerLine) const
{
    MUX_ASSERT(averageItemsPerLine >= 1.0);

    const int lineIndex = static_cast<int>(itemIndex / averageItemsPerLine);

    return lineIndex;
}

// Returns the largest line width for a range of lines.
// When item sizing info is used, that range is the sized lines.
// Otherwise it's the frozen lines only.
float LinedFlowLayout::GetLinesDesiredWidth()
{
    if (m_firstFrozenLineIndex == -1)
    {
        // This occurs when GetFirstAndLastDisplayedLineIndexes returns firstDisplayedLineIndex==-1
        // because the scrollViewport is still 0.0 for example.
        return 0.0;
    }

    MUX_ASSERT(m_firstSizedLineIndex >= 0);
    MUX_ASSERT(m_lastSizedLineIndex >= m_firstSizedLineIndex);
    MUX_ASSERT(m_firstSizedItemIndex >= 0);
    MUX_ASSERT(m_lastSizedItemIndex >= m_firstSizedItemIndex);
    MUX_ASSERT(m_firstFrozenLineIndex >= 0);
    MUX_ASSERT(m_lastFrozenLineIndex >= m_firstFrozenLineIndex);
    MUX_ASSERT(m_firstFrozenItemIndex >= 0);
    MUX_ASSERT(m_lastFrozenItemIndex >= m_firstFrozenItemIndex);

    const bool usesArrangeWidthInfo = UsesArrangeWidthInfo();
    const float minItemSpacing = static_cast<float>(MinItemSpacing());
    const int firstLineIndex = usesArrangeWidthInfo ? m_firstSizedLineIndex : m_firstFrozenLineIndex;
    const int lastLineIndex = usesArrangeWidthInfo ? m_lastSizedLineIndex : m_lastFrozenLineIndex;
    int itemIndex = usesArrangeWidthInfo ? m_firstSizedItemIndex : m_firstFrozenItemIndex;
    float maxLineWidth{};

    MUX_ASSERT(!usesArrangeWidthInfo || m_itemsInfoFirstIndex >= 0);

    for (int lineIndex = firstLineIndex; lineIndex <= lastLineIndex; lineIndex++)
    {
        const int lineItemsCount = m_lineItemCounts[lineIndex - m_firstSizedLineIndex];
        float lineWidth{};

        for (int lineItemIndex = 0; lineItemIndex < lineItemsCount; lineItemIndex++)
        {
            float itemWidth{};

            if (usesArrangeWidthInfo)
            {
                itemWidth = m_itemsInfoArrangeWidths[itemIndex - m_itemsInfoFirstIndex];
            }
            else
            {
                MUX_ASSERT(m_elementManager.IsDataIndexRealized(itemIndex));

                if (const auto element = m_elementManager.GetRealizedElement(itemIndex))
                {
                    itemWidth = element.DesiredSize().Width;
                }
            }

            if (lineItemIndex > 0)
            {
                lineWidth += minItemSpacing;
            }

            lineWidth += itemWidth;

            itemIndex++;
        }

        maxLineWidth = std::max(lineWidth, maxLineWidth);
    }

    return maxLineWidth;
}

double LinedFlowLayout::GetMaxWidthFromItemsInfo(
    int itemIndex) const
{
    MUX_ASSERT(itemIndex >= 0);
    MUX_ASSERT(itemIndex < m_itemCount);
    MUX_ASSERT(m_itemsInfoMaxWidth >= 0.0 || m_itemsInfoMaxWidth == -1.0);

    if (UsesFastPathLayout())
    {
        // Fast path layout
        if (itemIndex < static_cast<int>(m_itemsInfoMaxWidthsForFastPath.size()))
        {
            return std::min(m_itemsInfoMaxWidth, m_itemsInfoMaxWidthsForFastPath[itemIndex]);
        }
    }
    else
    {
        // Regular path layout
        if (itemIndex - m_itemsInfoFirstIndex >= 0 &&
            itemIndex - m_itemsInfoFirstIndex < static_cast<int>(m_itemsInfoMaxWidthsForRegularPath.size()))
        {
            return std::min(m_itemsInfoMaxWidth, m_itemsInfoMaxWidthsForRegularPath[itemIndex - m_itemsInfoFirstIndex]);
        }
    }

    return m_itemsInfoMaxWidth;
}

double LinedFlowLayout::GetMinWidthFromItemsInfo(
    int itemIndex) const
{
    MUX_ASSERT(itemIndex >= 0);
    MUX_ASSERT(itemIndex < m_itemCount);
    MUX_ASSERT(m_itemsInfoMinWidth >= 0.0 || m_itemsInfoMinWidth == -1.0);

    if (UsesFastPathLayout())
    {
        // Fast path layout
        if (itemIndex < static_cast<int>(m_itemsInfoMinWidthsForFastPath.size()))
        {
            return std::max(m_itemsInfoMinWidth, m_itemsInfoMinWidthsForFastPath[itemIndex]);
        }
    }
    else
    {
        // Regular path layout
        if (itemIndex - m_itemsInfoFirstIndex >= 0 &&
            itemIndex - m_itemsInfoFirstIndex < static_cast<int>(m_itemsInfoMinWidthsForRegularPath.size()))
        {
            return std::max(m_itemsInfoMinWidth, m_itemsInfoMinWidthsForRegularPath[itemIndex - m_itemsInfoFirstIndex]);
        }
    }

    return m_itemsInfoMinWidth;
}

void LinedFlowLayout::GetNextLockedItem(
    std::map<int, int>* internalLockedItemIndexes,
    bool forward,
    int beginLineIndex,
    int endLineIndex,
    int itemIndex,
    int* lockedItemIndex,
    int* lockedLineIndex) const
{
    *lockedItemIndex = *lockedLineIndex = -1;

    int distance = std::numeric_limits<int>::max();

    if (internalLockedItemIndexes != nullptr)
    {
        for (const auto& lockedItemIterator : *internalLockedItemIndexes)
        {
            if (forward &&
                lockedItemIterator.first > itemIndex &&
                lockedItemIterator.first - itemIndex < distance &&
                lockedItemIterator.second >= beginLineIndex &&
                lockedItemIterator.second <= endLineIndex)
            {
                distance = lockedItemIterator.first - itemIndex;
                *lockedItemIndex = lockedItemIterator.first;
                *lockedLineIndex = lockedItemIterator.second;
            }
            else if (!forward &&
                lockedItemIterator.first < itemIndex &&
                itemIndex - lockedItemIterator.first < distance &&
                lockedItemIterator.second <= beginLineIndex &&
                lockedItemIterator.second >= endLineIndex)
            {
                distance = itemIndex - lockedItemIterator.first;
                *lockedItemIndex = lockedItemIterator.first;
                *lockedLineIndex = lockedItemIterator.second;
            }
        }
    }

    for (const auto& lockedItemIterator : m_lockedItemIndexes)
    {
        if (forward &&
            lockedItemIterator.first > itemIndex &&
            lockedItemIterator.first - itemIndex < distance &&
            lockedItemIterator.second >= beginLineIndex &&
            lockedItemIterator.second <= endLineIndex)
        {
            distance = lockedItemIterator.first - itemIndex;
            *lockedItemIndex = lockedItemIterator.first;
            *lockedLineIndex = lockedItemIterator.second;
        }
        else if (!forward &&
            lockedItemIterator.first < itemIndex &&
            itemIndex - lockedItemIterator.first < distance &&
            lockedItemIterator.second <= beginLineIndex &&
            lockedItemIterator.second >= endLineIndex)
        {
            distance = itemIndex - lockedItemIterator.first;
            *lockedItemIndex = lockedItemIterator.first;
            *lockedLineIndex = lockedItemIterator.second;
        }
    }
}

float LinedFlowLayout::GetRealizationRectHeightDeficit(
    winrt::VirtualizingLayoutContext const& context,
    double actualLineHeight,
    double lineSpacing) const
{
    // This layout performs poorly when there are less than 20 lines to operate with in 5 scroll viewports.
    // The target realization height is at least 3 viewports and 12 lines, assuming that 2 more viewports worth
    // of unrealized sized items are used via the ItemsInfoRequested event. Otherwise the realization rect falls back to 5 viewports.
    constexpr float minimumCacheLength = 2.0f;
    constexpr float minimumCacheLineCount = 4.0f; // At least 4 x (minimumCacheLength + 1) = 12 lines worth of items.
    const float realizationRectHeight = context.RealizationRect().Height;
    const float nearRealizationRect = GetRoundedFloat(context.RealizationRect().Y);
    const float lineHeight = static_cast<float>(actualLineHeight + lineSpacing);
    const float scrollViewport = context.VisibleRect().Height;

    // Ensure there is a viewport buffer before and after the visible viewport.
    float realizationRectHeightDeficit = std::max(0.0f, (minimumCacheLength + 1.0f) * scrollViewport - realizationRectHeight);

    // Ensure there are 12 lines in the resulting 3 viewports.
    realizationRectHeightDeficit = std::max(realizationRectHeightDeficit, (minimumCacheLength + 1.0f) * minimumCacheLineCount * lineHeight - realizationRectHeight);

    MUX_ASSERT(realizationRectHeightDeficit >= 0.0);

    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT, METH_NAME, this, realizationRectHeightDeficit);

    return realizationRectHeightDeficit;
}

// Returns the range of realized items given a larger range of sized items.
void LinedFlowLayout::GetRealizedItemsFromSizedItems(
    int realizedLineCount,
    int lineCount,
    int* unrealizedNearItemCount,
    int* realizedItemCount) const
{
    int unrealizedNearItemCountTmp = static_cast<int>(std::round(m_unrealizedNearLineCount * m_averageItemsPerLine.second));

    unrealizedNearItemCountTmp = std::min(m_itemCount - 1, unrealizedNearItemCountTmp);
    unrealizedNearItemCountTmp = std::max(m_unrealizedNearLineCount, unrealizedNearItemCountTmp);
    unrealizedNearItemCountTmp = std::max(m_firstSizedItemIndex, unrealizedNearItemCountTmp);

    int realizedItemCountTmp{};

    if (m_unrealizedNearLineCount + realizedLineCount == lineCount)
    {
        realizedItemCountTmp = m_itemCount - unrealizedNearItemCountTmp;
    }
    else
    {
        realizedItemCountTmp = static_cast<int>(std::round(realizedLineCount * m_averageItemsPerLine.second));
    }

    realizedItemCountTmp = std::max(1, realizedItemCountTmp);
    realizedItemCountTmp = std::max(realizedLineCount, realizedItemCountTmp);
    realizedItemCountTmp = std::min(m_lastSizedItemIndex - unrealizedNearItemCountTmp + 1, realizedItemCountTmp);

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"unrealizedNearItemCount", unrealizedNearItemCountTmp);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"realizedItemCount", realizedItemCountTmp);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"realized item index range", unrealizedNearItemCountTmp, unrealizedNearItemCountTmp + realizedItemCountTmp - 1);

    MUX_ASSERT(unrealizedNearItemCountTmp < m_itemCount);
    MUX_ASSERT(unrealizedNearItemCountTmp >= m_unrealizedNearLineCount);
    MUX_ASSERT((m_unrealizedNearLineCount == 0 && unrealizedNearItemCountTmp == 0) || (m_unrealizedNearLineCount > 0 && unrealizedNearItemCountTmp > 0));
    MUX_ASSERT(realizedItemCountTmp > 0);
    MUX_ASSERT(realizedItemCountTmp >= realizedLineCount);
    MUX_ASSERT(unrealizedNearItemCountTmp + realizedItemCountTmp <= m_itemCount);

    *unrealizedNearItemCount = unrealizedNearItemCountTmp;
    *realizedItemCount = realizedItemCountTmp;
}

float LinedFlowLayout::GetSizedItemsRectHeight(
    winrt::VirtualizingLayoutContext const& context,
    double actualLineHeight,
    double lineSpacing) const
{
    // This layout performs poorly when there are less than 20 lines to operate with in 5 scroll viewports.
    // The target rect height for the realized + unrealized sized items is at least 5 viewports and 20 lines.
    constexpr float minimumCacheLength = 4.0f;
    constexpr float minimumCacheLineCount = 4.0f;

    // Ensure there are 4 buffer viewports before and after the visible viewport.
    const float sizedItemsRectHeight = (minimumCacheLength + 1.0f) * context.VisibleRect().Height;

    // Ensure there are 20 lines in the resulting 5 viewports.
    return std::max(sizedItemsRectHeight, (minimumCacheLength + 1.0f) * minimumCacheLineCount * static_cast<float>(actualLineHeight + lineSpacing));
}

double LinedFlowLayout::GetRoundedDouble(
    double value) const
{
    return std::round(value * m_roundingScaleFactor) / m_roundingScaleFactor;
}

float LinedFlowLayout::GetRoundedFloat(
    float value) const
{
    return static_cast<float>(std::round(value * m_roundingScaleFactor) / m_roundingScaleFactor);
}

void LinedFlowLayout::InitializeForRelayout(
    int sizedLineCount,
    int& firstStillSizedLineIndex,
    int& lastStillSizedLineIndex,
    int& firstStillSizedItemIndex,
    int& lastStillSizedItemIndex)
{
    EnsureLineItemCounts(sizedLineCount);

    firstStillSizedLineIndex = -1;
    lastStillSizedLineIndex = -1;
    firstStillSizedItemIndex = -1;
    lastStillSizedItemIndex = -1;
}

// Causes potential items info to be discarded and triggers a complete re-layout so that
// the ItemsInfoRequested event gets raised again to retrieve updated sizing information.
void LinedFlowLayout::InvalidateItemsInfo()
{
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    InvalidateLayout(true /*forceRelayout*/, true /*resetItemsInfo*/, true /*invalidateMeasure*/);
}

// - forceRelayout == True:     forces a re-layout in the next measure pass.
// - resetItemsInfo == True:    resets the items info gathered so far.
// - invalidateMeasure == True: triggers a new measure pass.
void LinedFlowLayout::InvalidateLayout(
    bool forceRelayout,
    bool resetItemsInfo,
    bool invalidateMeasure)
{
    MUX_ASSERT(forceRelayout || resetItemsInfo || invalidateMeasure);

    if (forceRelayout)
    {
        // Perform a complete re-layout during the next layout pass.
        NotifyLinedFlowLayoutInvalidatedDbg(winrt::LinedFlowLayoutInvalidationTrigger::InvalidateLayoutCall);
        m_forceRelayout = true;
    }

    if (resetItemsInfo)
    {
        // Discard any potential item sizing info previously collected through the ItemsInfoRequested event.
        ResetItemsInfo();
    }

    if (invalidateMeasure)
    {
        // Trigger a new layout pass.
        __super::InvalidateMeasure();
    }
}

// Invokes InvalidateLayout asynchronously with only invalidateMeasure == True to trigger a new measure pass.
void LinedFlowLayout::InvalidateMeasureAsync()
{
    m_dispatcherQueue.TryEnqueue(winrt::DispatcherQueueHandler(
        [strongThis = get_strong()]()
    {
        strongThis->InvalidateLayout(false /*forceRelayout*/, false /*resetItemsInfo*/, true /*invalidateMeasure*/);
    }));
}

// Starts the timer used to trigger an asynchronous measure pass when no items info was provided by the ItemsInfoRequested
// event. Invoked after an item was realized or when the timer expires and the tickCount is still smaller than 7.
// The timer interval begins at 100ms and is then increased by 50% each time it is re-started, until tickCount reaches 7.
// By then the interval is 1.7s and the total time elapsed is about 5s when the timer is no longer re-started.
// Asynchronous measure passes are triggered to check if the ItemsRepeater's children have a new desired size.
// This is done because when a child is re-measured because its content changed, the Xaml layout engine uses its previous
// available size, preventing the ItemsRepeater from being re-measured. The use of LinedFlowLayout::ItemRangesHaveNewDesiredWidths
// in the asynchronous measure pass will ensure that new desired widths for the frozen items will trigger a full re-layout.
void LinedFlowLayout::InvalidateMeasureTimerStart(
    int tickCount)
{
    MUX_ASSERT(!UsesArrangeWidthInfo());

    winrt::DispatcherTimer invalidateMeasureTimer = nullptr;

    if (m_invalidateMeasureTimer)
    {
        invalidateMeasureTimer = m_invalidateMeasureTimer.get();

        if (invalidateMeasureTimer.IsEnabled())
        {
            invalidateMeasureTimer.Stop();
        }
    }
    else
    {
        invalidateMeasureTimer = winrt::DispatcherTimer();

        invalidateMeasureTimer.Tick({ this, &LinedFlowLayout::InvalidateMeasureTimerTick });
        m_invalidateMeasureTimer.set(invalidateMeasureTimer);
    }

    m_invalidateMeasureTimerTickCount = tickCount;

    constexpr double timerFactor = 1.5;
    const double timerMultiplier = std::pow(timerFactor, tickCount);

    constexpr int64_t timerBase = 1000000; // 100ms
    const int64_t timerInterval = static_cast<int64_t>(timerBase * timerMultiplier);

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"tickCount", tickCount);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"timerInterval", timerInterval);

    invalidateMeasureTimer.Interval(winrt::TimeSpan::duration(timerInterval));
    invalidateMeasureTimer.Start();
}

// Stops the timer used to trigger an asynchronous measure pass when no items info was provided by the ItemsInfoRequested event.
void LinedFlowLayout::InvalidateMeasureTimerStop(bool isForDestructor)
{
    if (isForDestructor)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
    }
    else
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);
    }

    if (winrt::DispatcherTimer invalidateMeasureTimer = isForDestructor ? m_invalidateMeasureTimer.safe_get() : m_invalidateMeasureTimer.get())
    {
        if (invalidateMeasureTimer.IsEnabled())
        {
            invalidateMeasureTimer.Stop();
        }
    }
}

// Invoked when the timer used to trigger asynchronous measure passes expires.
// Re-starts it with a longer interval until m_invalidateMeasureTimerTickCount
// reaches the value 7, for a total of 8 invalidations.
void LinedFlowLayout::InvalidateMeasureTimerTick(
    winrt::IInspectable const& /*sender*/,
    winrt::IInspectable const& /*args*/)
{
    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    InvalidateMeasureAsync();

    // The asynchronous measure pass is triggered 8 times to check if the ItemsRepeater's children
    // have a new desired size. This is done because when a child is re-measured because its content changed,
    // the Xaml layout engine uses its previous available size, preventing the ItemsRepeater from being re-measured.
    constexpr int maxInvalidateMeasureTimerTickCount = 7;

    if (!UsesArrangeWidthInfo() && m_invalidateMeasureTimerTickCount < maxInvalidateMeasureTimerTickCount)
    {
        InvalidateMeasureTimerStart(m_invalidateMeasureTimerTickCount + 1);
    }
    else
    {
        InvalidateMeasureTimerStop(false /*isForDestructor*/);
    }
}

// Determines whether using a smaller adjustedAvailableWidth may lead to an ItemsLayout with a smaller drawback.
bool LinedFlowLayout::IsItemsLayoutContractionWorthy(
    ItemsLayout const& itemsLayout)
{
    if (itemsLayout.m_smallestTailItemWidth == std::numeric_limits<double>::max() || itemsLayout.m_smallestTailItemWidth == 0.0)
    {
        return false;
    }

    const int sizedLineCount = static_cast<int>(itemsLayout.m_lineItemWidths.size());

    if (sizedLineCount > 1)
    {
        for (int lineVectorIndex = 0; lineVectorIndex < sizedLineCount; lineVectorIndex++)
        {
            if (itemsLayout.m_lineItemCounts[lineVectorIndex] > 1)
            {
                return true;
            }
        }
    }

    return false;
}

bool LinedFlowLayout::IsItemsLayoutEqualizationWorthy(
    ItemsLayout const& itemsLayout,
    bool withHeadItem)
{
    if ((withHeadItem && itemsLayout.m_bestEqualizingHeadItemIndex == std::numeric_limits<int>::max()) ||
        (!withHeadItem && itemsLayout.m_bestEqualizingTailItemIndex == std::numeric_limits<int>::max()))
    {
        return false;
    }

    const int sizedLineCount = static_cast<int>(itemsLayout.m_lineItemWidths.size());

    if (sizedLineCount > 1)
    {
        for (int lineVectorIndex = 0; lineVectorIndex < sizedLineCount; lineVectorIndex++)
        {
            if (itemsLayout.m_lineItemCounts[lineVectorIndex] > 1)
            {
                return true;
            }
        }
    }

    return false;
}

// Determines whether using a larger adjustedAvailableWidth may lead to an ItemsLayout with a smaller drawback.
bool LinedFlowLayout::IsItemsLayoutExpansionWorthy(
    ItemsLayout const& itemsLayout)
{
    if (itemsLayout.m_smallestHeadItemWidth == std::numeric_limits<double>::max() || itemsLayout.m_smallestHeadItemWidth == 0.0)
    {
        return false;
    }

    const int sizedLineCount = static_cast<int>(itemsLayout.m_lineItemWidths.size());
    bool isExpansionWorthy = false;

    if (sizedLineCount > 1)
    {
        for (int lineVectorIndex = 0; lineVectorIndex < sizedLineCount; lineVectorIndex++)
        {
            if (itemsLayout.m_lineItemCounts[lineVectorIndex] > 1)
            {
                isExpansionWorthy = true;
                break;
            }
        }
    }

    if (isExpansionWorthy)
    {
        int lineVectorIndex;

        for (lineVectorIndex = 0; lineVectorIndex < sizedLineCount; lineVectorIndex++)
        {
            if (itemsLayout.m_lineItemWidths[lineVectorIndex] > itemsLayout.m_availableLineItemsWidth)
            {
                break;
            }
        }

        isExpansionWorthy = lineVectorIndex < sizedLineCount;
    }

    MUX_ASSERT(!isExpansionWorthy || itemsLayout.m_smallestHeadItemWidth > 0);

    return isExpansionWorthy;
}

// Returns True when 'itemIndex' is locked in a line within the [beginLineIndex, endLineIndex] range.
bool LinedFlowLayout::IsLockedItem(
    bool forward,
    int beginLineIndex,
    int endLineIndex,
    int itemIndex)
{
    const auto& lockedItemIterator = m_lockedItemIndexes.find(itemIndex);

    if (lockedItemIterator != m_lockedItemIndexes.end())
    {
        const int lineIndex = lockedItemIterator->second;

        if (forward)
        {
            if (lineIndex >= beginLineIndex && lineIndex <= endLineIndex)
            {
                return true;
            }
        }
        else
        {
            if (lineIndex <= beginLineIndex && lineIndex >= endLineIndex)
            {
                return true;
            }
        }
    }

    return false;
}

bool LinedFlowLayout::IsVirtualizingContext(
    winrt::VirtualizingLayoutContext const& context) const
{
    if (context)
    {
        const auto rect = context.RealizationRect();
        const bool hasInfiniteSize = (rect.Height == std::numeric_limits<float>::infinity() || rect.Width == std::numeric_limits<float>::infinity());
        return !hasInfiniteSize;
    }
    return false;
}

bool LinedFlowLayout::LineHasInternalLockedItem(
    std::map<int, int> const& internalLockedItemIndexes,
    int lineIndex,
    bool before,
    int itemIndex) const
{
    for (const auto& lockedItemIterator : internalLockedItemIndexes)
    {
        if (lockedItemIterator.second == lineIndex)
        {
            const int lockedItemIndex = lockedItemIterator.first;

            if ((before && lockedItemIndex <= itemIndex) || (!before && lockedItemIndex >= itemIndex))
            {
                return true;
            }
        }
    }

    return false;
}

bool LinedFlowLayout::LineHasLockedItem(
    int lineIndex,
    bool before,
    int itemIndex)
{
    for (const auto& lockedItemIterator : m_lockedItemIndexes)
    {
        if (lockedItemIterator.second == lineIndex)
        {
            const int lockedItemIndex = lockedItemIterator.first;

            if ((before && lockedItemIndex <= itemIndex) || (!before && lockedItemIndex >= itemIndex))
            {
                return true;
            }
        }
    }

    return false;
}

// Returns a boolean triplet covering 3 adjacent item ranges: items preceding the frozen ones, the frozen items, and finally the items following the frozen ones.
// A True value is returned either when an item is recorded in m_elementDesiredWidths and not in oldElementDesiredWidths because of fast scrolling, or
// m_elementDesiredWidths & oldElementDesiredWidths have an item with different widths.
std::tuple<bool, bool, bool> LinedFlowLayout::ItemRangesHaveNewDesiredWidths(
    std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> oldElementDesiredWidths,
    int beginRealizedItemIndex,
    int endRealizedItemIndex)
{
    MUX_ASSERT(m_itemsInfoFirstIndex == -1);
    MUX_ASSERT(m_elementDesiredWidths);
    MUX_ASSERT(oldElementDesiredWidths);
    MUX_ASSERT(beginRealizedItemIndex <= endRealizedItemIndex);
    MUX_ASSERT(beginRealizedItemIndex <= m_firstFrozenItemIndex);
    MUX_ASSERT(endRealizedItemIndex >= m_lastFrozenItemIndex);

    bool preFrozenItemHasNewDesiredWidth = false;
    bool frozenItemHasNewDesiredWidth = false;

    for (int itemIndex = beginRealizedItemIndex; itemIndex <= endRealizedItemIndex; itemIndex++)
    {
        if (m_elementManager.IsDataIndexRealized(itemIndex))
        {
            if (const auto element = m_elementManager.GetRealizedElement(itemIndex /*dataIndex*/))
            {
                const auto elementTracker = tracker_ref<winrt::UIElement>(this, element);
                const bool hasNewDesiredWidth = m_elementDesiredWidths->find(elementTracker) != m_elementDesiredWidths->end();

                if (hasNewDesiredWidth)
                {
                    const bool hasOldDesiredWidth = oldElementDesiredWidths->find(elementTracker) != oldElementDesiredWidths->end();
                    const float newDesiredWidth = m_elementDesiredWidths->at(elementTracker);
                    bool desiredWidthChanged = false;

                    MUX_ASSERT(element.DesiredSize().Width == newDesiredWidth);

                    if (hasOldDesiredWidth)
                    {
                        const float oldDesiredWidth = oldElementDesiredWidths->at(elementTracker);

                        desiredWidthChanged = oldDesiredWidth != newDesiredWidth;

#ifdef DBG
                        if (desiredWidthChanged)
                        {
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginRealizedItemIndex", beginRealizedItemIndex);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endRealizedItemIndex", endRealizedItemIndex);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"index causing relayout for changed desired size", itemIndex);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"oldDesiredWidth", oldDesiredWidth);
                        }
                        else if (itemIndex == m_logItemIndexDbg)
                        {
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginRealizedItemIndex", beginRealizedItemIndex);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endRealizedItemIndex", endRealizedItemIndex);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"oldDesiredWidth", oldDesiredWidth);
                        }
#endif
                    }
#ifdef DBG
                    else
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"index causing relayout for new desired size", itemIndex);
                    }
#endif

                    if (desiredWidthChanged || !hasOldDesiredWidth)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"newDesiredWidth", newDesiredWidth);

                        NotifyLinedFlowLayoutInvalidatedDbg(winrt::LinedFlowLayoutInvalidationTrigger::ItemDesiredWidthChange);

                        if (itemIndex < m_firstFrozenItemIndex)
                        {
                            MUX_ASSERT(!preFrozenItemHasNewDesiredWidth);

                            preFrozenItemHasNewDesiredWidth = true;

                            // Now that preFrozenItemHasNewDesiredWidth was set for the initial unfrozen items range, skip the remaining initial unfrozen items after itemIndex.
                            itemIndex = m_firstFrozenItemIndex - 1;
                        }
                        else if (itemIndex <= m_lastFrozenItemIndex)
                        {
                            MUX_ASSERT(!frozenItemHasNewDesiredWidth);

                            frozenItemHasNewDesiredWidth = true;

                            // Now that frozenItemHasNewDesiredWidth was set for the frozen items range, skip the remaining frozen items after itemIndex.
                            itemIndex = m_lastFrozenItemIndex;
                        }
                        else
                        {
#ifdef DBG
                            if (preFrozenItemHasNewDesiredWidth)
                            {
                                if (frozenItemHasNewDesiredWidth)
                                {
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <true, true, true>");
                                }
                                else
                                {
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <true, false, true>");
                                }
                            }
                            else
                            {
                                if (frozenItemHasNewDesiredWidth)
                                {
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <false, true, true>");
                                }
                                else
                                {
                                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <false, false, true>");
                                }
                            }
#endif
                            return std::tuple<bool, bool, bool>(preFrozenItemHasNewDesiredWidth, frozenItemHasNewDesiredWidth, true /*postFrozenItemHasNewDesiredWidth*/);
                        }
                    }
#ifdef DBG
                    else if (itemIndex == m_logItemIndexDbg)
                    {
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"newDesiredWidth", newDesiredWidth);
                    }
#endif
                }
            }
        }
    }

#ifdef DBG
    if (preFrozenItemHasNewDesiredWidth)
    {
        if (frozenItemHasNewDesiredWidth)
        {
            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <true, true, false>");
        }
        else
        {
            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <true, false, false>");
        }
    }
    else
    {
        if (frozenItemHasNewDesiredWidth)
        {
            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <false, true, false>");
        }
        else
        {
            LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"returns <false, false, false>");
        }
    }
#endif

    return std::tuple<bool, bool, bool>(preFrozenItemHasNewDesiredWidth, frozenItemHasNewDesiredWidth, false /*postFrozenItemHasNewDesiredWidth*/);
}

int LinedFlowLayout::LineItemsCountTotal(
    int expectedTotal)
{
    int sizedItemCount = 0;

    for (const int lineItemsCount : m_lineItemCounts)
    {
        sizedItemCount += lineItemsCount;
    }

    MUX_ASSERT(expectedTotal == 0 || sizedItemCount == expectedTotal);

    return sizedItemCount;
}

bool LinedFlowLayout::LockItemToLineInternal(
    std::map<int, int>& internalLockedItemIndexes,
    int beginSizedLineIndex,
    int endSizedLineIndex,
    int beginSizedItemIndex,
    int endSizedItemIndex,
    int lineIndex,
    int itemIndex)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, lineIndex, itemIndex);

    MUX_ASSERT(!(beginSizedLineIndex < endSizedLineIndex && beginSizedItemIndex >= endSizedItemIndex));
    MUX_ASSERT(!(beginSizedLineIndex > endSizedLineIndex && beginSizedItemIndex <= endSizedItemIndex));
    MUX_ASSERT(std::abs(beginSizedLineIndex - endSizedLineIndex) <= std::abs(beginSizedItemIndex - endSizedItemIndex));

    if (lineIndex > itemIndex)
    {
        return false;
    }

    MUX_ASSERT(lineIndex <= itemIndex);

    const int lineCount = GetLineCount(m_averageItemsPerLine.second);

    MUX_ASSERT(lineCount > 0);
    MUX_ASSERT(lineIndex < lineCount);

    if (lineCount - lineIndex > m_itemCount - itemIndex)
    {
        return false;
    }

    if (std::abs(beginSizedLineIndex - lineIndex) > std::abs(beginSizedItemIndex - itemIndex))
    {
        return false;
    }

    if (std::abs(endSizedLineIndex - lineIndex) > std::abs(endSizedItemIndex - itemIndex))
    {
        return false;
    }

    int beforeLockedItemIndex, beforeLockedLineIndex, afterLockedItemIndex, afterLockedLineIndex;

    GetNextLockedItem(
        &internalLockedItemIndexes,
        false /*forward*/,
        lineCount - 1 /*beginLineIndex*/,
        0 /*endLineIndex*/,
        itemIndex,
        &beforeLockedItemIndex,
        &beforeLockedLineIndex);

    GetNextLockedItem(
        &internalLockedItemIndexes,
        true /*forward*/,
        0 /*beginLineIndex*/,
        lineCount - 1 /*endLineIndex*/,
        itemIndex,
        &afterLockedItemIndex,
        &afterLockedLineIndex);

    if ((beforeLockedItemIndex == -1 || (itemIndex - beforeLockedItemIndex >= lineIndex - beforeLockedLineIndex)) &&
        (afterLockedItemIndex == -1 || (afterLockedItemIndex - itemIndex >= afterLockedLineIndex - lineIndex)))
    {
        internalLockedItemIndexes.insert(std::pair<int, int>(itemIndex, lineIndex));

#ifdef DBG
        VerifyInternalLockedItemsDbg(
            internalLockedItemIndexes,
            beginSizedLineIndex,
            endSizedLineIndex,
            beginSizedItemIndex,
            endSizedItemIndex);
#endif
        return true;
    }

    return false;
}

// The ItemsInfo struct returned as the third tuple element is only filled when the first element, lineCount, is -1.
// This indicates that the regular path layout must take over because the ItemsInfoRequested event did not provide sufficient info for the fast path.
// The returned lineCount is > 0 when the fast path was successfully taken. In that case, the second element returned is the largest line width.
// When is the fast path taken?
//   When an ItemsInfoRequested event handler provided aspect ratios for all items in the source collection.
// When is the regular path taken as a fallback after this method returns?
//   - No ItemsInfoRequested event handler,
//   - Or ItemsInfoRequested event handler provided incomplete information (it does not cover the entire source collection).
// Fast path algorithm overview:
//   - aspect ratios, min and max widths gathered for the entire source collection are used to do a layout of all items.
//   - when a negative or nil aspect ratio is provided for an item, the average aspect ratio is used for it instead.
//   - items are cumulated on the current line as long as their desired width does not exceed the available width.
//   - once the next item goes beyond the available width, a key decision needs to be made: should that item be added to the current line or should it create a new line?
//   - two scale factors are then computed:
//     * the scale factor < 1 required to shrink the items in the current line to fit the available width, were the next item appended.
//     * the scale factor > 1 required to grow the items in the current line to fill the available width, were the next item create a new line.
//   - whichever resulting scale factor is closest to 1 wins. The next item is appended or creates a new line accordingly.
// Fast path characteristics versus regular path:
//   - the fast path lays out the entire collection at once. Scrolling does not raise the ItemsInfoRequested event, and does not trigger a layout.
//     It just causes realized items to be recycled, and new item realizations.
//   - the m_lineItemCounts and m_itemsInfoArrangeWidths vectors grow indefinitely as the source collection grows.
//   - after a certain collection size threshold, the time required to do the full collection layout and the memory requirements negate the efficiency of the algorithm.
//   - it is qualified as 'fast' because it employs a single pass of all items with simple decisions as to whether each item should be the beginning of a new line or not.
//     The regular path involves iterative passes for assigning items to increasingly equalized lines.
//   - a new available width always triggers a full re-layout of the entire collection. In the regular path case, no re-layout is required as long as the average-items-per-line
//     does not change.
//   - the fast path does not have to stick to a line count based on the average item aspect ratio. It has thus more freedom to lay out items with less clipping.
std::tuple<int, float, LinedFlowLayout::ItemsInfo> LinedFlowLayout::MeasureConstrainedLinesFastPath(
    winrt::VirtualizingLayoutContext const& context,
    float availableWidth,
    double actualLineHeight,
    bool forceLayoutWithoutItemsInfoRequest)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, availableWidth, actualLineHeight);

    MUX_ASSERT(m_itemCount > 0);

    float maxLineWidth{ 0.0f };
    bool unlockItems{ false };

    do
    {
        if (m_forceRelayout || m_previousAvailableWidth != availableWidth || forceLayoutWithoutItemsInfoRequest)
        {
            if (forceLayoutWithoutItemsInfoRequest)
            {
                // MeasureConstrainedLinesFastPath is invoked with forceLayoutWithoutItemsInfoRequest==True when
                // a transition from the regular path to the fast path was enabled in MeasureConstrainedLinesRegularPath
                // because (m_forceRelayout || m_previousAvailableWidth != availableWidth) == False.
                // In that case, the ItemsInfoRequested handler already provided the info for the entire data source.
                // Reset forceLayoutWithoutItemsInfoRequest so that RaiseItemsInfoRequested is called in the case
                // m_forceRelayout is set to True by EnsureItemRangeFastPath below.
                forceLayoutWithoutItemsInfoRequest = false;
            }
            else
            {
                int itemsRangeStartIndex{};
                int itemsRangeRequestedLength{};

                // Based on the average items per line, the realization rect size and offset, compute the item range for the ItemsInfoRequested event.
                GetItemsInfoRequestedRange(
                    context,
                    availableWidth,
                    actualLineHeight,
                    &itemsRangeStartIndex,
                    &itemsRangeRequestedLength);

                MUX_ASSERT(itemsRangeStartIndex >= 0);
                MUX_ASSERT(itemsRangeRequestedLength > 0);
                MUX_ASSERT(itemsRangeStartIndex + itemsRangeRequestedLength - 1 < m_itemCount);

                const ItemsInfo itemsInfo = RaiseItemsInfoRequested(itemsRangeStartIndex, itemsRangeRequestedLength);

#ifdef DBG
                LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"ItemsInfo for fast path");
                LogItemsInfoDbg(itemsRangeStartIndex, itemsRangeRequestedLength, itemsInfo);
#endif

                // Fall back to regular layout path when ItemsInfo::m_itemsRangeStartIndex is not 0.
                if (itemsInfo.m_itemsRangeStartIndex != 0)
                {
                    if (m_itemsInfoFirstIndex == -1)
                    {
                        ResetItemsInfo();
                    }

                    return std::tuple<int, float, ItemsInfo>(-1 /*lineCount*/, 0.0f /*maxLineWidth*/, itemsInfo);
                }

                // Fall back to regular layout path when ItemsInfo::m_itemsRangeLength is smaller than the collection item count.
                if (itemsInfo.m_itemsRangeLength < m_itemCount)
                {
                    if (m_itemsInfoFirstIndex == -1)
                    {
                        ResetItemsInfo();
                    }

                    return std::tuple<int, float, ItemsInfo>(-1 /*lineCount*/, 0.0f /*maxLineWidth*/, itemsInfo);
                }

                // Carry on with the fast path and reset items info from the potential prior regular path.
                ResetItemsInfo();

                m_itemsInfoMinWidth = itemsInfo.m_minWidth < 0.0 ? -1.0 : itemsInfo.m_minWidth;
                m_itemsInfoMaxWidth = itemsInfo.m_maxWidth < 0.0 ? -1.0 : itemsInfo.m_maxWidth;
            }

            maxLineWidth = ComputeItemsLayoutFastPath(
                availableWidth,
                actualLineHeight);

            // Immediately clear the arrays collected from the ItemsInfoRequested event handler.
            // ComputeItemsLayoutFastPath called above is the only method making use of them.
            ResetItemsInfoForFastPath();

            m_forceRelayout = false;
            unlockItems = true;
        }
        else if (!UsesFastPathLayout())
        {
            return std::tuple<int, float, ItemsInfo>(-1 /*lineCount*/, 0.0f /*maxLineWidth*/, s_emptyItemsInfo);
        }

        MUX_ASSERT(!m_forceRelayout);

        EnsureItemRangeFastPath(
            context,
            actualLineHeight);
    }
    while (m_forceRelayout);

    if (m_isVirtualizingContext)
    {
        if (m_elementManager.GetRealizedElementCount() > 0)
        {
            MeasureItemRange(
                actualLineHeight,
                m_elementManager.GetFirstRealizedDataIndex() /*beginRealizedItemIndex*/,
                m_elementManager.GetFirstRealizedDataIndex() + m_elementManager.GetRealizedElementCount() - 1 /*endRealizedItemIndex*/);
        }

        if (maxLineWidth == 0.0f)
        {
            maxLineWidth = m_maxLineWidth;
        }
    }
    else
    {
        MeasureItemRange(
            actualLineHeight,
            0 /*beginRealizedItemIndex*/,
            m_itemCount - 1 /*endRealizedItemIndex*/);
    }

    // Set m_averageItemsPerLine for the next re-layout. It is used above to estimate the requested item range for the ItemsInfoRequested event.
    const int lineCount = static_cast<int>(m_lineItemCounts.size());

    MUX_ASSERT(lineCount >= 1);

    // In the multi-line cases, the last line does not contribute to the average evaluation because it is likely incomplete.
    const double averageItemsPerLine = (lineCount == 1) ? m_itemCount : (m_itemCount - m_lineItemCounts[lineCount - 1]) / static_cast<double>(lineCount - 1);

    SetAverageItemsPerLine(std::pair<double, double>(averageItemsPerLine /*averageItemsPerLineRaw*/, averageItemsPerLine /*averageItemsPerLineSnapped*/), false /*unlockItems*/);

    if (unlockItems)
    {
        UnlockItems();
    }

    return std::tuple<int, float, ItemsInfo>(lineCount, maxLineWidth, s_emptyItemsInfo);
}

// Items measurement when the non-scrolling dimension is constrained.
// Algorithm details:
// The layout deals with up to 9 stacks of lines:
// ******************************************
// * E: Unrealized lines                    *
// ******************************************
// * D: Unrealized sized lines              *
// ******************************************
// * C: Realized unfrozen lines             *
// ******************************************
// * B: Realized frozen lines               *
// ******************************************
// * A: Realized frozen displayed lines     *
// ******************************************
// * B: Realized frozen lines               *
// ******************************************
// * C: Realized unfrozen lines             *
// ******************************************
// * D: Unrealized sized lines              *
// ******************************************
// * E: Unrealized lines                    *
// ******************************************
// A, B, C, D, E is the decreasing order for the likelihood of such lines being present.
// Frozen lines contain items that keep their assigned line between successive layouts.
// Because the layout performs poorly with fewer than 5 viewports worth of sizing information,
// the ItemsInfoRequested event is used to gather sizing information for 5 scroll viewports (i.e. 5 x VirtualizingLayoutContext.VisibleRect).
// The resulting information covers the zones D C B A B C D, i.e. any line within zones A B C & D is called 'sized'.
// Likewise, any line within zones A B & C is called 'realized'.
// A zone grows to 1.0 scroll viewport.
// B zone grows to 0.8 scroll viewport.
// C zone grows to 0.2 scroll viewport.
// D zone grows to 1.0 scroll viewport.
// D + C + B + A + B + C + D = 1.0 + 0.2 + 0.8 + 1.0 + 0.8 + 0.2 + 1.0 = 5.0 scroll viewports.
// When the ItemsInfoRequested handler does not provide the minimum information requested, the D zones shrink to nothing while the C zones grow to 1.2 viewports.
// The more lines in a zone the better the ability to optimize its items distribution.
// A better items distribution means:
// - smaller items shrinking to accommodate the available width in the non-scrolling direction,
// - smaller items expansion when ItemsStretch is Fill,
// - smaller items spacing when ItemsStretch is None.
// Displayed lines A and their surrounding lines B are frozen so that items are not re-shuffled while on screen.
// B zone exists for two reasons:
// - the off-UI-thread aspect of scrolling (independent scrolling), so that items freshly brought into view are not reshuffled,
// - have enough lines to perform an acceptable distribution.
// Average items per line:
//   The average items per line is computed based on realized items desired size aspect ratios. The effective average items per line is
//   always a power of 1.1 (i.e. 1.0, 1.1, 1.21, 1.331, 1.4641 etc...), and thus only changes when a new actual value differs by at least 10%.
//   This keeps the effective average number stable, since a change causes a complete re-layout.
// Aspect ratio:
//   The desired size of an item provides a desired aspect ratio (width / height). A weight is given to that aspect ratio: the first aspect ratio computed
//   for a newly realized item gets a weight of 1. Each time that item is re-measured the weight is incremented by 1 until it reaches 16.
//   This is to give more credibility to items that had the time to be properly populated (with an Image for example).
//   Using a weighted average aspect ratio contributes to the stability of the effective average items per line.
// Total line count:
//   The effective average items per line and datasource item count are used to compute the total line count E + D + C + B + A + B + C + D + E.
//   Example: 1000 datasource items / 4.1772 effective average items per line = 240 total lines.
// Realized item count:
//   The effective average items per line and realized line count are used to compute the realized item count.
//   Example: 50 realized lines x 4.1772 effective average items per line = 209 realized items.
// Item locking capability:
//   An item can be locked into a line by calling 'int lineIndex = LinedFlowLayout.LockItemToLine(itemIndex)'.
//   If the item in question belongs to a current A or B zone, its current line assignment is returned.
//   Otherwise it belongs to a C, D or E zone and the returned line index is based on the itemIndex and effective average items per line.
//   When the datasource or effective average items per line changes, the locks are invalidated and the ItemsUnlocked event is raised.
// Line height:
//   When LinedFlowLayout.LineHeight is NaN, the item for the first datasource entry is used to compute LinedFlowLayout.ActualLineHeight.
//   Otherwise ActualLineHeight just reflects the LineHeight value.
// Items distribution:
//   A valid items distributions verifies:
//   - each line has at least one item,
//   - locked items belong to their locked line.
//   Given I items to distribute among L lines, there are (I - L)^L combinations. Which one to pick?
//   A distribution is associated with a drawback number. The smaller that drawback, the better. The LinedFlowLayout computes a few distributions
//   and picks the one with the lowest drawback.
//   Distribution example:
//   ---- ------ --- ------------    *
//   |  | |    | | | |          |    *
//   ---- ------ --- ------------    *
//   -------- --------- --- ----- ---*--
//   |      | |       | | | |   | |  * |
//   -------- --------- --- ----- ---*--
//   ----- ------- ------ -----      *
//   |   | |     | |    | |   |      *
//   ----- ------- ------ -----      *
//   The available width, marked by the * vertical line is 1000px. The desired item widths and LinedFlowLayout.ItemsSpacing provide a desired width for each line.
//   That desired width can be smaller or greater than the available width, as shown in the example above.
//   The calculated drawback is the sum of the squared (desired width - available width).
//   For example (800 - 1000)^2 + (1100 - 1000)^2 + (700 - 1000)^2.
//   When LinedFlowLayout.ItemsStretch is None, the very last line does not participate in the drawback evaluation if its desired width is smaller than the available width.
//   Potentially four items in a distribution are special:
//   ---- ------ --- ------------            *
//   |  | |    | | | |          |            *
//   ---- ------ --- ------------            *
//   ------------ --------- --- ----- -----  *
//   |          | |       | | | |   | |ST |  *
//   ------------ --------- --- ----- -----  *
//   ----- ------- ------ ----- ---------    *
//   |SH | |     | |    | |   | |       |    *
//   ----- ------- ------ ----- ---------    *
//   -------- ----- ------- ------           *
//   |      | |   | |     | |    |           *
//   -------- ----- ------- ------           *
//   - the Smallest Head (SH) item
//   - the Smallest Tail (ST) item
//   The SH item is the smallest item such that by increasing the available width, it will be assigned the prior line (the increase is SH width + ItemsSpacing).
//   The ST item is the smallest item such that by decreasing the available width, it will be assigned the next line (the decrease is SH width + ItemsSpacing).
//   ---- ------ --- ----------              *
//   |  | |    | | | |        |              *
//   ---- ------ --- ----------              *
//   -------------- ------- --- ----- ----- -*-------
//   |BEH         | |     | | | |   | |   | |*      |
//   -------------- ------- --- ----- ----- -*-------
//   ----- ------- ------ ----- ------- -----*--
//   |   | |     | |    | |   | |     | |BET * |
//   ----- ------- ------ ----- ------- -----*--
//   -------- ----- ------- ------           *
//   |      | |   | |     | |    |           *
//   -------- ----- ------- ------           *
//   - the Best Equalizing Head (BEH) item
//   - the Best Equalizing Tail (BET) item
//   The BEH item provides the most drawback improvement if it moves to the prior line.
//   The BET item provides the most drawback improvement if it moves to the next line.
//   Best distribution evaluation:
//     Phase 1: evaluate distribution for an available width equal to the MeasureOverride's availableWidth.
//     Phase 2: evaluate distribution for an available width equal to the resulting average desired line width of Phase 1.
//     Phase 3: evaluate distributions with an available width within 30% of the average desired line width using the SH and ST items of prior distributions. Pick the distribution with the lowest drawback.
//     Phase 4: keeping the available width from Phase 3, fine tune that distribution by moving BEH / BET items into their neighboring lines using internal locks.
//              Whichever item among BEH & BET is present and provides the largest drawback improvement is moved.
// Items scaling:
//   Once the best items distribution is picked, each item is scaled up or down (or unscaled) in order to accommodate the final available width and LinedFlowLayout.ItemsStretch setting.
//   When a desired line width is smaller than the available width and ItemsStretch is Fill, the items are uniformly scaled up and cropped to completly fill the line.
//   When a desired line width is greater than the available width, the items are uniformly scaled down and cropped to just fill the line.
//   All items are measured based on the scaled available widths.
// Complete layouts:
//   A complete re-layout is performed when:
//   - the datasource changed,
//   - LineHeight, ItemsSpacing or LineSpacing changed,
//   - the effective average items per line changed,
//   - a frozen item gets a new desired width.
//   For a re-layout, an items distribution is performed for all realized lines, all at once.
// Partial layouts:
//   When scrolling, items distributions are performed for lines in the updated C & D zones.
int LinedFlowLayout::MeasureConstrainedLinesRegularPath(
    winrt::VirtualizingLayoutContext const& context,
    ItemsInfo const& itemsInfo,
    float availableWidth,
    double actualLineHeight)
{
    const double lineSpacing = LineSpacing();

    MUX_ASSERT(actualLineHeight > 0.0);

    const int newRecommendedAnchorIndex = context.RecommendedAnchorIndex();
    const double scrollViewport = context.VisibleRect().Height;

    MUX_ASSERT(m_isVirtualizingContext == (scrollViewport != std::numeric_limits<double>::infinity()));

    int lineCount = 0;
    int lineIndexApproximationCache = 0;
    bool isDisconnectedAnchorVirtualized = false;

    do
    {        
        float nearRealizationRect = GetRoundedFloat(context.RealizationRect().Y);
        float farRealizationRect = nearRealizationRect + context.RealizationRect().Height;
        float realizationRectHeight = 0.0f;
        double clampedNearRealizationRect = 0;
        double scrollOffset = 0;
        
        if (m_isVirtualizingContext)
        {
            // It is assumed that the ItemsInfoRequested event will produce sizing information for lines around the realized ones when ItemsRepeater.VerticalCacheSize is smaller than 4. Only 3 viewports are forcefully realized.
            // In the case ItemsInfoRequested can not provide the requested info, it is advised to bump up the ItemsRepeater.VerticalCacheSize to 4 from the default 2.
            const float realizationRectHeightDeficit = GetRealizationRectHeightDeficit(context, actualLineHeight, lineSpacing) / 2.0f / m_measureCountdown + static_cast<float>(lineIndexApproximationCache * (actualLineHeight + lineSpacing));
            const float inflatedNearRealizationRect = nearRealizationRect - realizationRectHeightDeficit;
            const float inflatedFarRealizationRect = farRealizationRect + realizationRectHeightDeficit;
            bool realizationRectCenteredAroundAnchor = false;

            if (m_anchorIndex != -1 || newRecommendedAnchorIndex != -1)
            {
                const int anchorLineIndex = GetLineIndex(newRecommendedAnchorIndex == -1 ? m_anchorIndex : newRecommendedAnchorIndex, false /*usesFastPathLayout*/);
                const float anchorLineNearEdge = static_cast<float>(anchorLineIndex * (actualLineHeight + lineSpacing));

                // Check if the normal inflated realization rect overlaps the anticipated anchor line, with 1 line of margin of error in the line index approximation.
                if (anchorLineNearEdge - 2 * actualLineHeight - lineSpacing <= inflatedNearRealizationRect || anchorLineNearEdge + actualLineHeight + lineSpacing >= inflatedFarRealizationRect)
                {
                    // The anchor index would not be realized if the normal path (realizationRectCenteredAroundAnchor == false) were followed.
                    // A non-inflated realization rect is centered around the middle of the anticipated anchor line.
                    const float anchorLineMiddle = static_cast<float>(anchorLineNearEdge + actualLineHeight / 2.0);

                    realizationRectHeight = farRealizationRect - nearRealizationRect;
                    nearRealizationRect = GetRoundedFloat(anchorLineMiddle - realizationRectHeight / 2.0f);
                    farRealizationRect = nearRealizationRect + realizationRectHeight;
                    scrollOffset = std::max(0.0, anchorLineMiddle - scrollViewport / 2.0);
                    realizationRectCenteredAroundAnchor = true;

                    // Same comments and treatment as in LinedFlowLayout::EnsureItemRangeFastPath.
                    // Sometimes during a bring-into-view operation the RecommendedAnchorIndex momentarily switches from the target index N to -1 and then back to the target index N.
                    // To avoid momentarily setting m_anchorIndex to -1, its value is preserved c_maxAnchorIndexRetentionCount times, based purely on experimentations.
                    // Without this momentary retention the bring-into-view operation lands on incorrect offsets. If m_anchorIndex were to be preserved without a countdown, large offset changes
                    // through ScrollView.ScrollTo calls would not complete.
                    // TODO: Bug 41896418 - Investigate why the occurrences of -1 sometimes come up - even when the ScrollView anchoring is turned on.
                    //                      Does such a countdown need to be used? What is a reliable countdown starting value?
                    if (newRecommendedAnchorIndex == -1)
                    {
                        MUX_ASSERT(m_anchorIndexRetentionCountdown >= 1 && m_anchorIndexRetentionCountdown <= c_maxAnchorIndexRetentionCount);

                        m_anchorIndexRetentionCountdown--;

                        if (m_anchorIndexRetentionCountdown == 0)
                        {
                            m_anchorIndex = -1;
                        }
                    }
                    else
                    {
                        m_anchorIndexRetentionCountdown = c_maxAnchorIndexRetentionCount;
                        m_anchorIndex = newRecommendedAnchorIndex;
                    }

                    // Layout is invalidated to ensure that m_anchorIndexRetentionCountdown is decremented from c_maxAnchorIndexRetentionCount all the way to 0 (a few lines above).
                    // Otherwise m_anchorIndex could be stuck to a value that prevents the realization window from moving to the actual scroller's offset.
                    InvalidateMeasureAsync();

                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_DBL_INT, METH_NAME, this, scrollOffset, m_anchorIndex);
                }
            }

            if (!realizationRectCenteredAroundAnchor)
            {
                if (m_anchorIndex != -1)
                {
                    m_anchorIndex = -1;
                }

                nearRealizationRect = inflatedNearRealizationRect;
                farRealizationRect = inflatedFarRealizationRect;
                realizationRectHeight = farRealizationRect - nearRealizationRect;
                scrollOffset = context.VisibleRect().Y;
            }

            clampedNearRealizationRect = std::max(0.0, static_cast<double>(nearRealizationRect));
            m_unrealizedNearLineCount = static_cast<int>(clampedNearRealizationRect / (actualLineHeight + lineSpacing));
        }
        else
        {
            // All lines are realized in non-virtualizing mode.
            m_unrealizedNearLineCount = 0;

            MUX_ASSERT(context.VisibleRect().Y == 0.0f);
        }
    
        if (m_averageItemsPerLine.second == 0.0)
        {
            MUX_ASSERT(m_averageItemsPerLine.first == 0.0);

            SetAverageItemsPerLine(GetAverageItemsPerLine(availableWidth));
        }

        std::set<double> averageItemsPerLineProcessed;
        bool forceRelayout = false;

        if (m_forceRelayout)
        {
            forceRelayout = true;
            m_forceRelayout = false;
        }
        else if (m_previousAvailableWidth != availableWidth)
        {
            // Perform a re-layout for example when the width of the owning control changed.
            forceRelayout = true;
        }

        do
        {
            averageItemsPerLineProcessed.insert(m_averageItemsPerLine.second);

            std::pair<double, double> newAverageItemsPerLine{ m_averageItemsPerLine.first, 0.0 };

            lineCount = MeasureConstrainedLines(
                context,
                averageItemsPerLineProcessed,
                itemsInfo,
                forceRelayout,
                availableWidth,
                nearRealizationRect,
                farRealizationRect,
                scrollViewport,
                scrollOffset,
                actualLineHeight,
                &newAverageItemsPerLine);

            if (lineCount == -1)
            {
                // This indicates the fast path can be engaged as the ItemsInfoRequested event handler provided ratios for the entire collection.
                return -1;
            }

            if (newAverageItemsPerLine.second == m_averageItemsPerLine.second)
            {
                // This branch is guaranteed to be hit and end the loop because averageItemsPerLineProcessed is never populated twice with the same value. 
                forceRelayout = false;
            }
            else
            {
                // The method MeasureConstrainedLines is not expected to request a new average items per line that was already processed in this layout pass.
                MUX_ASSERT(averageItemsPerLineProcessed.find(newAverageItemsPerLine.second) == averageItemsPerLineProcessed.end());

                const std::pair<double, double> oldAverageItemsPerLine = m_averageItemsPerLine;

                SetAverageItemsPerLine(newAverageItemsPerLine);

                NotifyLinedFlowLayoutInvalidatedDbg(winrt::LinedFlowLayoutInvalidationTrigger::SnappedAverageItemsPerLineChange);

                if (m_isVirtualizingContext)
                {
                    // Adjust the realization window, scroll offset and number of unrealized lines before the realized ones according to the new
                    // average items per line. The new scroll offset is an approximate value based on the old offset & old and new average items
                    // per line. Scroll anchoring through VerticalAnchorRatio will potentially correct that approximation.

                    lineCount = GetLineCount(m_averageItemsPerLine.second);
                    MUX_ASSERT(lineCount != 0);

                    const double linesSpacing = (lineCount - 1) * lineSpacing;
                    const double scrollExtent = lineCount * actualLineHeight + linesSpacing;
                    const double maxScrollOffset = std::max(0.0, scrollExtent - scrollViewport);
                    const double newScrollOffset = std::min(maxScrollOffset, GetRoundedDouble(scrollOffset * oldAverageItemsPerLine.second / newAverageItemsPerLine.second));

                    nearRealizationRect = static_cast<float>(GetRoundedDouble(nearRealizationRect + newScrollOffset - scrollOffset));
                    farRealizationRect = nearRealizationRect + realizationRectHeight;
                    clampedNearRealizationRect = std::max(0.0, static_cast<double>(nearRealizationRect));
                    m_unrealizedNearLineCount = static_cast<int>(clampedNearRealizationRect / (actualLineHeight + lineSpacing));
                    scrollOffset = newScrollOffset;
                }

                forceRelayout = true;
            }
        }
        while (forceRelayout);

        isDisconnectedAnchorVirtualized = newRecommendedAnchorIndex != -1 && !m_elementManager.IsDataIndexRealized(newRecommendedAnchorIndex);

        if (isDisconnectedAnchorVirtualized)
        {
            // This rare case occurs when GetLineIndex for the anchor index returned an approximation off enough that the anchor element was actually not realized in the iteration above.
            // Increasing the cache by one line so that the next iteration has a much greater probability of realizing that anchor.
            lineIndexApproximationCache++;
        }
    }
    while (isDisconnectedAnchorVirtualized);

    // Clear the arrays collected from the ItemsInfoRequested event handler.
    ResetItemsInfoForFastPath();

#ifdef DBG
    if (m_isVirtualizingContext)
    {
        const int newRecommendedAnchorIndexDbg = context.RecommendedAnchorIndex();

        MUX_ASSERT(newRecommendedAnchorIndexDbg == -1 || m_elementManager.IsDataIndexRealized(newRecommendedAnchorIndexDbg));

        const int anchorLineIndexDbg = newRecommendedAnchorIndexDbg == -1 ? -1 : GetLineIndex(newRecommendedAnchorIndexDbg, false /*usesFastPathLayout*/);

        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"realized anchor", anchorLineIndexDbg, newRecommendedAnchorIndexDbg);
    }
#endif

    return lineCount;
}

// Returns the final line count when the regular path is kept to the end,
// or -1 when the ItemsInfoRequested provided ratios for the entire data source,
// indicating that the fast path must be engaged.
int LinedFlowLayout::MeasureConstrainedLines(
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
    std::pair<double, double>* newAverageItemsPerLine)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, forceRelayout);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_unrealizedNearLineCount", m_unrealizedNearLineCount);

    MUX_ASSERT(!UsesFastPathLayout());
    MUX_ASSERT(actualLineHeight > 0.0);

    *newAverageItemsPerLine = m_averageItemsPerLine;

    const int lineCount = GetLineCount(m_averageItemsPerLine.second);

    if (m_unrealizedNearLineCount >= lineCount)
    {
        ResetLinesInfo();
        ResetSizedLines();
        return lineCount;
    }

    MUX_ASSERT(m_unrealizedNearLineCount < lineCount);
    MUX_ASSERT(m_isVirtualizingContext == IsVirtualizingContext(context));

    const double lineSpacing = LineSpacing();
    const double linesSpacing = lineCount == 0 ? 0.0 : (lineCount - 1) * lineSpacing;
    const double linesHeight = lineCount * actualLineHeight + linesSpacing;
    const double clampedFarRealizationRect = std::max(0.0, std::min(linesHeight, static_cast<double>(farRealizationRect)));
    const int unrealizedFarLineCount = static_cast<int>((linesHeight - clampedFarRealizationRect) / (actualLineHeight + lineSpacing));
    const int realizedLineCount = lineCount - m_unrealizedNearLineCount - unrealizedFarLineCount;

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"realizedLineCount", realizedLineCount);

    MUX_ASSERT(realizedLineCount <= lineCount);

    if (realizedLineCount == 0)
    {
        ResetLinesInfo();
        ResetSizedLines();
        return lineCount;
    }

    MUX_ASSERT(realizedLineCount > 0);

    // Determine the rect size which includes the unrealized sized and realized items.
    const float sizedItemsRectHeight = GetSizedItemsRectHeight(context, actualLineHeight, lineSpacing);
    const float realizationRectHeightDeficit = std::max(0.0f, sizedItemsRectHeight - farRealizationRect + nearRealizationRect);
    const float nearSizedItemsRect = nearRealizationRect - realizationRectHeightDeficit / 2.0f;
    const float farSizedItemsRect = farRealizationRect + realizationRectHeightDeficit / 2.0f;
    const double clampedNearSizedItemsRect = std::max(0.0, static_cast<double>(nearSizedItemsRect));

    m_unsizedNearLineCount = static_cast<int>(clampedNearSizedItemsRect / (actualLineHeight + lineSpacing));

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_unsizedNearLineCount", m_unsizedNearLineCount);

    MUX_ASSERT(m_unsizedNearLineCount <= m_unrealizedNearLineCount);

    const double clampedFarSizedItemsRect = std::max(0.0, std::min(linesHeight, static_cast<double>(farSizedItemsRect)));
    const int unsizedFarLineCount = static_cast<int>((linesHeight - clampedFarSizedItemsRect) / (actualLineHeight + lineSpacing));
    const int sizedLineCount = lineCount - m_unsizedNearLineCount - unsizedFarLineCount;

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"sizedLineCount", sizedLineCount);

    MUX_ASSERT(sizedLineCount <= lineCount);
    MUX_ASSERT(sizedLineCount >= realizedLineCount);

    const int oldFirstRealizedDataIndex = m_elementManager.GetFirstRealizedDataIndex(); // -1 and unused in non-virtualizing mode.
    const int oldLastRealizedDataIndex = oldFirstRealizedDataIndex == -1 ? -1 : oldFirstRealizedDataIndex + m_elementManager.GetRealizedElementCount() - 1;
    const int oldFirstSizedItemIndex = m_firstSizedItemIndex;
    const int oldLastSizedItemIndex = m_lastSizedItemIndex;
    const int oldFirstSizedLineIndex = m_firstSizedLineIndex;
    const int oldLastSizedLineIndex = m_lastSizedLineIndex;

    m_firstSizedLineIndex = m_unsizedNearLineCount;
    m_lastSizedLineIndex = m_unsizedNearLineCount + sizedLineCount - 1;

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_firstSizedLineIndex", m_firstSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_lastSizedLineIndex", m_lastSizedLineIndex);

    std::vector<int> oldLineItemCounts;
    int oldFirstItemsInfoIndex = m_itemsInfoFirstIndex;
    int oldLastItemsInfoIndex = m_itemsInfoFirstIndex == -1 ? -1 : m_itemsInfoFirstIndex + static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size()) - 1;

    if (!forceRelayout)
    {
        oldLineItemCounts = m_lineItemCounts;
    }

    std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> oldElementAvailableWidths;
    std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> oldElementDesiredWidths;

    if (m_elementAvailableWidths)
    {
        oldElementAvailableWidths = m_elementAvailableWidths;
        m_elementAvailableWidths = nullptr;
    }
    EnsureElementAvailableWidths();

    if (m_elementDesiredWidths)
    {
        oldElementDesiredWidths = m_elementDesiredWidths;
        m_elementDesiredWidths = nullptr;
    }
    EnsureElementDesiredWidths();
    
    int firstStillSizedLineIndex;
    int lastStillSizedLineIndex;
    int firstStillSizedItemIndex;
    int lastStillSizedItemIndex;
    int firstStillRealizedItemIndex{ -1 };
    int lastStillRealizedItemIndex{ -1 };

    InitializeForRelayout(
        sizedLineCount,
        firstStillSizedLineIndex,
        lastStillSizedLineIndex,
        firstStillSizedItemIndex,
        lastStillSizedItemIndex);

    if (!forceRelayout && !oldLineItemCounts.empty())
    {
        if (m_firstSizedLineIndex >= oldFirstSizedLineIndex && m_firstSizedLineIndex <= oldLastSizedLineIndex)
        {
            firstStillSizedLineIndex = m_firstSizedLineIndex;
            lastStillSizedLineIndex = std::min(oldLastSizedLineIndex, m_lastSizedLineIndex);
        }
        if (m_lastSizedLineIndex >= oldFirstSizedLineIndex && m_lastSizedLineIndex <= oldLastSizedLineIndex)
        {
            lastStillSizedLineIndex = m_lastSizedLineIndex;
            firstStillSizedLineIndex = std::max(oldFirstSizedLineIndex, m_firstSizedLineIndex);
        }
        if (m_firstSizedLineIndex <= oldFirstSizedLineIndex && m_lastSizedLineIndex >= oldLastSizedLineIndex)
        {
            firstStillSizedLineIndex = oldFirstSizedLineIndex;
            lastStillSizedLineIndex = oldLastSizedLineIndex;
        }

        MUX_ASSERT(!(firstStillSizedLineIndex == -1 && lastStillSizedLineIndex != -1));
        MUX_ASSERT(!(firstStillSizedLineIndex != -1 && lastStillSizedLineIndex == -1));

        if (firstStillSizedLineIndex != -1)
        {
            MUX_ASSERT(lastStillSizedLineIndex != -1);

            // When firstStillSizedLineIndex != -1 (and necessarily lastStillSizedLineIndex != -1),
            // lines oldLineItemCounts[Max(oldFirstSizedLineIndex, m_firstSizedLineIndex) - oldFirstSizedLineIndex]
            // to oldLineItemCounts[Min(oldLastSizedLineIndex, m_lastSizedLineIndex) - oldFirstSizedLineIndex] would become frozen without adjustment.
            const int oldFirstLineVectorIndex = std::max(oldFirstSizedLineIndex, m_firstSizedLineIndex) - oldFirstSizedLineIndex;
            const int oldLastLineVectorIndex = std::min(oldLastSizedLineIndex, m_lastSizedLineIndex) - oldFirstSizedLineIndex;

            MUX_ASSERT(oldFirstLineVectorIndex >= 0);
            MUX_ASSERT(oldFirstLineVectorIndex < static_cast<int>(oldLineItemCounts.size()));
            MUX_ASSERT(oldLastLineVectorIndex >= 0);
            MUX_ASSERT(oldLastLineVectorIndex < static_cast<int>(oldLineItemCounts.size()));

            firstStillSizedItemIndex = oldFirstSizedItemIndex;

            for (int lineVectorIndex = 0; lineVectorIndex < oldFirstLineVectorIndex; lineVectorIndex++)
            {
                firstStillSizedItemIndex += oldLineItemCounts[lineVectorIndex];
            }

            lastStillSizedItemIndex = firstStillSizedItemIndex;

            for (int lineVectorIndex = oldFirstLineVectorIndex; lineVectorIndex <= oldLastLineVectorIndex; lineVectorIndex++)
            {
                m_lineItemCounts[firstStillSizedLineIndex - m_firstSizedLineIndex + lineVectorIndex - oldFirstLineVectorIndex] =
                    oldLineItemCounts[lineVectorIndex];

                lastStillSizedItemIndex += oldLineItemCounts[lineVectorIndex];
            }

            lastStillSizedItemIndex--;

            MUX_ASSERT(firstStillSizedItemIndex <= lastStillSizedItemIndex);
        }
    }

    int sizedItemCount = 0;

    if (m_isVirtualizingContext)
    {
        // Figure out how many items fill the sized rect.

        MUX_ASSERT(m_unsizedNearLineCount + sizedLineCount <= lineCount);

        if (firstStillSizedLineIndex != -1 && firstStillSizedLineIndex == m_firstSizedLineIndex && lastStillSizedLineIndex == m_lastSizedLineIndex)
        {
            sizedItemCount = LineItemsCountTotal(0);
        }
        else if (firstStillSizedLineIndex != -1 && 0 == m_firstSizedLineIndex && lastStillSizedLineIndex == m_lastSizedLineIndex)
        {
            sizedItemCount = LineItemsCountTotal(0) + firstStillSizedItemIndex;
        }
        else if (m_unsizedNearLineCount + sizedLineCount < lineCount)
        {
            sizedItemCount = static_cast<int>(std::round(sizedLineCount * m_averageItemsPerLine.second));

            if (firstStillSizedLineIndex != -1 && 0 == m_firstSizedLineIndex)
            {
                MUX_ASSERT(oldFirstSizedItemIndex != -1);
                MUX_ASSERT(oldLastSizedItemIndex != -1);

                const int oldSizedItemCount = oldLastSizedItemIndex - oldFirstSizedItemIndex + 1;

                if (sizedItemCount < oldSizedItemCount)
                {
                    sizedItemCount = oldSizedItemCount;
                }
            }

            sizedItemCount = std::min(sizedItemCount, m_itemCount);
        }
    }

    int unsizedNearItemCount = 0;

    if (firstStillSizedLineIndex != -1)
    {
        MUX_ASSERT(m_isVirtualizingContext);

        if (m_firstSizedLineIndex == 0)
        {
            MUX_ASSERT(m_unsizedNearLineCount == 0);

            unsizedNearItemCount = 0;
        }
        else
        {
            unsizedNearItemCount = firstStillSizedItemIndex;

            if (oldFirstSizedLineIndex > m_firstSizedLineIndex)
            {
                if (sizedItemCount == 0)
                {
                    MUX_ASSERT(m_unsizedNearLineCount + sizedLineCount == lineCount);

                    unsizedNearItemCount -=
                        std::min(unsizedNearItemCount, static_cast<int>(std::round((oldFirstSizedLineIndex - m_firstSizedLineIndex) * m_averageItemsPerLine.second)));
                }
                else
                {
                    const int lineItemsCountTotal = LineItemsCountTotal(0);
                    const int newSizedItemCount = sizedItemCount - lineItemsCountTotal;
                    const int newNearSizedLineCount = oldFirstSizedLineIndex - m_firstSizedLineIndex;
                    const int newFarSizedLineCount = std::max(0, m_lastSizedLineIndex - oldLastSizedLineIndex);

                    // newSizedItemCount < newNearSizedLineCount + newFarSizedLineCount occurs when the average items per line of the still sized lines is
                    // greater than m_averageItemsPerLine & there are not enough items left to distribute at least one item per new sized line. Do a re-layout in those cases too.
                    if (unsizedNearItemCount < m_unsizedNearLineCount + newSizedItemCount ||
                        newSizedItemCount < newNearSizedLineCount + newFarSizedLineCount)
                    {
                        // Performing complete re-layout.
                        InitializeForRelayout(
                            sizedLineCount,
                            firstStillSizedLineIndex,
                            lastStillSizedLineIndex,
                            firstStillSizedItemIndex,
                            lastStillSizedItemIndex);
                        firstStillRealizedItemIndex = -1;
                        lastStillRealizedItemIndex = -1;
                        oldLineItemCounts.clear();
                        unsizedNearItemCount = static_cast<int>(std::round(m_unsizedNearLineCount * m_averageItemsPerLine.second));
                    }
                    else
                    {
                        // There are at least as many new sized items as there are new sized lines.
                        MUX_ASSERT(newSizedItemCount >= newNearSizedLineCount + newFarSizedLineCount);

                        // New near and far sized items allocations are proportional to the new line counts.
                        const int newNearSizedItemCount = std::max(newNearSizedLineCount, newNearSizedLineCount * newSizedItemCount / (newNearSizedLineCount + newFarSizedLineCount));

                        unsizedNearItemCount -= newNearSizedItemCount;
                    }
                }
            }
            else if (unsizedNearItemCount + sizedItemCount + unsizedFarLineCount > m_itemCount)
            {
                sizedItemCount = m_itemCount - unsizedNearItemCount - unsizedFarLineCount;
            }
        }
    }
    else if (m_isVirtualizingContext)
    {
        unsizedNearItemCount = static_cast<int>(std::round(m_unsizedNearLineCount * m_averageItemsPerLine.second));
    }

    MUX_ASSERT(unsizedNearItemCount < m_itemCount);
    MUX_ASSERT(unsizedNearItemCount >= m_unsizedNearLineCount);
    MUX_ASSERT((m_unsizedNearLineCount == 0 && unsizedNearItemCount == 0) || (m_unsizedNearLineCount > 0 && unsizedNearItemCount > 0));

    if (m_unsizedNearLineCount + sizedLineCount == lineCount)
    {
        sizedItemCount = m_itemCount - unsizedNearItemCount;
    }

    MUX_ASSERT(sizedItemCount > 0);
    MUX_ASSERT(sizedItemCount >= sizedLineCount);
    MUX_ASSERT(unsizedNearItemCount + sizedItemCount <= m_itemCount);

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"sizedItemCount", sizedItemCount);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"unsizedNearItemCount", unsizedNearItemCount);

    m_firstSizedItemIndex = unsizedNearItemCount;
    m_lastSizedItemIndex = m_firstSizedItemIndex + sizedItemCount - 1;

    MUX_ASSERT((firstStillSizedItemIndex == -1) == (lastStillSizedItemIndex == -1));
    MUX_ASSERT(m_firstSizedItemIndex <= firstStillSizedItemIndex || firstStillSizedItemIndex == -1);
    MUX_ASSERT(m_lastSizedItemIndex >= lastStillSizedItemIndex || lastStillSizedItemIndex == -1);

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_firstSizedItemIndex", m_firstSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_lastSizedItemIndex", m_lastSizedItemIndex);

    EnsureAndResizeItemAspectRatios(
        scrollViewport,
        actualLineHeight,
        lineSpacing);

    if (m_isVirtualizingContext && UpdateItemsInfo(itemsInfo, oldFirstItemsInfoIndex, oldLastItemsInfoIndex))
    {
        // The fast path was enabled. Reset the fields specific to the regular path, and return -1 to indicate that
        // the fast path must be engaged as ratios were provided for the entire data source.
        ExitRegularPath();
        return -1;
    }

    int unrealizedNearItemCount{};
    int realizedItemCount{};

    if (!m_isVirtualizingContext || static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size()) < sizedItemCount)
    {
        // The ItemsInfoRequested handler did not provide sizing information for all sized items, or the context is not virtualizing.
        // Fall back to realizing all sized items instead.
        unrealizedNearItemCount = unsizedNearItemCount;
        realizedItemCount = sizedItemCount;

        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"unrealizedNearItemCount fallback", unrealizedNearItemCount);
        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"realizedItemCount fallback", realizedItemCount);
        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"realized item index range fallback", unrealizedNearItemCount, unrealizedNearItemCount + realizedItemCount - 1);
    }
    else
    {
        MUX_ASSERT(m_itemsInfoFirstIndex != -1);

        GetRealizedItemsFromSizedItems(
            realizedLineCount,
            lineCount,
            &unrealizedNearItemCount,
            &realizedItemCount);
    }

    MUX_ASSERT(sizedItemCount >= realizedItemCount);
    MUX_ASSERT(firstStillRealizedItemIndex == -1);
    MUX_ASSERT(lastStillRealizedItemIndex == -1);

    if (firstStillSizedLineIndex != -1)
    {
        MUX_ASSERT(lastStillSizedLineIndex != -1);

        const int newFirstRealizedDataIndex = unrealizedNearItemCount;
        const int newLastRealizedDataIndex = newFirstRealizedDataIndex + realizedItemCount - 1;

        MUX_ASSERT(m_firstSizedItemIndex <= newFirstRealizedDataIndex);
        MUX_ASSERT(m_lastSizedItemIndex >= newLastRealizedDataIndex);

        if (newFirstRealizedDataIndex >= oldFirstRealizedDataIndex && newFirstRealizedDataIndex <= oldLastRealizedDataIndex)
        {
            firstStillRealizedItemIndex = newFirstRealizedDataIndex;
            lastStillRealizedItemIndex = std::min(oldLastRealizedDataIndex, newLastRealizedDataIndex);
        }
        else if (newLastRealizedDataIndex >= oldFirstRealizedDataIndex && newLastRealizedDataIndex <= oldLastRealizedDataIndex)
        {
            lastStillRealizedItemIndex = newLastRealizedDataIndex;
            firstStillRealizedItemIndex = std::max(oldFirstRealizedDataIndex, newFirstRealizedDataIndex);
        }
    }

    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstStillSizedLineIndex", firstStillSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastStillSizedLineIndex", lastStillSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstStillSizedItemIndex", firstStillSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastStillSizedItemIndex", lastStillSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstStillRealizedItemIndex", firstStillRealizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastStillRealizedItemIndex", lastStillRealizedItemIndex);

    // Discard the first realized items fallen off the realization window.
    if (m_elementManager.GetFirstRealizedDataIndex() != -1 && unrealizedNearItemCount > m_elementManager.GetFirstRealizedDataIndex())
    {
        MUX_ASSERT(m_isVirtualizingContext);

        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"discarded realized head range",
            m_elementManager.GetFirstRealizedDataIndex(), unrealizedNearItemCount - 1);

        m_elementManager.DiscardElementsOutsideWindow(
            false /*forward*/,
            std::min(unrealizedNearItemCount, m_elementManager.GetFirstRealizedDataIndex() + m_elementManager.GetRealizedElementCount()) - 1 /*startIndex*/);
    }

    MUX_ASSERT(unrealizedNearItemCount <= m_elementManager.GetFirstRealizedDataIndex() || -1 == m_elementManager.GetFirstRealizedDataIndex());

    if (m_elementManager.GetFirstRealizedDataIndex() != -1 && unrealizedNearItemCount < m_elementManager.GetFirstRealizedDataIndex())
    {
        MUX_ASSERT(oldFirstRealizedDataIndex > 0);

        // Ensure and measure the realized items before the old first realized item.
        EnsureAndMeasureItemRange(
            context,
            availableWidth,
            actualLineHeight,
            false /*forward*/,
            std::min(oldFirstRealizedDataIndex - 1, unrealizedNearItemCount + realizedItemCount - 1) /*beginRealizedItemIndex*/,
            unrealizedNearItemCount /*endRealizedItemIndex*/);

        // Ensure and measure the realized & unfrozen items after the old first realized item.
        MUX_ASSERT(unrealizedNearItemCount == m_elementManager.GetFirstRealizedDataIndex());

        if (firstStillRealizedItemIndex != -1)
        {
            if (firstStillRealizedItemIndex > 0 && oldFirstRealizedDataIndex <= firstStillRealizedItemIndex - 1)
            {
                EnsureAndMeasureItemRange(
                    context,
                    availableWidth,
                    actualLineHeight,
                    true /*forward*/,
                    oldFirstRealizedDataIndex /*beginRealizedItemIndex*/,
                    firstStillRealizedItemIndex - 1 /*endRealizedItemIndex*/);
            }

            if (lastStillRealizedItemIndex + 1 <= unrealizedNearItemCount + realizedItemCount - 1)
            {
                EnsureAndMeasureItemRange(
                    context,
                    availableWidth,
                    actualLineHeight,
                    true /*forward*/,
                    lastStillRealizedItemIndex + 1 /*beginRealizedItemIndex*/,
                    unrealizedNearItemCount + realizedItemCount - 1 /*endRealizedItemIndex*/);
            }
        }
        else if (unrealizedNearItemCount + realizedItemCount - 1 >= oldFirstRealizedDataIndex)
        {
            EnsureAndMeasureItemRange(
                context,
                availableWidth,
                actualLineHeight,
                true /*forward*/,
                oldFirstRealizedDataIndex /*beginRealizedItemIndex*/,
                unrealizedNearItemCount + realizedItemCount - 1 /*endRealizedItemIndex*/);
        }
    }
    else
    {
        // Ensure and measure the realized & unfrozen items.
        MUX_ASSERT(unrealizedNearItemCount == m_elementManager.GetFirstRealizedDataIndex() || m_elementManager.GetFirstRealizedDataIndex() == -1);

        if (firstStillRealizedItemIndex != -1)
        {
            if (firstStillRealizedItemIndex > 0 && unrealizedNearItemCount <= firstStillRealizedItemIndex - 1)
            {
                EnsureAndMeasureItemRange(
                    context,
                    availableWidth,
                    actualLineHeight,
                    true /*forward*/,
                    unrealizedNearItemCount /*beginRealizedItemIndex*/,
                    firstStillRealizedItemIndex - 1 /*endRealizedItemIndex*/);
            }

            if (lastStillRealizedItemIndex + 1 <= unrealizedNearItemCount + realizedItemCount - 1)
            {
                EnsureAndMeasureItemRange(
                    context,
                    availableWidth,
                    actualLineHeight,
                    true /*forward*/,
                    lastStillRealizedItemIndex + 1 /*beginRealizedItemIndex*/,
                    unrealizedNearItemCount + realizedItemCount - 1 /*endRealizedItemIndex*/);
            }
        }
        else
        {
            EnsureAndMeasureItemRange(
                context,
                availableWidth,
                actualLineHeight,
                true /*forward*/,
                unrealizedNearItemCount /*beginRealizedItemIndex*/,
                unrealizedNearItemCount + realizedItemCount - 1 /*endRealizedItemIndex*/);
        }
    }

    // Discard the last realized items fallen off the realization window.
    if (m_isVirtualizingContext && unrealizedFarLineCount > 0 && m_elementManager.GetRealizedElementCount() > realizedItemCount)
    {
        LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"discarded realized tail range",
            unrealizedNearItemCount + realizedItemCount, m_elementManager.GetFirstRealizedDataIndex() + m_elementManager.GetRealizedElementCount() - 1);

        m_elementManager.DiscardElementsOutsideWindow(
            true /*forward*/,
            unrealizedNearItemCount + realizedItemCount /*startIndex*/);
    }

    MUX_ASSERT(unrealizedNearItemCount >= 0);
    MUX_ASSERT(m_isVirtualizingContext || unrealizedNearItemCount == 0);
    MUX_ASSERT(!m_isVirtualizingContext || m_elementManager.GetFirstRealizedDataIndex() == unrealizedNearItemCount);
    MUX_ASSERT(m_elementManager.GetRealizedElementCount() == realizedItemCount);

    if (const auto firstElement = m_elementManager.GetRealizedElement(unrealizedNearItemCount))
    {
        UpdateRoundingScaleFactor(firstElement);
    }
    
    std::pair<double, double> averageItemsPerLine = GetAverageItemsPerLine(availableWidth);

    // Only request a new average items per line value when it has never been attempted in this layout pass.
    // Otherwise carry on with the current m_averageItemsPerLine value.
    if (averageItemsPerLine.second == m_averageItemsPerLine.second)
    {
        // When the snapped average-items-per-line value is unchanged, just save the raw value if it changed.
        if (averageItemsPerLine.first != m_averageItemsPerLine.first)
        {
            // The new raw value becomes the source of the snapped value and the reference point for detecting
            // deltas accross the median point (1.1^(N+1) - 1.1^N) / 2 greater than 0.1.
            SetAverageItemsPerLine(averageItemsPerLine);
        }
    }
    else if (averageItemsPerLineProcessed.find(averageItemsPerLine.second) == averageItemsPerLineProcessed.end())
    {
        *newAverageItemsPerLine = averageItemsPerLine;

        m_firstSizedLineIndex = -1;
        m_lastSizedLineIndex = -1;
        return lineCount;
    }

    MUX_ASSERT(!(firstStillSizedLineIndex != -1 && forceRelayout));
    MUX_ASSERT(unrealizedNearItemCount >= 0);
    MUX_ASSERT(m_isVirtualizingContext || unrealizedNearItemCount == 0);
    MUX_ASSERT(!m_isVirtualizingContext || m_elementManager.GetFirstRealizedDataIndex() == unrealizedNearItemCount);
    MUX_ASSERT(unsizedNearItemCount <= unrealizedNearItemCount);

    const bool itemHasNewDesiredWidth = ComputeFrozenItemsAndLayout(
        context,
        oldLineItemCounts,
        oldElementAvailableWidths,
        oldElementDesiredWidths,
        scrollViewport,
        scrollOffset,
        lineSpacing,
        actualLineHeight,
        availableWidth,
        nearSizedItemsRect,
        farSizedItemsRect,
        nearRealizationRect,
        farRealizationRect,
        lineCount,
        sizedLineCount,
        unsizedNearItemCount,
        sizedItemCount,
        firstStillSizedLineIndex,
        lastStillSizedLineIndex,
        firstStillSizedItemIndex,
        lastStillSizedItemIndex);

#ifdef DBG
    const int sizedItemCountDbg = LineItemsCountTotal(m_lastSizedItemIndex - m_firstSizedItemIndex + 1);

    VerifyLockedItemsDbg();
#endif

    if (itemHasNewDesiredWidth)
    {
        // When at least one item has a new desired width, the average items-per-line evaluation may change.
        averageItemsPerLine = GetAverageItemsPerLine(availableWidth);

        if (averageItemsPerLine.second != m_averageItemsPerLine.second &&
            averageItemsPerLineProcessed.find(averageItemsPerLine.second) == averageItemsPerLineProcessed.end())
        {
            // Computed average items-per-line is different and has not been processed yet.
            m_firstSizedLineIndex = -1;
            m_lastSizedLineIndex = -1;

            // Return the new average which will trigger a new layout.
            *newAverageItemsPerLine = averageItemsPerLine;
        }
    }

    return lineCount;
}

// Measures a range of realized items based on the final arrange widths stored in m_itemsInfoArrangeWidths.
void LinedFlowLayout::MeasureItemRange(
    double actualLineHeight,
    int beginRealizedItemIndex,
    int endRealizedItemIndex)
{
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginRealizedItemIndex", beginRealizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endRealizedItemIndex", endRealizedItemIndex);

    MUX_ASSERT(beginRealizedItemIndex >= 0);
    MUX_ASSERT(beginRealizedItemIndex <= endRealizedItemIndex);
    MUX_ASSERT(m_itemsInfoArrangeWidths.size() > 0);

    const int itemsInfoArrangeWidthsOffset = m_itemsInfoFirstIndex == -1 ? 0 : m_itemsInfoFirstIndex;

    for (int realizedItemIndex = beginRealizedItemIndex; realizedItemIndex <= endRealizedItemIndex; realizedItemIndex++)
    {
        MUX_ASSERT(m_elementManager.IsDataIndexRealized(realizedItemIndex));

        const auto element = m_elementManager.GetRealizedElement(realizedItemIndex /*dataIndex*/);

        // Making sure the element is not null for safety.
        if (element)
        {
            MUX_ASSERT(realizedItemIndex - itemsInfoArrangeWidthsOffset<static_cast<int>(m_itemsInfoArrangeWidths.size()));

            const float arrangeWidth = m_itemsInfoArrangeWidths[realizedItemIndex - itemsInfoArrangeWidthsOffset];

            if (arrangeWidth >= 0.0f)
            {
                element.Measure(winrt::Size{ arrangeWidth, static_cast<float>(actualLineHeight) });
            }

#ifdef DBG
            if (realizedItemIndex == m_logItemIndexDbg)
            {
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"measured with elementAvailableSize",
                    arrangeWidth,
                    actualLineHeight);
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"element.DesiredSize",
                    element.DesiredSize().Width,
                    element.DesiredSize().Height);
            }
#endif
        }
    }
}

// Measures a range of realized items based on previously computed available widths elementAvailableWidths.
// Re-populates m_elementAvailableWidths with the old & re-applied available widths.
void LinedFlowLayout::MeasureItemRangeRegularPath(
    std::shared_ptr<std::map<tracker_ref<winrt::UIElement>, float>> elementAvailableWidths,
    double actualLineHeight,
    int beginRealizedItemIndex,
    int endRealizedItemIndex)
{
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"with ElementAvailableSize", beginRealizedItemIndex, endRealizedItemIndex);

    MUX_ASSERT(m_itemsInfoFirstIndex == -1);
    MUX_ASSERT(beginRealizedItemIndex <= endRealizedItemIndex);
    MUX_ASSERT(elementAvailableWidths);

    for (int realizedItemIndex = beginRealizedItemIndex; realizedItemIndex <= endRealizedItemIndex; realizedItemIndex++)
    {
        MUX_ASSERT(m_elementManager.IsDataIndexRealized(realizedItemIndex));

        const auto element = m_elementManager.GetRealizedElement(realizedItemIndex /*dataIndex*/);

        // Making sure the element is not null for safety.
        if (element)
        {
            const auto elementTracker = tracker_ref<winrt::UIElement>(this, element);
            float elementAvailableWidth{ -1.0f };

            if (elementAvailableWidths->find(elementTracker) != elementAvailableWidths->end())
            {
                // The element has an existing recorded available width because it needs to be scaled up or down, to fill or fit the available line width respectively.
                elementAvailableWidth = elementAvailableWidths->at(elementTracker);

                MUX_ASSERT(m_elementAvailableWidths);
                MUX_ASSERT(m_elementAvailableWidths->find(elementTracker) == m_elementAvailableWidths->end());

                // Maintain that recording and measure the element again with it.
                m_elementAvailableWidths->insert(std::pair<tracker_ref<winrt::UIElement>, float>(elementTracker, elementAvailableWidth));
            }
            else if (const auto frameworkElement = element.try_as<winrt::FrameworkElement>())
            {
                // The element has no recorded available width because it is not scaled down or up, so only its desired width is recorded.
                const float minWidth = static_cast<float>(frameworkElement.MinWidth());

                if (element.DesiredSize().Width == minWidth)
                {
                    // Making sure the item is measured and stretched according to the MinWidth value (otherwise background-colored bands appear on the items' left/right edges).
                    elementAvailableWidth = minWidth;
                }
            }

            if (elementAvailableWidth != -1.0f)
            {
                element.Measure(winrt::Size{ elementAvailableWidth, static_cast<float>(actualLineHeight) });
            }
#ifdef DBG
            if (realizedItemIndex == m_logItemIndexDbg)
            {
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"measured with elementAvailableSize",
                    elementAvailableWidth,
                    actualLineHeight);
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"element.DesiredSize",
                    element.DesiredSize().Width,
                    element.DesiredSize().Height);
            }
#endif
        }
    }
}

// Items measurement when the non-scrolling dimension is unconstrained.
float LinedFlowLayout::MeasureUnconstrainedLine(
    winrt::VirtualizingLayoutContext const& context)
{
    MUX_ASSERT(m_itemCount == context.ItemCount());
    MUX_ASSERT(m_isVirtualizingContext == IsVirtualizingContext(context));
    MUX_ASSERT(!UsesArrangeWidthInfo());

    const float actualLineHeight = static_cast<float>(ActualLineHeight());
    const winrt::Size measureAvailableSize{ std::numeric_limits<float>::infinity(), actualLineHeight };
    float desiredWidth = 0.0f;
    bool isNewlyRealizedElementMeasuredWithMinWidth = false;

    for (int itemIndex = 0; itemIndex < m_itemCount; itemIndex++)
    {
        bool isElementNewlyRealized = false;

        if (m_isVirtualizingContext && !m_elementManager.IsDataIndexRealized(itemIndex))
        {
            EnsureElementRealized(true /*forward*/, itemIndex);

            isElementNewlyRealized = true;
        }

        if (auto element = m_elementManager.GetAt(itemIndex))
        {
            element.Measure(measureAvailableSize);

            if (const auto frameworkElement = element.try_as<winrt::FrameworkElement>())
            {
                const float minWidth = static_cast<float>(frameworkElement.MinWidth());

                if (element.DesiredSize().Width == minWidth)
                {
                    // Making sure the item is stretched according to the MinWidth value.
                    element.Measure(winrt::Size{ minWidth, actualLineHeight });

                    if (isElementNewlyRealized)
                    {
                        isNewlyRealizedElementMeasuredWithMinWidth = true;
                    }
                }
            }

            desiredWidth += element.DesiredSize().Width;
        }
    }

    if (isNewlyRealizedElementMeasuredWithMinWidth)
    {
        InvalidateMeasureTimerStart(0 /*tickCount*/);
    }

    SetAverageItemsPerLine(SnapAverageItemsPerLine(
        m_averageItemsPerLine.first /*oldAverageItemsPerLineRaw*/,
        m_itemCount /*newAverageItemsPerLineRaw*/));

    if (m_itemCount > 0)
    {
        desiredWidth += static_cast<float>((m_itemCount - 1) * MinItemSpacing());
    }

    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_DBL, METH_NAME, this, desiredWidth);

    return desiredWidth;
}

// Raises the LayoutsTestHooks LinedFlowLayoutInvalidated event for testing purposes.
void LinedFlowLayout::NotifyLinedFlowLayoutInvalidatedDbg(
    winrt::LinedFlowLayoutInvalidationTrigger const& invalidationTrigger)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, static_cast<int>(invalidationTrigger));

    com_ptr<LayoutsTestHooks> globalTestHooks = LayoutsTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        globalTestHooks->NotifyLinedFlowLayoutInvalidated(*this, invalidationTrigger);
    }
}

// Raises the LayoutsTestHooks LinedFlowLayoutItemLocked event for testing purposes.
void LinedFlowLayout::NotifyLinedFlowLayoutItemLockedDbg(
    int itemIndex,
    int lineIndex)
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, itemIndex, lineIndex);

    com_ptr<LayoutsTestHooks> globalTestHooks = LayoutsTestHooks::GetGlobalTestHooks();

    if (globalTestHooks)
    {
        globalTestHooks->NotifyLinedFlowLayoutItemLocked(*this, itemIndex, lineIndex);
    }
}

LinedFlowLayout::ItemsInfo LinedFlowLayout::RaiseItemsInfoRequested(
    int itemsRangeStartIndex,
    int itemsRangeRequestedLength)
{
    MUX_ASSERT(itemsRangeStartIndex >= 0);
    MUX_ASSERT(itemsRangeRequestedLength > 0);
    MUX_ASSERT(itemsRangeStartIndex + itemsRangeRequestedLength - 1 < m_itemCount);

    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemsRangeStartIndex", itemsRangeStartIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemsRangeRequestedLength", itemsRangeRequestedLength);

    if (m_itemsInfoRequestedEventSource)
    {
        auto itemsInfoRequestedEventArgs = winrt::make_self<LinedFlowLayoutItemsInfoRequestedEventArgs>(*this, itemsRangeStartIndex, itemsRangeRequestedLength);

        if (itemsInfoRequestedEventArgs != nullptr)
        {
            m_itemsInfoRequestedEventSource(*this, *itemsInfoRequestedEventArgs);

            // Disconnect the LinedFlowLayout from the LinedFlowLayoutItemsInfoRequestedEventArgs so that any subsequent calls to
            // SetDesiredAspectRatios/SetMinWiths/SetMaxWiths have no effect.
            itemsInfoRequestedEventArgs->ResetLinedFlowLayout();

            // The original LinedFlowLayoutItemsInfoRequestedEventArgs.ItemsRangeStartIndex value may have been overwritten by the handler to a smaller value.
            // This allows a handler to provide more information than requested, because it's available and performant to do so. It reduces subsequent needs to
            // raise the ItemsRangeStartIndex event again.
            return ItemsInfo{
                itemsInfoRequestedEventArgs->ItemsRangeStartIndex(),
                itemsInfoRequestedEventArgs->ItemsRangeLength(),
                static_cast<float>(itemsInfoRequestedEventArgs->MinWidth()),
                static_cast<float>(itemsInfoRequestedEventArgs->MaxWidth()) };
        }
    }

    return s_emptyItemsInfo;
}

// Returns the index of the first sized item, or -1 if there is none.
// This property is meant to be accessed by LinedFlowLayout consumers
// when a sizing information changed and LinedFlowLayout.InvalidateItemsInfo
// may need to be called. If the sizing information changed for an item outside
// the [RequestedRangeStartIndex, RequestedRangeStartIndex + RequestedRangeLength - 1]
// range, there is no need to call InvalidateItemsInfo. It would trigger a wasteful
// layout pass.
int LinedFlowLayout::RequestedRangeStartIndex()
{
    return UsesFastPathLayout() ? 0 : m_itemsInfoFirstIndex;
}

// Returns the number of sized items. Like RequestedRangeStartIndex, this property
// is meant to be used to determine whether LinedFlowLayout.InvalidateItemsInfo must be
// called after a sizing information change (i.e. a temporary aspect ratio needs to
// be replaced with its final value for example).
int LinedFlowLayout::RequestedRangeLength()
{
    return UsesFastPathLayout() ? m_itemCount : static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size());
}

// Based on info provided by a potential ItemsInfoRequested event handler, returns True when the info covers the entire data source and the fast path must be engaged, and False to stay in the regular path.
// Before returning False, this method updates the m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath, m_itemsInfoArrangeWidths and
// m_itemsInfoFirstIndex fields. The m_itemsInfoMinWidth and m_itemsInfoMaxWidth fields are updated irrespective of the return value.
bool LinedFlowLayout::RequestItemsInfo(
    ItemsInfo const& itemsInfo,
    int beginSizedItemIndex,
    int endSizedItemIndex)
{
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"beginSizedItemIndex", beginSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"endSizedItemIndex", endSizedItemIndex);

    MUX_ASSERT(beginSizedItemIndex <= endSizedItemIndex);
    MUX_ASSERT(beginSizedItemIndex >= m_firstSizedItemIndex);
    MUX_ASSERT(endSizedItemIndex <= m_lastSizedItemIndex);

    ItemsInfo newItemsInfo{};
    int desiredAspectRatiosCount{};

    if (itemsInfo.m_itemsRangeStartIndex != -1 &&
        beginSizedItemIndex >= itemsInfo.m_itemsRangeStartIndex &&
        endSizedItemIndex <= itemsInfo.m_itemsRangeStartIndex + itemsInfo.m_itemsRangeLength - 1)
    {
        // The items info gathered during the fast path attempt covers the current request.
        // Use it instead of launching a new request.
        newItemsInfo = itemsInfo;

        desiredAspectRatiosCount = newItemsInfo.m_itemsRangeLength;

        MUX_ASSERT(desiredAspectRatiosCount == static_cast<int>(m_itemsInfoDesiredAspectRatiosForFastPath.size()));
        MUX_ASSERT(desiredAspectRatiosCount >= endSizedItemIndex - beginSizedItemIndex + 1);
    }
    else
    {
        const int itemsRangeStartIndex{ beginSizedItemIndex };
        const int itemsRangeRequestedLength{ endSizedItemIndex - beginSizedItemIndex + 1 };

        MUX_ASSERT(itemsRangeStartIndex + itemsRangeRequestedLength - 1 < m_itemCount);

        newItemsInfo = RaiseItemsInfoRequested(itemsRangeStartIndex, itemsRangeRequestedLength);

#ifdef DBG
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"ItemsInfo for regular path");
        LogItemsInfoDbg(itemsRangeStartIndex, itemsRangeRequestedLength, newItemsInfo);
#endif

        desiredAspectRatiosCount = newItemsInfo.m_itemsRangeLength;

        if (desiredAspectRatiosCount <= 0)
        {
            // No aspect ratios were provided. Discard all info and use the fallback pass with larger realization rect.
            ResetItemsInfo();
            return false;
        }

        MUX_ASSERT(newItemsInfo.m_itemsRangeStartIndex >= 0);
        MUX_ASSERT(newItemsInfo.m_itemsRangeStartIndex <= beginSizedItemIndex);
        MUX_ASSERT(newItemsInfo.m_itemsRangeStartIndex + desiredAspectRatiosCount - 1 >= endSizedItemIndex);
    }

    m_itemsInfoMinWidth = newItemsInfo.m_minWidth < 0.0 ? -1.0 : newItemsInfo.m_minWidth;
    m_itemsInfoMaxWidth = newItemsInfo.m_maxWidth < 0.0 ? -1.0 : newItemsInfo.m_maxWidth;

    const int minWidthsCount = static_cast<int>(m_itemsInfoMinWidthsForFastPath.size());
    const int maxWidthsCount = static_cast<int>(m_itemsInfoMaxWidthsForFastPath.size());
    const int itemsRangeLength = std::min(desiredAspectRatiosCount, m_itemCount - newItemsInfo.m_itemsRangeStartIndex);

    MUX_ASSERT(itemsRangeLength > 0);

    if (itemsRangeLength == m_itemCount)
    {
        // The ItemsInfoRequested event handler provided ratios for the entire data source. This situation can occur when a measure path
        // is performed and (m_forceRelayout || m_previousAvailableWidth != availableWidth) evaluated to False in the initial fast path attempt,
        // in LinedFlowLayout::MeasureConstrainedLinesFastPath. The ItemsInfoRequested event was thus skipped. Here a transition from regular path
        // to fast path is enabled. One of two unfortunate behaviors needs to be picked:
        // a. Remain in the regular path until a future measure path with m_forceRelayout==True or a different availableWidth is triggered.
        //    The m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath
        //    vectors could grow very large hindering performance.
        // b. Stop the regular path and backtrack to the fast path since aspect ratios were provided for the entire collection.
        //    This can result in a re-arrangement of the visible items.
        // Option b. is picked for its perf benefits.
        return true;
    }

    if (m_itemsInfoFirstIndex == -1)
    {
        m_itemsInfoFirstIndex = newItemsInfo.m_itemsRangeStartIndex;
    }
    else
    {
        m_itemsInfoFirstIndex = std::min(newItemsInfo.m_itemsRangeStartIndex, m_itemsInfoFirstIndex);
    }

    const int minItemsInfoRequiredSize = std::min(newItemsInfo.m_itemsRangeStartIndex + itemsRangeLength - m_itemsInfoFirstIndex, m_itemCount);

    if (static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size()) < minItemsInfoRequiredSize)
    {
        m_itemsInfoDesiredAspectRatiosForRegularPath.resize(minItemsInfoRequiredSize, -1.0);
    }

    if (minWidthsCount > 0 && static_cast<int>(m_itemsInfoMinWidthsForRegularPath.size()) < minItemsInfoRequiredSize)
    {
        m_itemsInfoMinWidthsForRegularPath.resize(minItemsInfoRequiredSize, -1.0);
    }

    if (maxWidthsCount > 0 && static_cast<int>(m_itemsInfoMaxWidthsForRegularPath.size()) < minItemsInfoRequiredSize)
    {
        m_itemsInfoMaxWidthsForRegularPath.resize(minItemsInfoRequiredSize, -1.0);
    }

    if (static_cast<int>(m_itemsInfoArrangeWidths.size()) < minItemsInfoRequiredSize)
    {
        m_itemsInfoArrangeWidths.resize(minItemsInfoRequiredSize, -1.0f);
    }

    const int itemsInfoIndexStart = newItemsInfo.m_itemsRangeStartIndex - m_itemsInfoFirstIndex;

    for (int index = 0; index < itemsRangeLength; index++)
    {
        const double desiredAspectRatio = m_itemsInfoDesiredAspectRatiosForFastPath[index];

        LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"desiredAspectRatio", desiredAspectRatio);

        SetDesiredAspectRatioFromItemsInfo(
            m_itemsInfoFirstIndex + itemsInfoIndexStart + index,
            desiredAspectRatio);

        MUX_ASSERT(m_itemsInfoDesiredAspectRatiosForRegularPath[itemsInfoIndexStart + index] == desiredAspectRatio);

        if (minWidthsCount > 0 && index < minWidthsCount)
        {
            LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"minWidth", m_itemsInfoMinWidthsForFastPath[index]);

            m_itemsInfoMinWidthsForRegularPath[itemsInfoIndexStart + index] = m_itemsInfoMinWidthsForFastPath[index];
        }

        if (maxWidthsCount > 0 && index < maxWidthsCount)
        {
            LINEDFLOWLAYOUT_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"maxWidth", m_itemsInfoMaxWidthsForFastPath[index]);

            m_itemsInfoMaxWidthsForRegularPath[itemsInfoIndexStart + index] = m_itemsInfoMaxWidthsForFastPath[index];
        }
    }

    return false;
}

void LinedFlowLayout::ResetItemsInfo()
{
    m_itemsInfoDesiredAspectRatiosForRegularPath.clear();
    m_itemsInfoMinWidthsForRegularPath.clear();
    m_itemsInfoMaxWidthsForRegularPath.clear();
    m_itemsInfoArrangeWidths.clear();
    m_itemsInfoFirstIndex = -1;
    m_itemsInfoMinWidth = -1.0f;
    m_itemsInfoMaxWidth = -1.0f;
}

void LinedFlowLayout::ResetItemsInfoForFastPath()
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_itemsInfoDesiredAspectRatiosForFastPath.clear();
    m_itemsInfoMinWidthsForFastPath.clear();
    m_itemsInfoMaxWidthsForFastPath.clear();
}

void LinedFlowLayout::ResetLinesInfo()
{
    m_lineItemCounts.clear();
}

void LinedFlowLayout::ResetSizedLines()
{
    m_firstSizedLineIndex = -1;
    m_lastSizedLineIndex = -1;
    m_firstSizedItemIndex = -1;
    m_lastSizedItemIndex = -1;
    m_firstFrozenLineIndex = -1;
    m_lastFrozenLineIndex = -1;
    m_firstFrozenItemIndex = -1;
    m_lastFrozenItemIndex = -1;
}

// Stores the item's arrange width in the m_itemsInfoArrangeWidths vector.
void LinedFlowLayout::SetArrangeWidthFromItemsInfo(
    int itemIndex,
    float arrangeWidth)
{
#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemIndex", itemIndex);

    if (itemIndex == m_logItemIndexDbg)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"arrangeWidth", arrangeWidth);
    }
    else
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"arrangeWidth", arrangeWidth);
    }
#endif

    MUX_ASSERT(m_itemsInfoFirstIndex >= -1);

    // m_itemsInfoFirstIndex >=  0: Regular path layout case, with items info available (m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath).
    // m_itemsInfoFirstIndex == -1: Fast path layout case, without items info available.
    const int itemsInfoArrangeWidthsIndex = itemIndex - (m_itemsInfoFirstIndex == -1 ? 0 : m_itemsInfoFirstIndex);

    MUX_ASSERT(itemsInfoArrangeWidthsIndex >= 0);
    MUX_ASSERT(itemsInfoArrangeWidthsIndex < static_cast<int>(m_itemsInfoArrangeWidths.size()));

    m_itemsInfoArrangeWidths[itemsInfoArrangeWidthsIndex] = arrangeWidth;
}

void LinedFlowLayout::SetAverageItemsPerLine(
    std::pair<double, double> averageItemsPerLine,
    bool unlockItems)
{
    if (averageItemsPerLine.second == m_averageItemsPerLine.second)
    {
        // When the old and new snapped average-items-per-line are identical,
        // just retain the raw value change.
        m_averageItemsPerLine.first = averageItemsPerLine.first;
    }
    else
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_DBL_DBL, METH_NAME, this, m_averageItemsPerLine.second, averageItemsPerLine.second);

        m_averageItemsPerLine = averageItemsPerLine;

        if (unlockItems)
        {
            UnlockItems();
        }

        com_ptr<LayoutsTestHooks> globalTestHooks = LayoutsTestHooks::GetGlobalTestHooks();

        if (globalTestHooks)
        {
            globalTestHooks->NotifyLinedFlowLayoutSnappedAverageItemsPerLineChanged(*this);
        }
    }
}

void LinedFlowLayout::SetDesiredAspectRatioFromItemsInfo(
    int itemIndex,
    double desiredAspectRatio)
{
#ifdef DBG
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemIndex", itemIndex);

    if (itemIndex == m_logItemIndexDbg)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"desiredAspectRatio", desiredAspectRatio);
    }
    else
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"desiredAspectRatio", desiredAspectRatio);
    }

    MUX_ASSERT(itemIndex < m_itemCount);
    MUX_ASSERT(m_itemsInfoFirstIndex >= 0);
    MUX_ASSERT(itemIndex - m_itemsInfoFirstIndex >= 0);
    MUX_ASSERT(itemIndex - m_itemsInfoFirstIndex < static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size()));
#endif

    m_itemsInfoDesiredAspectRatiosForRegularPath[itemIndex - m_itemsInfoFirstIndex] = desiredAspectRatio;
}

// Returns the total arrange width for the provided index range.
float LinedFlowLayout::SetItemRangeArrangeWidth(
    int beginItemIndex,
    int endItemIndex,
    double actualLineHeight,
    double averageAspectRatio,
    double scaleFactor)
{
    MUX_ASSERT(beginItemIndex <= endItemIndex);
    MUX_ASSERT(beginItemIndex >= 0);
    MUX_ASSERT(endItemIndex < static_cast<int>(m_itemsInfoArrangeWidths.size()));

    float totalArrangeWidth = 0.0f;

    for (int itemIndex = beginItemIndex; itemIndex <= endItemIndex; itemIndex++)
    {
        const float arrangeWidth = GetArrangeWidthFromItemsInfo(itemIndex, actualLineHeight, averageAspectRatio, scaleFactor);

        SetArrangeWidthFromItemsInfo(itemIndex, arrangeWidth);

        totalArrangeWidth += arrangeWidth;
    }

    return totalArrangeWidth;
}

// Snaps the provided average items per line to a power of 1.1, taking into account the old averageItemsPerLine raw (i.e. unsnapped) value.
std::pair<double, double> LinedFlowLayout::SnapAverageItemsPerLine(
    double oldAverageItemsPerLineRaw,
    double newAverageItemsPerLineRaw) const
{
    MUX_ASSERT(oldAverageItemsPerLineRaw == 0.0 || oldAverageItemsPerLineRaw >= 1.0);
    MUX_ASSERT(newAverageItemsPerLineRaw == 0.0 || newAverageItemsPerLineRaw >= 1.0);

    // A snapped average-items-per-line value must be a power of 1.1.
    constexpr double valuePower = 1.1;
    // When the raw value crosses the median line betweeen two successive powers of 1.1, the old snapped value is retained unless the raw delta is greater than 0.1.
    constexpr double distanceThreshold = 0.1;
    const double appliedValuePower = m_forcedAverageItemsPerLineDividerDbg == 0.0 ? valuePower : m_forcedAverageItemsPerLineDividerDbg;
    const double oldAverageItemsPerLineSnapped = (oldAverageItemsPerLineRaw == 0.0) ? 0.0 : SnapToPower(oldAverageItemsPerLineRaw, appliedValuePower);
    const double newAverageItemsPerLineSnapped = (newAverageItemsPerLineRaw == 0.0) ? 0.0 : SnapToPower(newAverageItemsPerLineRaw, appliedValuePower);

    if (oldAverageItemsPerLineSnapped != newAverageItemsPerLineSnapped &&
        std::abs(oldAverageItemsPerLineRaw - newAverageItemsPerLineRaw) <= distanceThreshold)
    {
        // The old and new snapped values are different, but their respective raw values are close (i.e. within a distanceThreshold delta).
        // Instead of returning newAverageItemsPerLineSnapped, return oldAverageItemsPerLineSnapped so the applied average items per line
        // is more stable when the raw values hover around the middle point between 1.1^N and 1.1^(N+1).
        // Example:
        // 1.1^14 = 3.7975, 1.1^15 = 4.1772. Their middle point is about 3.9874.
        // oldAverageItemsPerLineRaw = 3.95 snapped to oldAverageItemsPerLineSnapped = 1.1^14 = 3.7975.
        // newAverageItemsPerLineRaw = 4.00 would normally snap to 1.1^15 = 4.1772, but because |3.95 - 4.00| <= 0.1, the old pair is returned.
        return std::pair<double, double>(oldAverageItemsPerLineRaw, oldAverageItemsPerLineSnapped);
    }

    return std::pair<double, double>(newAverageItemsPerLineRaw, newAverageItemsPerLineSnapped);
}

// Snaps the provided value to a power of valuePower.
// Example: value=3.75 snaps to 1.1^14 = 3.7975, valuePower being 1.1.
double LinedFlowLayout::SnapToPower(
    double value,
    double valuePower) const
{
#ifdef DBG
    constexpr double defaultValuePowerDbg = 1.1;

    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"value", value);
    if (valuePower != defaultValuePowerDbg)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"valuePower", valuePower);
    }

    MUX_ASSERT(value >= 1.0);
#endif

    const double dividerLog = std::log(valuePower);
    const double valueLog = std::log(value);
    const double valueExp = std::ceil(valueLog / dividerLog);
    const double valueRndCeil = std::pow(valuePower, valueExp);
    const double valueRndFloor = valueRndCeil / valuePower;

#ifdef DBG
    if (valuePower != defaultValuePowerDbg)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"dividerLog", dividerLog);
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"valueLog", valueLog);
    }
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"valueExp", valueExp);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"valueRndCeil", valueRndCeil);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"valueRndFloor", valueRndFloor);
#endif

    // Return valuePower^valueExp or valuePower^(valueExp - 1), whichever is closest to value.
    if (std::abs(valueRndCeil - value) <= std::abs(valueRndFloor - value))
    {
        return valueRndCeil;
    }
    else
    {
        return valueRndFloor;
    }
}

// Raises the ItemsUnlocked event when there are locked items.
// This event is raised when previously locked items are cleared and declared unlocked because the source collection
// or the average items per line changed.
void LinedFlowLayout::UnlockItems()
{
    if (m_lockedItemIndexes.size() > 0 || m_isFirstOrLastItemLocked)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, m_lockedItemIndexes.size(), m_isFirstOrLastItemLocked);

        m_lockedItemIndexes.clear();
        m_isFirstOrLastItemLocked = false;
        m_itemsUnlockedEventSource(*this, nullptr);
    }
}

// Computes the ActualLineHeight property value. Returns True when it changed.
bool LinedFlowLayout::UpdateActualLineHeight(
    winrt::VirtualizingLayoutContext const& context,
    winrt::Size const& availableSize)
{
    const double lineHeight = LineHeight();
    const double oldActualLineHeight = ActualLineHeight();
    double newActualLineHeight{ 0.0 };

    if (std::isnan(lineHeight))
    {
        if (context.ItemCount() > 0)
        {
            winrt::UIElement element = nullptr;
            winrt::Size desiredSize{ -1.0f, -1.0f };

            // Element for first data item determines ActualLineHeight value.
            if (oldActualLineHeight != 0.0 &&
                m_elementManager.IsDataIndexRealized(0) &&
                (element = m_elementManager.GetRealizedElement(0)))
            {
                // Check if the realized element at index 0 has a recorded desired or available size
                // to restore after measurement with the provided 'availableSize'.
                desiredSize = GetDesiredSizeForAvailableSize(0, element, availableSize, oldActualLineHeight);

                if (desiredSize.Height != -1.0f)
                {
                    newActualLineHeight = desiredSize.Height;
                }
                // Else use context.GetOrCreateElementAt instead.
            }

            if (desiredSize.Height == -1.0f)
            {
                // The element for the first item is not realized or has no recorded measurement size, so generate a new element and let it be recycled.
                // Since LineHeight is NaN, this element is not meant to have a height based on content asynchronously loaded.
                // That would lead to a temporary incorrect ActualLineHeight and potentially numerous item realizations.
                if (element = context.GetOrCreateElementAt(0 /*dataIndex*/, winrt::ElementRealizationOptions::ForceCreate))
                {
                    element.Measure(availableSize);
                    newActualLineHeight = element.DesiredSize().Height;
                    //TODO: Bug 41896454.
                    //      Why is this RecycleElement call triggering endless measure passes?
                    //      This element needs to be discarded or else bring-into-view operations to index 0 will be animated (because index 0 is considered realized).
                    //context.RecycleElement(element);
                }
            }
        }
    }
    else
    {
        newActualLineHeight = lineHeight;
    }

    if (oldActualLineHeight != newActualLineHeight)
    {
        LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_DBL, METH_NAME, this, newActualLineHeight);

        ActualLineHeight(newActualLineHeight);

        return true;
    }

    return false;
}

// Updates the m_aspectRatios storage based on the items info collected from the ItemsInfoRequested event handler.
// The aspect ratios put in the storage immediately get the maximum weight c_maxAspectRatioWeight as the application-provided
// information is fully trusted.
// The application may not provide accurate aspect ratios though. It may temporarily provide placeholder ratios until the
// accurate values are evaluated, at which point it calls LinedFlowLayout.InvalidateItemsInfo. m_aspectRatios then gets updated
// with new aspect ratios with the maximum weight c_maxAspectRatioWeight again.
// The application may also provide aspect ratios <= 0 instead of valid placeholder values. Those temporary values, unlike
// the positive placeholder values, are not stored in m_aspectRatios. The average aspect ratio is a replacement for negative values.
void LinedFlowLayout::UpdateAspectRatiosWithItemsInfo()
{
    MUX_ASSERT(m_aspectRatios != nullptr);

    const int itemsInfoCount = static_cast<int>(m_itemsInfoDesiredAspectRatiosForRegularPath.size());
    const double actualLineHeight = ActualLineHeight();

    for (int index = 0; index < itemsInfoCount; index++)
    {
        double desiredAspectRatio = m_itemsInfoDesiredAspectRatiosForRegularPath[index];

        if (desiredAspectRatio <= 0.0)
        {
            continue;
        }

        const double minWidth = GetMinWidthFromItemsInfo(m_itemsInfoFirstIndex + index);
        const double maxWidth = GetMaxWidthFromItemsInfo(m_itemsInfoFirstIndex + index);

        if (minWidth > 0.0 || maxWidth >= 0.0)
        {
            double desiredWidth = desiredAspectRatio * actualLineHeight;

            if (minWidth > 0.0 && minWidth > desiredWidth)
            {
                desiredWidth = minWidth;
            }

            if (maxWidth >= 0.0 && maxWidth < desiredWidth)
            {
                desiredWidth = maxWidth;
            }

            desiredAspectRatio = desiredWidth / actualLineHeight;
        }

        LinedFlowLayoutItemAspectRatios::ItemAspectRatio itemAspectRatio = m_aspectRatios->GetAt(m_itemsInfoFirstIndex + index);
        const float aspectRatio = static_cast<float>(desiredAspectRatio);
        bool updateItemAspectRatio = false;

        if (itemAspectRatio.m_weight == 0)
        {
            itemAspectRatio.m_aspectRatio = aspectRatio;
            itemAspectRatio.m_weight = c_maxAspectRatioWeight;
            updateItemAspectRatio = true;

#ifdef DBG
            if (m_itemsInfoFirstIndex + index == m_logItemIndexDbg)
            {
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"ItemsInfo aspect ratio", aspectRatio);
            }
#endif
        }
        else
        {
            MUX_ASSERT(itemAspectRatio.m_aspectRatio >= 0.0f);
            MUX_ASSERT(itemAspectRatio.m_weight >= 1);
            MUX_ASSERT(itemAspectRatio.m_weight <= c_maxAspectRatioWeight);

            if (itemAspectRatio.m_weight != c_maxAspectRatioWeight)
            {
                itemAspectRatio.m_weight = c_maxAspectRatioWeight;
                updateItemAspectRatio = true;
            }

            // Ignore aspect ratio deltas coming from pixel snapping rounding. The aspect ratios coming from UIElement.Measure are preserved.
            if (std::abs(itemAspectRatio.m_aspectRatio - aspectRatio) > 0.5 / (m_roundingScaleFactor * actualLineHeight))
            {
#ifdef DBG
                if (m_itemsInfoFirstIndex + index == m_logItemIndexDbg)
                {
                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"logItemIndexDbg", m_logItemIndexDbg);
                    LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"ItemsInfo aspect ratio overwriting", itemAspectRatio.m_aspectRatio, aspectRatio);
                }
#endif
                itemAspectRatio.m_aspectRatio = aspectRatio;
                updateItemAspectRatio = true;
            }
        }

        if (updateItemAspectRatio)
        {
            m_aspectRatios->SetAt(m_itemsInfoFirstIndex + index, itemAspectRatio);
        }
    }
}

// Raises the ItemsInfoRequested event to potentially fill the m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath,
// m_itemsInfoFirstIndex, m_itemsInfoMinWidth, m_itemsInfoMaxWidth fields with item sizing information.
// Returns True when the provided info covers the entire data source and the fast path must be engaged, and False to stay in the regular path.
bool LinedFlowLayout::UpdateItemsInfo(
    ItemsInfo const& itemsInfo,
    int oldFirstItemsInfoIndex,
    int oldLastItemsInfoIndex)
{
#ifdef DBG
    LogItemsInfoDbg(-1, -1, itemsInfo);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"oldFirstItemsInfoIndex", oldFirstItemsInfoIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"oldLastItemsInfoIndex", oldLastItemsInfoIndex);
#endif

    // Raise ItemsInfoRequested events.
    if (oldFirstItemsInfoIndex == -1 || m_lastSizedItemIndex < oldFirstItemsInfoIndex || m_firstSizedItemIndex > oldLastItemsInfoIndex)
    {
        // No overlapping of old items info and new sized items. Build the items info from scratch.
        ResetItemsInfo();

        // Raise event for [m_firstSizedItemIndex, m_lastSizedItemIndex] range.
        if (RequestItemsInfo(itemsInfo, m_firstSizedItemIndex /*beginSizedItemIndex*/, m_lastSizedItemIndex /*endSizedItemIndex*/))
        {
            return true;
        }

        UpdateAspectRatiosWithItemsInfo();
    }
    else if (m_firstSizedItemIndex < oldFirstItemsInfoIndex || m_lastSizedItemIndex > oldLastItemsInfoIndex)
    {
        // Partial overlapping.
        MUX_ASSERT(oldLastItemsInfoIndex != -1);

        // TODO as part of perf Task 42298247 - see if these copies can be avoided by copying vector elements 'in place'.
        std::vector<double> oldItemsInfoDesiredAspectRatios = m_itemsInfoDesiredAspectRatiosForRegularPath;
        std::vector<double> oldItemsInfoMinWidths = m_itemsInfoMinWidthsForRegularPath;
        std::vector<double> oldItemsInfoMaxWidths = m_itemsInfoMaxWidthsForRegularPath;
        std::vector<float> oldItemsInfoArrangeWidths = m_itemsInfoArrangeWidths;

        const int newItemsInfoCount = m_lastSizedItemIndex - m_firstSizedItemIndex + 1;

        EnsureItemsInfoDesiredAspectRatios(newItemsInfoCount /*itemCount*/);

        if (m_itemsInfoMinWidthsForRegularPath.size() > 0)
        {
            EnsureItemsInfoMinWidths(newItemsInfoCount /*itemCount*/);
        }

        if (m_itemsInfoMaxWidthsForRegularPath.size() > 0)
        {
            EnsureItemsInfoMaxWidths(newItemsInfoCount /*itemCount*/);
        }

        EnsureItemsInfoArrangeWidths(newItemsInfoCount /*itemCount*/);

        m_itemsInfoFirstIndex = m_firstSizedItemIndex;

        const bool oldFirstOverlaps = oldFirstItemsInfoIndex >= m_firstSizedItemIndex && oldFirstItemsInfoIndex <= m_lastSizedItemIndex;
        const bool oldLastOverlaps = oldLastItemsInfoIndex >= m_firstSizedItemIndex && oldLastItemsInfoIndex <= m_lastSizedItemIndex;
        const bool newFirstOverlaps = m_firstSizedItemIndex >= oldFirstItemsInfoIndex && m_firstSizedItemIndex <= oldLastItemsInfoIndex;
        const bool newLastOverlaps = m_lastSizedItemIndex >= oldFirstItemsInfoIndex && m_lastSizedItemIndex <= oldLastItemsInfoIndex;

        if (oldFirstOverlaps || oldLastOverlaps || newFirstOverlaps || newLastOverlaps)
        {
            MUX_ASSERT(!newFirstOverlaps || !newLastOverlaps);

            // Fill m_itemsInfoDesiredAspectRatiosForRegularPath, m_itemsInfoMinWidthsForRegularPath, m_itemsInfoMaxWidthsForRegularPath, m_itemsInfoArrangeWidths vectors with overlapping
            // oldItemsInfoDesiredAspectRatios, oldItemsInfoMinWidths, oldItemsInfoMaxWidths, oldItemsInfoArrangeWidths ones.
            if (oldFirstOverlaps && oldLastOverlaps)
            {
                CopyItemsInfo(
                    oldItemsInfoDesiredAspectRatios,
                    oldItemsInfoMinWidths,
                    oldItemsInfoMaxWidths,
                    oldItemsInfoArrangeWidths,
                    0 /*oldStart*/,
                    oldFirstItemsInfoIndex - m_firstSizedItemIndex /*newStart*/,
                    oldLastItemsInfoIndex - oldFirstItemsInfoIndex + 1 /*copyCount*/);
            }
            else if (newFirstOverlaps && oldLastOverlaps)
            {
                CopyItemsInfo(
                    oldItemsInfoDesiredAspectRatios,
                    oldItemsInfoMinWidths,
                    oldItemsInfoMaxWidths,
                    oldItemsInfoArrangeWidths,
                    m_firstSizedItemIndex - oldFirstItemsInfoIndex /*oldStart*/,
                    0 /*newStart*/,
                    oldLastItemsInfoIndex - m_firstSizedItemIndex + 1 /*copyCount*/);
            }
            else if (oldFirstOverlaps && newLastOverlaps)
            {
                CopyItemsInfo(
                    oldItemsInfoDesiredAspectRatios,
                    oldItemsInfoMinWidths,
                    oldItemsInfoMaxWidths,
                    oldItemsInfoArrangeWidths,
                    0 /*oldStart*/,
                    oldFirstItemsInfoIndex - m_firstSizedItemIndex /*newStart*/,
                    m_lastSizedItemIndex - oldFirstItemsInfoIndex + 1 /*copyCount*/);
            }
        }

        if (m_firstSizedItemIndex < oldFirstItemsInfoIndex)
        {
            // Raise event for [m_firstSizedItemIndex, oldFirstItemsInfoIndex - 1] range.
            if (RequestItemsInfo(itemsInfo, m_firstSizedItemIndex /*beginSizedItemIndex*/, oldFirstItemsInfoIndex - 1 /*endSizedItemIndex*/))
            {
                return true;
            }
        }

        if (m_lastSizedItemIndex > oldLastItemsInfoIndex)
        {
            // Raise event for [oldLastItemsInfoIndex + 1, m_lastSizedItemIndex] range.
            if (RequestItemsInfo(itemsInfo, oldLastItemsInfoIndex + 1 /*beginSizedItemIndex*/, m_lastSizedItemIndex /*endSizedItemIndex*/))
            {
                return true;
            }
        }

        UpdateAspectRatiosWithItemsInfo();
    }
    else
    {
        // Full overlapping. [m_firstSizedItemIndex, m_lastSizedItemIndex] is a subset of [oldFirstItemsInfoIndex, oldLastItemsInfoIndex].
        // m_itemsInfoDesiredAspectRatiosForRegularPath left unchanged.
        MUX_ASSERT(m_itemsInfoFirstIndex != -1);
        MUX_ASSERT(m_firstSizedItemIndex >= oldFirstItemsInfoIndex);
        MUX_ASSERT(m_firstSizedItemIndex <= oldLastItemsInfoIndex);
        MUX_ASSERT(m_lastSizedItemIndex >= oldFirstItemsInfoIndex);
        MUX_ASSERT(m_lastSizedItemIndex <= oldLastItemsInfoIndex);
    }

    return false;
}

void LinedFlowLayout::UpdateRoundingScaleFactor(
    winrt::UIElement const& xamlRootReference)
{
    try
    {
        m_roundingScaleFactor = static_cast<float>(xamlRootReference.XamlRoot().RasterizationScale());
    }
    catch (winrt::hresult_error)
    {
        // Calling GetForCurrentView on threads without a CoreWindow throws an error.
        // In this circumstance, we'll just always round to the closest integer.
        m_roundingScaleFactor = 1.0f;
    }
}

// Returns True when an ItemsInfoRequested handler provided sizing information for
// the request item range or more.  In either cases, m_itemsInfoArrangeWidths is populated
// with arrange widths resulting from the provided information.
// When returning True,
//  - when the handler provided information for less than the entire source collection, m_itemsInfoFirstIndex is >= 0.
//  - when the handler provided information the entire source collection, m_itemsInfoFirstIndex is -1.
bool LinedFlowLayout::UsesArrangeWidthInfo() const
{
    return m_itemsInfoArrangeWidths.size() > 0;
}

// Returns True when an ItemsInfoRequested handler provided sizing information for
// the entire source collection.  In that case, the fast path can be performed, and
// it is characterized by having a populated m_itemsInfoArrangeWidths vector and m_itemsInfoFirstIndex remaining at -1.
bool LinedFlowLayout::UsesFastPathLayout() const
{
    return UsesArrangeWidthInfo() && m_itemsInfoFirstIndex == -1;
}

#ifdef DBG

winrt::hstring LinedFlowLayout::DependencyPropertyToStringDbg(
    winrt::IDependencyProperty const& dependencyProperty)
{
    if (dependencyProperty == s_ItemsJustificationProperty)
    {
        return L"ItemsJustification";
    }
    else if (dependencyProperty == s_ItemsStretchProperty)
    {
        return L"ItemsStretch";
    }
    else if (dependencyProperty == s_MinItemSpacingProperty)
    {
        return L"MinItemSpacing";
    }
    else if (dependencyProperty == s_LineSpacingProperty)
    {
        return L"LineSpacing";
    }
    else if (dependencyProperty == s_LineHeightProperty)
    {
        return L"LineHeight";
    }
    else if (dependencyProperty == s_ActualLineHeightProperty)
    {
        return L"ActualLineHeight";
    }
    else
    {
        return L"UNKNOWN";
    }
}

void LinedFlowLayout::LogElementManagerDbg()
{
    const int firstRealizedDataIndex = m_elementManager.GetFirstRealizedDataIndex();

    if (firstRealizedDataIndex != - 1)
    {
        const int realizedElementCount = m_elementManager.GetRealizedElementCount();

        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstRealizedDataIndex", firstRealizedDataIndex);
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastRealizedDataIndex", firstRealizedDataIndex + realizedElementCount - 1);
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"realizedElementCount", realizedElementCount);
    }
}

void LinedFlowLayout::LogItemsInfoDbg(
    int itemsRangeStartIndex,
    int itemsRangeRequestedLength,
    ItemsInfo const& itemsInfo)
{
    if (itemsRangeStartIndex != -1 && itemsRangeRequestedLength != -1)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemsRangeStartIndex", itemsRangeStartIndex);
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemsRangeRequestedLength", itemsRangeRequestedLength);
    }

    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"ii.m_itemsRangeStartIndex", itemsInfo.m_itemsRangeStartIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"ii.m_itemsRangeLength", itemsInfo.m_itemsRangeLength);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"ii.m_minWidth", itemsInfo.m_minWidth);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"ii.m_maxWidth", itemsInfo.m_maxWidth);

    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_itemsInfoDesiredAspectRatiosForFastPath.size", m_itemsInfoDesiredAspectRatiosForFastPath.size());
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_itemsInfoMinWidthsForFastPath.size", m_itemsInfoMinWidthsForFastPath.size());
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_itemsInfoMaxWidthsForFastPath.size", m_itemsInfoMaxWidthsForFastPath.size());
}

void LinedFlowLayout::LogItemsLayoutDbg(
    ItemsLayout const& itemsLayout,
    double averageLineItemsWidth) const
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"drawback", itemsLayout.m_drawback);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"availableLineItemsWidth", itemsLayout.m_availableLineItemsWidth);
    if (averageLineItemsWidth > 0)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"averageLineItemsWidth",
            averageLineItemsWidth);
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"delta (%)",
            std::abs(itemsLayout.m_availableLineItemsWidth - averageLineItemsWidth) / averageLineItemsWidth * 100.0);
    }

    if (itemsLayout.m_smallestHeadLineIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"smallestHeadLineIndex", itemsLayout.m_smallestHeadLineIndex);
    }

    if (itemsLayout.m_smallestHeadItemIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"smallestHeadItemIndex", itemsLayout.m_smallestHeadItemIndex);
    }

    if (itemsLayout.m_smallestHeadItemWidth != std::numeric_limits<double>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"smallestHeadItemWidth", itemsLayout.m_smallestHeadItemWidth);
    }

    if (itemsLayout.m_smallestTailLineIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"smallestTailLineIndex", itemsLayout.m_smallestTailLineIndex);
    }

    if (itemsLayout.m_smallestTailItemIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"smallestTailItemIndex", itemsLayout.m_smallestTailItemIndex);
    }

    if (itemsLayout.m_smallestTailItemWidth != std::numeric_limits<double>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"smallestTailItemWidth", itemsLayout.m_smallestTailItemWidth);
    }

    if (itemsLayout.m_bestEqualizingHeadLineIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"bestEqualizingHeadLineIndex", itemsLayout.m_bestEqualizingHeadLineIndex);
    }

    if (itemsLayout.m_bestEqualizingHeadItemIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"bestEqualizingHeadItemIndex", itemsLayout.m_bestEqualizingHeadItemIndex);
    }

    if (itemsLayout.m_bestEqualizingHeadItemDrawbackImprovement != 0.0)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"bestEqualizingHeadItemDrawbackImprovement", itemsLayout.m_bestEqualizingHeadItemDrawbackImprovement);
    }

    if (itemsLayout.m_bestEqualizingTailLineIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"bestEqualizingTailLineIndex", itemsLayout.m_bestEqualizingTailLineIndex);
    }

    if (itemsLayout.m_bestEqualizingTailItemIndex != std::numeric_limits<int>::max())
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"bestEqualizingTailItemIndex", itemsLayout.m_bestEqualizingTailItemIndex);
    }

    if (itemsLayout.m_bestEqualizingTailItemDrawbackImprovement != 0.0)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"bestEqualizingTailItemDrawbackImprovement", itemsLayout.m_bestEqualizingTailItemDrawbackImprovement);
    }

    // Uncomment for verbose tracing of line item counts and widths:
    //for (int index = 0; index < static_cast<int>(itemsLayout.m_lineItemWidths.size()); index++)
    //{
    //    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"line item count", index, itemsLayout.m_lineItemCounts[index]);
    //    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"line size", itemsLayout.m_lineItemWidths[index]);
    //}
}

void LinedFlowLayout::LogLayoutDbg()
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"averageItemAspectRatioDbg", m_averageItemAspectRatioDbg);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"averageItemsPerLine raw", m_averageItemsPerLine.first);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_DBL, METH_NAME, this, L"averageItemsPerLine snapped", m_averageItemsPerLine.second);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"measureCountdown", m_measureCountdown);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"itemCount", m_itemCount);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"unsizedNearLineCount", m_unsizedNearLineCount);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstSizedLineIndex", m_firstSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastSizedLineIndex", m_lastSizedLineIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstSizedItemIndex", m_firstSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastSizedItemIndex", m_lastSizedItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"unrealizedNearLineCount", m_unrealizedNearLineCount);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstRealizedItemIndex", FirstRealizedItemIndexDbg());
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastRealizedItemIndex", LastRealizedItemIndexDbg());
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstFrozenLineIndex", m_firstFrozenLineIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastFrozenLineIndex", m_lastFrozenLineIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"firstFrozenItemIndex", m_firstFrozenItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lastFrozenItemIndex", m_lastFrozenItemIndex);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"m_previousAvailableWidth", m_previousAvailableWidth);

    for (const auto& lockedItemIterator : m_lockedItemIndexes)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"lockedItem", lockedItemIterator.first, lockedItemIterator.second);
    }

    for (const int lineItemsCount : m_lineItemCounts)
    {
        LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"lineItemsCount", lineItemsCount);
    }
}

void LinedFlowLayout::LogVirtualizingLayoutContextDbg(
    winrt::VirtualizingLayoutContext const& context) const
{
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"ItemCount", context.ItemCount());
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"RecommendedAnchorIndex", context.RecommendedAnchorIndex());
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"RealizationRect", context.RealizationRect().Y, context.RealizationRect().Height);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT_FLT, METH_NAME, this, L"VisibleRect", context.VisibleRect().Y, context.VisibleRect().Height);
    LINEDFLOWLAYOUT_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_FLT, METH_NAME, this, L"LayoutOrigin", context.LayoutOrigin().Y);
}

void LinedFlowLayout::VerifyInternalLockedItemsDbg(
    std::map<int, int> const& internalLockedItemIndexes,
    int beginSizedLineIndex,
    int endSizedLineIndex,
    int beginSizedItemIndex,
    int endSizedItemIndex)
{
    MUX_ASSERT(!(beginSizedLineIndex < endSizedLineIndex && beginSizedItemIndex >= endSizedItemIndex));
    MUX_ASSERT(!(beginSizedLineIndex > endSizedLineIndex && beginSizedItemIndex <= endSizedItemIndex));
    MUX_ASSERT(std::abs(beginSizedLineIndex - endSizedLineIndex) <= std::abs(beginSizedItemIndex - endSizedItemIndex));

    if (internalLockedItemIndexes.size() == 0)
    {
        return;
    }

    int previousLockedItemIndex = endSizedItemIndex >= beginSizedItemIndex ? beginSizedItemIndex : endSizedItemIndex;
    int previousLockedLineIndex = endSizedItemIndex >= beginSizedItemIndex ? beginSizedLineIndex : endSizedLineIndex;

    for (const auto& lockedItemIterator : internalLockedItemIndexes)
    {
        const int lockedItemIndex = lockedItemIterator.first;
        const int lockedLineIndex = lockedItemIterator.second;

        MUX_ASSERT(lockedLineIndex >= previousLockedLineIndex);
        MUX_ASSERT(lockedItemIndex > previousLockedItemIndex);
        MUX_ASSERT(lockedItemIndex - previousLockedItemIndex >= lockedLineIndex - previousLockedLineIndex);

        previousLockedItemIndex = lockedItemIndex;
        previousLockedLineIndex = lockedLineIndex;
    }

    const int lockedItemIndex = endSizedItemIndex >= beginSizedItemIndex ? endSizedItemIndex : beginSizedItemIndex;
    const int lockedLineIndex = endSizedItemIndex >= beginSizedItemIndex ? endSizedLineIndex : beginSizedLineIndex;

    MUX_ASSERT(lockedLineIndex >= previousLockedLineIndex);
    MUX_ASSERT(lockedItemIndex > previousLockedItemIndex);
    MUX_ASSERT(lockedItemIndex - previousLockedItemIndex >= lockedLineIndex - previousLockedLineIndex);
}

void LinedFlowLayout::VerifyLockedItemsDbg()
{
    if (m_lockedItemIndexes.size() == 0)
    {
        return;
    }

    int sizedItemIndex = m_firstSizedItemIndex;

    for (int sizedLineVectorIndex = 0; sizedLineVectorIndex < static_cast<int>(m_lineItemCounts.size()); sizedLineVectorIndex++)
    {
        const int lineItemsCount = m_lineItemCounts[sizedLineVectorIndex];
        const int lineIndex = sizedLineVectorIndex + m_firstSizedLineIndex;

        for (int itemVectorIndex = 0; itemVectorIndex < lineItemsCount; itemVectorIndex++)
        {
            if (sizedItemIndex >= m_firstFrozenItemIndex &&
                sizedItemIndex <= m_lastFrozenItemIndex &&
                m_lockedItemIndexes.find(sizedItemIndex) != m_lockedItemIndexes.end() &&
                m_lockedItemIndexes[sizedItemIndex] != lineIndex)
            {
                LINEDFLOWLAYOUT_TRACE_INFO(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, m_lockedItemIndexes[sizedItemIndex], lineIndex);
                MUX_ASSERT(false);
            }

            sizedItemIndex++;
        }
    }
}

#endif

#pragma endregion
