// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WinEventLogLevels.h"
#include "LayoutsTestHooks.g.h"

class LayoutsTestHooks :
    public winrt::implementation::LayoutsTestHooksT<LayoutsTestHooks>
{
public:
    static com_ptr<LayoutsTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks->get_strong();
    }

    static winrt::IndexBasedLayoutOrientation GetLayoutForcedIndexBasedLayoutOrientation(winrt::IInspectable const& layout);
    static void SetLayoutForcedIndexBasedLayoutOrientation(winrt::IInspectable const& layout, winrt::IndexBasedLayoutOrientation forcedIndexBasedLayoutOrientation);
    static void ResetLayoutForcedIndexBasedLayoutOrientation(winrt::IInspectable const& layout);

    static void LayoutInvalidateMeasure(winrt::IInspectable const& layout, bool relayout);

    static int GetLayoutFirstRealizedItemIndex(winrt::IInspectable const& layout);
    static int GetLayoutLastRealizedItemIndex(winrt::IInspectable const& layout);

    static int GetLinedFlowLayoutFirstFrozenItemIndex(winrt::IInspectable const& linedFlowLayout);
    static int GetLinedFlowLayoutLastFrozenItemIndex(winrt::IInspectable const& linedFlowLayout);

    static double GetLinedFlowLayoutAverageItemAspectRatio(winrt::IInspectable const& linedFlowLayout);
    static double GetLinedFlowLayoutRawAverageItemsPerLine(winrt::IInspectable const& linedFlowLayout);
    static double GetLinedFlowLayoutSnappedAverageItemsPerLine(winrt::IInspectable const& linedFlowLayout);

    static double GetLinedFlowLayoutForcedAverageItemAspectRatio(winrt::IInspectable const& linedFlowLayout);
    static void SetLinedFlowLayoutForcedAverageItemAspectRatio(winrt::IInspectable const& linedFlowLayout, double forcedAverageItemAspectRatio);

    static double GetLinedFlowLayoutForcedAverageItemsPerLineDivider(winrt::IInspectable const& linedFlowLayout);
    static void SetLinedFlowLayoutForcedAverageItemsPerLineDivider(winrt::IInspectable const& linedFlowLayout, double forcedAverageItemsPerLineDivider);

    static double GetLinedFlowLayoutForcedWrapMultiplier(winrt::IInspectable const& linedFlowLayout);
    static void SetLinedFlowLayoutForcedWrapMultiplier(winrt::IInspectable const& linedFlowLayout, double forcedWrapMultiplier);

    static bool GetLinedFlowLayoutIsFastPathSupported(winrt::IInspectable const& linedFlowLayout);
    static void SetLinedFlowLayoutIsFastPathSupported(winrt::IInspectable const& linedFlowLayout, bool isFastPathSupported);

    static int GetLinedFlowLayoutLogItemIndex(winrt::IInspectable const& linedFlowLayout);
    static void SetLinedFlowLayoutLogItemIndex(winrt::IInspectable const& linedFlowLayout, int logItemIndex);

    static winrt::event_token LinedFlowLayoutSnappedAverageItemsPerLineChanged(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value);
    static void LinedFlowLayoutSnappedAverageItemsPerLineChanged(winrt::event_token const& token);
    static void NotifyLinedFlowLayoutSnappedAverageItemsPerLineChanged(winrt::IInspectable const& linedFlowLayout);

    static winrt::event_token LinedFlowLayoutInvalidated(winrt::TypedEventHandler<winrt::IInspectable, winrt::LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs> const& value);
    static void LinedFlowLayoutInvalidated(winrt::event_token const& token);
    static void NotifyLinedFlowLayoutInvalidated(winrt::IInspectable const& linedFlowLayout, winrt::LinedFlowLayoutInvalidationTrigger const& invalidationTrigger);

    static winrt::event_token LinedFlowLayoutItemLocked(winrt::TypedEventHandler<winrt::IInspectable, winrt::LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs> const& value);
    static void LinedFlowLayoutItemLocked(winrt::event_token const& token);
    static void NotifyLinedFlowLayoutItemLocked(winrt::IInspectable const& linedFlowLayout, int itemIndex, int lineIndex);

    static int GetLinedFlowLayoutLineIndex(winrt::IInspectable const& linedFlowLayout, int itemIndex);

    static void ClearLinedFlowLayoutItemAspectRatios(winrt::IInspectable const& linedFlowLayout);
    static void UnlockLinedFlowLayoutItems(winrt::IInspectable const& linedFlowLayout);

private:
    static LayoutsTestHooks* s_testHooks;

    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable>> m_linedFlowLayoutSnappedAverageItemsPerLineChangedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs>> m_linedFlowLayoutInvalidatedEventSource;
    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs>> m_linedFlowLayoutItemLockedEventSource;

    static void EnsureHooks();
};
