// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include <DoubleUtil.h>
#include "TypeLogging.h"
#include "ItemsView.h"
#include "RuntimeProfiler.h"
#include "ItemsViewTestHooks.h"
#include "ItemsViewItemInvokedEventArgs.h"
#include "NullSelector.h"
#include "SingleSelector.h"
#include "ItemContainerRevokers.h"

#pragma region IControlOverrides

void ItemsView::OnKeyDown(
    winrt::KeyRoutedEventArgs const& args)
{
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::KeyRoutedEventArgsToString(args).c_str());

    __super::OnKeyDown(args);

    if (args.Handled())
    {
        return;
    }

    switch (args.Key())
    {
    case winrt::VirtualKey::A:
    {
        if (m_selector != nullptr)
        {
            const winrt::ItemsViewSelectionMode selectionMode = SelectionMode();

            if (selectionMode != winrt::ItemsViewSelectionMode::None &&
                selectionMode != winrt::ItemsViewSelectionMode::Single &&
                (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down)
            {
                m_selector->SelectAll();
                args.Handled(true);
            }
        }
        break;
    }
    }
}

#pragma endregion

// Returns True when the provided virtual key and navigation key are canceling each other.
bool ItemsView::AreNavigationKeysOpposite(
    const winrt::VirtualKey& key1,
    const winrt::VirtualKey& key2) const
{
    MUX_ASSERT(IsNavigationKey(key1));
    MUX_ASSERT(IsNavigationKey(key2));

    return (key1 == winrt::VirtualKey::Left && key2 == winrt::VirtualKey::Right) ||
           (key1 == winrt::VirtualKey::Right && key2 == winrt::VirtualKey::Left) ||
           (key1 == winrt::VirtualKey::Up && key2 == winrt::VirtualKey::Down) ||
           (key1 == winrt::VirtualKey::Down && key2 == winrt::VirtualKey::Up);
}

// Returns True when ScrollView.ComputedVerticalScrollMode is Enabled.
bool ItemsView::CanScrollVertically()
{
    if (auto const& scrollView = m_scrollView.get())
    {
        return scrollView.ComputedVerticalScrollMode() == winrt::ScrollingScrollMode::Enabled;
    }
    return false;
}

// Returns the index of the closest focusable element to the current element following the provided direction, or -1 when no element was found.
// hasIndexBasedLayoutOrientation indicates whether the Layout has one orientation that has an index-based layout.
int ItemsView::GetAdjacentFocusableElementByDirection(
    const winrt::FocusNavigationDirection& focusNavigationDirection,
    bool hasIndexBasedLayoutOrientation)
{
    int currentElementIndex = GetCurrentElementIndex();

    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, focusNavigationDirection, currentElementIndex);

    MUX_ASSERT(
        focusNavigationDirection == winrt::FocusNavigationDirection::Up ||
        focusNavigationDirection == winrt::FocusNavigationDirection::Down ||
        focusNavigationDirection == winrt::FocusNavigationDirection::Left ||
        focusNavigationDirection == winrt::FocusNavigationDirection::Right);

    if (currentElementIndex == -1)
    {
        return -1;
    }

    auto currentElement = TryGetElement(currentElementIndex);

    if (currentElement == nullptr)
    {
        const bool startBringItemIntoViewSuccess = StartBringItemIntoViewInternal(true /*throwOutOfBounds*/, false /* throwOnAnyFailure */, currentElementIndex, nullptr /* options */);

        currentElementIndex = GetCurrentElementIndex();

        if (!startBringItemIntoViewSuccess || currentElementIndex == -1)
        {
            return -1;
        }

        currentElement = TryGetElement(currentElementIndex);

        if (currentElement == nullptr)
        {
            return -1;
        }
    }

    auto const& itemsRepeater = m_itemsRepeater.get();

    MUX_ASSERT(itemsRepeater != nullptr);

    auto const& itemsSourceView = itemsRepeater.ItemsSourceView();

    MUX_ASSERT(itemsSourceView != nullptr);

    const int itemsCount = itemsSourceView.Count();

    MUX_ASSERT(itemsCount > 0);

    const bool useKeyboardNavigationReferenceHorizontalOffset =
        focusNavigationDirection == winrt::FocusNavigationDirection::Up ||
        focusNavigationDirection == winrt::FocusNavigationDirection::Down;

    MUX_ASSERT((useKeyboardNavigationReferenceHorizontalOffset && m_keyboardNavigationReferenceRect.X != -1.0f) ||
        (!useKeyboardNavigationReferenceHorizontalOffset && m_keyboardNavigationReferenceRect.Y != -1.0f));

    const winrt::Rect currentElementRect = GetElementRect(currentElement, itemsRepeater);
    const winrt::Point keyboardNavigationReferenceOffsetPoint = GetUpdatedKeyboardNavigationReferenceOffset();
    const float keyboardNavigationReferenceOffset = useKeyboardNavigationReferenceHorizontalOffset ? keyboardNavigationReferenceOffsetPoint.X : keyboardNavigationReferenceOffsetPoint.Y;

    MUX_ASSERT(keyboardNavigationReferenceOffset != -1.0f);

    bool getPreviousFocusableElement = focusNavigationDirection == winrt::FocusNavigationDirection::Up || focusNavigationDirection == winrt::FocusNavigationDirection::Left;
    bool traversalDirectionChanged = false;
    int closestElementIndex = -1;
    int itemIndex = getPreviousFocusableElement ? currentElementIndex - 1 : currentElementIndex + 1;
    float smallestDistance = std::numeric_limits<float>::max();
    float smallestNavigationDirectionDistance = std::numeric_limits<float>::max();
    float smallestNoneNavigationDirectionDistance = std::numeric_limits<float>::max();
    const double roundingScaleFactor = GetRoundingScaleFactor(itemsRepeater);

    while ((getPreviousFocusableElement && itemIndex >= 0) || (!getPreviousFocusableElement && itemIndex < itemsCount))
    {
        auto element = itemsRepeater.TryGetElement(itemIndex);

        if (element == nullptr)
        {
            if (hasIndexBasedLayoutOrientation)
            {
                if (smallestNoneNavigationDirectionDistance == std::numeric_limits<float>::max())
                {
                    // Ran out of realized items and smallestNoneNavigationDirectionDistance still has its initial float::max value.
                    // Assuming no qualifying item will be found, return -1 instead of realizing more items and bringing them into view.
                    return -1;
                }

                // Bring the still unrealized item into view to realize it.
                const bool startBringItemIntoViewSuccess = StartBringItemIntoViewInternal(true /* throwOutOfBounds */, false /* throwOnAnyFailure */, itemIndex, nullptr /*options*/);

                if (!startBringItemIntoViewSuccess)
                {
                    return -1;
                }

                element = itemsRepeater.TryGetElement(itemIndex);
            }
            else
            {
                // When the Layout has no index-based orientation, all items are traversed to find the closest one.
                // First a traversal from the current item to one end, and then from the current item to the other end.
                if (traversalDirectionChanged)
                {
                    // Both traversals were performed. Return the resulting closest index.
                    return closestElementIndex;
                }
                else
                {
                    // Do the second traversal in the opposite direction.
                    traversalDirectionChanged = true;
                    getPreviousFocusableElement = !getPreviousFocusableElement;
                    itemIndex = getPreviousFocusableElement ? currentElementIndex - 1 : currentElementIndex + 1;
                    continue;
                }
            }
        }

        MUX_ASSERT(element != nullptr);

        if (IsFocusableElement(element))
        {
            float navigationDirectionDistance = std::numeric_limits<float>::max();
            float noneNavigationDirectionDistance = std::numeric_limits<float>::max();

            GetDistanceToKeyboardNavigationReferenceOffset(
                focusNavigationDirection,
                currentElementRect,
                element,
                itemsRepeater,
                keyboardNavigationReferenceOffset,
                roundingScaleFactor,
                &navigationDirectionDistance,
                &noneNavigationDirectionDistance);

            MUX_ASSERT(navigationDirectionDistance >= 0.0f);
            MUX_ASSERT(noneNavigationDirectionDistance >= 0.0f);

            if (navigationDirectionDistance <= 1.0f / static_cast<float>(roundingScaleFactor) && noneNavigationDirectionDistance <= 1.0f / static_cast<float>(roundingScaleFactor))
            {
                // Stop the search right away since an element at the target point was found.
                return itemIndex;
            }
            else if (hasIndexBasedLayoutOrientation)
            {
                // When the Layout has an index-based orientation, its orthogonal orientation defines the primary (favored) distance. The index-based orientation defines a secondary distance.
                if (noneNavigationDirectionDistance < smallestNoneNavigationDirectionDistance ||
                    (noneNavigationDirectionDistance == smallestNoneNavigationDirectionDistance && navigationDirectionDistance < smallestNavigationDirectionDistance))
                {
                    smallestNoneNavigationDirectionDistance = noneNavigationDirectionDistance;
                    smallestNavigationDirectionDistance = navigationDirectionDistance;
                    closestElementIndex = itemIndex;
                }
                else if (noneNavigationDirectionDistance > smallestNoneNavigationDirectionDistance)
                {
                    return closestElementIndex;
                }
            }
            else
            {
                // When the Layout has no index-based orientation, the typical Euclidean distance is used.
                const float distance = std::pow(navigationDirectionDistance, 2.0f) + std::pow(noneNavigationDirectionDistance, 2.0f);

                if (distance < smallestDistance)
                {
                    smallestDistance = distance;
                    closestElementIndex = itemIndex;
                }
            }
        }

        itemIndex = getPreviousFocusableElement ? itemIndex - 1 : itemIndex + 1;
    }

    return closestElementIndex;
}

// Returns the index of the previous or next focusable element from the current element, or -1 when no element was found.
int ItemsView::GetAdjacentFocusableElementByIndex(
    bool getPreviousFocusableElement)
{
    int currentElementIndex = GetCurrentElementIndex();

    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT_INT, METH_NAME, this, getPreviousFocusableElement, currentElementIndex);

    if (currentElementIndex == -1)
    {
        return -1;
    }

    auto const& currentElement = TryGetElement(currentElementIndex);

    if (currentElement == nullptr)
    {
        // Realize the current element so its neighbors are available for evaluation.
        const bool startBringItemIntoViewSuccess = StartBringItemIntoViewInternal(false /*throwOutOfBounds*/, false /* throwOnAnyFailure */, currentElementIndex, nullptr /*options*/);

        currentElementIndex = GetCurrentElementIndex();

        if (!startBringItemIntoViewSuccess || currentElementIndex == -1 || TryGetElement(currentElementIndex) == nullptr)
        {
            return -1;
        }
    }

    auto const& itemsRepeater = m_itemsRepeater.get();

    MUX_ASSERT(itemsRepeater != nullptr);

    auto const& itemsSourceView = itemsRepeater.ItemsSourceView();

    MUX_ASSERT(itemsSourceView != nullptr);

    const int itemsCount = itemsSourceView.Count();

    MUX_ASSERT(itemsCount > 0);

    // Because we are dealing with an index-based layout, the search is only done in one direction.
    int itemIndex = getPreviousFocusableElement ? currentElementIndex - 1 : currentElementIndex + 1;

    while ((getPreviousFocusableElement && itemIndex >= 0) || (!getPreviousFocusableElement && itemIndex < itemsCount))
    {
        auto element = itemsRepeater.TryGetElement(itemIndex);

        if (element == nullptr)
        {
            const bool startBringItemIntoViewSuccess = StartBringItemIntoViewInternal(false /*throwOutOfBounds*/, false /* throwOnAnyFailure */, itemIndex, nullptr /*options*/);

            if (!startBringItemIntoViewSuccess)
            {
                return -1;
            }

            element = itemsRepeater.TryGetElement(itemIndex);
        }

        MUX_ASSERT(element != nullptr);

        if (IsFocusableElement(element))
        {
            return itemIndex;
        }

        itemIndex = getPreviousFocusableElement ? itemIndex - 1 : itemIndex + 1;
    }

    return -1;
}

// When focusNavigationDirection is FocusNavigationDirection::Up or FocusNavigationDirection::Down, keyboardNavigationReferenceOffset indicates a vertical line
// to get the distance from 'element'. Otherwise keyboardNavigationReferenceOffset indicates a horizontal line.
void ItemsView::GetDistanceToKeyboardNavigationReferenceOffset(
    const winrt::FocusNavigationDirection& focusNavigationDirection,
    const winrt::Rect& currentElementRect,
    const winrt::UIElement& element,
    const winrt::ItemsRepeater& itemsRepeater,
    float keyboardNavigationReferenceOffset,
    double roundingScaleFactor,
    float* navigationDirectionDistance,
    float* noneNavigationDirectionDistance) const
{
    MUX_ASSERT(focusNavigationDirection == winrt::FocusNavigationDirection::Up ||
        focusNavigationDirection == winrt::FocusNavigationDirection::Down ||
        focusNavigationDirection == winrt::FocusNavigationDirection::Left ||
        focusNavigationDirection == winrt::FocusNavigationDirection::Right);
    MUX_ASSERT(element != nullptr);
    MUX_ASSERT(itemsRepeater != nullptr);
    MUX_ASSERT(navigationDirectionDistance != nullptr);
    MUX_ASSERT(noneNavigationDirectionDistance != nullptr);
    MUX_ASSERT(keyboardNavigationReferenceOffset != -1.0f);
    MUX_ASSERT(roundingScaleFactor > 0.0);

    *noneNavigationDirectionDistance = *navigationDirectionDistance = std::numeric_limits<float>::max();

    winrt::Rect elementRect = GetElementRect(element, itemsRepeater);
    const double roundingMargin = roundingScaleFactor <= 1.0 ? 0.5 : 2.0 / roundingScaleFactor;

    if (focusNavigationDirection == winrt::FocusNavigationDirection::Up &&
        elementRect.Y + elementRect.Height > currentElementRect.Y + roundingMargin)
    {
        // This element is disqualified because it is not placed at the top of currentElement.
        return;
    }

    if (focusNavigationDirection == winrt::FocusNavigationDirection::Down &&
        elementRect.Y + roundingMargin < currentElementRect.Y + currentElementRect.Height)
    {
        // This element is disqualified because it is not placed at the bottom of currentElement.
        return;
    }

    if (focusNavigationDirection == winrt::FocusNavigationDirection::Left &&
        elementRect.X + elementRect.Width > currentElementRect.X + roundingMargin)
    {
        // This element is disqualified because it is not placed at the left of currentElement.
        return;
    }

    if (focusNavigationDirection == winrt::FocusNavigationDirection::Right &&
        elementRect.X + roundingMargin < currentElementRect.X + currentElementRect.Width)
    {
        // This element is disqualified because it is not placed at the right of currentElement.
        return;
    }

    switch (focusNavigationDirection)
    {
    case winrt::FocusNavigationDirection::Up:
        *noneNavigationDirectionDistance = currentElementRect.Y - elementRect.Y - elementRect.Height;
        break;
    case winrt::FocusNavigationDirection::Down:
        *noneNavigationDirectionDistance = elementRect.Y - currentElementRect.Y - currentElementRect.Height;
        break;
    case winrt::FocusNavigationDirection::Left:
        *noneNavigationDirectionDistance = currentElementRect.X - elementRect.X - elementRect.Width;
        break;
    case winrt::FocusNavigationDirection::Right:
        *noneNavigationDirectionDistance = elementRect.X - currentElementRect.X - currentElementRect.Width;
        break;
    }

    MUX_ASSERT(*noneNavigationDirectionDistance >= -roundingMargin);

    *noneNavigationDirectionDistance = std::max(0.0f, *noneNavigationDirectionDistance);

    if (focusNavigationDirection == winrt::FocusNavigationDirection::Up || focusNavigationDirection == winrt::FocusNavigationDirection::Down)
    {
        *navigationDirectionDistance = std::abs(elementRect.X + elementRect.Width / 2.0f - keyboardNavigationReferenceOffset);
    }
    else
    {
        *navigationDirectionDistance = std::abs(elementRect.Y + elementRect.Height / 2.0f - keyboardNavigationReferenceOffset);
    }
}

// Returns the position within the ItemsRepeater + size of the provided element as a Rect.
// The potential element.Margin is not included in the returned rectangle.
winrt::Rect ItemsView::GetElementRect(
    const winrt::UIElement& element,
    const winrt::ItemsRepeater& itemsRepeater) const
{
    MUX_ASSERT(element != nullptr);
    MUX_ASSERT(itemsRepeater != nullptr);

    auto const generalTransform = element.TransformToVisual(itemsRepeater);
    auto const elementOffset = generalTransform.TransformPoint(winrt::Point(0, 0));

    if (auto const elementAsFE = element.as<winrt::FrameworkElement>())
    {
        auto const farSideOffset = generalTransform.TransformPoint(winrt::Point(
            static_cast<float>(elementAsFE.ActualWidth()),
            static_cast<float>(elementAsFE.ActualHeight())));

        return winrt::Rect{
            elementOffset.X,
            elementOffset.Y,
            farSideOffset.X - elementOffset.X,
            farSideOffset.Y - elementOffset.Y };
    }
    else
    {
        return winrt::Rect{
            elementOffset.X,
            elementOffset.Y,
            0.0f,
            0.0f };
    }
}

winrt::IndexBasedLayoutOrientation ItemsView::GetLayoutIndexBasedLayoutOrientation()
{
    if (auto const& layout = Layout())
    {
        return layout.IndexBasedLayoutOrientation();
    }
    return winrt::IndexBasedLayoutOrientation::None;
}

// Returns the number of most recently queued navigation keys of the same kind.
int ItemsView::GetTrailingNavigationKeyCount() const
{
    MUX_ASSERT(!m_navigationKeysToProcess.empty());

    winrt::VirtualKey lastNavigationKey = m_navigationKeysToProcess.back();
    int count = 0;
    auto iterator = m_navigationKeysToProcess.end();
    iterator--;

    while (*iterator == lastNavigationKey)
    {
        count++;
        if (iterator == m_navigationKeysToProcess.begin())
        {
            break;
        }
        iterator--;
    }

    MUX_ASSERT(count > 0);

    return count;
}

// Returns the Point within the ItemsRepeater representing the reference for non-index-based orientation keyboard navigation.
// Both X and Y are relevant for Layouts that have no index-based orientation.
winrt::Point ItemsView::GetUpdatedKeyboardNavigationReferenceOffset()
{
    if (m_keyboardNavigationReferenceIndex != -1)
    {
        MUX_ASSERT(m_keyboardNavigationReferenceRect.X != -1.0f);
        MUX_ASSERT(m_keyboardNavigationReferenceRect.Y != -1.0f);

        if (auto const& itemsRepeater = m_itemsRepeater.get())
        {
            if (auto const& keyboardNavigationReferenceElement = itemsRepeater.TryGetElement(m_keyboardNavigationReferenceIndex))
            {
                winrt::Rect keyboardNavigationReferenceRect = GetElementRect(keyboardNavigationReferenceElement, itemsRepeater);

                if (keyboardNavigationReferenceRect.X + keyboardNavigationReferenceRect.Width >= 0 && keyboardNavigationReferenceRect.Y + keyboardNavigationReferenceRect.Height >= 0)
                {
                    if (keyboardNavigationReferenceRect != m_keyboardNavigationReferenceRect)
                    {
                        UpdateKeyboardNavigationReference();
                    }
                }
                // Else the keyboard navigation reference element was pinned and placed out of bounds. Use the cached m_keyboardNavigationReferenceRect.
            }
            // Else the keyboard navigation reference element was unrealized. Use the cached m_keyboardNavigationReferenceRect.
        }
    }

    MUX_ASSERT((m_keyboardNavigationReferenceRect.X == -1.0f && m_keyboardNavigationReferenceIndex == -1) || (m_keyboardNavigationReferenceRect.X != -1.0f && m_keyboardNavigationReferenceIndex != -1));

    return GetKeyboardNavigationReferenceOffset();
}

// Returns True when the provided incoming navigation key is canceled
// - because of input throttling
// - because it is the opposite of the last queued key.
// This method also clears all queued up keys when the incoming key is Home or End.
bool ItemsView::IsCancelingNavigationKey(
    const winrt::VirtualKey& key,
    bool isRepeatKey)
{
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"key", key);
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"isRepeatKey", isRepeatKey);
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"queued keys size", m_navigationKeysToProcess.size());

    MUX_ASSERT(IsNavigationKey(key));

    if (m_navigationKeysToProcess.empty())
    {
        return false;
    }

    // Maximum number of unprocessed keystrokes of the same kind being queued in m_navigationKeysToProcess.
    constexpr int maxRepeatQueuedNavigationKeys = 3;    // Do not queue more than 3 identical navigation keys with a repeat count > 1.
    constexpr int maxNonRepeatQueuedNavigationKeys = 6; // Do not queue more than 6 identical navigation keys with a repeat count = 1.
    // Keystrokes without holding down the key are more likely to have a repeat count of 1 and are more liberally queued up because more intentional.

    winrt::VirtualKey lastNavigationKey = m_navigationKeysToProcess.back();

    if (lastNavigationKey == key)
    {
        // Incoming key is identical to the last queued up.
        const int trailingNavigationKeyCount = GetTrailingNavigationKeyCount();

        if ((trailingNavigationKeyCount >= maxRepeatQueuedNavigationKeys && isRepeatKey) ||
            (trailingNavigationKeyCount >= maxNonRepeatQueuedNavigationKeys && !isRepeatKey))
        {
            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Navigation keys throttling.");

            // Incoming key is canceled to avoid lenghty processing after keyboard input stops.
            return true;
        }
    }

    switch (key)
    {
    case winrt::VirtualKey::Home:
    case winrt::VirtualKey::End:
    {
        // Any navigation key queued before a Home or End key can be canceled.
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Navigation keys list cleared for Home/End.");

        m_navigationKeysToProcess.clear();
        break;
    }
    default:
    {
        if (AreNavigationKeysOpposite(key, lastNavigationKey))
        {
            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Incoming key and last queued navigation key canceled.");

            // The incoming key and last queued navigation key are canceling each other and neither need to be processed.
            m_navigationKeysToProcess.pop_back();
            return true;
        }
        break;
    }
    }

    return false;
}

bool ItemsView::IsFocusableElement(
    winrt::UIElement const& element) const
{
    if (element.Visibility() == winrt::Visibility::Collapsed)
    {
        return false;
    }

    auto const& control = element.try_as<winrt::Control>();

    return control && (control.IsEnabled() || control.AllowFocusWhenDisabled()) && control.IsTabStop();
}

bool ItemsView::IsLayoutOrientationIndexBased(bool horizontal)
{
    const winrt::IndexBasedLayoutOrientation indexBasedLayoutOrientation = GetLayoutIndexBasedLayoutOrientation();

    return (horizontal && indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::LeftToRight) ||
        (!horizontal && indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::TopToBottom);
}

bool ItemsView::IsNavigationKey(
    const winrt::VirtualKey& key) const
{
    switch (key)
    {
    case winrt::VirtualKey::Home:
    case winrt::VirtualKey::End:
    case winrt::VirtualKey::Left:
    case winrt::VirtualKey::Right:
    case winrt::VirtualKey::Up:
    case winrt::VirtualKey::Down:
    case winrt::VirtualKey::PageUp:
    case winrt::VirtualKey::PageDown:
        return true;
    }

    return false;
}

void ItemsView::OnItemsViewElementGettingFocus(
    const winrt::UIElement& element,
    const winrt::GettingFocusEventArgs& args)
{
    auto const& focusState = args.FocusState();

    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, TypeLogging::FocusStateToString(focusState).c_str());

    if (focusState == winrt::FocusState::Keyboard)
    {
        const int elementIndex = GetElementIndex(element);
        int currentElementIndex = GetCurrentElementIndex();
        bool focusMovingIntoItemsRepeater{ true };

        // Check if the focus comes from an element outside the scope of the inner ItemsRepeater.
        if (auto const& itemsRepeater = m_itemsRepeater.get())
        {
            auto const& oldFocusedElement = args.OldFocusedElement();

            if (oldFocusedElement && SharedHelpers::IsAncestor(oldFocusedElement, itemsRepeater, false /*checkVisibility*/))
            {
                focusMovingIntoItemsRepeater = false;
            }
        }

        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"ElementIndex", elementIndex);
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"CurrentElementIndex", currentElementIndex);
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"FocusMovingIntoItemsRepeater", focusMovingIntoItemsRepeater);
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Direction", static_cast<int>(args.Direction()));

        if (currentElementIndex != elementIndex && focusMovingIntoItemsRepeater)
        {
            if (currentElementIndex == -1)
            {
                auto const& direction = args.Direction();

                if (direction == winrt::FocusNavigationDirection::Previous || direction == winrt::FocusNavigationDirection::Next)
                {
                    // Tabbing (direction == FocusNavigationDirection::Next) or Shift-Tabbing (direction == FocusNavigationDirection::Previous) into
                    // the ItemsRepeater while there is no current element.

                    // Retrieve the focusable index on the top/left corner for Tabbing, or bottom/right corner for Shift-Tabbing.
                    int cornerFocusableItem = ItemsView::GetCornerFocusableItem(direction == winrt::FocusNavigationDirection::Next /*isForTopLeftItem*/);
                    MUX_ASSERT(cornerFocusableItem != -1);

                    // Set that index as the current one.
                    SetCurrentElementIndex(cornerFocusableItem, winrt::FocusState::Unfocused, false /*forceKeyboardNavigationReferenceReset*/);
                    MUX_ASSERT(cornerFocusableItem == GetCurrentElementIndex());

                    // Allow TrySetNewFocusedElement to be called below for that new current element.
                    currentElementIndex = cornerFocusableItem;
                }
            }

            if (currentElementIndex != -1)
            {
                // The ItemsView has a current element other than the one receiving focus, and focus is moving into the ItemsRepeater.
                auto currentElement = TryGetElement(currentElementIndex);

                if (currentElement == nullptr)
                {
                    // Realize the current element.
                    const bool startBringItemIntoViewSuccess = StartBringItemIntoViewInternal(true /* throwOutOfBounds*/, false /* throwOnAnyFailure */, currentElementIndex, nullptr /*options*/);

                    currentElementIndex = GetCurrentElementIndex();

                    if (startBringItemIntoViewSuccess && currentElementIndex != -1)
                    {
                        currentElement = TryGetElement(currentElementIndex);
                    }
                }

                if (currentElement != nullptr)
                {
                    // Redirect focus to the current element.
                    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"Redirecting focus to CurrentElementIndex", currentElementIndex);

                    bool success = args.TrySetNewFocusedElement(currentElement);

                    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"TrySetNewFocusedElement result", success);

                    return;
                }
            }
        }

        if (currentElementIndex != elementIndex)
        {
            SetCurrentElementIndex(elementIndex, winrt::FocusState::Unfocused, false /*forceKeyboardNavigationReferenceReset*/);
        }

        // Selection is not updated when focus moves into the ItemsRepeater.
        if (m_selector != nullptr && !focusMovingIntoItemsRepeater)
        {
            const bool isCtrlDown = (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
            const bool isShiftDown = (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Shift) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

            auto scopeGuard = gsl::finally([this]()
            {
                m_isProcessingInteraction = false;
            });

            m_isProcessingInteraction = true;

            bool isIndexPathValid = false;
            const winrt::IndexPath indexPath = GetElementIndexPath(element, &isIndexPathValid);

            if (isIndexPathValid)
            {
                m_selector->OnFocusedAction(indexPath, isCtrlDown, isShiftDown);
            }
        }
    }
}

// Process the Home/End, arrows and page navigation keystrokes before the inner ScrollView gets a chance to do it by simply scrolling.
void ItemsView::OnItemsViewElementKeyDown(
    const winrt::IInspectable& sender,
    const winrt::KeyRoutedEventArgs& args)
{
    const auto element = sender.try_as<winrt::UIElement>();

    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"sender element index", element ? GetElementIndex(element) : -1);
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"current element index", GetCurrentElementIndex());
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"key", args.Key());
    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"key repeat count", args.KeyStatus().RepeatCount);

    MUX_ASSERT(!args.Handled());

    const auto key = args.Key();

    if (!IsNavigationKey(key))
    {
        return;
    }

    if (IsCancelingNavigationKey(key, args.KeyStatus().RepeatCount > 1 /*isRepeatKey*/))
    {
        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Key canceled.");

        // Mark the event as handled to prevent the outer ScrollView from processing it.
        args.Handled(true);
        return;
    }

    QueueNavigationKey(key);

    if (m_navigationKeyBringIntoViewPendingCount == 0 && m_navigationKeyBringIntoViewCorrelationId == -1 && m_navigationKeyProcessingCountdown == 0)
    {
        // No ItemsView::OnScrollViewBringingIntoView call is pending and there is no count down to a stable layout, so process the key right away.
        if (ProcessNavigationKeys())
        {
            args.Handled(true);
        }
    }
    else
    {
#ifdef DBG
        if (m_navigationKeyBringIntoViewPendingCount > 0)
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Key processing delayed until bring-into-view started & completed.");
        }
        else
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Key processing delayed until bring-into-view completed.");
        }
#endif
        // Even though the key will be processed asynchronously, it is marked as handled to prevent the parents, like the ScrollView, from processing it.
        args.Handled(true);
    }
}

#ifdef DBG
void ItemsView::OnItemsViewElementLosingFocusDbg(
    const winrt::UIElement& element,
    const winrt::LosingFocusEventArgs& args)
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_INT, METH_NAME, this, GetElementIndex(element));
}
#endif

void ItemsView::OnItemsViewItemContainerItemInvoked(
    const winrt::ItemContainer& itemContainer,
    const winrt::ItemContainerInvokedEventArgs& args)
{
    ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, TypeLogging::ItemContainerInteractionTriggerToString(args.InteractionTrigger()).c_str(), GetElementIndex(itemContainer));

    auto const& interactionTrigger = args.InteractionTrigger();
    bool handled = args.Handled();

    switch (interactionTrigger)
    {
        case winrt::ItemContainerInteractionTrigger::PointerReleased:
        {
            handled |= ProcessInteraction(itemContainer, winrt::FocusState::Pointer);
            break;
        }

        case winrt::ItemContainerInteractionTrigger::EnterKey:
        case winrt::ItemContainerInteractionTrigger::SpaceKey:
        {
            handled |= ProcessInteraction(itemContainer, winrt::FocusState::Keyboard);
            break;
        }

        case winrt::ItemContainerInteractionTrigger::Tap:
        case winrt::ItemContainerInteractionTrigger::DoubleTap:
        case winrt::ItemContainerInteractionTrigger::AutomationInvoke:
        {
            break;
        }

        default:
        {
            return;
        }
    }

    if (!args.Handled() &&
        interactionTrigger != winrt::ItemContainerInteractionTrigger::PointerReleased &&
        CanRaiseItemInvoked(interactionTrigger, itemContainer))
    {
        MUX_ASSERT(GetElementIndex(itemContainer) != -1);

        RaiseItemInvoked(itemContainer);
    }

    args.Handled(handled);
}

#ifdef DBG_VERBOSE
void ItemsView::OnItemsViewItemContainerSizeChangedDbg(
    const winrt::IInspectable& sender,
    const winrt::SizeChangedEventArgs& args)
{
    auto const& element = sender.try_as<winrt::UIElement>();

    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"ItemContainer index:", GetElementIndex(element));
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, args.PreviousSize().Width, args.PreviousSize().Height);
    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_FLT_FLT, METH_NAME, this, args.NewSize().Width, args.NewSize().Height);
}
#endif

bool ItemsView::ProcessInteraction(
    const winrt::UIElement& element,
    const winrt::FocusState& focusState)
{
    MUX_ASSERT(element != nullptr);

    const int elementIndex = GetElementIndex(element);

    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, TypeLogging::FocusStateToString(focusState).c_str(), elementIndex);

    MUX_ASSERT(elementIndex >= 0);

    // When the focusState is Pointer, the element not only gets focus but is also brought into view by SetFocusElementIndex's StartBringIntoView call.
    const bool handled = SetCurrentElementIndex(elementIndex, focusState, true /*forceKeyboardNavigationReferenceReset*/, focusState == winrt::FocusState::Pointer /*startBringIntoView*/);

    if (m_selector != nullptr)
    {
        const bool isCtrlDown = (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;
        const bool isShiftDown = (winrt::InputKeyboardSource::GetKeyStateForCurrentThread(winrt::VirtualKey::Shift) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

        auto scopeGuard = gsl::finally([this]()
        {
            m_isProcessingInteraction = false;
        });

        m_isProcessingInteraction = true;

        bool isIndexPathValid = false;
        const winrt::IndexPath indexPath = GetElementIndexPath(element, &isIndexPathValid);

        if (isIndexPathValid)
        {
            m_selector->OnInteractedAction(indexPath, isCtrlDown, isShiftDown);
        }
    }

    return handled;
}

// Processes the queued up navigation keys while there is no pending bring-into-view operation which needs to settle
// before any subsequent processing, so items are realized, layed out and ready for identification of the target item.
bool ItemsView::ProcessNavigationKeys()
{
    MUX_ASSERT(m_navigationKeyBringIntoViewPendingCount == 0);
    MUX_ASSERT(m_navigationKeyBringIntoViewCorrelationId == -1);
    MUX_ASSERT(m_navigationKeyProcessingCountdown == 0);

    while (!m_navigationKeysToProcess.empty() &&
        m_navigationKeyBringIntoViewPendingCount == 0 &&
        m_navigationKeyBringIntoViewCorrelationId == -1 &&
        m_navigationKeyProcessingCountdown == 0)
    {
        const winrt::VirtualKey navigationKey = m_navigationKeysToProcess.front();

        m_lastNavigationKeyProcessed = navigationKey;
        m_navigationKeysToProcess.pop_front();

#ifdef DBG
        switch (navigationKey)
        {
        case winrt::VirtualKey::Left:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Left key dequeued.");
            break;
        }
        case winrt::VirtualKey::Right:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Right key dequeued.");
            break;
        }
        case winrt::VirtualKey::Up:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Up key dequeued.");
            break;
        }
        case winrt::VirtualKey::Down:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Down key dequeued.");
            break;
        }
        case winrt::VirtualKey::PageUp:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"PageUp key dequeued.");
            break;
        }
        case winrt::VirtualKey::PageDown:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"PageDown key dequeued.");
            break;
        }
        case winrt::VirtualKey::Home:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Home key dequeued.");
            break;
        }
        case winrt::VirtualKey::End:
        {
            ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"End key dequeued.");
            break;
        }
        }
#endif

        bool forceKeyboardNavigationReferenceReset = false;
        int newCurrentElementIndexToFocus = -1;

        switch (navigationKey)
        {
        case winrt::VirtualKey::Home:
        case winrt::VirtualKey::End:
        {
            if (auto const& itemsRepeater = m_itemsRepeater.get())
            {
                auto const& itemsSourceView = itemsRepeater.ItemsSourceView();

                if (itemsSourceView == nullptr)
                {
                    return false;
                }

                const int itemsCount = itemsSourceView.Count();

                if (itemsCount == 0)
                {
                    return false;
                }

                const bool targetIsFirstItem = navigationKey == winrt::VirtualKey::Home;
                const int itemIndex = targetIsFirstItem ? 0 : itemsCount - 1;
                winrt::BringIntoViewOptions options;

                // When processing the Home key, the top/left corner of the first focusable element is aligned to the top/left corner of the viewport.
                // When processing the End key, the bottom/right corner of the last focusable element is aligned to the bottom/right corner of the viewport.
                options.HorizontalAlignmentRatio(targetIsFirstItem ? 0.0 : 1.0);
                options.VerticalAlignmentRatio(targetIsFirstItem ? 0.0 : 1.0);

                const bool startBringItemIntoViewSuccess = StartBringItemIntoViewInternal(false /*throwOutOfBounds*/, false /* throwOnAnyFailure */, itemIndex, options);

                if (!startBringItemIntoViewSuccess)
                {
                    return false;
                }

                // Now that the target item is realized and moving into view, check if it needs to take keyboard focus.
                if (auto const& layout = Layout())
                {
                    auto const& focusableObject = targetIsFirstItem ? winrt::FocusManager::FindFirstFocusableElement(itemsRepeater) : winrt::FocusManager::FindLastFocusableElement(itemsRepeater);

                    if (auto const& focusableElement = focusableObject.try_as<winrt::UIElement>())
                    {
                        const int index = GetElementIndex(focusableElement);

                        MUX_ASSERT(index != -1);

                        if (index != GetCurrentElementIndex())
                        {
                            forceKeyboardNavigationReferenceReset = true;
                            newCurrentElementIndexToFocus = index;
                        }
                    }
                }
            }
            break;
        }
        case winrt::VirtualKey::Left:
        case winrt::VirtualKey::Right:
        case winrt::VirtualKey::Up:
        case winrt::VirtualKey::Down:
        {
            const bool isLayoutOrientationIndexBased = IsLayoutOrientationIndexBased(navigationKey == winrt::VirtualKey::Left || navigationKey == winrt::VirtualKey::Right /*horizontal*/);
            const bool isRightToLeftDirection = FlowDirection() == winrt::FlowDirection::RightToLeft;

            if (isLayoutOrientationIndexBased)
            {
                const bool getPreviousFocusableElement =
                    (navigationKey == winrt::VirtualKey::Left && !isRightToLeftDirection) ||
                    (navigationKey == winrt::VirtualKey::Right && isRightToLeftDirection) ||
                    navigationKey == winrt::VirtualKey::Up;
                const int index = GetAdjacentFocusableElementByIndex(getPreviousFocusableElement);

                if (index != -1 && index != GetCurrentElementIndex())
                {
                    forceKeyboardNavigationReferenceReset = true;
                    newCurrentElementIndexToFocus = index;
                }
            }
            else
            {
                const bool hasIndexBasedLayoutOrientation = GetLayoutIndexBasedLayoutOrientation() != winrt::IndexBasedLayoutOrientation::None;
                winrt::FocusNavigationDirection focusNavigationDirection;

                switch (navigationKey)
                {
                case winrt::VirtualKey::Left:
                    focusNavigationDirection = isRightToLeftDirection ? winrt::FocusNavigationDirection::Right : winrt::FocusNavigationDirection::Left;
                    break;
                case winrt::VirtualKey::Right:
                    focusNavigationDirection = isRightToLeftDirection ? winrt::FocusNavigationDirection::Left : winrt::FocusNavigationDirection::Right;
                    break;
                case winrt::VirtualKey::Up:
                    focusNavigationDirection = winrt::FocusNavigationDirection::Up;
                    break;
                case winrt::VirtualKey::Down:
                    focusNavigationDirection = winrt::FocusNavigationDirection::Down;
                    break;
                }

                const int index = GetAdjacentFocusableElementByDirection(focusNavigationDirection, hasIndexBasedLayoutOrientation);

                if (index != -1 && index != GetCurrentElementIndex())
                {
                    if (!hasIndexBasedLayoutOrientation)
                    {
                        forceKeyboardNavigationReferenceReset = true;
                    }

                    newCurrentElementIndexToFocus = index;
                }
            }
            break;
        }
        case winrt::VirtualKey::PageUp:
        case winrt::VirtualKey::PageDown:
        {
            const winrt::IndexBasedLayoutOrientation indexBasedLayoutOrientation = GetLayoutIndexBasedLayoutOrientation();
            const bool isHorizontalDistanceFavored = indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::TopToBottom;
            const bool isVerticalDistanceFavored = indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::LeftToRight;
            const bool isForPageUp = navigationKey == winrt::VirtualKey::PageUp;
            const bool isPageNavigationRailed{ true }; // Keeping this variable for now. It could be set to an ItemsView.IsPageNavigationRailed() property to opt out of railing.
            bool useKeyboardNavigationReferenceHorizontalOffset{ false };
            bool useKeyboardNavigationReferenceVerticalOffset{ false };
            double horizontalViewportRatio{}, verticalViewportRatio{};

            // First phase: Check if target element is on the current page.
            if (isVerticalDistanceFavored || CanScrollVertically())
            {
                if (isPageNavigationRailed)
                {
                    useKeyboardNavigationReferenceHorizontalOffset = true;
                }
                else
                {
                    horizontalViewportRatio = isForPageUp ? -std::numeric_limits<double>::max() : std::numeric_limits<double>::max();
                }

                verticalViewportRatio = isForPageUp ? 0.0 : 1.0;
            }
            else if (isHorizontalDistanceFavored)
            {
                horizontalViewportRatio = isForPageUp ? 0.0 : 1.0;

                if (isPageNavigationRailed)
                {
                    useKeyboardNavigationReferenceVerticalOffset = true;
                }
                else
                {
                    verticalViewportRatio = isForPageUp ? -std::numeric_limits<double>::max() : std::numeric_limits<double>::max();
                }
            }

            MUX_ASSERT(!isPageNavigationRailed || horizontalViewportRatio == 0.0 || verticalViewportRatio == 0.0);
            MUX_ASSERT(!useKeyboardNavigationReferenceHorizontalOffset || !useKeyboardNavigationReferenceVerticalOffset);

            int index = GetItemInternal(
                horizontalViewportRatio,
                verticalViewportRatio,
                isHorizontalDistanceFavored,
                isVerticalDistanceFavored,
                useKeyboardNavigationReferenceHorizontalOffset,
                useKeyboardNavigationReferenceVerticalOffset,
                true /*capItemEdgesToViewportRatioEdges*/,
                true /*forFocusableItemsOnly*/);

            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, isForPageUp ? L"PageUp - phase 1" : L"PageDown - phase 1", index);

            if (index != -1)
            {
                if (!isPageNavigationRailed || indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::None)
                {
                    forceKeyboardNavigationReferenceReset = true;
                }

                if (index != GetCurrentElementIndex())
                {
                    // Target element is on the current page.
                    newCurrentElementIndexToFocus = index;
                }
                else
                {
                    // Find target on the neighboring page.
                    if (isVerticalDistanceFavored || CanScrollVertically())
                    {
                        verticalViewportRatio = isForPageUp ? -1.0 : 2.0;
                    }
                    else if (isHorizontalDistanceFavored)
                    {
                        horizontalViewportRatio = isForPageUp ? -1.0 : 2.0;
                    }

                    MUX_ASSERT(!isPageNavigationRailed || horizontalViewportRatio == 0.0 || verticalViewportRatio == 0.0);

                    index = GetItemInternal(
                        horizontalViewportRatio,
                        verticalViewportRatio,
                        isHorizontalDistanceFavored,
                        isVerticalDistanceFavored,
                        useKeyboardNavigationReferenceHorizontalOffset,
                        useKeyboardNavigationReferenceVerticalOffset,
                        true /*capItemEdgesToViewportRatioEdges*/,
                        true /*forFocusableItemsOnly*/);

                    ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, isForPageUp ? L"PageUp - phase 2" : L"PageDown - phase 2", index);

                    if (index != -1)
                    {
                        if (index != GetCurrentElementIndex())
                        {
                            // Found target element on neighboring page.
                            newCurrentElementIndexToFocus = index;
                        }
                        else if (isPageNavigationRailed)
                        {
                            MUX_ASSERT(useKeyboardNavigationReferenceHorizontalOffset || useKeyboardNavigationReferenceVerticalOffset);

                            // Beginning or end of items reached while railing is turned on. Turn it off and try again.
                            if (isVerticalDistanceFavored || CanScrollVertically())
                            {
                                horizontalViewportRatio = isForPageUp ? -std::numeric_limits<double>::max() : std::numeric_limits<double>::max();
                                verticalViewportRatio = isForPageUp ? 0.0 : 1.0;
                            }
                            else if (isHorizontalDistanceFavored)
                            {
                                horizontalViewportRatio = isForPageUp ? 0.0 : 1.0;
                                verticalViewportRatio = isForPageUp ? -std::numeric_limits<double>::max() : std::numeric_limits<double>::max();
                            }

                            index = GetItemInternal(
                                horizontalViewportRatio,
                                verticalViewportRatio,
                                isHorizontalDistanceFavored,
                                isVerticalDistanceFavored,
                                false /*useKeyboardNavigationReferenceHorizontalOffset*/,
                                false /*useKeyboardNavigationReferenceVerticalOffset*/,
                                true  /*capItemEdgesToViewportRatioEdges*/,
                                true  /*forFocusableItemsOnly*/);

                            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, isForPageUp ? L"PageUp - phase 3" : L"PageDown - phase 3", index);

                            if (index != -1 && index != GetCurrentElementIndex())
                            {
                                // Target element is first or last focusable element.
                                forceKeyboardNavigationReferenceReset = true;
                                newCurrentElementIndexToFocus = index;
                            }
                        }
                    }
                }
            }
            break;
        }
        }

        if (newCurrentElementIndexToFocus != -1)
        {
            SetCurrentElementIndex(newCurrentElementIndexToFocus, winrt::FocusState::Keyboard, forceKeyboardNavigationReferenceReset, false /*startBringIntoView*/, true /*expectBringIntoView*/);

            if (m_navigationKeysToProcess.empty())
            {
                return true;
            }
        }
    }

    return false;
}

// Queues the incoming navigation key for future processing in ProcessNavigationKeys.
void ItemsView::QueueNavigationKey(
    const winrt::VirtualKey& key)
{
    MUX_ASSERT(IsNavigationKey(key));

#ifdef DBG
    switch (key)
    {
    case winrt::VirtualKey::Home:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Home key queued.");
        break;
    }
    case winrt::VirtualKey::End:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"End key queued.");
        break;
    }
    case winrt::VirtualKey::Left:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Left key queued.");
        break;
    }
    case winrt::VirtualKey::Right:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Right key queued.");
        break;
    }
    case winrt::VirtualKey::Up:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Up key queued.");
        break;
    }
    case winrt::VirtualKey::Down:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Down key queued.");
        break;
    }
    case winrt::VirtualKey::PageUp:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"PageUp key queued.");
        break;
    }
    case winrt::VirtualKey::PageDown:
    {
        ITEMSVIEW_TRACE_INFO(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"PageDown key queued.");
        break;
    }
    }
#endif

    m_navigationKeysToProcess.push_back(key);
}

bool ItemsView::SetFocusElementIndex(
    int index,
    winrt::FocusState focusState,
    bool startBringIntoView,
    bool expectBringIntoView)
{
    MUX_ASSERT(!startBringIntoView || !expectBringIntoView);

    if (index != -1 && focusState != winrt::FocusState::Unfocused)
    {
        if (auto const& element = TryGetElement(index))
        {
            const bool success = SetFocus(element, focusState);

            ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT_INT, METH_NAME, this, L"index", index, success);

            if (success)
            {
                if (auto const& scrollView = m_scrollView.get())
                {
                    if (expectBringIntoView)
                    {
                        m_navigationKeyBringIntoViewPendingCount++;

                        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"m_navigationKeyBringIntoViewPendingCount incremented", m_navigationKeyBringIntoViewPendingCount);
                    }
                    else if (startBringIntoView)
                    {
                        ITEMSVIEW_TRACE_INFO_DBG(*this, TRACE_MSG_METH_METH, METH_NAME, this, L"UIElement::StartBringIntoView");

                        element.StartBringIntoView();
                    }
                }
            }

            return success;
        }
    }

    return false;
}

// Updates the values of m_keyboardNavigationReferenceIndex and m_keyboardNavigationReferenceRect based on the current element.
void ItemsView::UpdateKeyboardNavigationReference()
{
    int currentElementIndex = GetCurrentElementIndex();

    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_INT, METH_NAME, this, currentElementIndex);

    const winrt::Point oldKeyboardNavigationReferenceOffset = GetKeyboardNavigationReferenceOffset();

    m_keyboardNavigationReferenceIndex = currentElementIndex;

    if (currentElementIndex != -1)
    {
        if (auto const& itemsRepeater = m_itemsRepeater.get())
        {
            if (auto const& currentElement = TryGetElement(currentElementIndex))
            {
                auto const generalTransform = currentElement.TransformToVisual(itemsRepeater);
                auto const currentElementOffset = generalTransform.TransformPoint(winrt::Point(0, 0));

                if (auto const currentElementAsFE = currentElement.as<winrt::FrameworkElement>())
                {
                    m_keyboardNavigationReferenceRect = winrt::Rect{
                        currentElementOffset.X,
                        currentElementOffset.Y,
                        static_cast<float>(currentElementAsFE.ActualWidth()),
                        static_cast<float>(currentElementAsFE.ActualHeight()) };
                }
                else
                {
                    m_keyboardNavigationReferenceRect = winrt::Rect{
                        currentElementOffset.X,
                        currentElementOffset.Y,
                        0.0f,
                        0.0f };
                }
            }
            else
            {
                m_keyboardNavigationReferenceIndex = -1;
                m_keyboardNavigationReferenceRect = { -1.0f, -1.0f, -1.0f, -1.0f };
            }
        }
        else
        {
            m_keyboardNavigationReferenceIndex = -1;
            m_keyboardNavigationReferenceRect = { -1.0f, -1.0f, -1.0f, -1.0f };
        }
    }
    else
    {
        m_keyboardNavigationReferenceRect = { -1.0f, -1.0f, -1.0f, -1.0f };
    }

    ITEMSVIEW_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, TypeLogging::RectToString(m_keyboardNavigationReferenceRect).c_str(), m_keyboardNavigationReferenceIndex);

    com_ptr<ItemsViewTestHooks> globalTestHooks = ItemsViewTestHooks::GetGlobalTestHooks();

    if (globalTestHooks != nullptr)
    {
        const winrt::Point newKeyboardNavigationReferenceOffset = GetKeyboardNavigationReferenceOffset();
        const winrt::IndexBasedLayoutOrientation indexBasedLayoutOrientation = GetLayoutIndexBasedLayoutOrientation();

        if ((oldKeyboardNavigationReferenceOffset.X != newKeyboardNavigationReferenceOffset.X && (indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::LeftToRight || indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::None)) ||
            (oldKeyboardNavigationReferenceOffset.Y != newKeyboardNavigationReferenceOffset.Y && (indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::TopToBottom || indexBasedLayoutOrientation == winrt::IndexBasedLayoutOrientation::None)))
        {
            globalTestHooks->NotifyKeyboardNavigationReferenceOffsetChanged(*this);
        }
    }
}
