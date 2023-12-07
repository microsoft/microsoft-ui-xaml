// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "LayoutsTestHooksFactory.h"
#include "LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs.h"
#include "LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs.h"
#include "LinedFlowLayout.h"

/* static */
winrt::IndexBasedLayoutOrientation LayoutsTestHooks::GetLayoutForcedIndexBasedLayoutOrientation(
    winrt::IInspectable const& layout)
{
    if (auto instance = layout.as<Layout>())
    {
        return instance->GetForcedIndexBasedLayoutOrientation();
    }

    return winrt::IndexBasedLayoutOrientation::None;
}

/* static */
void LayoutsTestHooks::SetLayoutForcedIndexBasedLayoutOrientation(
    winrt::IInspectable const& layout,
    winrt::IndexBasedLayoutOrientation forcedIndexBasedLayoutOrientation)
{
    if (auto instance = layout.as<Layout>())
    {
        instance->SetForcedIndexBasedLayoutOrientation(forcedIndexBasedLayoutOrientation);
    }
}

/* static */
void LayoutsTestHooks::ResetLayoutForcedIndexBasedLayoutOrientation(
    winrt::IInspectable const& layout)
{
    if (auto instance = layout.as<Layout>())
    {
        instance->ResetForcedIndexBasedLayoutOrientation();
    }
}

/* static */
void LayoutsTestHooks::LayoutInvalidateMeasure(
    winrt::IInspectable const& layout,
    bool relayout)
{
    if (relayout)
    {
        if (auto instance = layout.as<LinedFlowLayout>())
        {
            instance->InvalidateLayout();
            return;
        }
    }

    if (auto instance = layout.as<Layout>())
    {
        instance->InvalidateMeasure();
    }
}

/* static */
int LayoutsTestHooks::GetLayoutFirstRealizedItemIndex(
    winrt::IInspectable const& layout)
{
    if (auto instance = layout.as<LinedFlowLayout>())
    {
        return instance->FirstRealizedItemIndexDbg();
    }

    return -1;
}

/* static */
int LayoutsTestHooks::GetLayoutLastRealizedItemIndex(
    winrt::IInspectable const& layout)
{
    if (auto instance = layout.as<LinedFlowLayout>())
    {
        return instance->LastRealizedItemIndexDbg();
    }

    return -1;
}

/* static */
int LayoutsTestHooks::GetLinedFlowLayoutFirstFrozenItemIndex(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->FirstFrozenItemIndexDbg();
    }

    return -1;
}

/* static */
int LayoutsTestHooks::GetLinedFlowLayoutLastFrozenItemIndex(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->LastFrozenItemIndexDbg();
    }

    return -1;
}

/* static */
double LayoutsTestHooks::GetLinedFlowLayoutAverageItemAspectRatio(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->AverageItemAspectRatioDbg();
    }

    return 0.0;
}

/* static */
double LayoutsTestHooks::GetLinedFlowLayoutRawAverageItemsPerLine(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->RawAverageItemsPerLineDbg();
    }

    return 0.0;
}

/* static */
double LayoutsTestHooks::GetLinedFlowLayoutSnappedAverageItemsPerLine(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->SnappedAverageItemsPerLineDbg();
    }

    return 0.0;
}

/* static */
double LayoutsTestHooks::GetLinedFlowLayoutForcedAverageItemAspectRatio(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->ForcedAverageItemAspectRatioDbg();
    }

    return 0.0;
}

/* static */
void LayoutsTestHooks::SetLinedFlowLayoutForcedAverageItemAspectRatio(
    winrt::IInspectable const& linedFlowLayout,
    double forcedAverageItemAspectRatio)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        instance->ForcedAverageItemAspectRatioDbg(forcedAverageItemAspectRatio);
    }
}

/* static */
double LayoutsTestHooks::GetLinedFlowLayoutForcedAverageItemsPerLineDivider(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->ForcedAverageItemsPerLineDividerDbg();
    }

    return 0.0;
}

/* static */
void LayoutsTestHooks::SetLinedFlowLayoutForcedAverageItemsPerLineDivider(
    winrt::IInspectable const& linedFlowLayout,
    double forcedAverageItemsPerLineDivider)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        instance->ForcedAverageItemsPerLineDividerDbg(forcedAverageItemsPerLineDivider);
    }
}

/* static */
double LayoutsTestHooks::GetLinedFlowLayoutForcedWrapMultiplier(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->ForcedWrapMultiplierDbg();
    }

    return 0.0;
}

/* static */
void LayoutsTestHooks::SetLinedFlowLayoutForcedWrapMultiplier(
    winrt::IInspectable const& linedFlowLayout,
    double forcedWrapMultiplier)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        instance->ForcedWrapMultiplierDbg(forcedWrapMultiplier);
    }
}

/* static */
bool LayoutsTestHooks::GetLinedFlowLayoutIsFastPathSupported(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->IsFastPathSupportedDbg();
    }

    return false;
}

/* static */
void LayoutsTestHooks::SetLinedFlowLayoutIsFastPathSupported(
    winrt::IInspectable const& linedFlowLayout,
    bool isFastPathSupported)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        instance->IsFastPathSupportedDbg(isFastPathSupported);
    }
}

/* static */
int LayoutsTestHooks::GetLinedFlowLayoutLogItemIndex(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->LogItemIndexDbg();
    }

    return -1;
}

/* static */
void LayoutsTestHooks::SetLinedFlowLayoutLogItemIndex(
    winrt::IInspectable const& linedFlowLayout,
    int logItemIndex)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        instance->LogItemIndexDbg(logItemIndex);
    }
}

/* static */
int LayoutsTestHooks::GetLinedFlowLayoutLineIndex(
    winrt::IInspectable const& linedFlowLayout,
    int itemIndex)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        return instance->GetLineIndexDbg(itemIndex);
    }

    return -1;
}

/* static */
void LayoutsTestHooks::ClearLinedFlowLayoutItemAspectRatios(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        instance->ClearItemAspectRatios();
    }
}

/* static */
void LayoutsTestHooks::UnlockLinedFlowLayoutItems(
    winrt::IInspectable const& linedFlowLayout)
{
    if (auto instance = linedFlowLayout.as<LinedFlowLayout>())
    {
        instance->UnlockItems();
    }
}

/* static */
winrt::event_token LayoutsTestHooks::LinedFlowLayoutSnappedAverageItemsPerLineChanged(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value)
{
    EnsureHooks();

    return s_testHooks->m_linedFlowLayoutSnappedAverageItemsPerLineChangedEventSource.add(value);
}

/* static */
void LayoutsTestHooks::LinedFlowLayoutSnappedAverageItemsPerLineChanged(winrt::event_token const& token)
{
    EnsureHooks();

    s_testHooks->m_linedFlowLayoutSnappedAverageItemsPerLineChangedEventSource.remove(token);
}

/* static */
void LayoutsTestHooks::NotifyLinedFlowLayoutSnappedAverageItemsPerLineChanged(
    winrt::IInspectable const& linedFlowLayout)
{
    EnsureHooks();

    if (s_testHooks->m_linedFlowLayoutSnappedAverageItemsPerLineChangedEventSource)
    {
        s_testHooks->m_linedFlowLayoutSnappedAverageItemsPerLineChangedEventSource(linedFlowLayout, nullptr);
    }
}

/* static */
winrt::event_token LayoutsTestHooks::LinedFlowLayoutInvalidated(winrt::TypedEventHandler<winrt::IInspectable, winrt::LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs> const& value)
{
    EnsureHooks();

    return s_testHooks->m_linedFlowLayoutInvalidatedEventSource.add(value);
}

/* static */
void LayoutsTestHooks::LinedFlowLayoutInvalidated(winrt::event_token const& token)
{
    EnsureHooks();

    s_testHooks->m_linedFlowLayoutInvalidatedEventSource.remove(token);
}

/* static */
void LayoutsTestHooks::NotifyLinedFlowLayoutInvalidated(
    winrt::IInspectable const& linedFlowLayout,
    winrt::LinedFlowLayoutInvalidationTrigger const& invalidationTrigger)
{
    EnsureHooks();

    if (s_testHooks->m_linedFlowLayoutInvalidatedEventSource)
    {
        auto linedFlowLayoutInvalidatedEventArgs = winrt::make<LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs>(invalidationTrigger);

        s_testHooks->m_linedFlowLayoutInvalidatedEventSource(linedFlowLayout, linedFlowLayoutInvalidatedEventArgs);
    }
}

/* static */
winrt::event_token LayoutsTestHooks::LinedFlowLayoutItemLocked(winrt::TypedEventHandler<winrt::IInspectable, winrt::LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs> const& value)
{
    EnsureHooks();

    return s_testHooks->m_linedFlowLayoutItemLockedEventSource.add(value);
}

/* static */
void LayoutsTestHooks::LinedFlowLayoutItemLocked(winrt::event_token const& token)
{
    EnsureHooks();

    s_testHooks->m_linedFlowLayoutItemLockedEventSource.remove(token);
}

/* static */
void LayoutsTestHooks::NotifyLinedFlowLayoutItemLocked(
    winrt::IInspectable const& linedFlowLayout,
    int itemIndex,
    int lineIndex)
{
    EnsureHooks();

    if (s_testHooks->m_linedFlowLayoutItemLockedEventSource)
    {
        auto linedFlowLayoutItemLockedEventArgs = winrt::make<LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs>(itemIndex, lineIndex);

        s_testHooks->m_linedFlowLayoutItemLockedEventSource(linedFlowLayout, linedFlowLayoutItemLockedEventArgs);
    }
}
